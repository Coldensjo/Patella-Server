# The game server, game, built from the units in this folder; game_flags, the
# compile options of its units (tools/convert compiles some of them too); and
# the tests of its units.

find_package(Threads REQUIRED)

add_executable(game
	main.cc
	mudinc.cc
	utils.cc
	script.cc
	strings.cc
	config.cc
	tomlfile.cc
	time.cc
	objects.cc
	shm.cc
	map.cc
	mapdata.cc
	houses.cc
	housesdata.cc
	threads.cc
	writer.cc
	dbfuncs.cc
	info.cc
	triggers.cc
	triggersdata.cc
	magic.cc
	magicdata.cc
	operate.cc
	crmain.cc
	crplayer.cc
	crnonpl.cc
	npcdata.cc
	monsterdata.cc
	cract.cc
	crcombat.cc
	crskill.cc
	connections.cc
	sending.cc
	receiving.cc
	communication.cc
	reader.cc
	query.cc
	TRSAPrivateKey.cc
	TXTEASymmetricKey.cc
	vlong.cc
	rsakeyfile.cc
	scripting.cc
	scripting_magic.cc
	scripting_triggers.cc
	scripting_items.cc
	scripting_npcs.cc
	scripting_monsters.cc
	scripting_events.cc
	reload.cc
)
target_include_directories(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# -Wno-write-strings: many prototypes take `char *` and are called with
#   string literals.
# -fwrapv, -fno-strict-aliasing: the code relies on signed arithmetic
#   wrapping and on reading memory through pointers of another type.
# -U_FORTIFY_SOURCE: Ubuntu's g++ turns on _FORTIFY_SOURCE with -O2, which
#   aborts on buffer overruns the server otherwise survives (for example an
#   error text of 100 to 127 characters in script.cc's ErrorString[100]).
# -Wno-format-overflow, -Wno-format-truncation, -Wno-stringop-truncation,
#   -Wno-stringop-overflow: the code formats and copies into fixed buffers
#   without bounds; those calls are kept as they are.
add_library(game_flags INTERFACE)
target_compile_options(game_flags INTERFACE
	-Wno-write-strings -fwrapv -fno-strict-aliasing -U_FORTIFY_SOURCE
	-Wno-format-overflow -Wno-format-truncation -Wno-stringop-truncation -Wno-stringop-overflow
)
target_link_libraries(game PRIVATE game_flags Threads::Threads tomlplusplus sol2)

# The tests of the game's units. game_test_scripting: the Lua engine
# (scripting.cc) on its own, with utils.cc for print, error and random; its
# scripts are made in a scratch folder the test creates.
add_executable(game_test_scripting tests/test_scripting.cc scripting.cc utils.cc)
target_include_directories(game_test_scripting PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(game_test_scripting PRIVATE game_flags sol2)
add_test(NAME game_scripting COMMAND game_test_scripting)

# game_test_events: the game events (scripting_events.cc) on the engine, with
# tomlfile.cc for events.toml; its files are made in a scratch folder the test
# creates.
add_executable(game_test_events tests/test_events.cc scripting_events.cc scripting.cc tomlfile.cc utils.cc)
target_include_directories(game_test_events PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(game_test_events PRIVATE game_flags tomlplusplus sol2)
add_test(NAME game_events COMMAND game_test_events)

# game_test_reload: the reload's driver (reload.cc) with parts of the test's
# own: the order of the steps, failures in read and loadScripts, a second
# request refused.
add_executable(game_test_reload tests/test_reload.cc reload.cc scripting.cc utils.cc)
target_include_directories(game_test_reload PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(game_test_reload PRIVATE game_flags tomlplusplus sol2)
add_test(NAME game_reload COMMAND game_test_reload)

# game_test_items_reload: the items part of a reload (objects.cc) with scratch
# items.toml and conversion.toml, and the start against the old loaders on the
# repository's data.
add_executable(game_test_items_reload tests/test_items_reload.cc objects.cc strings.cc tomlfile.cc utils.cc mudinc.cc)
target_include_directories(game_test_items_reload PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(game_test_items_reload PRIVATE game_flags tomlplusplus Threads::Threads)
add_test(NAME game_items_reload COMMAND game_test_items_reload --data ${PROJECT_SOURCE_DIR}/data)

# game_test_monsterreload: the reload's checks of monsterdata.cc (refusals,
# changes), on files written in a scratch folder.
add_executable(game_test_monsterreload tests/test_monsterreload.cc monsterdata.cc tomlfile.cc utils.cc)
target_include_directories(game_test_monsterreload PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(game_test_monsterreload PRIVATE game_flags tomlplusplus sol2)
add_test(NAME game_monsterreload COMMAND game_test_monsterreload)

# game_test_magic_reload: the staged magic tables of the reload (magicdata.cc)
# with scratch files.
add_executable(game_test_magic_reload tests/test_magic_reload.cc magicdata.cc tomlfile.cc strings.cc utils.cc mudinc.cc)
target_include_directories(game_test_magic_reload PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(game_test_magic_reload PRIVATE game_flags tomlplusplus)
add_test(NAME game_magic_reload COMMAND game_test_magic_reload)

# game_test_triggers_reload: the staged trigger tables (triggersdata.cc); it
# links convert_common (tools/convert), which compiles triggersdata.cc with
# the units it needs (objects.cc, map.cc, ...) and the converter's stubs.
add_executable(game_test_triggers_reload tests/test_triggers_reload.cc)
target_link_libraries(game_test_triggers_reload PRIVATE game_flags convert_common)
add_test(NAME game_triggers_reload COMMAND game_test_triggers_reload)
