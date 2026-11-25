# patella-admin, the command line tool of the game's admin port (config.toml's
# [admin]; src/admin.hh has the protocol, data/admin.toml the commands).
add_executable(patella-admin patella-admin.cc)
target_link_libraries(patella-admin PRIVATE tomlplusplus)
