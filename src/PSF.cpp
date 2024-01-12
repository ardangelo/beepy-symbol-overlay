#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include <memory>
#include <string>
#include <stdexcept>

#include "PSF.hpp"

using namespace std::literals;

static constexpr auto psf1_magic = 0x0436;
static constexpr auto psf1_charwidth = 8;
static constexpr auto unicode_table_offs = 0x2000;
static constexpr auto unicode_table_delim = 0xffff;

/*
00002000: 0000 0000 a900 ffff 2601 ffff 3501 ffff  ........&...5...
00002010: 0204 ffff 6626 c825 fdff ffff 0904 ffff  ....f&.%........

0000 <- 0000, 0000, 00a9
0001 <- 0126
0002 <- 0135
0003 <- 0402
0004 <- 2666, 25c8, fffd
*/

static PSF::Utf16Table
gen_psf1_utf16_table(unsigned char const* psf_data, size_t psf_size)
{
	auto result = std::unordered_map<uint16_t, uint16_t>{};

	// Read entries
	auto psf_idx = uint16_t{0};
	auto unicode_val = uint16_t{0};
	auto ptr = (psf_data + unicode_table_offs);
	while (ptr + (sizeof(uint16_t) - 1) < (psf_data + psf_size)) {
		unicode_val = *(uint16_t const*)ptr;
		if (unicode_val == unicode_table_delim) {
			psf_idx++;
		} else {
			result[unicode_val] = psf_idx;
		}
		ptr += sizeof(uint16_t);
	}

	return result;
}

PSF::PSF(unsigned char const* psf_data, size_t psf_size)
	: m_psfData{psf_data}
	, m_psfSize{psf_size}
	, m_table{}
	, m_header{*(psf1_header const*)m_psfData}
{
	// Validate header
	if (m_header.magic != psf1_magic) {
		throw std::runtime_error("invalid PSF1 magic number");
	}
	if (m_header.charsize == 0) {
		throw std::runtime_error("header reports zero charsize");
	}

	// Read UTF16 translation table
	m_table = gen_psf1_utf16_table(m_psfData, m_psfSize);
}

size_t PSF::getHeight()
{
	return m_header.charsize;
}

size_t PSF::getWidth()
{
	return psf1_charwidth;
}

void PSF::drawUtf16(uint16_t utf16,
	unsigned char *buf, int buf_width, int buf_height,
	int x, int y, int scale)
{
	// Map UTF16 to PSF index
	auto utf16Psfindex = m_table.find(utf16);
	if (utf16Psfindex == m_table.end()) {
		return;
	}
	auto idx = utf16Psfindex->second;

	// Check character
	if ((idx < 0) || (idx > 0x1ff)) {
		throw std::logic_error("character out of range");
	}

	// Read character
	if (m_header.charsize == 0) {
		throw std::runtime_error("header has zero charsize");
	}
	if (sizeof(m_header) + (m_header.charsize * idx) >= m_psfSize) {
		throw std::runtime_error("character index out of range");
	}
	auto glyph = m_psfData + sizeof(m_header) + (m_header.charsize * idx);

	// Starting coordinates
	y = (y < 0) ? buf_height + y : y;
	if (y >= buf_height) {
		return;
	}
	x = (x < 0) ? buf_width + x : x;
	if (x >= buf_width) {
		return;
	}

	// Draw character, black on white
	for (int sy = 0; sy < m_header.charsize; sy++) {

		auto dy = y + (scale * sy);
		if (dy >= buf_height) {
			break;
		}

		for (int sx = 0; sx < psf1_charwidth; sx++) {

			auto dx = x + (scale * sx);
			if (dx >= buf_width) {
				break;
			}

			auto pixel = (glyph[sy] >> (7 - sx)) & 1;
			for (int dsx = 0; dsx < scale; dsx++) {
				for (int dsy = 0; dsy < scale; dsy++) {
					buf[((dy + dsy) * buf_width) + dx + dsx] = (pixel ? 0x00 : 0xff);
				}
			}
		}
	}
}
