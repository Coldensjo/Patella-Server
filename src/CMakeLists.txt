find_package(Threads REQUIRED)

add_executable(game
	main.cc
	mudinc.cc
	utils.cc
	script.cc
	strings.cc
	config.cc
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
# -Wno-write-strings: the original prototypes take `char *` (as its DWARF
#   says) and are called with string literals, which gcc 3.2 accepted.
# -fwrapv, -fno-strict-aliasing: the 2006 code relies on signed arithmetic
#   wrapping and on reading memory through pointers of another type.
# -U_FORTIFY_SOURCE: Ubuntu's g++ turns on _FORTIFY_SOURCE with -O2, which
#   aborts on buffer overruns that the original survives (for example an
#   error text of 100 to 127 characters in ErrorString[100]).
# -Wno-format-overflow, -Wno-format-truncation, -Wno-stringop-truncation,
#   -Wno-stringop-overflow: the original formats and copies into fixed
#   buffers without bounds; those calls are kept as they are.
target_compile_options(game PRIVATE
	-Wno-write-strings -fwrapv -fno-strict-aliasing -U_FORTIFY_SOURCE
	-Wno-format-overflow -Wno-format-truncation -Wno-stringop-truncation -Wno-stringop-overflow
)
target_link_libraries(game PRIVATE Threads::Threads rsa_private_key)
