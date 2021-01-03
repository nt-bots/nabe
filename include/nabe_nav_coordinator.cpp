#include "nabe_nav_coordinator.h"

#include "nav_parser.h"
#include "nabe_keyvalues.h"
#include "nabe_pathfinder.h"
#include "nabe_filesystem.h"

#include "print_helpers.h"

#include <iostream>
#include <list>

NABE_NavCoordinator::NABE_NavCoordinator(NABE_PathFinder* owner, const std::string& map_name, const char* maps_path, const char* navs_path)
	: m_owner(owner), m_maps_path(maps_path), m_navs_path(navs_path), m_loaded(false)
{
	m_map = new NABE_GameMap(m_owner->GetMapFolderPath(), map_name);
	m_loaded = LoadMapNavData();
}

NABE_NavCoordinator::~NABE_NavCoordinator()
{
	delete m_map;
}

size_t NABE_NavCoordinator::GetBspSize(const std::string& map_name)
{
	if (map_name.empty()) {
		print(Warning, "%s Attempted to get bsp size of empty.", __FUNCTION__);
		return 0;
	}

	auto map_path = m_owner->GetMapFolderPath();
	map_path /= (map_name + ".bsp");

	if (!fs::exists(map_path)) {
		print(Error, "%s: Map path doesn't exist: ", __FUNCTION__);
		std::cerr << map_path << std::endl;
		return 0;
	}
	return fs::file_size(map_path);
}

bool streq(const char* s1, const char* s2)
{
	return (strcmp(s1, s2) == 0);
}

bool strcontains(const char* s1, const char* s2)
{
	return (strstr(s1, s2) != nullptr);
}

bool NABE_NavCoordinator::LoadMapNavData()
{
	if (m_map->map_name.empty()) {
		print(Error, "%s Attempted to load map data of empty.", __FUNCTION__);
		return false;
	}
	else if (m_map->map_size == 0) {
		print(Error, "%s Attempted to load map data without valid bsp size.", __FUNCTION__);
		return false;
	}
	else if (m_loaded) {
		print(Error, "%s Attempted to load map data into an already occupied class instance.", __FUNCTION__);
		return false;
	}

	NABE_KeyValues kv;
	NavParser parser;
	if (!parser.Parse(m_map->map_name.c_str(), m_maps_path, m_navs_path, kv)) {
		print(Error, "%s Map nav data parsing failed.", __FUNCTION__);
		return false;
	}

	// TODO: handle NABE_KeyValues here.
	// need to generate a bunch of NABE_Areas, and store pointers to vector.
	auto skvs = kv.GetSkvs();
	bool exiting_section = false;
	NABE_Area* entry = nullptr;
	bool area_dir_entered[NavDirType::NUM_DIRECTIONS]{ false };
	NavDirType this_dir = NORTH;
	HidingSpot* hiding_spot = nullptr;
	SpotEncounter* encounter_spot = nullptr;
	enum class SpotType { HidingSpot, EncounterSpot, EncounterSpot_SpotAlongThisPath } spot_type = SpotType::HidingSpot;
	SpotOrder* spot_order = nullptr;

	for (auto& p : skvs) {
		// See what kind of node this is
		const bool is_root_node = !(*p.section) && streq(p.key, "nav") && !(*p.value);

		const bool is_section_node = streq(p.section, "nav") && *p.key && !(*p.value);

		const bool is_navigation_areas = streq(p.section, "navigation_areas") && *p.key && !(*p.value);
		const bool is_navigation_areas_nw_corner = *p.section && streq(p.key, "extents_nw_corner") && !(*p.value);
		const bool is_navigation_areas_se_corner = *p.section && streq(p.key, "extents_se_corner") && !(*p.value);
		const bool is_navigation_areas_dirs = *p.section && streq(p.key, "dirs") && !(*p.value);
		const bool is_navigation_areas_dirs_subsection_entry = streq(p.section, "dirs") && *p.key && !(*p.value);
		const bool is_navigation_areas_dirs_subsection_entry_north = streq(p.section, "north") && *p.key && !(*p.value);
		const bool is_navigation_areas_dirs_subsection_entry_east = streq(p.section, "east") && *p.key && !(*p.value);
		const bool is_navigation_areas_dirs_subsection_entry_south = streq(p.section, "south") && *p.key && !(*p.value);
		const bool is_navigation_areas_dirs_subsection_entry_west = streq(p.section, "west") && *p.key && !(*p.value);
		const bool is_navigation_areas_dirs_subsection_connections = streq(p.section, "connects_to_area_id") && *p.key && !(*p.value);
		const bool is_navigation_areas_hiding_spots_entry = *p.section && streq(p.key, "hiding_spots") && !(*p.value);
		const bool is_navigation_areas_hiding_spots = streq(p.section, "hiding_spots") && *p.key && !(*p.value);
		const bool is_navigation_areas_hiding_spot_origin = strcontains(p.section, "spot_") && streq(p.key, "pos") && !(*p.value);
		const bool is_approach_areas_entry = *p.section && streq(p.key, "approach_areas") && !(*p.value);
		const bool is_approach_areas = streq(p.section, "approach_areas") && *p.key && !(*p.value);
		const bool is_encounter_spot_entry = *p.section && streq(p.key, "encounter_spots") && !(*p.value);
		const bool is_encounter_spot = streq(p.section, "encounter_spots") && *p.key && !(*p.value);
		const bool is_encounter_spot_spots_along_path_entry = *p.section && streq(p.key, "spots_along_this_path") && !(*p.value);
		const bool is_encounter_spot_spots_along_path = streq(p.section, "spots_along_this_path") && *p.key && !(*p.value);
		const bool is_ladder_directions_entry = *p.section && streq(p.key, "ladder_directions") && !(*p.value);
		const bool is_ladder_directions = streq(p.section, "ladder_directions") && *p.key && !(*p.value);
		
		const bool is_nav_teams_entry = *p.section && streq(p.key, "nav_teams") && !(*p.value);
		const bool is_nav_teams = streq(p.section, "nav_teams") && *p.key && !(*p.value);

		const bool is_sub_node = *p.section && *p.key && *p.value;

		if (is_navigation_areas) {
			// Commit previous entry, if it exists
			if (entry != nullptr) {
				m_areas.push_back(entry);
				entry = nullptr;
			}
		}
		else if (is_encounter_spot_entry) {
			if (encounter_spot != nullptr) {
				entry->m_spotEncounterList.push_back(encounter_spot);
				encounter_spot = nullptr;
			}
		}
		else if (is_encounter_spot_spots_along_path_entry) {
			if (spot_order != nullptr) {
				if (encounter_spot == nullptr) {
					print(Error, "%s: encounter_spot == nullptr", __FUNCTION__);
					return false;
				}
				encounter_spot->spotList.push_back(*spot_order);
				spot_order = nullptr;
			}
		}

		// Sanity check
		size_t num_bools_hit = 0;
		if (is_root_node) { ++num_bools_hit; }
		if (is_section_node) { ++num_bools_hit; }
		if (is_navigation_areas) { ++num_bools_hit; }
		if (is_navigation_areas_nw_corner) { ++num_bools_hit; }
		if (is_navigation_areas_se_corner) { ++num_bools_hit; }
		if (is_navigation_areas_dirs) { ++num_bools_hit; }
		if (is_navigation_areas_dirs_subsection_entry) { ++num_bools_hit; }
		if (is_navigation_areas_dirs_subsection_entry_north) { ++num_bools_hit; }
		if (is_navigation_areas_dirs_subsection_entry_east) { ++num_bools_hit; }
		if (is_navigation_areas_dirs_subsection_entry_south) { ++num_bools_hit; }
		if (is_navigation_areas_dirs_subsection_entry_west) { ++num_bools_hit; }
		if (is_navigation_areas_dirs_subsection_connections) { ++num_bools_hit; }
		if (is_navigation_areas_hiding_spots_entry) { ++num_bools_hit; }
		if (is_navigation_areas_hiding_spots) { ++num_bools_hit; }
		if (is_navigation_areas_hiding_spot_origin) { ++num_bools_hit; }
		if (is_approach_areas_entry) { ++num_bools_hit; }
		if (is_approach_areas) { ++num_bools_hit; }
		if (is_encounter_spot_entry) { ++num_bools_hit; }
		if (is_encounter_spot) { ++num_bools_hit; }
		if (is_encounter_spot_spots_along_path_entry) { ++num_bools_hit; }
		if (is_encounter_spot_spots_along_path) { ++num_bools_hit; }
		if (is_ladder_directions_entry) { ++num_bools_hit; }
		if (is_ladder_directions) { ++num_bools_hit; }
		if (is_nav_teams_entry) { ++num_bools_hit; }
		if (is_nav_teams) { ++num_bools_hit; }
		if (is_sub_node) { ++num_bools_hit; }
		if (num_bools_hit == 0) {
			print(Error, "%s Unknown kind of node: (%s), (%s), (%s)",
				__FUNCTION__, p.section, p.key, p.value);
			return false;
		}
		else if (num_bools_hit != 1) {
			print(Error, "%s Hit more than one compare bools, this should never happen! (%s), (%s), (%s)",
				__FUNCTION__, p.section, p.key, p.value);
			return false;
		}

		if (is_navigation_areas_dirs_subsection_entry_north) {
			this_dir = NORTH;
		}
		if (is_navigation_areas_dirs_subsection_entry_east) {
			this_dir = EAST;
		}
		else if (is_navigation_areas_dirs_subsection_entry_south) {
			this_dir = SOUTH;
		}
		else if (is_navigation_areas_dirs_subsection_entry_west) {
			this_dir = WEST;
		}

		if (is_navigation_areas_hiding_spots_entry) {
			spot_type = SpotType::HidingSpot;
		}
		else if (is_encounter_spot_entry) {
			spot_type = SpotType::EncounterSpot;
		}
		else if (is_encounter_spot_spots_along_path_entry) {
			spot_type = SpotType::EncounterSpot_SpotAlongThisPath;
		}

		// Don't need to do anything to root nodes
		if (is_root_node) {
			exiting_section = false;
			continue;
		}
		
		// Preparing for a section entry
		else if (is_section_node) {
			exiting_section = false;
			continue;
		}

		// Section entry

		// Meta section
		if (streq(p.section, "meta")) {
			exiting_section = false;
			//print(Info, "%s: %s", p.key, p.value);
			continue;
		}
		// Header section, do some checks on data validity
		else if (streq(p.section, "header")) {
			exiting_section = false;
			if (streq(p.key, "magic")) {
				// This is a string representation of the header hex check (0xFEEDFACE)
				constexpr auto header_magic_number_check = "FEEDFACE";
				if (!streq(p.value, header_magic_number_check)) {
					print(Error, "%s Failed the magic number check.", __FUNCTION__);
					return false;
				}
			}
			else if (streq(p.key, "version")) {
				const auto nav_version = std::stoi(std::string{ p.value });
				if (nav_version <= 0) {
					print(Error, "Invalid nav_version (%d)", nav_version);
					return false;
				}
				const auto supported_nav_versions = { 9, };
				bool is_supported = false;
				for (auto supported_version : supported_nav_versions) {
					if (nav_version == nav_version) {
						is_supported = true;
						break;
					}
				}
				if (!is_supported) {
					print(Error, "%s Nav version %d is not supported.",
						__FUNCTION__, nav_version);
					return false;
				}
			}
			else if (streq(p.key, "bsp_size")) {
				const auto nav_reported_bsp_size = std::stoi(std::string{ p.value });
				if (nav_reported_bsp_size <= 0) {
					print(Error, "%s Invalid nav_reported_bsp_size (%d)", __FUNCTION__, nav_reported_bsp_size);
					return false;
				}
				else if (nav_reported_bsp_size != m_map->map_size) {
					print(Error, "%s BSP size mismatch between parsed nav size (%d bytes) and actual size (%d bytes).",
						__FUNCTION__, nav_reported_bsp_size, m_map->map_size);
					std::cout << "BSP lookup path: " << fs::absolute(m_owner->GetMapFolderPath())
						<< " (map: " << m_map->map_name << ")" << std::endl;
					return false;
				}
			}
			else {
				print(Error, "%s Unrecognized %s section entry: %s", __FUNCTION__, p.section, p.key);
				return false;
			}
			continue;
		}
		else {
			if (entry == nullptr) {
				if (streq(p.section, "navigation_areas")) {
					entry = new NABE_Area();
				}
				//print(Info, "Section: \"%s\" Key: \"%s\" Value: \"%s\"", p.section, p.key, p.value);
			}
			else {
				if (strcontains(p.section, "area_") && streq(p.key, "id")) {
					entry->m_id = std::stoi(std::string{ p.value });
					if (entry->m_id <= 0) {
						print(Error, "Invalid id from data: \"%s\" --> %d",
							p.value, entry->m_id);
						return false;
					}
				}
				else if (streq(p.key, "attribute_flags")) {
					entry->SetAttributes(std::stoi(std::string{ p.value }));
				}
				else if (streq(p.section, "extents_nw_corner")) {
					if (streq(p.key, "origin")) {
						entry->m_extent.lo = Vector(p.value);
					}
					else if (streq(p.key, "implicit_height")) {
						entry->m_neZ = std::stof(std::string{ p.value });
					}
					else {
						print(Error, "Unknown SKV tuple: %s - %s - %s", p.section, p.key, p.value);
						return false;
					}
				}
				else if (streq(p.section, "extents_se_corner")) {
					if (streq(p.key, "origin")) {
						entry->m_extent.hi = Vector(p.value);
					}
					else if (streq(p.key, "implicit_height")) {
						entry->m_swZ = std::stof(std::string{ p.value });
					}
					else {
						print(Error, "Unknown SKV tuple: %s - %s - %s", p.section, p.key, p.value);
						return false;
					}
				}
				else if (streq(p.section, "north") || streq(p.section, "east") ||
					streq(p.section, "south") || streq(p.section, "west"))
				{
					area_dir_entered[this_dir] = true;
					for (size_t i = 0; i < sizeof(area_dir_entered); ++i) {
						if (i == this_dir) {
							continue;
						}
						area_dir_entered[i] = false;
					}
				}
				else if (streq(p.key, "connects_to_area_id")) {
					for (size_t dir = 0; dir < sizeof(area_dir_entered); ++dir) {
						if (dir == this_dir) {
							int id = std::stoi(std::string{ p.value });
							if (id <= 0) {
								print(Error, "%s Invalid id: %d", __FUNCTION__, id);
								return false;
							}
							bool found_area = false;
							for (auto& area : m_areas) {
								if (area->GetID() == id) {
									found_area = true;
									entry->ConnectTo(area, static_cast<NavDirType>(dir));
									break;
								}
							}
							// This area doesn't exist yet, have to try this again once they're all populated
							if (!found_area) {
								switch (dir) {
								case NavDirType::NORTH:
									m_pending_area_connections_north.push_back({ entry, id });
									break;
								case NavDirType::EAST:
									m_pending_area_connections_east.push_back({ entry, id });
									break;
								case NavDirType::SOUTH:
									m_pending_area_connections_south.push_back({ entry, id });
									break;
								case NavDirType::WEST:
									m_pending_area_connections_west.push_back({ entry, id });
									break;
								default:
									print(Error, "%s: Fell through a switch", __FUNCTION__);
									return false;
								}
							}
							break;
						}
					}
				}
				else if (strcontains(p.section, "spot_")) {
					if (spot_type == SpotType::HidingSpot) {
						if (hiding_spot == nullptr) {
							hiding_spot = new HidingSpot();
						}
						if (streq(p.key, "id")) {
							const int spot_id = std::stoi(std::string{ p.value });
							if (spot_id < 0) {
								print(Error, "%s Invalid spot id: %d", __FUNCTION__, spot_id);
								return false;
							}
							hiding_spot->m_id = spot_id;
						}
						else if (streq(p.key, "pos")) {
							// skip; need to go one level deeper for "origin"
						}
						else if (streq(p.key, "flags")) {
							const int flags = std::stoi(std::string{ p.value });
							if (flags < 0) {
								print(Error, "%s Invalid spot flags: %d", __FUNCTION__, flags);
								return false;
							}
							hiding_spot->m_flags = flags;
							// "flags" is the final hiding_spots entry, so push this hiding spot to the list
							entry->m_hidingSpotList.push_back(hiding_spot);
							// Clear the local pointer for a future hiding_spot to use.
							// The hiding_spot list takes ownership of this memory, so we don't have to track it here anymore.
							hiding_spot = nullptr;
						}
						else {
							print(Error, "%s Unknown SKV tuple: %s - %s - %s", p.section, p.key, p.value, __FUNCTION__);
							delete hiding_spot;
							return false;
						}
					}
					else if (spot_type == SpotType::EncounterSpot) {
						if (*p.value) {
							if (encounter_spot == nullptr) {
								encounter_spot = new SpotEncounter();
							}

							const int value = std::stoi(std::string{ p.value });

							if (streq(p.key, "from_area_id")) {
								if (value <= 0) {
									print(Error, "%s Invalid from id: %d", __FUNCTION__, value);
									return false;
								}
								else if (!SetSpotEncounter_FromArea(encounter_spot, value)) {
									bool already_pending = false;
									for (auto& e : m_pending_encounter_spots) {
										if (e.second == encounter_spot) {
											already_pending = true;
											break;
										}
									}
									if (!already_pending) {
										m_pending_encounter_spots.push_back({ entry, encounter_spot });
									}
								}
							}
							else if (streq(p.key, "to_area_id")) {
								if (value <= 0) {
									print(Error, "%s Invalid to id: %d", __FUNCTION__, value);
									return false;
								}
								else if (!SetSpotEncounter_ToArea(encounter_spot, value)) {
									bool already_pending = false;
									for (auto& e : m_pending_encounter_spots) {
										if (e.second == encounter_spot) {
											already_pending = true;
											break;
										}
									}
									if (!already_pending) {
										m_pending_encounter_spots.push_back({ entry, encounter_spot });
									}
								}
							}
							else if (streq(p.key, "from_dir")) {
								encounter_spot->fromDir = static_cast<NavDirType>(value);
							}
							else if (streq(p.key, "to_dir")) {
								encounter_spot->toDir = static_cast<NavDirType>(value);
							}
							else {
								print(Error, "%s Unknown SKV tuple: %s - %s - %s", p.section, p.key, p.value, __FUNCTION__);
								delete encounter_spot;
								delete hiding_spot;
								delete entry;
								return false;
							}
						}
						else {
							if (!streq(p.key, "spots_along_this_path")) {
								print(Error, "%s Unknown SKV tuple: %s - %s", p.section, p.key, __FUNCTION__);
								delete encounter_spot;
								delete hiding_spot;
								delete entry;
								return false;
							}
						}
					}
					// Encounter spot --> Spot along this path
					else {
						if (is_sub_node) {
							if (spot_order == nullptr) {
								spot_order = new SpotOrder();
								spot_order->spot = new HidingSpot();
							}
							if (streq(p.key, "id")) {
								spot_order->spot->m_id = std::stoi(std::string{ p.value });
								spot_order->spot->m_area = entry;
							}
							else if (streq(p.key, "t")) {
								spot_order->t = std::stof(std::string{ p.value });
							}
						}
					}
				}
				else if (hiding_spot != nullptr && is_sub_node) {
					if (streq(p.section, "pos") && streq(p.key, "origin")) {
						Vector origin = p.value;
						hiding_spot->m_pos = origin;
					}
					else {
						print(Error, "%s Unknown SKV tuple: %s - %s - %s", p.section, p.key, p.value, __FUNCTION__);
						delete hiding_spot;
						delete entry;
						return false;
					}
				}
				else if (strcontains(p.section, "area_") && strcontains(p.key, "_area_")) {
					const size_t number_start_index = strlen("area_");
					if (!isdigit(p.section[number_start_index])) {
						print(Error, "Area doesn't start with a positive integer: \"%s\"", p.key);
						return false;
					}

					std::string string_index{ p.section };
					string_index.erase(0, number_start_index);
					const int area_index = std::stoi(string_index);
					const int value = std::stoi(std::string{ p.value });

					if (streq(p.key, "this_area_id")) {
						SetApproachInfo_ThisAreaId(entry, area_index, value);
					}
					else if (streq(p.key, "prev_area_id")) {
						SetApproachInfo_PrevAreaId(entry, area_index, value);
					}
					else if (streq(p.key, "next_area_id")) {
						SetApproachInfo_NextAreaId(entry, area_index, value);
					}
					else if (streq(p.key, "prev_area_to_here_how_type")) {
						entry->m_approach[area_index].prevToHereHow = static_cast<NavTraverseType>(value);
					}
					else if (streq(p.key, "here_to_next_area_how_type")) {
						entry->m_approach[area_index].hereToNextHow = static_cast<NavTraverseType>(value);
					}
					else {
						print(Error, "%s Unknown SKV tuple: %s - %s - %s", p.section, p.key, p.value, __FUNCTION__);
						delete entry;
						return false;
					}
				}
			}
		}
		
		if (exiting_section) {
			if (entry != nullptr) {
				m_areas.push_back(entry);
				// Clear local pointer after push.
				// The area list takes ownership of the memory.
				entry = nullptr;
			}
		}
	}
	if (entry != nullptr) {
		m_areas.push_back(entry);
		entry = nullptr;
	}

	m_pending_area_connections_north.unique();
	for (auto& pending_connection : m_pending_area_connections_north) {
		auto connector = pending_connection.first;
		auto connection = GetAreaById(pending_connection.second);
		if (!connection) {
			print(Error, "%s: Failed to find area by id: %d", __FUNCTION__, pending_connection.second); // TODO: clear memory
			return false;
		}
		connector->ConnectTo(connection, NavDirType::NORTH);
	}
	m_pending_area_connections_north.clear();

	m_pending_area_connections_east.unique();
	for (auto& pending_connection : m_pending_area_connections_east) {
		auto connector = pending_connection.first;
		auto connection = GetAreaById(pending_connection.second);
		if (!connection) {
			print(Error, "%s: Failed to find area by id: %d", __FUNCTION__, pending_connection.second); // TODO: clear memory
			return false;
		}
		connector->ConnectTo(connection, NavDirType::EAST);
	}
	m_pending_area_connections_east.clear();

	m_pending_area_connections_south.unique();
	for (auto& pending_connection : m_pending_area_connections_south) {
		auto connector = pending_connection.first;
		auto connection = GetAreaById(pending_connection.second);
		if (!connection) {
			print(Error, "%s: Failed to find area by id: %d", __FUNCTION__, pending_connection.second); // TODO: clear memory
			return false;
		}
		connector->ConnectTo(connection, NavDirType::SOUTH);
	}
	m_pending_area_connections_south.clear();

	m_pending_area_connections_west.unique();
	for (auto& pending_connection : m_pending_area_connections_west) {
		auto connector = pending_connection.first;
		auto connection = GetAreaById(pending_connection.second);
		if (!connection) {
			print(Error, "%s: Failed to find area by id: %d", __FUNCTION__, pending_connection.second); // TODO: clear memory
			return false;
		}
		connector->ConnectTo(connection, NavDirType::WEST);
	}
	m_pending_area_connections_west.clear();

	// TODO: encounter spots
	m_pending_encounter_spots.unique();
	for (auto& pending_enc_spot : m_pending_encounter_spots) {
		auto enc = pending_enc_spot.second;
		if (enc->m_pending_from_connect != AREA_ID_NONE) {
			auto from = GetAreaById(enc->m_pending_from_connect);
			if (!from) {
				print(Error, "%s: Failed to get pending \"from\" area by id: %d",
					__FUNCTION__, enc->m_pending_from_connect);
				return false; // TODO: clear memory
			}
			NavConnect from_conn;
			from_conn.area = from;
			from_conn.id = from->GetID();
			enc->from = from_conn;
			enc->m_pending_from_connect = AREA_ID_NONE;
		}
		if (enc->m_pending_to_connect != AREA_ID_NONE) {
			auto to = GetAreaById(enc->m_pending_to_connect);
			if (!to) {
				print(Error, "%s: Failed to get pending \"to\" area by id: %d",
					__FUNCTION__, enc->m_pending_to_connect);
				return false; // TODO: clear memory
			}
			NavConnect to_conn;
			to_conn.area = to;
			to_conn.id = to->GetID();
			enc->from = to_conn;
			enc->m_pending_to_connect = AREA_ID_NONE;
		}
	}
	m_pending_encounter_spots.clear();

	for (auto& area : m_areas) {
		area->CalculateCenter();
	}

	return true;
}

bool NABE_NavCoordinator::SetApproachInfo_ThisAreaId(NABE_Area* target, const size_t area_n, const int id)
{
	int target_id;

	if (area_n >= MAX_APPROACH_AREAS) {
		print(Error, "%s: Out of bounds area index: %d", __FUNCTION__, area_n);
		return false;
	}
	else if (id < 0) {
		if (id != AREA_ID_NONE) {
			print(Error, "%s: Invalid approach id: %d", __FUNCTION__, id);
			return false;
		}
		else {
			if (target->m_pending_approaches_this_area_id[area_n] == AREA_ID_NONE) {
				print(Error, "%s: Both approach id and pending id are zeroed", __FUNCTION__);
				return false;
			}
			target_id = target->m_pending_approaches_this_area_id[area_n];
		}
	}
	else {
		target_id = id;
	}

	NavConnect conn;
	for (auto& p : m_areas) {
		if (p->GetID() == target_id) {
			conn.area = p;
			conn.id = target_id;
			target->m_approach[area_n].here = conn;
			target->m_pending_approaches_this_area_id[area_n] = AREA_ID_NONE;
			return true;
		}
	}

	if (target->m_pending_approaches_this_area_id[area_n] == AREA_ID_NONE) {
		target->m_pending_approaches_this_area_id[area_n] = id;
	}
	else if (target->m_pending_approaches_this_area_id[area_n] != target_id) {
		print(Error, "%s: Overlapping assignment to approach areas[%d]", __FUNCTION__, area_n);
	}
	return false;
}

bool NABE_NavCoordinator::SetApproachInfo_PrevAreaId(NABE_Area* target, const size_t area_n, const int id)
{
	int target_id;

	if (area_n >= MAX_APPROACH_AREAS) {
		print(Error, "%s: Out of bounds area index: %d", __FUNCTION__, area_n);
		return false;
	}
	else if (id < 0) {
		if (id != AREA_ID_NONE) {
			print(Error, "%s: Invalid approach id: %d", __FUNCTION__, id);
			return false;
		}
		else {
			if (target->m_pending_approaches_prev_area_id[area_n] == AREA_ID_NONE) {
				print(Error, "%s: Both approach id and pending id are zeroed", __FUNCTION__);
				return false;
			}
			target_id = target->m_pending_approaches_prev_area_id[area_n];
		}
	}
	else {
		target_id = id;
	}

	NavConnect conn;
	for (auto& p : m_areas) {
		if (p->GetID() == target_id) {
			conn.area = p;
			conn.id = target_id;
			target->m_approach[area_n].prev = conn;
			target->m_pending_approaches_prev_area_id[area_n] = AREA_ID_NONE;
			return true;
		}
	}

	if (target->m_pending_approaches_prev_area_id[area_n] == AREA_ID_NONE) {
		target->m_pending_approaches_prev_area_id[area_n] = id;
	}
	else if (target->m_pending_approaches_prev_area_id[area_n] != target_id) {
		print(Error, "%s: Overlapping assignment to approach areas[%d]", __FUNCTION__, area_n);
	}
	return false;
}

bool NABE_NavCoordinator::SetApproachInfo_NextAreaId(NABE_Area* target, const size_t area_n, const int id)
{
	int target_id;

	if (area_n >= MAX_APPROACH_AREAS) {
		print(Error, "%s: Out of bounds area index: %d", __FUNCTION__, area_n);
		return false;
	}
	else if (id < 0) {
		if (id != AREA_ID_NONE) {
			print(Error, "%s: Invalid approach id: %d", __FUNCTION__, id);
			return false;
		}
		else {
			if (target->m_pending_approaches_next_area_id[area_n] == AREA_ID_NONE) {
				print(Error, "%s: Both approach id and pending id are zeroed", __FUNCTION__);
				return false;
			}
			target_id = target->m_pending_approaches_next_area_id[area_n];
		}
	}
	else {
		target_id = id;
	}

	NavConnect conn;
	for (auto& p : m_areas) {
		if (p->GetID() == target_id) {
			conn.area = p;
			conn.id = target_id;
			target->m_approach[area_n].next = conn;
			target->m_pending_approaches_next_area_id[area_n] = AREA_ID_NONE;
			return true;
		}
	}

	if (target->m_pending_approaches_next_area_id[area_n] == AREA_ID_NONE) {
		target->m_pending_approaches_next_area_id[area_n] = id;
	}
	else if (target->m_pending_approaches_next_area_id[area_n] != target_id) {
		print(Error, "%s: Overlapping assignment to approach areas[%d]", __FUNCTION__, area_n);
	}
	return false;
}

bool NABE_NavCoordinator::SetSpotEncounter_FromArea(SpotEncounter* target, const int id)
{
	int target_id;

	if (id < 0) {
		if (id != AREA_ID_NONE) {
			print(Error, "%s: Invalid encounter id: %d", __FUNCTION__, id);
			return false;
		}
		else {
			if (target->m_pending_from_connect == AREA_ID_NONE) {
				print(Error, "%s: Both encounter id and pending id are zeroed", __FUNCTION__);
				return false;
			}
			target_id = target->m_pending_from_connect;
		}
	}
	else {
		target_id = id;
	}

	NavConnect conn;
	for (auto& p : m_areas) {
		if (p->GetID() == target_id) {
			conn.area = p;
			conn.id = target_id;
			target->from = conn;
			target->m_pending_from_connect = AREA_ID_NONE;
			return true;
		}
	}

	if (target->m_pending_from_connect == AREA_ID_NONE) {
		target->m_pending_from_connect = target_id;
	}
	else if (target->m_pending_from_connect != target_id) {
		print(Error, "%s: Overlapping assignment to spot encounter", __FUNCTION__);
	}
	return false;
}

bool NABE_NavCoordinator::SetSpotEncounter_ToArea(SpotEncounter* target, const int id)
{
	int target_id;

	if (id < 0) {
		if (id != AREA_ID_NONE) {
			print(Error, "%s: Invalid encounter id: %d", __FUNCTION__, id);
			return false;
		}
		else {
			if (target->m_pending_to_connect == AREA_ID_NONE) {
				print(Error, "%s: Both encounter id and pending id are zeroed", __FUNCTION__);
				return false;
			}
			target_id = target->m_pending_to_connect;
		}
	}
	else {
		target_id = id;
	}

	NavConnect conn;
	for (auto& p : m_areas) {
		if (p->GetID() == target_id) {
			conn.area = p;
			conn.id = target_id;
			target->to = conn;
			target->m_pending_to_connect = AREA_ID_NONE;
			return true;
		}
	}

	if (target->m_pending_to_connect == AREA_ID_NONE) {
		target->m_pending_to_connect = target_id;
	}
	else if (target->m_pending_to_connect != target_id) {
		print(Error, "%s: Overlapping assignment to spot encounter", __FUNCTION__);
	}
	return false;
}

NABE_Area* NABE_NavCoordinator::GetAreaById(const int id)
{
	for (auto& area : m_areas) {
		if (area->GetID() == id) {
			return area;
		}
	}
	return nullptr;
}

NABE_Area* NABE_NavCoordinator::GetAreaByPos(const Vector& pos)
{
	NABE_Area* closest = nullptr;
	float closest_distance;

	for (auto& area : m_areas) {
		const auto distance = area->GetCenter().DistTo(pos);
		if (!closest || distance < closest_distance) {
			closest = area;
			closest_distance = distance;
		}
	}
	return closest;
}