#include <string.h>

#include <string>
#include <vector>

#include "KeymapRender.hpp"

using namespace std::literals;

static constexpr auto fret_height = 4;
static constexpr auto cell_padding = 1;
static constexpr auto char_padding = 1;

static constexpr auto num_rows = 3;
static constexpr auto num_cols = 10;

static const auto symkey_alpha_table
	= std::vector<std::vector<std::pair<int, int>>>
	{ { {16, 'Q'}, {17, 'W'}, {18, 'E'}, {19, 'R'}, {20, 'T'}, {21, 'Y'}
	  , {22, 'U'}, {23, 'I'}, {24, 'O'}, {25, 'P'} }
	, { {30, 'A'}, {31, 'S'}, {32, 'D'}, {33, 'F'}, {34, 'G'}, {35, 'H'}
	  , {36, 'J'}, {37, 'K'}, {38, 'L'} }
	, { { 0,'\0'}, {44, 'Z'}, {45, 'X'}, {46, 'C'}, {47, 'V'}, {48, 'B'}
	  , {49, 'N'}, {50, 'M'}, {113, '$'} }
};

template <typename RenderFunc>
static void render_map(unsigned char* pix, size_t width, size_t height, size_t cell_width, size_t cell_height,
	PSF& psf, RenderFunc&& render)
{
	// Set to white background
	::memset(pix, 0xff, width * height);

	// Render rows
	for (size_t row = 0; row < num_rows; row++) {

		// Render fret
		::memset(&pix[(row * cell_height) * width], 0,
			width * fret_height);

		// Render key contents
		for (size_t col = 0; col < 10; col++) {

			// Render cell padding
			for (size_t y = fret_height; y < cell_height; y++) {
				for (size_t x = 0; x < cell_padding; x++) {
					auto dx = (col * cell_width) + x;
					auto dy = (row * cell_height) + y;
					pix[dy * width + dx] = 0;
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

			// Render mapped key, don't render alpha key if no mapped key
			if (!render(row, col, symkey)) {
				continue;
			}

			// Render alpha key
			psf.drawUtf16(alpha_utf16,
				pix, width, height,
				// Left-align on left half, right-align on right half
				(col * cell_width) + ((col < 5)
					? cell_padding + char_padding
					: cell_width - (char_padding + psf.getWidth())),
				(row * cell_height) + fret_height + char_padding,
				1);
		}
	}
}

// Common initialization
KeymapRender::KeymapRender(unsigned char const* psf_data, size_t psf_size)
	: m_psf{psf_data, psf_size}
	, m_cellWidth{40}
	, m_cellHeight{fret_height + char_padding + 2 * m_psf.getHeight()}
	, m_width{400}
	, m_height{num_rows * m_cellHeight}
	, m_pix{new unsigned char[m_width * m_height]}
{}

KeymapRender::KeymapRender(unsigned char const* psf_data, size_t psf_size, Keymap const& keymap)
	: KeymapRender(psf_data, psf_size)
{
	render_map(m_pix.get(), m_width, m_height, m_cellWidth, m_cellHeight, m_psf,
		[this, &keymap](size_t row, size_t col, int symkey) {

			// Look up symbol
			auto symkeyUtf16 = keymap.find(symkey);
			if (symkeyUtf16 == keymap.end()) {
				return false;
			}

			// Render mapped key
			m_psf.drawUtf16(symkeyUtf16->second,
				m_pix.get(), m_width, m_height,
				// Centered, 2x scale
				(col * m_cellWidth) + (m_cellWidth / 2 - m_psf.getWidth()),
				(row * m_cellHeight) + fret_height
					+ (m_cellHeight / 2 - m_psf.getHeight()),
				2);

			return true;
		}
	);
}

KeymapRender::KeymapRender(unsigned char const* psf_data, size_t psf_size, Picmap const& picmap, size_t pic_width, size_t pic_height)
	: KeymapRender(psf_data, psf_size)
{}
