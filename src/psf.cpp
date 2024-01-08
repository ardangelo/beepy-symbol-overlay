#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include <memory>
#include <string>
#include <stdexcept>

#include "psf.hpp"

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
gen_psf1_utf16_table(FILE* fp)
{
	auto result = std::unordered_map<uint16_t, uint16_t>{};

	// Seek to entries
	if (auto rc = ::fseek(fp, unicode_table_offs, SEEK_SET)) {
		throw std::runtime_error("failed to seek to Unicode table");
	}

	// Read entries
	auto psf_idx = uint16_t{0};
	auto unicode_val = uint16_t{0};
	while (::fread(&unicode_val, sizeof(unicode_val), 1, fp) == 1) {
		if (unicode_val == unicode_table_delim) {
			psf_idx++;
		} else {
			result[unicode_val] = psf_idx;
		}
	}

	return result;
}

PSF::PSF(char const* psf_path)
	: m_file{::fopen(psf_path, "rb"), ::fclose}
	, m_table{}
	, m_header{}
{
	// Read header
	if (::fread(&m_header, sizeof(m_header), 1, m_file.get()) != 1) {
		throw std::runtime_error("failed to read header from "s + psf_path);;
	}

	// Validate header
	if (m_header.magic != psf1_magic) {
		throw std::runtime_error("invalid PSF1 magic number");
	}
	if (m_header.charsize == 0) {
		throw std::runtime_error("header reports zero charsize");
	}

	// Read UTF16 translation table
	m_table = gen_psf1_utf16_table(m_file.get());
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
	int x, int y)
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
	if (auto rc = ::fseek(m_file.get(),
		sizeof(m_header) + (m_header.charsize * idx), SEEK_SET)) {
		throw std::runtime_error("failed to seek to char at "s + std::to_string(idx));
	}
	if (m_header.charsize == 0) {
		throw std::runtime_error("header has zero charsize");
	}
	unsigned char glyph[m_header.charsize];
	if (::fread(glyph, m_header.charsize, 1, m_file.get()) != 1) {
		throw std::runtime_error("failed to read char at "s + std::to_string(idx));
	}

	// Starting coordinates
	y = (y < 0) ? buf_height + y : y;
	if (y >= buf_height) {
		return;
	}
	x = (x < 0) ? buf_width + x : x;
	if (x >= buf_width) {
		return;
	}

	// Draw character at double height, black on white
	for (int sy = 0; sy < m_header.charsize; sy++) {

		auto dy = 2 * (y + sy);
		if (dy >= buf_height) {
			break;
		}

		for (int sx = 0; sx < psf1_charwidth; sx++) {

			auto dx = 2 * (x + sx);
			if (dx >= buf_width) {
				break;
			}

			auto pixel = (glyph[sy] >> (7 - sx)) & 1;
			buf[((dy + 0) * buf_width) + dx + 0] = (pixel ? 0x00 : 0xff);
			buf[((dy + 0) * buf_width) + dx + 1] = (pixel ? 0x00 : 0xff);
			buf[((dy + 1) * buf_width) + dx + 0] = (pixel ? 0x00 : 0xff);
			buf[((dy + 1) * buf_width) + dx + 1] = (pixel ? 0x00 : 0xff);
		}
	}
}
