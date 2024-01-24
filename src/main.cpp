#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <fstream>
#include <tuple>

#include "Overlay.hpp"
#include "KeymapRender.hpp"

#include "getopt.hpp"

// src/x11name_to_utf16.cpp
extern uint16_t x11name_to_utf16(std::string const& x11name);

// Converted font
extern "C" {
extern const char _binary_font_psf_start;
extern const char _binary_font_psf_end;
}
static const auto psf_start = (unsigned char const*)&_binary_font_psf_start;
static const auto psf_size = (size_t)(
	(unsigned char const*)&_binary_font_psf_end
		- (unsigned char const*)&_binary_font_psf_start);

using namespace std::literals;

#ifndef DEFAULT_KEYMAP_PATH
#define DEFAULT_KEYMAP_PATH "/usr/share/kbd/keymaps/beepy-kbd.map"
#endif
static auto const default_keymap_path = DEFAULT_KEYMAP_PATH;

// Convert X keymap into map from keycode to x11name
static auto parse_keymap(char const* keymap_path)
{
	auto result = std::map<int, std::string>{};

	auto keymap = std::ifstream{keymap_path};

	//altgr keycode 50 = guillemotright
	auto line = std::string{};
	while (std::getline(keymap, line)) {

		// Ignore empty or comment lines
		if (line.empty() || (line[0] == '#')) {
			continue;
		}

		// Find altgr modifier lines
		if (line.find("altgr ") != 0) {
			continue;
		}

		// Get keycode
		auto keycode_delim = std::string{"keycode "};
		auto keycode_at = line.find(keycode_delim);
		if (keycode_at == std::string::npos) {
			continue;
		}
		auto keycode = int{};
		try {
			keycode = std::stoi(line.substr(keycode_at + keycode_delim.size()));
		} catch (std::exception const& ex) {
			throw std::runtime_error("failed to parse line: "s + line);
		}

		// Get mapping name
		auto equals_delim = std::string{"="};
		auto equals_at = line.find(equals_delim);
		if (equals_at == std::string::npos) {
			continue;
		}
		auto mapping = line.substr(equals_at + equals_delim.size());

		// Trim comments and whitespace
		auto hash_at = mapping.find("#");
		if (hash_at != std::string::npos) {
			mapping = mapping.substr(0, hash_at);
		}
		mapping.erase(0, mapping.find_first_not_of(" "));
		mapping.erase(mapping.find_last_not_of(" ") + 1);

		result[keycode] = std::move(mapping);
	}

	return result;
}

static void usage(char const* const* argv)
{
	fprintf(stderr, "usage: %s sharp_dev [--clear-all] [--meta] [--keymap=<path>] sharp_dev \n", argv[0]);
	fprintf(stderr, "sharp_dev    Sharp device to command (e.g. /dev/dri/card0)\n");
	fprintf(stderr, "--clear-all  Clear all overlays and exit\n");
	fprintf(stderr, "--meta       Display Meta mode keymap instead of Symbol keymap\n");
	fprintf(stderr, "--keymap     Path to X11 keymap to show for Symbol\n");
	fprintf(stderr, "  (default %s)\n", default_keymap_path);
}

static auto parse_argv(int argc, char** argv)
{
	auto clear_all = false;
	auto meta = false;
	auto keymapPath = std::string{default_keymap_path};
	auto sharpDev = std::string{};


	constexpr auto ClearAll = Argv::make_Option("clear-all", 'c');
	constexpr auto Help = Argv::make_Option("help", 'h');
	constexpr auto Meta = Argv::make_Option("meta", 'm');
	constexpr auto KeymapPath = Argv::make_Param("keymap", 'k');

	Argv::GNUOption opts[] = {
		ClearAll, Help, Meta,
		KeymapPath,
		Argv::GNUOptionDone
	};

	auto incomingArgv = Argv::IncomingArgv{opts, argc, argv};
	while (true) {
		auto&& [c, opt] = incomingArgv.get_next();
		if (c < 0) {
			break;
		}

		switch (c) {

		case ClearAll.val:
			clear_all = true;
			break;

		case Meta.val:
			meta = true;
			break;

		case KeymapPath.val:
			keymapPath = std::move(opt);
			break;

		case Help.val:
			usage(argv);
			exit(0);

		default:
			fprintf(stderr, "Unexpected argument: -%c\n", c);
			usage(argv);
			exit(1);
		}
	}

	auto&& [rest_argc, rest_argv] = incomingArgv.get_rest();
	if (rest_argc == 0) {
		fprintf(stderr, "Expected sharp_dev argument\n");
		usage(argv);
		exit(1);
	}

	sharpDev = std::string{rest_argv[0]};

	return std::make_tuple(clear_all, meta, std::move(keymapPath), std::move(sharpDev));
}

int main(int argc, char** argv)
{
	// Parse arguments
	auto&& [clear_all, meta, keymapPath, sharpDev] = parse_argv(argc, argv);

	// Parse keymap
	auto symkeyX11names = parse_keymap(keymapPath.c_str());

	// Build symkey map
	auto keymap = KeymapRender::Keymap{};
	for (auto const& [symkey, x11name] : symkeyX11names) {		
		auto sym_utf16 = x11name_to_utf16(x11name);
		if (sym_utf16 == 0x0) {
			continue;
		}
		keymap[symkey] = sym_utf16;
	}
	auto keymapRender = KeymapRender{psf_start, psf_size, keymap};

	auto session = SharpSession{sharpDev.c_str()};
	auto overlay = Overlay{session, 0, -(int)keymapRender.getHeight(),
		keymapRender.getWidth(), keymapRender.getHeight(), keymapRender.get()};

	overlay.show();
	overlay.eject();

	return 0;
}
