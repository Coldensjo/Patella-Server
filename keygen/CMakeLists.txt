# rsakeygen, which makes the key file (key.pem) of the game server and the
# login server and prints the modulus clients need (src/rsakeyfile.hh), and
# the test of the key files. No key is built into any program: the game reads
# the file config.toml's rsa_key names, the login server the one --key names.
add_executable(rsakeygen
	rsakeygen.cc
	${PROJECT_SOURCE_DIR}/src/rsakeyfile.cc
	${PROJECT_SOURCE_DIR}/src/vlong.cc
)
target_include_directories(rsakeygen PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_options(rsakeygen PRIVATE -fwrapv -fno-strict-aliasing)

# Reading, checking and writing key files: round trips, PKCS#8, and the
# files that must be refused. The keys are made when the test runs.
add_executable(keygen_test_rsakeyfile
	tests/test_rsakeyfile.cc
	${PROJECT_SOURCE_DIR}/src/rsakeyfile.cc
	${PROJECT_SOURCE_DIR}/src/vlong.cc
)
target_include_directories(keygen_test_rsakeyfile PRIVATE ${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/querymanager/tests)
target_compile_options(keygen_test_rsakeyfile PRIVATE -fwrapv -fno-strict-aliasing)
add_test(NAME keygen_rsakeyfile COMMAND keygen_test_rsakeyfile)
