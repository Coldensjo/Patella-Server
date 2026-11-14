# The RSA key of the game server and the login server. src/RSA_PrivateKey.hh
# (CipSoft's key) is not in git. Without it, rsakeygen makes a new key once, in
# the build directory, and prints the modulus clients need.
add_executable(rsakeygen
	rsakeygen.cc
	${PROJECT_SOURCE_DIR}/src/vlong.cc
)
target_include_directories(rsakeygen PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_options(rsakeygen PRIVATE -fwrapv -fno-strict-aliasing)

# PATELLA_RSA_KEY_HEADER: a header in rsakeygen's format to use instead of
# src/RSA_PrivateKey.hh or a generated key, copied into the build directory as
# RSA_PrivateKey.hh. tools/repro_check.py gives all its builds one key this way.
# It cannot be combined with src/RSA_PrivateKey.hh: the sources include the key
# with quotes, which finds the file next to them before any include directory.
set(PATELLA_RSA_KEY_HEADER "" CACHE FILEPATH "RSA key header to use instead of src/RSA_PrivateKey.hh or a generated one")

add_library(rsa_private_key INTERFACE)
if(PATELLA_RSA_KEY_HEADER)
	if(NOT EXISTS ${PATELLA_RSA_KEY_HEADER})
		message(FATAL_ERROR "PATELLA_RSA_KEY_HEADER: no such file: ${PATELLA_RSA_KEY_HEADER}")
	endif()
	if(EXISTS ${PROJECT_SOURCE_DIR}/src/RSA_PrivateKey.hh)
		message(FATAL_ERROR "PATELLA_RSA_KEY_HEADER is set, but src/RSA_PrivateKey.hh exists and would be included instead")
	endif()
	set(RSA_KEY_DIR ${PROJECT_BINARY_DIR}/rsa-key)
	configure_file(${PATELLA_RSA_KEY_HEADER} ${RSA_KEY_DIR}/RSA_PrivateKey.hh COPYONLY)
	message(STATUS "RSA key: ${PATELLA_RSA_KEY_HEADER}")
	target_include_directories(rsa_private_key INTERFACE ${RSA_KEY_DIR})
elseif(EXISTS ${PROJECT_SOURCE_DIR}/src/RSA_PrivateKey.hh)
	message(STATUS "RSA key: src/RSA_PrivateKey.hh")
	target_include_directories(rsa_private_key INTERFACE ${PROJECT_SOURCE_DIR}/src)
else()
	set(RSA_KEY_DIR ${PROJECT_BINARY_DIR}/generated)
	file(MAKE_DIRECTORY ${RSA_KEY_DIR})
	message(STATUS "RSA key: ${RSA_KEY_DIR}/RSA_PrivateKey.hh (generated if missing)")
	add_custom_command(
		OUTPUT ${RSA_KEY_DIR}/RSA_PrivateKey.hh
		COMMAND rsakeygen ${RSA_KEY_DIR}/RSA_PrivateKey.hh
		DEPENDS rsakeygen
		COMMENT "Making an RSA key"
	)
	add_custom_target(rsa_key DEPENDS ${RSA_KEY_DIR}/RSA_PrivateKey.hh)
	target_include_directories(rsa_private_key INTERFACE ${RSA_KEY_DIR})
	add_dependencies(rsa_private_key rsa_key)
endif()
