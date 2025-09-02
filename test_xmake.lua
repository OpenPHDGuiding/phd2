-- Test xmake configuration
set_project("test")
set_version("1.0.0")

target("test")
    set_kind("binary")
    add_files("src/core/phd.cpp")
