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
{ {16, 25}, {30, 38}, {43, 50} };
static const auto alphaUtf16Table = std::unordered_map<int, uint16_t>
{ {16, 'Q'}
, {17, 'W'}
, {18, 'E'}
, {19, 'R'}
, {20, 'T'}
, {21, 'Y'}
, {22, 'U'}
, {23, 'I'}
, {24, 'O'}
, {25, 'P'}

, {30, 'A'}
, {31, 'S'}
, {32, 'D'}
, {33, 'F'}
, {34, 'G'}
, {35, 'H'}
, {36, 'J'}
, {37, 'K'}
, {38, 'L'}

, {44, 'Z'}
, {45, 'X'}
, {46, 'C'}
, {47, 'V'}
, {48, 'B'}
, {49, 'N'}
, {50, 'M'}
, {113, '$'}
};

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

	// Open PSF renderer
	auto psf = PSF{default_psf_path};

	// Render table
	auto symkey_cols = [](std::vector<std::pair<int, int>> const& symkey_rows) {
		auto max = 0;
		for (auto const& [lo, hi] : symkey_rows) {
			auto width = hi - lo + 1;
			if (width > max) { max = width; }
		}
		return max;
	}(symkey_rows);
	auto pix_height = 2 * symkey_rows.size() * psf.getHeight();
	auto pix_width = 4 * symkey_cols * psf.getWidth();
	unsigned char pix[pix_width * pix_height];
	::memset(pix, 0/*0xff*/, pix_width * pix_height);
	for (size_t row = 0; row < symkey_rows.size(); row++) {
		for (auto symkey = symkey_rows[row].first;
			symkey <= symkey_rows[row].second; symkey++) {

			auto keycodeX11name = keycodeX11names.find(symkey);
			if (keycodeX11name == keycodeX11names.end()) {
				continue;
			}

			auto alphaUtf16 = alphaUtf16Table.find(symkey);
			if (alphaUtf16 == alphaUtf16Table.end()) {
				continue;
			}

			auto sym_utf16 = x11name_to_utf16(keycodeX11name->second);
			if (sym_utf16 == 0x0) {
				continue;
			}

			auto x = 2 * (symkey - symkey_rows[row].first) * psf.getWidth();
			auto y = row * psf.getHeight();
			psf.drawUtf16(alphaUtf16->second,
				pix, pix_width, pix_height, x, y);
			psf.drawUtf16(sym_utf16,
				pix, pix_width, pix_height, x + psf.getWidth(), y);
		}
	}

	auto session = SharpSession{sharp_dev};
	auto overlay = Overlay{session, 0, 0, pix_width, pix_height, pix};

	overlay.show();
	char c;
	read(0, &c, 1);
	overlay.hide();

	return 0;
}
