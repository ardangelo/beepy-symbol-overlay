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

struct psf1_header
{
	uint16_t magic;
	uint8_t  mode;
	uint8_t  charsize;
};

/*
00002000: 0000 0000 a900 ffff 2601 ffff 3501 ffff  ........&...5...
00002010: 0204 ffff 6626 c825 fdff ffff 0904 ffff  ....f&.%........

0000 <- 0000, 0000, 00a9
0001 <- 0126
0002 <- 0135
0003 <- 0402
0004 <- 2666, 25c8, fffd
*/

std::unordered_map<uint16_t, uint16_t>
gen_psf1_utf16_table(char const *psf_path)
{
	auto result = std::unordered_map<uint16_t, uint16_t>{};

	// Open font file
	auto file = std::unique_ptr<FILE, decltype(&::fclose)>
		{::fopen(psf_path, "rb"), ::fclose};
	if (!file) {
		throw std::runtime_error("could not open "s + psf_path);
	}

	// Seek to entries
	if (auto rc = ::fseek(file.get(), unicode_table_offs, SEEK_SET)) {
		throw std::runtime_error("failed to seek to Unicode table");
	}

	// Read entries
	auto psf_idx = uint16_t{0};
	auto unicode_val = uint16_t{0};
	while (::fread(&unicode_val, sizeof(unicode_val), 1, file.get()) == 1) {
		if (unicode_val == unicode_table_delim) {
			psf_idx++;
		} else {
			result[unicode_val] = psf_idx;
		}
	}

	return result;
}

void draw_psf1_character(char const *psf_path, uint32_t c,
	unsigned char *buf, int buf_width, int buf_height, int x, int y)
{
	// Check character
	if ((c < 0) || (c > 0x1ff)) {
		throw std::logic_error("character out of range");
	}

	// Open font file
	auto file = std::unique_ptr<FILE, decltype(&::fclose)>
		{::fopen(psf_path, "rb"), ::fclose};
	if (!file) {
		throw std::runtime_error("could not open "s + psf_path);
	}

	// Read header
	auto header = psf1_header{};
	::fread(&header, sizeof(psf1_header), 1, file.get());

	// Validate header
	if (header.magic != psf1_magic) {
		throw std::runtime_error("invalid PSF1 magic number");
	}

	// Read character
	if (auto rc = ::fseek(file.get(),
		sizeof(psf1_header) + (header.charsize * c), SEEK_SET)) {
		throw std::runtime_error("failed to seek to char at "s + std::to_string(c));
	}
	unsigned char glyph[header.charsize];
	if (::fread(glyph, header.charsize, 1, file.get()) != 1) {
		throw std::runtime_error("failed to read char at "s + std::to_string(c));
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

	// Draw character
	for (int sy = 0; sy < header.charsize; sy++) {

		auto dy = y + sy;
		if (dy >= buf_height) {
			break;
		}

		for (int sx = 0; sx < psf1_charwidth; sx++) {

			auto dx = x + sx;
			if (dx >= buf_width) {
				break;
			}

			auto pixel = (glyph[sy] >> (7 - sx)) & 1;
			buf[(dy * buf_width) + dx] = (pixel ? 0xFF : 0x00);
		}
	}
}
