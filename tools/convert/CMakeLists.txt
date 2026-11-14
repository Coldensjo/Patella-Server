# patella-convert, which turns the original installation's files into the TOML
# files the game reads (convert.cc), and its tests. It reads each original file
# with the game's own reader and writes what the game ended up with, so it
# compiles the game units those readers need, with the game's flags.

set(GAME_SOURCE ${PROJECT_SOURCE_DIR}/src)

add_library(convert_common STATIC
	tibia_config.cc
	config_writer.cc
	objects_srv.cc
	items_writer.cc
	circles_dat.cc
	circles_writer.cc
	tomlwrite.cc
	${GAME_SOURCE}/config.cc
	${GAME_SOURCE}/magicdata.cc
	${GAME_SOURCE}/objects.cc
	${GAME_SOURCE}/strings.cc
	${GAME_SOURCE}/tomlfile.cc
	${GAME_SOURCE}/script.cc
	${GAME_SOURCE}/utils.cc
	${GAME_SOURCE}/mudinc.cc
)
target_include_directories(convert_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${GAME_SOURCE})
target_link_libraries(convert_common PUBLIC game_flags tomlplusplus)

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
