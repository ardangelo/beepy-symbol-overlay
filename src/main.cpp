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

static const auto symkey_alpha_table
	= std::vector<std::vector<std::pair<int, int>>>
	{ { {16, 'Q'}, {17, 'W'}, {18, 'E'}, {19, 'R'}, {20, 'T'}, {21, 'Y'}
	  , {22, 'U'}, {23, 'I'}, {24, 'O'}, {25, 'P'} }
	, { {30, 'A'}, {31, 'S'}, {32, 'D'}, {33, 'F'}, {34, 'G'}, {35, 'H'}
	  , {36, 'J'}, {37, 'K'}, {38, 'L'} }
	, { { 0,'\0'}, {44, 'Z'}, {45, 'X'}, {46, 'C'}, {47, 'V'}, {48, 'B'}
	  , {49, 'N'}, {50, 'M'}, {113, '$'} }
};

class KeymapRender
{
public: // types
	using Keymap = std::map<int, uint16_t>;

private: // members
	static constexpr auto fret_height = 4;
	static constexpr auto cell_padding = 1;
	static constexpr auto char_padding = 1;

	static constexpr auto num_rows = 3;
	static constexpr auto num_cols = 10;

	PSF m_psf;
	size_t m_cellWidth, m_cellHeight;
	size_t m_width, m_height;
	std::unique_ptr<unsigned char> m_pix;

public: // interface
	KeymapRender(char const* psf_path, Keymap const& keymap);

	auto getWidth() const { return m_width; }
	auto getHeight() const { return m_height; }
	auto get() const { return m_pix.get(); }
};

KeymapRender::KeymapRender(char const* psf_path, Keymap const& keymap)
	: m_psf{psf_path}
	, m_cellWidth{40}
	, m_cellHeight{fret_height + char_padding + 2 * m_psf.getHeight()}
	, m_width{400}
	, m_height{num_rows * m_cellHeight}
	, m_pix{new unsigned char[m_width * m_height]}
{
	// Set to white background
	::memset(m_pix.get(), 0xff, m_width * m_height);

	// Render rows
	for (size_t row = 0; row < num_rows; row++) {

		// Render fret
		::memset(&m_pix.get()[(row * m_cellHeight) * m_width], 0,
			m_width * fret_height);

		// Render key contents
		for (size_t col = 0; col < 10; col++) {

			// Render cell padding
			for (size_t y = fret_height; y < m_cellHeight; y++) {
				for (size_t x = 0; x < cell_padding; x++) {
					auto dx = (col * m_cellWidth) + x;
					auto dy = (row * m_cellHeight) + y;
					m_pix.get()[dy * m_width + dx] = 0;
				}
			}

			// Get alpha / symbol keys
			if ((symkey_alpha_table.size() <= row)
			 || (symkey_alpha_table[row].size() <= col)) {
				continue;
			}
			auto const& [symkey, alpha_utf16] = symkey_alpha_table[row][col];
			if (symkey == 0) {
				continue;
			}

			// Look up key mapping
			auto symkeyUtf16 = keymap.find(symkey);
			if (symkeyUtf16 == keymap.end()) {
				continue;
			}

			// Render alpha key
			m_psf.drawUtf16(alpha_utf16,
				m_pix.get(), m_width, m_height,
				// Left-align on left half, right-align on right half
				(col * m_cellWidth) + ((col < 5)
					? cell_padding + char_padding
					: m_cellWidth - (char_padding + m_psf.getWidth())),
				(row * m_cellHeight) + fret_height + char_padding,
				1);

			// Render mapped key
			m_psf.drawUtf16(symkeyUtf16->second,
				m_pix.get(), m_width, m_height,
				// Centered, 2x scale
				(col * m_cellWidth) + (m_cellWidth / 2 - m_psf.getWidth()),
				(row * m_cellHeight) + fret_height
					+ (m_cellHeight / 2 - m_psf.getHeight()),
				2);
		}
	}
}

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
	auto keymapRender = KeymapRender{default_psf_path, keymap};

	auto session = SharpSession{sharp_dev};
	auto overlay = Overlay{session, 0, -(int)keymapRender.getHeight(),
		keymapRender.getWidth(), keymapRender.getHeight(), keymapRender.get()};

	overlay.show();
	char c;
	read(0, &c, 1);
	overlay.hide();

	return 0;
}
