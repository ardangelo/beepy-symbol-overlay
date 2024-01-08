#pragma once

#include <unordered_map>

std::unordered_map<uint16_t, uint16_t>
gen_psf1_utf16_table(char const *psf_path);

void draw_psf1_character(char const *psf_path, uint32_t c,
	unsigned char *buf, int buf_width, int buf_height,
	int x, int y);

