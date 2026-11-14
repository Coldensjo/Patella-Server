# patella-convert, which turns the original installation's files into the TOML
# files the game reads (convert.cc), and its tests. It reads each original file
# with the game's own reader and writes what the game ended up with, so it
# compiles the game units those readers need, with the game's flags.

set(GAME_SOURCE ${PROJECT_SOURCE_DIR}/src)

add_library(convert_common STATIC
	tibia_config.cc
	config_writer.cc
	tomlwrite.cc
	${GAME_SOURCE}/config.cc
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
