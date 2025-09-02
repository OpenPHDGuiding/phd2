-- PHD2 Guiding - xmake build configuration
-- Copyright 2014-2025, Max Planck Society and contributors
-- All rights reserved.

-- Set minimum xmake version
set_xmakever("2.8.0")

-- Project information
set_project("phd2")
set_version("2.6.13")
set_description("PHD2 auto-guiding software")

-- Language and standards
set_languages("c++14")
set_warnings("all")

-- Build modes
add_rules("mode.debug", "mode.release")

-- Platform detection and configuration
if is_plat("windows") then
    add_defines("OS_WINDOWS=1", "_CRT_SECURE_NO_WARNINGS")
    set_runtimes("MD")
elseif is_plat("macosx") then
    add_defines("OS_MACOSX=1")
    set_policy("build.macosx.deployment_target", "10.14")
elseif is_plat("linux") then
    add_defines("OS_LINUX=1")
end

-- Global compiler options
if is_mode("debug") then
    add_defines("DEBUG", "_DEBUG")
    set_symbols("debug")
    set_optimize("none")
else
    add_defines("NDEBUG")
    set_symbols("hidden")
    set_optimize("fastest")
end

-- Global include directories
add_includedirs("src")
add_includedirs("src/cameras")
add_includedirs("src/mounts")
add_includedirs("src/stepguiders")
add_includedirs("src/rotators")
add_includedirs("src/guiding")
add_includedirs("src/ui")
add_includedirs("src/core")
add_includedirs("src/communication")
add_includedirs("src/logging")
add_includedirs("src/utilities")
add_includedirs("cameras")
add_includedirs(".")
add_includedirs("contributions/MPI_IS_gaussian_process/src")
add_includedirs("contributions/MPI_IS_gaussian_process/tools")
add_includedirs("/usr/include/eigen3")
add_includedirs("/usr/include/libusb-1.0")

-- Global definitions
add_defines("HAVE_TYPE_TRAITS")

-- Disable optional features that require additional dependencies
-- add_defines("OPENCV_CAMERA")  -- Disabled: requires OpenCV
add_defines("NO_OPENCV_CAMERA")  -- Explicitly disable OpenCV camera support

-- Platform-specific global settings
if is_plat("windows") then
    if is_arch("x64") then
        add_defines("WIN64")
    end
    add_syslinks("user32", "gdi32", "winspool", "comdlg32", "advapi32", "shell32")
    add_syslinks("ole32", "oleaut32", "uuid", "odbc32", "odbccp32", "winmm")
elseif is_plat("linux") then
    add_syslinks("pthread", "m", "dl")
    add_defines("_FILE_OFFSET_BITS=64", "__WXGTK__", "WXUSINGDLL")
elseif is_plat("macosx") then
    add_frameworks("Carbon", "Cocoa", "IOKit", "QuickTime", "System", "WebKit", "AudioToolbox", "OpenGL", "CoreFoundation")
end

-- Options for build configuration
option("opensource_only")
    set_default(false)
    set_showmenu(true)
    set_description("Build only open source components")
option_end()

option("use_system_libusb")
    set_default(false)
    set_showmenu(true)
    set_description("Use system libusb instead of bundled version")
option_end()

option("use_system_gtest")
    set_default(false)
    set_showmenu(true)
    set_description("Use system Google Test instead of fetched version")
option_end()

option("use_system_libindi")
    set_default(false)
    set_showmenu(true)
    set_description("Use system libindi instead of building from source")
option_end()

-- Include subdirectories for modular configuration
includes("xmake/dependencies.lua")
includes("xmake/cameras.lua")
includes("xmake/platforms.lua")
includes("xmake/localization.lua")
includes("xmake/documentation.lua")
includes("xmake/targets.lua")

-- Main targets will be defined in the included files

-- Set up documentation and localization targets
-- Note: These will be called from the included module files

-- Temporary simple target for testing
target("phd2")
    set_kind("binary")
    add_files("src/core/*.cpp")
    add_files("src/logging/*.cpp")
    add_files("src/guiding/*.cpp")
    add_files("src/guiding/algorithms/*.cpp")
    add_files("src/ui/*.cpp")
    add_files("src/ui/dialogs/*.cpp")
    add_files("src/ui/tools/*.cpp")
    add_files("src/cameras/*.cpp")
    add_files("src/cameras/drivers/*.cpp")
    remove_files("src/cameras/drivers/cam_opencv.cpp")
    add_files("src/mounts/*.cpp")
    add_files("src/mounts/drivers/*.cpp")
    add_files("src/stepguiders/*.cpp")
    add_files("src/rotators/*.cpp")
    add_files("src/communication/**/*.cpp")
    add_files("src/communication/onboard_st4.cpp")
    add_files("src/utilities/*.cpp")
    add_files("contributions/MPI_IS_gaussian_process/src/*.cpp")
    add_files("contributions/MPI_IS_gaussian_process/tools/*.cpp")

    -- Add wxWidgets manually for now
    add_includedirs("/usr/lib/x86_64-linux-gnu/wx/include/gtk3-unicode-3.2", "/usr/include/wx-3.2")
    add_defines("_FILE_OFFSET_BITS=64", "WXUSINGDLL", "__WXGTK__")
    add_cxflags("-pthread")

    -- Add wxWidgets libraries
    add_linkdirs("/usr/lib/x86_64-linux-gnu")
    add_links("wx_gtk3u_xrc-3.2", "wx_gtk3u_html-3.2", "wx_gtk3u_qa-3.2", "wx_gtk3u_aui-3.2", "wx_gtk3u_core-3.2", "wx_baseu_xml-3.2", "wx_baseu_net-3.2", "wx_baseu-3.2")

    -- Add X11 and other system libraries
    add_syslinks("X11", "curl", "cfitsio", "usb-1.0", "indidriver", "indiclient")
