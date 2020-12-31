#!/usr/bin/env python3

from __future__ import print_function
import builtins as __builtin__
def print(*args, **kwargs):
    """Overridden print() function for this script.
    Toggle the global PRINT_DEBUG to decide whether to print output."""
    return None if not PRINT_DEBUG else __builtin__.print(*args, **kwargs)

import os
from struct import unpack

# https://github.com/gorgitko/valve-keyvalues-python
from valve-keyvalues-python/valve_keyvalues_python.keyvalues import KeyValues

# Whether print(...) lines in this file should produce standard output.
PRINT_DEBUG = False

# Please note that this version string is intended to be used for tracking format changes,
# so changing it may have side effects on parsing.
NEO_BOT_FILE_VERSION = "0.2"
NEO_BOT_FILE_DESCRIPTION = "Neotokyo bot navigation file"

IS_LITTLE_ENDIAN = True
ENDIANNESS = "" if IS_LITTLE_ENDIAN else ">"

# Flip this to silence the custom area data nag, if you're certain it's not an issue for your use case.
SUPPRESS_CUSTOM_DATA_WARNING = False

def nav_parse(map, map_path, nav_path):

    kv = KeyValues()
    
    kv["nav"] = dict()
    kv["nav"]["meta"] = dict()
    kv["nav"]["meta"]["description"] = NEO_BOT_FILE_DESCRIPTION
    kv["nav"]["meta"]["version"] = NEO_BOT_FILE_VERSION
    
    kv["nav"]["header"] = dict()

    with open(nav_path, mode="rb") as nav_file:
        NAV_FORMAT_HEADER_ID = 0xFEEDFACE
        magic = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
        print("Magic number: %X" % magic)
        assert magic == NAV_FORMAT_HEADER_ID
        kv["nav"]["header"]["magic"] = "%X" % magic
        
        version = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
        print("Nav version: %d" % version)
        assert version >= 1
        kv["nav"]["header"]["version"] = "%d" % version
        
        if version < 6:
            print("NOTE: This parser was written for Source; if you get broken results, " +\
                "double check the parsing matches your GoldSrc version structs!")
        
        if version >= 10:
            subversion = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
            print("Sub-version number: %d" % subversion)
            kv["nav"]["header"]["subversion"] = "%d" % subversion
        
        if version >= 4:
            bsp_size = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
            print("Nav file \"%s\" is built for a BSP of size: %d bytes" % (map, bsp_size))
            assert bsp_size == os.stat(map_path).st_size, \
                "BSP expected size (%d bytes) didn't match its actual size (%d bytes) for file: \"%s\"" \
                % (bsp_size, os.stat(map_path).st_size, map_path)
            kv["nav"]["header"]["bsp_size"] = "%d" % bsp_size
        
        if version >= 14:
            is_analyzed = unpack(ENDIANNESS + "B", nav_file.read(1))[0]
            print("Is analyzed: %r" % is_analyzed)
            kv["nav"]["header"]["is_analyzed"] = "%d" % is_analyzed
        
        if version >= 5:
            kv["nav"]["places"] = dict()
            
            num_places = unpack(ENDIANNESS + "H", nav_file.read(2))[0]
            print("Num places: %d" % num_places)
            
            for place in range(num_places):
                place_name_len = unpack(ENDIANNESS + "H", nav_file.read(2))[0]
                place_name = unpack(ENDIANNESS + "s", nav_file.read(place_name_len))[0]
                kv["nav"]["places"]["place_%d" % place] = dict()
                kv["nav"]["places"]["place_%d" % place]["place_name"] = "%s" % place_name
            if version >= 12:
                has_unnamed_areas = unpack(ENDIANNESS + "B", nav_file.read(1))[0]
                print("Has unnamed areas: %r" % has_unnamed_areas)
                kv["nav"]["places"]["has_unnamed_areas"] = "%d" % has_unnamed_areas
        
        if version >= 10 and not SUPPRESS_CUSTOM_DATA_WARNING:
            print("\nNOTE: Version of this nav file is %d, and versions 10 and higher support custom area data." % version)
            print("If such custom data exists, you may have to implement it here manually for your game to ensure correct parsing.")
            print("(You can silence this warning with the SUPPRESS_CUSTOM_DATA_WARNING global.)\n")
        
        # TODO: confirm that this stuff below conforms with version 10 and higher.
        # NT is using v.9, so don't have to bother with it, for now.
        
        kv["nav"]["navigation_areas"] = dict()
        
        num_navigation_areas = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
        print("Num navigation areas: %d" % num_navigation_areas)
        for nav_area in range(num_navigation_areas):
            kv_area_id = "area_%d" % nav_area
            kv["nav"]["navigation_areas"][kv_area_id] = dict()
            
            print("Navigation area %d:" % nav_area)
            nav_area_id = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
            kv["nav"]["navigation_areas"][kv_area_id]["id"] = "%d" % nav_area_id
            if version >= 13:
                nav_attribute_flags = unpack(ENDIANNESS + "i", nav_file.read(4))[0]
            elif version >= 9:
                nav_attribute_flags = unpack(ENDIANNESS + "H", nav_file.read(2))[0]
            else:
                nav_attribute_flags = unpack(ENDIANNESS + "B", nav_file.read(1))[0]
            kv["nav"]["navigation_areas"][kv_area_id]["attribute_flags"] = "%d" % nav_attribute_flags
            
            # Extent of the area, expressed as the NW and SE corners (X/Y/Z)
            nav_area_extents_nw_corner = [
                unpack(ENDIANNESS + "f", nav_file.read(4))[0],
                unpack(ENDIANNESS + "f", nav_file.read(4))[0],
                unpack(ENDIANNESS + "f", nav_file.read(4))[0]
            ]
            nav_area_extents_se_corner = [
                unpack(ENDIANNESS + "f", nav_file.read(4))[0],
                unpack(ENDIANNESS + "f", nav_file.read(4))[0],
                unpack(ENDIANNESS + "f", nav_file.read(4))[0]
            ]
            # Heights of implicit corners
            nav_area_extents_nw_corner.append(unpack(ENDIANNESS + "f", nav_file.read(4))[0])
            nav_area_extents_se_corner.append(unpack(ENDIANNESS + "f", nav_file.read(4))[0])
            
            kv["nav"]["navigation_areas"][kv_area_id]["extents_nw_corner"] = dict()
            kv["nav"]["navigation_areas"][kv_area_id]["extents_nw_corner"]["origin"] = "%f %f %f" % (nav_area_extents_nw_corner[0], nav_area_extents_nw_corner[1], nav_area_extents_nw_corner[2])
            kv["nav"]["navigation_areas"][kv_area_id]["extents_nw_corner"]["implicit_height"] = "%f" % nav_area_extents_nw_corner[3]
            
            kv["nav"]["navigation_areas"][kv_area_id]["extents_se_corner"] = dict()
            kv["nav"]["navigation_areas"][kv_area_id]["extents_se_corner"]["origin"] = "%f %f %f" % (nav_area_extents_se_corner[0], nav_area_extents_se_corner[1], nav_area_extents_se_corner[2])
            kv["nav"]["navigation_areas"][kv_area_id]["extents_se_corner"]["implicit_height"] = "%f" % nav_area_extents_se_corner[3]
            
            # Connections of adjacent areas (order: north, east, south, west)
            kv["nav"]["navigation_areas"][kv_area_id]["dirs"] = dict()
            kv_dir_ids = [ "north", "east", "south", "west" ]
            for dir in range(4):
                kv_dir_id = "%s" % kv_dir_ids[dir]
                kv["nav"]["navigation_areas"][kv_area_id]["dirs"][kv_dir_id] = dict()
                num_connections = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
                for connection in range(num_connections):
                    kv["nav"]["navigation_areas"][kv_area_id]["dirs"][kv_dir_id]["connection_%d" % connection] = dict()
                    conn_id = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
                    kv["nav"]["navigation_areas"][kv_area_id]["dirs"][kv_dir_id]["connection_%d" % connection]\
                        ["connects_to_area_id"] = "%d" % conn_id
                    
                    print("Direction %d, connection %d/%d has id: %d" % (dir, connection + 1, num_connections, conn_id))
            print("- - - - -")
            
            kv["nav"]["navigation_areas"][kv_area_id]["hiding_spots"] = dict()
            num_hiding_spots = unpack(ENDIANNESS + "B", nav_file.read(1))[0]
            for hiding_spot in range(num_hiding_spots):
                kv["nav"]["navigation_areas"][kv_area_id]["hiding_spots"]["spot_%d" % hiding_spot] = dict()
                
                hiding_spot_id = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
                hiding_spot_pos = [
                    unpack(ENDIANNESS + "f", nav_file.read(4))[0],
                    unpack(ENDIANNESS + "f", nav_file.read(4))[0],
                    unpack(ENDIANNESS + "f", nav_file.read(4))[0]
                ]
                hiding_spot_flags = unpack(ENDIANNESS + "B", nav_file.read(1))[0]
                
                kv["nav"]["navigation_areas"][kv_area_id]["hiding_spots"]["spot_%d" % hiding_spot]["id"] = "%d" % hiding_spot_id
                kv["nav"]["navigation_areas"][kv_area_id]["hiding_spots"]["spot_%d" % hiding_spot]["pos"] = dict()
                kv["nav"]["navigation_areas"][kv_area_id]["hiding_spots"]["spot_%d" % hiding_spot]["pos"]["origin"] = "%f %f %f" % (hiding_spot_pos[0], hiding_spot_pos[1], hiding_spot_pos[2])
                kv["nav"]["navigation_areas"][kv_area_id]["hiding_spots"]["spot_%d" % hiding_spot]["flags"] = "%d" % hiding_spot_flags
                
                print("Hiding spot %d at %f %f %f has id %d and flags %d" % (hiding_spot, hiding_spot_pos[0], hiding_spot_pos[1], hiding_spot_pos[2],
                    hiding_spot_id, hiding_spot_flags))
            
            kv["nav"]["navigation_areas"][kv_area_id]["approach_areas"] = dict()
            num_approach_areas = unpack(ENDIANNESS + "B", nav_file.read(1))[0]
            for approach_area in range(num_approach_areas):
                kv["nav"]["navigation_areas"][kv_area_id]["approach_areas"]["area_%d" % approach_area] = dict()
                
                area_id = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
                prev_area_id = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
                prev_area_to_here_how_type = unpack(ENDIANNESS + "B", nav_file.read(1))[0]
                next_area_id = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
                here_to_next_area_how_type = unpack(ENDIANNESS + "B", nav_file.read(1))[0]
                
                kv["nav"]["navigation_areas"][kv_area_id]["approach_areas"]["area_%d" % approach_area]["this_area_id"] = "%d" % area_id
                kv["nav"]["navigation_areas"][kv_area_id]["approach_areas"]["area_%d" % approach_area]["prev_area_id"] = "%d" % prev_area_id
                kv["nav"]["navigation_areas"][kv_area_id]["approach_areas"]["area_%d" % approach_area]["prev_area_to_here_how_type"] = "%d" % prev_area_to_here_how_type
                kv["nav"]["navigation_areas"][kv_area_id]["approach_areas"]["area_%d" % approach_area]["next_area_id"] = "%d" % next_area_id
                kv["nav"]["navigation_areas"][kv_area_id]["approach_areas"]["area_%d" % approach_area]["here_to_next_area_how_type"] = "%d" % here_to_next_area_how_type
            
            kv["nav"]["navigation_areas"][kv_area_id]["encounter_spots"] = dict()
            num_encounter_spots = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
            for encounter_spot in range(num_encounter_spots):
                kv["nav"]["navigation_areas"][kv_area_id]["encounter_spots"]["spot_%d" % encounter_spot] = dict()
                
                from_area_id = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
                from_dir = unpack(ENDIANNESS + "B", nav_file.read(1))[0]
                to_area_id = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
                to_dir = unpack(ENDIANNESS + "B", nav_file.read(1))[0]
                num_spots_along_this_path = unpack(ENDIANNESS + "B", nav_file.read(1))[0]
                
                kv["nav"]["navigation_areas"][kv_area_id]["encounter_spots"]["spot_%d" % encounter_spot]["from_area_id"] = "%d" % from_area_id
                kv["nav"]["navigation_areas"][kv_area_id]["encounter_spots"]["spot_%d" % encounter_spot]["from_dir"] = "%d" % from_dir
                kv["nav"]["navigation_areas"][kv_area_id]["encounter_spots"]["spot_%d" % encounter_spot]["to_area_id"] = "%d" % to_area_id
                kv["nav"]["navigation_areas"][kv_area_id]["encounter_spots"]["spot_%d" % encounter_spot]["to_dir"] = "%d" % to_dir
                
                kv["nav"]["navigation_areas"][kv_area_id]["encounter_spots"]["spot_%d" % encounter_spot]["spots_along_this_path"] = dict()
                for spot_along_this_path in range(num_spots_along_this_path):
                    kv["nav"]["navigation_areas"][kv_area_id]["encounter_spots"]["spot_%d" % encounter_spot]["spots_along_this_path"]["spot_%d" % spot_along_this_path] = dict()
                
                    spot_id = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
                    t = unpack(ENDIANNESS + "B", nav_file.read(1))[0] # TODO: is this t a "type"?
                    
                    kv["nav"]["navigation_areas"][kv_area_id]["encounter_spots"]["spot_%d" % encounter_spot]["spots_along_this_path"]["spot_%d" % spot_along_this_path]["id"] = "%d" % spot_id
                    kv["nav"]["navigation_areas"][kv_area_id]["encounter_spots"]["spot_%d" % encounter_spot]["spots_along_this_path"]["spot_%d" % spot_along_this_path]["t"] = "%d" % t # TODO: is this t a "type"?
            
            place_dictionary_entry = unpack(ENDIANNESS + "H", nav_file.read(2))[0]
            kv["nav"]["navigation_areas"][kv_area_id]["place_dictionary_entry"] = "%d" % place_dictionary_entry
            
            # Ladder directions (order: up, down)
            kv["nav"]["navigation_areas"][kv_area_id]["ladder_directions"] = dict()
            kv_ladder_dir_names = [ "up", "down" ]
            for ladder_direction in range(2):
                kv["nav"]["navigation_areas"][kv_area_id]["ladder_directions"][kv_ladder_dir_names[ladder_direction]] = dict()
                num_ladders = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
                for ladder in range(num_ladders):
                    kv["nav"]["navigation_areas"][kv_area_id]["ladder_directions"][kv_ladder_dir_names[ladder_direction]]["ladder_%d" % ladder] = dict()
                    ladder_id = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
                    kv["nav"]["navigation_areas"][kv_area_id]["ladder_directions"][kv_ladder_dir_names[ladder_direction]]["ladder_%d" % ladder]["id"] = "%d" % ladder_id
            
            kv["nav"]["navigation_areas"][kv_area_id]["nav_teams"] = dict()
            MAX_NAV_TEAMS = 2
            for team in range(MAX_NAV_TEAMS):
                kv["nav"]["navigation_areas"][kv_area_id]["nav_teams"]["team_%d" % team] = dict()
                earliest_occupy_time = unpack(ENDIANNESS + "f", nav_file.read(4))[0]
                kv["nav"]["navigation_areas"][kv_area_id]["nav_teams"]["team_%d" % team]["earliest_occupy_time"] = "%f" % earliest_occupy_time
        
        num_ladders = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
        print("Iterating main ladder list (%d total)..." % num_ladders)
        
        kv["nav"]["ladders"] = dict()
        
        for ladder in range(num_ladders):
            kv["nav"]["ladders"]["ladder_%d" % ladder] = dict()
            
            ladder_id = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
            ladder_width = unpack(ENDIANNESS + "f", nav_file.read(4))[0]
            ladder_top_endpoint = [
                unpack(ENDIANNESS + "f", nav_file.read(4))[0],
                unpack(ENDIANNESS + "f", nav_file.read(4))[0],
                unpack(ENDIANNESS + "f", nav_file.read(4))[0]
            ]
            ladder_bottom_endpoint = [
                unpack(ENDIANNESS + "f", nav_file.read(4))[0],
                unpack(ENDIANNESS + "f", nav_file.read(4))[0],
                unpack(ENDIANNESS + "f", nav_file.read(4))[0]
            ]
            ladder_length = unpack(ENDIANNESS + "f", nav_file.read(4))[0]
            ladder_dir = unpack(ENDIANNESS + "I", nav_file.read(4))[0] # assuming the NavDir enum gets compiled as uint32
            ladder_id_top_forward_area = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
            ladder_id_top_left_area = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
            ladder_id_top_behind_area = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
            ladder_id_bottom_area = unpack(ENDIANNESS + "I", nav_file.read(4))[0]
            
            kv["nav"]["ladders"]["ladder_%d" % ladder]["id"] = "%d" % ladder_id
            kv["nav"]["ladders"]["ladder_%d" % ladder]["width"] = "%f" % ladder_width
            kv["nav"]["ladders"]["ladder_%d" % ladder]["top_endpoint"] = dict()
            kv["nav"]["ladders"]["ladder_%d" % ladder]["top_endpoint"]["origin"] = "%f %f %f" % (ladder_top_endpoint[0], ladder_top_endpoint[1], ladder_top_endpoint[2])
            kv["nav"]["ladders"]["ladder_%d" % ladder]["bottom_endpoint"] = dict()
            kv["nav"]["ladders"]["ladder_%d" % ladder]["bottom_endpoint"]["origin"] = "%f %f %f" % (ladder_bottom_endpoint[0], ladder_bottom_endpoint[1], ladder_bottom_endpoint[2])
            kv["nav"]["ladders"]["ladder_%d" % ladder]["length"] = "%f" % ladder_length
            kv["nav"]["ladders"]["ladder_%d" % ladder]["dir"] = "%d" % ladder_dir
            kv["nav"]["ladders"]["ladder_%d" % ladder]["id_of_top_forward_area"] = "%d" % ladder_id_top_forward_area
            kv["nav"]["ladders"]["ladder_%d" % ladder]["id_of_top_left_area"] = "%d" % ladder_id_top_left_area
            kv["nav"]["ladders"]["ladder_%d" % ladder]["id_of_top_behind_area"] = "%d" % ladder_id_top_behind_area
            kv["nav"]["ladders"]["ladder_%d" % ladder]["id_of_bottom_area"] = "%d" % ladder_id_bottom_area
        
        print("Finished parsing nav.")
        kv["nav"]["meta"]["nav_parse_successful"] = "%d" % True
        return kv

def nav_parse_c_bridge(map_name, maps_path, navs_path):
    map_full_path = os.path.join(maps_path, map_name + ".bsp")
    nav_full_path = os.path.join(navs_path, map_name + ".nav")
    
    assert os.path.isfile(map_full_path), "Couldn't find BSP file: \"%s\"" % map_full_path
    assert os.path.isfile(nav_full_path), "Couldn't find NAV file: \"%s\"" % nav_full_path
    
    kv = nav_parse(map_name, map_full_path, nav_full_path)
    return kv
    #for name, value in vars(kv).iteritems():
    #    print("setting descendent", name, ": ", value, "ancestor", name)
    
    #list = [
    #    
    #    kv["nav"]["meta"]["description"],
    #    kv["nav"]["meta"]["version"],
    #]
    #return list

def main(map_name, maps_path, navs_path):
    map_full_path = os.path.join(maps_path, map_name + ".bsp")
    nav_full_path = os.path.join(navs_path, map_name + ".nav")
    
    assert os.path.isfile(map_full_path), "Couldn't find BSP file: \"%s\"" % map_full_path
    assert os.path.isfile(nav_full_path), "Couldn't find NAV file: \"%s\"" % nav_full_path
    
    kv = nav_parse(map_name, map_full_path, nav_full_path)
    print("KeyValues:")
    kv.dump()
    
    kv_path = os.path.join(NAVS_PATH, map_name + ".kv")
    kv.write(kv_path)
    
    assert os.path.isfile(kv_path)

#cdef public int nav_parser_c_entry_point(out_kv) except -1:
#    print("We're in Python code now")
#    out_kv = nav_parse("nt_test")
#    print("And returning")
#    return 0

print("Python entry.")

# TODO: accept params
if __name__ == '__main__':
    main("nt_test", "foo", "bar")
