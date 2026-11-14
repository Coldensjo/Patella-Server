# The RSA key of the game server and the login server. src/RSA_PrivateKey.hh
# (the original server's key) is not in git. Without it, rsakeygen makes a new
# key once, in the build directory, and prints the modulus clients need.
add_executable(rsakeygen
	rsakeygen.cc
	${PROJECT_SOURCE_DIR}/src/vlong.cc
)
target_include_directories(rsakeygen PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_options(rsakeygen PRIVATE -fwrapv -fno-strict-aliasing)

add_library(rsa_private_key INTERFACE)
if(EXISTS ${PROJECT_SOURCE_DIR}/src/RSA_PrivateKey.hh)
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
