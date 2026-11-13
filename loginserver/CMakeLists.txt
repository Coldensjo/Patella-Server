find_package(Threads REQUIRED)

# The game server's own crypto units, built with the game's flags (see
# src/CMakeLists.txt): the login server uses the same RSA key and XTEA.
add_library(ls_crypto STATIC
	${PROJECT_SOURCE_DIR}/src/TRSAPrivateKey.cc
	${PROJECT_SOURCE_DIR}/src/TXTEASymmetricKey.cc
	${PROJECT_SOURCE_DIR}/src/vlong.cc
)
target_include_directories(ls_crypto PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_compile_options(ls_crypto PRIVATE
	-Wno-write-strings -fwrapv -fno-strict-aliasing -U_FORTIFY_SOURCE
	-Wno-format-overflow -Wno-format-truncation -Wno-stringop-truncation -Wno-stringop-overflow
)

# Login protocol and the query manager client. The framing, buffers and
# request numbers are the query manager's (qm_protocol), the log is its log.
add_library(ls_login STATIC
	src/account.cc
	src/listener.cc
	src/login.cc
	${PROJECT_SOURCE_DIR}/querymanager/src/log.cc
)
target_include_directories(ls_login PUBLIC src)
target_link_libraries(ls_login PUBLIC ls_crypto qm_protocol Threads::Threads)

add_executable(loginserver src/main.cc)
target_link_libraries(loginserver PRIVATE ls_login)

add_executable(ls_test_login tests/test_login.cc)
target_include_directories(ls_test_login PRIVATE ${PROJECT_SOURCE_DIR}/querymanager/tests)
target_link_libraries(ls_test_login PRIVATE ls_login)
add_test(NAME loginserver_login COMMAND ls_test_login)
