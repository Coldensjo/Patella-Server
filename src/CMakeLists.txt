# The game server, game, built from the units in this folder, and game_flags,
# the compile options of its units (tools/convert compiles some of them too).

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
	houses.cc
	threads.cc
	writer.cc
	dbfuncs.cc
	info.cc
	moveuse.cc
	magic.cc
	operate.cc
	crmain.cc
	crplayer.cc
	crnonpl.cc
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
target_link_libraries(game PRIVATE game_flags Threads::Threads tomlplusplus rsa_private_key)
