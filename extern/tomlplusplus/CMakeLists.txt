# toml++ v3.4.0, the TOML parser the game and its tools read their settings and
# data with. Source: https://github.com/marzer/tomlplusplus, release tag v3.4.0
# (commit 30172438cee64926dc41fdd9c11fb3ba5b2ba9de, 2023-10-13). include/toml++/toml.hpp
# is the single-header form, toml.hpp from the root of that tag, unchanged
# (sha256 6b5172ad4dd6519aec67b919181fa7a38a2234131e5b2afa232dfe444819783e).
# LICENSE is its MIT licence. Code includes it as <toml++/toml.hpp>.

add_library(tomlplusplus INTERFACE)
add_library(tomlplusplus::tomlplusplus ALIAS tomlplusplus)
# SYSTEM: the warnings of -Wall -Wextra are for our code, not the library's.
target_include_directories(tomlplusplus SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(tomlplusplus INTERFACE cxx_std_17)
