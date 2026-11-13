find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

# Protocol layer: framing, typed readers/writers, query catalogue. No storage, no sockets.
add_library(qm_protocol STATIC
	src/protocol.cc
	src/text.cc
)
target_include_directories(qm_protocol PUBLIC src)

# Storage interface and its SQLite implementation.
add_library(qm_storage STATIC
	src/sqlite_storage.cc
)
target_include_directories(qm_storage PUBLIC src)
target_link_libraries(qm_storage PUBLIC qm_protocol SQLite::SQLite3)

# Request handlers and the TCP server.
add_library(qm_server STATIC
	src/handlers.cc
	src/log.cc
	src/server.cc
)
target_include_directories(qm_server PUBLIC src)
target_link_libraries(qm_server PUBLIC qm_protocol qm_storage Threads::Threads)

# Seeding from a game server installation (.tibia, usr/, dat/owners.dat).
add_library(qm_seed STATIC
	src/script.cc
	src/seed.cc
)
target_include_directories(qm_seed PUBLIC src)
target_link_libraries(qm_seed PUBLIC qm_storage)

add_executable(querymanager src/main.cc)
target_link_libraries(querymanager PRIVATE qm_server)

add_executable(querymanager-seed src/seed_main.cc)
target_link_libraries(querymanager-seed PRIVATE qm_seed)

foreach(test protocol handlers server seed)
	add_executable(qm_test_${test} tests/test_${test}.cc)
	target_include_directories(qm_test_${test} PRIVATE tests)
	target_link_libraries(qm_test_${test} PRIVATE qm_server qm_seed)
	add_test(NAME querymanager_${test} COMMAND qm_test_${test})
endforeach()
