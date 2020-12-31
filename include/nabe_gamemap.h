#ifndef _NABENABE_GAMEMAP_H
#define _NABENABE_GAMEMAP_H

#include "nabe_filesystem.h"

#include <cstring>

// Purpose: Holds map data and the map's file size, or 0 if map couldn't be found.
struct NABE_GameMap {
	NABE_GameMap(const fs::path& map_path, const std::string& map_name) : map_name(map_name)
	{
		// If the map has a bsp extension, strip it away.
		constexpr auto bsp_ext = ".bsp";
		auto bsp_ext_pos = this->map_name.find(bsp_ext);
		if (bsp_ext_pos != std::string::npos) {
			this->map_name.replace(bsp_ext_pos, strlen(bsp_ext), "");
		}

		// Make sure map name doesn't use unsupported characters.
		for (auto& character : this->map_name) {
			if ((!isalnum(character)) && (character != '_') && (character != '-')) {
				print(Error, "%s: Invalid map name: \"%s\" Expected all characters to be alphanumeric, or '_', or '-'.",
					__FUNCTION__, this->map_name);
				if (character == '.') {
					print(Info, "Maybe a stray file extension?");
				}
				return;
			}
		}

		auto full_path = map_path;
		full_path /= map_name;
		full_path += bsp_ext;
		if (fs::exists(full_path)) {
			map_size = fs::file_size(full_path);
		}
	}

	std::string map_name;
	size_t map_size = 0;
};

#endif // _NABENABE_GAMEMAP_H
