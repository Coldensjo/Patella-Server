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
target_link_libraries(game PRIVATE Threads::Threads)
