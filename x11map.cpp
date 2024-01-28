#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <unordered_map>

extern "C" {
#include <X11/Xlib.h>

#include <xkbcommon/xkbcommon.h>

#include "libxkbcommon/src/ks_tables.h"
}

/*
00002000: 0000 0000 a900 ffff 2601 ffff 3501 ffff  ........&...5...
00002010: 0204 ffff 6626 c825 fdff ffff 0904 ffff  ....f&.%........
*/

static void gen_line(std::string const& name)
{
	static auto first = true;

	auto ks = XStringToKeysym(name.c_str());
	if (ks == NoSymbol) {
		return;
	}

	auto utf32 = xkb_keysym_to_utf32(ks);
	if ((0 < utf32) && (utf32 <= 0xffff)) {
		printf("%s { \"%s\", 0x%x }\n",
			(first) ? "{"  : ",",
			name.c_str(), utf32);
		first = false;
	}
}

int main(int argc, char** argv)
{
	printf("#pragma once\n");
	printf("#include <unordered_map>\n");
	printf("static auto x11name_to_utf16 = std::unordered_map<std::string, uint16_t>\n");

	// name1\0name2\0\0
	auto str = std::string{};
	auto cursor = keysym_names;
	while (true) {
		if (*cursor) {
			str.push_back(*cursor);
			cursor++;

		} else {
			gen_line(str);
			str = {};

			cursor++;
			if (*cursor == '\0') {
				break;
			}
		}
	}

	printf("};\n");

	return 0;
}
