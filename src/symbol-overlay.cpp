#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include <unistd.h>

#include <string>
#include <memory>
#include <stdexcept>

#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>

#include "ioctl_iface.h"

using namespace std::literals;

static auto const default_keymap_path = "/usr/share/kbd/keymaps/beepy-kbd.map";

static auto overlay_add(int fd, sharp_overlay_t overlay)
{
	auto param = sharp_memory_ioctl_ov_add_t { .in_overlay = &overlay };
	if (auto rc = ::ioctl(fd, DRM_IOCTL_SHARP_OV_ADD, &param)) {
		throw std::runtime_error(__func__ + " failed: "s + ::strerror(rc));
	}
	if ((param.out_storage == NULL)
	 || ((void*)param.in_overlay == (void*)&param)) {
		throw std::runtime_error(__func__ + " failed: ioctl returned invalid result"s);
	}
	return param.out_storage;
}

static void overlay_remove(int fd, void* storage)
{
	auto param = sharp_memory_ioctl_ov_rem_t { .storage = storage };
	if (auto rc = ::ioctl(fd, DRM_IOCTL_SHARP_OV_REM, &param)) {
		throw std::runtime_error(__func__ + " failed: "s + ::strerror(rc));
	}
}

static auto overlay_show(int fd, void *storage)
{
	auto param = sharp_memory_ioctl_ov_show_t { .in_storage = storage };
	if (auto rc = ::ioctl(fd, DRM_IOCTL_SHARP_OV_SHOW, &param)) {
		throw std::runtime_error(__func__ + " failed: "s + ::strerror(rc));
	}
	if ((param.out_display == NULL)
	 || ((void*)param.out_display == &param)) {
		throw std::runtime_error(__func__ + " failed: ioctl returned invalid result"s);
	}
	return param.out_display;
}

static void overlay_hide(int fd, void* display)
{
	auto param = sharp_memory_ioctl_ov_hide_t { .display = display };
	if (auto rc = ::ioctl(fd, DRM_IOCTL_SHARP_OV_HIDE, &param)) {
		throw std::runtime_error(__func__ + " failed: "s + ::strerror(rc));
	}
}

int main(int argc, char** argv)
{
	// Check arguments
	if ((argc != 2) && (argc != 3)) {
		fprintf(stderr, "usage: %s sharp_dev [keymap_path]\n");
		return 1;
	}

	// Get arguments
	auto sharp_dev = argv[1];
	auto keymap_path = (argc > 2)
		? argv[2]
		: default_keymap_path;

	unsigned char const pix[] =
		{ 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff
		, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00
		, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff
		, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00
		, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff
		, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00
		, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff
		, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00
	};
	auto overlay = sharp_overlay_t { .x = 20, .y = 20, .width = 8, .height = 8,
		.pixels = pix };
	auto fd = ::open(sharp_dev, O_RDWR);
	if (fd < 0) {
		throw std::runtime_error("open "s + sharp_dev + " failed: "s
			+ ::strerror(errno));
	}
	auto storage = overlay_add(fd, overlay);
	auto display = overlay_show(fd, storage);
	sleep(5);
	overlay_hide(fd, display);
	overlay_remove(fd, storage);

	return 0;
}
