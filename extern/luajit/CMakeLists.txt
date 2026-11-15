# LuaJIT 2.1, the Lua engine of the game's scripts (src/scripting.cc). Source:
# https://github.com/LuaJIT/LuaJIT, branch v2.1, commit
# 63c8f1a9b479ba5518cf6a170ff7729178bc3cfd (2026-11-11), the GitHub archive of
# that commit (sha256 8cad76f444b76001b7123dc626c5d0bfc24f1f790333737443345f7e6b41bb5f)
# unpacked into this folder unchanged except that its doc/ folder (the HTML
# manual) is left out. .relver holds the commit's time as the archive wrote it;
# LuaJIT's rolling version number is made from it. COPYRIGHT is its MIT licence.
#
# LuaJIT builds with its own Makefile in its source tree, so the build copies
# the tree into the build folder first and runs make there (as $(MAKE), so it
# needs a Makefile generator, CMake's default here), BUILDMODE=static,
# giving libluajit.a; nothing is written into this folder. Its compiler flags
# are its own: our warning rule does not apply to LuaJIT. The target luajit is
# that library, with the copy's src/ (lua.h, lauxlib.h, lualib.h, luajit.h,
# which the build generates) as a SYSTEM include.

set(LUAJIT_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(LUAJIT_TREE ${CMAKE_CURRENT_BINARY_DIR}/tree)
set(LUAJIT_LIBRARY ${LUAJIT_TREE}/src/libluajit.a)
file(GLOB_RECURSE LUAJIT_FILES CONFIGURE_DEPENDS ${LUAJIT_SOURCE}/src/* ${LUAJIT_SOURCE}/dynasm/* ${LUAJIT_SOURCE}/Makefile)
list(FILTER LUAJIT_FILES EXCLUDE REGEX "/CMakeLists\\.txt$")

add_custom_command(
	OUTPUT ${LUAJIT_LIBRARY} ${LUAJIT_TREE}/src/luajit.h
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${LUAJIT_TREE}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${LUAJIT_SOURCE} ${LUAJIT_TREE}
	COMMAND ${CMAKE_COMMAND} -E rm -f ${LUAJIT_TREE}/CMakeLists.txt
	COMMAND $(MAKE) -C ${LUAJIT_TREE} BUILDMODE=static
	DEPENDS ${LUAJIT_FILES}
	COMMENT "Building LuaJIT (static)"
	VERBATIM
)
add_custom_target(luajit_build DEPENDS ${LUAJIT_LIBRARY})

# The include folder must exist when CMake checks the imported target.
file(MAKE_DIRECTORY ${LUAJIT_TREE}/src)
add_library(luajit STATIC IMPORTED GLOBAL)
set_target_properties(luajit PROPERTIES
	IMPORTED_LOCATION ${LUAJIT_LIBRARY}
	INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${LUAJIT_TREE}/src
	INTERFACE_INCLUDE_DIRECTORIES ${LUAJIT_TREE}/src
	INTERFACE_LINK_LIBRARIES "m;dl"
)
add_dependencies(luajit luajit_build)
