#pragma once

#include <memory>
#include <unordered_map>

#include "PSF.hpp"

class KeymapRender
{
public: // types
	using Keymap = std::unordered_map<int, uint16_t>;

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
	KeymapRender(unsigned char const* psf_data, size_t psf_size, Keymap const& keymap);

	auto getWidth() const { return m_width; }
	auto getHeight() const { return m_height; }
	auto get() const { return m_pix.get(); }
};
