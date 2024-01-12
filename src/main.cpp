#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <fstream>

#include "Overlay.hpp"
#include "KeymapRender.hpp"

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

static auto const default_keymap_path = "/usr/share/kbd/keymaps/beepy-kbd.map";

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

int main(int argc, char** argv)
{
	// Check arguments
	if ((argc != 2) && (argc != 3)) {
		fprintf(stderr, "usage: %s sharp_dev [keymap_path]\n", argv[0]);
		return 1;
	}

	// Get arguments
	auto sharp_dev = argv[1];
	auto keymap_path = (argc > 2)
		? argv[2]
		: default_keymap_path;

	// Parse keymap
	auto symkeyX11names = parse_keymap(keymap_path);

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

	auto session = SharpSession{sharp_dev};
	auto overlay = Overlay{session, 0, -(int)keymapRender.getHeight(),
		keymapRender.getWidth(), keymapRender.getHeight(), keymapRender.get()};

	overlay.show();
#if 0
	char c;
	read(0, &c, 1);
#else
	sleep(5);
#endif
	overlay.hide();

	return 0;
}
