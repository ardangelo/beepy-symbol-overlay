#pragma once

#include <memory>
#include <unordered_map>

class PSF
{
public: // types
	using Utf16Table = std::unordered_map<uint16_t, uint16_t>;

	struct psf1_header
	{
		uint16_t magic;
		uint8_t  mode;
		uint8_t  charsize;
	}__attribute__((packed));

private: // members
	unsigned char const* m_psfData;
	size_t m_psfSize;
	Utf16Table m_table;
	psf1_header m_header;

public: // interface
	PSF(unsigned char const* psf_data, size_t psf_size);

	size_t getHeight();
	size_t getWidth();

	void drawUtf16(uint16_t utf16,
		unsigned char *buf, int buf_width, int buf_height,
		int x, int y, int scale);

};

