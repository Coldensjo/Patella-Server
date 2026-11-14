# patella-convert, which turns the original installation's files into the TOML
# files the game reads (convert.cc), and its tests. It reads each original file
# with the game's own reader and writes what the game ended up with, so it
# compiles the game units those readers need, with the game's flags. The
# player files (usr_files.cc, player_records.cc, usr_commands.cc; also for
# querymanager-seed's import) need the game's own player data code (dbfuncs.cc,
# query.cc) and the units it uses; Log and CleanHouseField, of the game units
# left out, are stubs (game_stubs.cc).

set(GAME_SOURCE ${PROJECT_SOURCE_DIR}/src)
find_package(Threads REQUIRED)

add_library(convert_common STATIC
	tibia_config.cc
	config_writer.cc
	objects_srv.cc
	items_writer.cc
	circles_dat.cc
	circles_writer.cc
	spells_reference.cc
	spells_writer.cc
	moveuse_dat.cc
	moveuse_writer.cc
	houses_dat.cc
	houses_writer.cc
	map_dat.cc
	map_writer.cc
	conversion_lst.cc
	conversion_writer.cc
	tomlwrite.cc
	usr_files.cc
	player_records.cc
	usr_commands.cc
	game_stubs.cc
	${GAME_SOURCE}/config.cc
	${GAME_SOURCE}/housesdata.cc
	${GAME_SOURCE}/magicdata.cc
	${GAME_SOURCE}/mapdata.cc
	${GAME_SOURCE}/moveusedata.cc
	${GAME_SOURCE}/objects.cc
	${GAME_SOURCE}/strings.cc
	${GAME_SOURCE}/tomlfile.cc
	${GAME_SOURCE}/script.cc
	${GAME_SOURCE}/utils.cc
	${GAME_SOURCE}/mudinc.cc
	${GAME_SOURCE}/dbfuncs.cc
	${GAME_SOURCE}/query.cc
	${GAME_SOURCE}/map.cc
	${GAME_SOURCE}/threads.cc
	${GAME_SOURCE}/time.cc
	${GAME_SOURCE}/shm.cc
)
target_include_directories(convert_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${GAME_SOURCE})
target_link_libraries(convert_common PUBLIC game_flags tomlplusplus Threads::Threads)

add_executable(patella-convert convert.cc)
target_link_libraries(patella-convert PRIVATE convert_common)

# The round trip of the config: what the game ends up with from a .tibia
# (through the old reader) and from its config.toml (through LoadConfig) is
# the same. convert_config uses made-up files; convert_config_original the
# original .tibia, PATELLA_ORIGINAL_TIBIA, and the committed data/config.toml,
# which must also be exactly what patella-convert makes of it. It is skipped
# when that .tibia is not there.
set(PATELLA_ORIGINAL_TIBIA "/mnt/c/Tibia/Patella-Gameserver/.tibia" CACHE FILEPATH
	"The original installation's .tibia, for the test convert_config_original")
add_executable(convert_test_config tests/test_config.cc)
target_link_libraries(convert_test_config PRIVATE convert_common)
add_test(NAME convert_config COMMAND convert_test_config)
add_test(NAME convert_config_original COMMAND convert_test_config --original ${PATELLA_ORIGINAL_TIBIA}
	${PROJECT_SOURCE_DIR}/data/config.toml /game)
set_tests_properties(convert_config_original PROPERTIES SKIP_RETURN_CODE 77)

# The round trip of the object types: the whole table the game ends up with
# from an objects.srv (through the old reader, ReadObjectsSrv) and from its
# items.toml (through LoadItems) is the same, field by field. convert_items
# uses made-up files; convert_items_original the original objects.srv,
# PATELLA_ORIGINAL_OBJECTS, and the committed data/items.toml, which must also
# be exactly what patella-convert makes of it. It is skipped when that
# objects.srv is not there.
set(PATELLA_ORIGINAL_OBJECTS "/mnt/c/Tibia/Patella-Gameserver/dat/objects.srv" CACHE FILEPATH
	"The original installation's objects.srv, for the test convert_items_original")
add_executable(convert_test_items tests/test_items.cc)
target_link_libraries(convert_test_items PRIVATE convert_common)
add_test(NAME convert_items COMMAND convert_test_items)
add_test(NAME convert_items_original COMMAND convert_test_items --original ${PATELLA_ORIGINAL_OBJECTS}
	${PROJECT_SOURCE_DIR}/data/items.toml)
set_tests_properties(convert_items_original PROPERTIES SKIP_RETURN_CODE 77)

# The round trip of the circles: the circles the game ends up with from a
# circles.dat (through the old reader, ReadCirclesDat) and from its
# circles.toml (through LoadCircles) are the same, field by field and in the
# same order. convert_circles uses made-up files; convert_circles_original the
# original circles.dat, PATELLA_ORIGINAL_CIRCLES, and the committed
# data/circles.toml, which must also be exactly what patella-convert makes of
# it. It is skipped when that circles.dat is not there.
set(PATELLA_ORIGINAL_CIRCLES "/mnt/c/Tibia/Patella-Gameserver/dat/circles.dat" CACHE FILEPATH
	"The original installation's circles.dat, for the test convert_circles_original")
add_executable(convert_test_circles tests/test_circles.cc)
target_link_libraries(convert_test_circles PRIVATE convert_common)
add_test(NAME convert_circles COMMAND convert_test_circles)
add_test(NAME convert_circles_original COMMAND convert_test_circles --original ${PATELLA_ORIGINAL_CIRCLES}
	${PROJECT_SOURCE_DIR}/data/circles.toml)
set_tests_properties(convert_circles_original PROPERTIES SKIP_RETURN_CODE 77)

# The round trip of the spells: the spell table the game ends up with from the
# table it had in its code (InitReferenceSpells) and from spells.toml (through
# LoadSpells) is the same, field by field. convert_spells uses made-up tables
# and files; convert_spells_original the committed data/spells.toml, which
# must also be exactly what patella-convert spells writes.
add_executable(convert_test_spells tests/test_spells.cc)
target_link_libraries(convert_test_spells PRIVATE convert_common)
add_test(NAME convert_spells COMMAND convert_test_spells)
add_test(NAME convert_spells_original COMMAND convert_test_spells --original ${PROJECT_SOURCE_DIR}/data/spells.toml)

# The round trip of the move and use rules: the rules the game ends up with
# from a moveuse.dat (through the old reader, ReadMoveUseDat) and from its
# moveuse.toml (through LoadMoveUseRules) are the same, entry by entry and in
# the same order. convert_moveuse uses made-up files; convert_moveuse_original
# the original moveuse.dat, PATELLA_ORIGINAL_MOVEUSE, with the committed
# data/items.toml for the object types, and the committed data/moveuse.toml,
# which must also be exactly what patella-convert makes of it. It is skipped
# when that moveuse.dat is not there.
set(PATELLA_ORIGINAL_MOVEUSE "/mnt/c/Tibia/Patella-Gameserver/dat/moveuse.dat" CACHE FILEPATH
	"The original installation's moveuse.dat, for the test convert_moveuse_original")
add_executable(convert_test_moveuse tests/test_moveuse.cc)
target_link_libraries(convert_test_moveuse PRIVATE convert_common)
add_test(NAME convert_moveuse COMMAND convert_test_moveuse)
add_test(NAME convert_moveuse_original COMMAND convert_test_moveuse --original ${PROJECT_SOURCE_DIR}/data/items.toml
	${PATELLA_ORIGINAL_MOVEUSE} ${PROJECT_SOURCE_DIR}/data/moveuse.toml)
set_tests_properties(convert_moveuse_original PROPERTIES SKIP_RETURN_CODE 77)

# The round trip of the houses: the areas, houses and house fields the game
# ends up with from a houseareas.dat and a houses.dat (through the old
# readers, ReadHouseAreasDat and ReadHousesDat) and from their houses.toml
# (through LoadHouseData) are the same, field by field and in the same order.
# convert_houses uses made-up files; convert_houses_original the original
# files, PATELLA_ORIGINAL_HOUSEAREAS and PATELLA_ORIGINAL_HOUSES, and the
# committed data/houses.toml, which must also be exactly what patella-convert
# makes of them. It is skipped when those files are not there.
set(PATELLA_ORIGINAL_HOUSEAREAS "/mnt/c/Tibia/Patella-Gameserver/dat/houseareas.dat" CACHE FILEPATH
	"The original installation's houseareas.dat, for the test convert_houses_original")
set(PATELLA_ORIGINAL_HOUSES "/mnt/c/Tibia/Patella-Gameserver/dat/houses.dat" CACHE FILEPATH
	"The original installation's houses.dat, for the test convert_houses_original")
add_executable(convert_test_houses tests/test_houses.cc)
target_link_libraries(convert_test_houses PRIVATE convert_common)
add_test(NAME convert_houses COMMAND convert_test_houses)
add_test(NAME convert_houses_original COMMAND convert_test_houses --original ${PATELLA_ORIGINAL_HOUSEAREAS}
	${PATELLA_ORIGINAL_HOUSES} ${PROJECT_SOURCE_DIR}/data/houses.toml)
set_tests_properties(convert_houses_original PROPERTIES SKIP_RETURN_CODE 77)

# The round trip of the map settings: the sectors' range, the refreshed
# cylinders, the memory settings, the start positions, the marks and the
# depots the game ends up with from a map.dat and the mem.dat it includes
# (through the old reader, ReadMapDat) and from their map.toml (through
# LoadMapData) are the same, field by field. convert_map uses made-up files;
# convert_map_original the original map.dat, PATELLA_ORIGINAL_MAP (with the
# mem.dat next to it), and the committed data/map.toml, which must also be
# exactly what patella-convert makes of it. It is skipped when that map.dat is
# not there.
set(PATELLA_ORIGINAL_MAP "/mnt/c/Tibia/Patella-Gameserver/dat/map.dat" CACHE FILEPATH
	"The original installation's map.dat, for the test convert_map_original")
add_executable(convert_test_map tests/test_map.cc)
target_link_libraries(convert_test_map PRIVATE convert_common)
add_test(NAME convert_map COMMAND convert_test_map)
add_test(NAME convert_map_original COMMAND convert_test_map --original ${PATELLA_ORIGINAL_MAP}
	${PROJECT_SOURCE_DIR}/data/map.toml)
set_tests_properties(convert_map_original PROPERTIES SKIP_RETURN_CODE 77)

# The round trip of the conversion of old object numbers: the table the game
# ends up with from a conversion.lst (through the old reader,
# ReadConversionLst) and from its conversion.toml (through LoadConversions) is
# the same, entry by entry. convert_conversion uses made-up files;
# convert_conversion_original the original conversion.lst,
# PATELLA_ORIGINAL_CONVERSION, and the committed data/conversion.toml, which
# must also be exactly what patella-convert makes of it. It is skipped when
# that conversion.lst is not there.
set(PATELLA_ORIGINAL_CONVERSION "/mnt/c/Tibia/Patella-Gameserver/dat/conversion.lst" CACHE FILEPATH
	"The original installation's conversion.lst, for the test convert_conversion_original")
add_executable(convert_test_conversion tests/test_conversion.cc)
target_link_libraries(convert_test_conversion PRIVATE convert_common)
add_test(NAME convert_conversion COMMAND convert_test_conversion)
add_test(NAME convert_conversion_original COMMAND convert_test_conversion --original ${PATELLA_ORIGINAL_CONVERSION}
	${PROJECT_SOURCE_DIR}/data/conversion.toml)
set_tests_properties(convert_conversion_original PROPERTIES SKIP_RETURN_CODE 77)

# The player data through the query manager (test_playerdata.cc): made-up
# player files read with the old reader must come back the same from
# LoadPlayerData after SavePlayerData and after querymanager-seed's import, and
# a save while the query manager is gone must keep the data until it is back.
# The query manager runs in the test's process.
add_executable(convert_test_playerdata tests/test_playerdata.cc)
target_link_libraries(convert_test_playerdata PRIVATE convert_common qm_server)
target_compile_definitions(convert_test_playerdata PRIVATE PATELLA_ITEMS_TOML="${PROJECT_SOURCE_DIR}/data/items.toml")
add_test(NAME convert_playerdata COMMAND convert_test_playerdata)
