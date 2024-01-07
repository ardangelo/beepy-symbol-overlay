#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <string>
#include <stdexcept>

#include "overlay.hpp"
#include "psf.hpp"

static auto const default_keymap_path = "/usr/share/kbd/keymaps/beepy-kbd.map";

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

	unsigned char pix[20 * 20] = {};
	draw_psf1_character("Uni1-VGA16.psf", 0xAE, pix, 20, 20, 0, 0);

	auto session = SharpSession{sharp_dev};
	auto overlay = Overlay{session, 10, 10, 20, 20, pix};

	overlay.show();
	sleep(5);
	overlay.hide();

	return 0;
}
