#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <fstream>

#include "overlay.hpp"
#include "psf.hpp"

using namespace std::literals;

// src/x11name_to_utf16.cpp
extern uint16_t x11name_to_utf16(std::string const& x11name);

static auto const default_keymap_path = "/usr/share/kbd/keymaps/beepy-kbd.map";
static auto const default_psf_path = "Uni1-VGA16.psf";

static const auto symkey_rows = std::vector<std::pair<int, int>>
	{ {16, 25}, {30, 38}, {44, 50} };

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
		fprintf(stderr, "usage: %s sharp_dev [keymap_path]\n");
		return 1;
	}

	// Get arguments
	auto sharp_dev = argv[1];
	auto keymap_path = (argc > 2)
		? argv[2]
		: default_keymap_path;

	// Parse keymap
	auto keycodeX11names = parse_keymap(keymap_path);

	// Generate UTF16 -> font index map from font
	auto utf16Psfindices = gen_psf1_utf16_table(default_psf_path);

	// Render table
	auto max_rows = 3;
	auto max_cols = 10;
	unsigned char pix[(3 * 16) * (10 * 8)] = {};
	for (size_t row = 0; row < symkey_rows.size(); row++) {
		for (auto symkey = symkey_rows[row].first;
			symkey <= symkey_rows[row].second; symkey++) {

			auto keycodeX11name = keycodeX11names.find(symkey);
			if (keycodeX11name == keycodeX11names.end()) {
printf("skip no name for keycode %d\n", symkey);
				continue;
			}

			auto utf16 = x11name_to_utf16(keycodeX11name->second);
			if (utf16 == 0x0) {
printf("skip no utf16 for %s\n", keycodeX11name->second.c_str());
				continue;
			}

			auto utf16Psfidx = utf16Psfindices.find(utf16);
			if (utf16Psfidx == utf16Psfindices.end()) {
printf("skip no index for utf16 %x\n", utf16);
				continue;
			}

			auto psf_idx = utf16Psfidx->second;
			auto x = (symkey - symkey_rows[row].first) * 8;
			auto y = row * 16;
printf("Draw %s idx %x at %d %d\n", keycodeX11name->second.c_str(), psf_idx, x, y);
			draw_psf1_character(default_psf_path, psf_idx, pix,
				10 * 8, 3 * 16, x, y);
		}
	}

/*
	for (int i = 0; i < 3 * 10; i++) {
		for (int j = 0; j < 10 * 8; j++) {
			auto p = pix[(i * 10 * 8) + j];
			printf(p ? "#" : " ");
		}
		printf("\n");
	}
	printf("\n");
*/
	auto session = SharpSession{sharp_dev};
	auto overlay = Overlay{session, 0, 0, 10 * 8, 3 * 16, pix};

	overlay.show();
	char c;
	read(0, &c, 1);
	overlay.hide();

	return 0;
}
