#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include <memory>
#include <string>
#include <stdexcept>

using namespace std::literals;

static constexpr auto psf1_magic = 0x0436;
static constexpr auto psf1_charwidth = 8;

struct psf1_header
{
	uint16_t magic;
	uint8_t  mode;
	uint8_t  charsize;
};

void draw_psf1_character(char const *psf_path, char c,
	unsigned char *buf, int buf_width, int buf_height, int x, int y)
{
	// Check character
	if ((c < 0) || (c >= 256)) {
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
	::fseek(file.get(), sizeof(psf1_header) + (header.charsize * c), SEEK_SET);
	unsigned char glyph[header.charsize];
	::fread(glyph, header.charsize, 1, file.get());

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
			buf[dy * buf_width + dx] = (pixel ? 0xFF : 0x00);
		}
	}
}
