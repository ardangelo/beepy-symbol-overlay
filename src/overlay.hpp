#pragma once

#include <memory>

class SharpSession
{
private: // members
	int m_fd;

public: // interface
	SharpSession(char const* sharp_dev);
	SharpSession(SharpSession&& expiring);
	~SharpSession();

	int get();
};

class Overlay
{
private: // members
	SharpSession &m_session;
	void *m_storage, *m_display;

public: // interface
	Overlay(SharpSession& session,
		int x, int y, int width, int height, unsigned char const* pix);
	Overlay(Overlay&& expiring);
	~Overlay();

	void show();
	void hide();
};
