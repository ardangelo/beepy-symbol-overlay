#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <sys/ioctl.h>

#include <stdexcept>

#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>

#include "ioctl_iface.h"

#include "Overlay.hpp"

using namespace std::literals;

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

static void overlay_clear(int fd)
{
	if (auto rc = ::ioctl(fd, DRM_IOCTL_SHARP_OV_CLEAR)) {
		throw std::runtime_error(__func__ + " failed: "s + ::strerror(rc));
	}
}

SharpSession::SharpSession(char const* sharp_dev)
	: m_fd{::open(sharp_dev, O_RDWR)}
{
	if (m_fd < 0) {
		throw std::runtime_error("failed to open "s + sharp_dev + ": "
			+ ::strerror(errno));
	}
}

SharpSession::SharpSession(SharpSession&& expiring)
	: m_fd{expiring.m_fd}
{
	expiring.m_fd = -1;
}

SharpSession::~SharpSession()
{
	if (m_fd >= 0) {
		::close(m_fd);
		m_fd = -1;
	}
}

int SharpSession::get()
{
	return m_fd;
}

Overlay::Overlay(SharpSession& session,
	int x, int y, size_t width, size_t height, unsigned char const* pix)
	: m_session{session}
	, m_storage{overlay_add(session.get(), sharp_overlay_t {
		.x = x, .y = y, .width = (int)width, .height = (int)height,
		.pixels = pix } )}
	, m_display{}
{}

Overlay::Overlay(Overlay&& expiring)
	: m_session{expiring.m_session}
	, m_storage{expiring.m_storage}
	, m_display{expiring.m_display}
{
	expiring.m_storage = nullptr;
	expiring.m_display = nullptr;
}

Overlay::~Overlay()
{
	if (m_display != nullptr) {
		hide();
	}

	if (m_storage != nullptr) {
		overlay_remove(m_session.get(), m_storage);
		m_storage = nullptr;
	}
}

void Overlay::show()
{
	if (m_storage && (m_display == nullptr)) {
		m_display = overlay_show(m_session.get(), m_storage);
	}
}

void Overlay::hide()
{
	if (m_display != nullptr) {
		overlay_hide(m_session.get(), m_display);
		m_display = nullptr;
	}
}

void Overlay::eject()
{
	m_storage = nullptr;
	m_display = nullptr;
}


void Overlay::clear_all(SharpSession& session)
{
	overlay_clear(session.get());
}
