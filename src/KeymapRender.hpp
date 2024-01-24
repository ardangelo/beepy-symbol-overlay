#pragma once

#include <memory>
#include <unordered_map>

#include "PSF.hpp"

class KeymapRender
{
public: // types
	using Keymap = std::unordered_map<int, uint16_t>;
	using Picmap = std::unordered_map<int, char const*>;

private: // members
	PSF m_psf;
	size_t m_cellWidth, m_cellHeight;
	size_t m_width, m_height;
	std::unique_ptr<unsigned char> m_pix;

private: // helpers
	KeymapRender(unsigned char const* psf_data, size_t psf_size);

public: // interface
	KeymapRender(unsigned char const* psf_data, size_t psf_size, Keymap const& keymap);
	KeymapRender(unsigned char const* psf_data, size_t psf_size, Picmap const& picmap, size_t pic_width, size_t pic_height);

	auto getWidth() const { return m_width; }
	auto getHeight() const { return m_height; }
	auto get() const { return m_pix.get(); }
};
