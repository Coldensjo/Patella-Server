# sol2 v3.3.0, the C++ bindings between the game and its Lua scripts. Source:
# https://github.com/ThePhD/sol2, release v3.3.0 (2022-06-25), the release's
# single-header files, unchanged: include/sol/sol.hpp (sha256
# e095a961a5189863745e6c101124fce944af991f3d4726a1e82c5b4a885a187f),
# include/sol/config.hpp (6c283673a16f0eeb3c56f8b8d72ccf7ed3f048816dbd2584ac58564c61315f02),
# include/sol/forward.hpp (8fc34d74e9b4b8baa381f5e6ab7b6f6b44114cd355c718505495943ff6b85740).
# sol.hpp names itself 3.2.3 (SOL_VERSION_STRING); the release did not change
# that string. LICENSE.txt is its MIT licence, from the tag. Code includes it as
# <sol/sol.hpp>.
#
# Every unit that includes sol2 gets the same settings from this target, since
# they must agree (one definition rule): SOL_LUAJIT=1 (LuaJIT, target luajit)
# and SOL_ALL_SAFETIES_ON=1 (arguments of the wrong type are Lua errors, not
# undefined behaviour).

add_library(sol2 INTERFACE)
# SYSTEM: the warnings of -Wall -Wextra are for our code, not the library's.
target_include_directories(sol2 SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(sol2 INTERFACE SOL_LUAJIT=1 SOL_ALL_SAFETIES_ON=1)
target_compile_features(sol2 INTERFACE cxx_std_17)
target_link_libraries(sol2 INTERFACE luajit)
