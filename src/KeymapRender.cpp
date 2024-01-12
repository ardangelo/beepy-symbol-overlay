#include <string.h>

#include <string>
#include <vector>

#include "KeymapRender.hpp"

using namespace std::literals;

static const auto symkey_alpha_table
	= std::vector<std::vector<std::pair<int, int>>>
	{ { {16, 'Q'}, {17, 'W'}, {18, 'E'}, {19, 'R'}, {20, 'T'}, {21, 'Y'}
	  , {22, 'U'}, {23, 'I'}, {24, 'O'}, {25, 'P'} }
	, { {30, 'A'}, {31, 'S'}, {32, 'D'}, {33, 'F'}, {34, 'G'}, {35, 'H'}
	  , {36, 'J'}, {37, 'K'}, {38, 'L'} }
	, { { 0,'\0'}, {44, 'Z'}, {45, 'X'}, {46, 'C'}, {47, 'V'}, {48, 'B'}
	  , {49, 'N'}, {50, 'M'}, {113, '$'} }
};

KeymapRender::KeymapRender(unsigned char const* psf_data, size_t psf_size, Keymap const& keymap)
	: m_psf{psf_data, psf_size}
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
