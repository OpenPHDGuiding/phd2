-- PHD2 Dependencies Configuration for xmake
-- Handles all external dependencies and third-party libraries

-- Package requirements
add_requires("pkgconfig", {optional = true})

-- Core system dependencies
if is_plat("windows") then
    -- Windows uses vcpkg for most dependencies
    add_requires("vcpkg::wxwidgets", {configs = {shared = false}})
    add_requires("vcpkg::cfitsio")
    add_requires("vcpkg::curl[ssl]")
    add_requires("vcpkg::eigen3")
    add_requires("vcpkg::opencv4")
else
    -- Unix-like systems
    add_requires("pkgconfig::gtk+-3.0", {optional = true})
    add_requires("pkgconfig::cfitsio", {optional = true})
    add_requires("pkgconfig::libcurl", {optional = true})
    add_requires("eigen3", {optional = true})
    add_requires("opencv", {optional = true})
    
    if is_plat("linux") then
        add_requires("pkgconfig::x11")
        add_requires("pkgconfig::libusb-1.0", {optional = true})
    end
end

-- Google Test for unit testing
if has_config("use_system_gtest") then
    add_requires("gtest")
else
    add_requires("gtest", {system = false})
end

-- INDI library for telescope control
if has_config("use_system_libindi") and is_plat("linux") then
    add_requires("pkgconfig::libindi", {optional = true})
    add_requires("pkgconfig::libnova", {optional = true})
end

-- Function to configure wxWidgets (global scope for use in targets)
function configure_wxwidgets(target)
    if is_plat("windows") then
        target:add("packages", "vcpkg::wxwidgets")
        target:add("defines", "WXUSINGDLL", "_UNICODE", "UNICODE")
    else
        -- Try to find wxWidgets using wx-config
        local wx_config = find_program("wx-config")
        if wx_config then
            -- Get required components flags
            local wx_cflags = os.iorunv(wx_config, {"--cflags"})
            local wx_libs = os.iorunv(wx_config, {"--libs", "aui,core,base,adv,html,net"})
            if wx_cflags and wx_libs then
                -- Parse and add cflags
                for _, flag in ipairs(wx_cflags:trim():split("%s+")) do
                    if flag:startswith("-I") then
                        target:add("includedirs", flag:sub(3))
                    elseif flag:startswith("-D") then
                        target:add("defines", flag:sub(3))
                    else
                        target:add("cxflags", flag)
                    end
                end
                -- Parse and add libs
                for _, flag in ipairs(wx_libs:trim():split("%s+")) do
                    if flag:startswith("-L") then
                        target:add("linkdirs", flag:sub(3))
                    elseif flag:startswith("-l") then
                        target:add("links", flag:sub(3))
                    elseif flag:startswith("-framework") then
                        -- macOS frameworks
                        local next_flag = true
                    else
                        target:add("ldflags", flag)
                    end
                end
            else
                cprint("${yellow}Warning: wx-config found but failed to get flags${clear}")
                -- Fallback to package config
                target:add("packages", "pkgconfig::wxwidgets")
            end
        else
            cprint("${yellow}Warning: wx-config not found, using pkg-config${clear}")
            target:add("packages", "pkgconfig::wxwidgets")
        end
        target:add("defines", "__WXGTK__", "WXUSINGDLL", "_FILE_OFFSET_BITS=64")
    end
end

-- Function to configure CFITSIO (global scope for use in targets)
configure_cfitsio = function(target)
    if is_plat("windows") then
        target:add("packages", "vcpkg::cfitsio")
    else
        target:add("packages", "pkgconfig::cfitsio")
    end
end

-- Function to configure libcurl (global scope for use in targets)
configure_curl = function(target)
    if is_plat("windows") then
        target:add("packages", "vcpkg::curl[ssl]")
    else
        target:add("packages", "pkgconfig::libcurl")
    end
end

-- Function to configure Eigen3 (global scope for use in targets)
configure_eigen3 = function(target)
    if is_plat("windows") then
        target:add("packages", "vcpkg::eigen3")
    else
        target:add("packages", "eigen3")
    end
end

-- Function to configure OpenCV (global scope for use in targets)
configure_opencv = function(target)
    if is_plat("windows") then
        target:add("packages", "vcpkg::opencv4")
    else
        target:add("packages", "opencv")
        if is_plat("linux") then
            target:add("packages", "pkgconfig::x11")
        end
    end
end

-- Function to configure libusb (global scope for use in targets)
configure_libusb = function(target)
    if is_plat("windows") then
        -- Windows builds libusb from source or uses bundled version
        return
    elseif has_config("use_system_libusb") and is_plat("linux") then
        target:add("packages", "pkgconfig::libusb-1.0")
    else
        -- Build libusb from bundled source
        target:add("deps", "usb_openphd")
    end
end

-- Function to configure INDI (global scope for use in targets)
configure_indi = function(target)
    if has_config("use_system_libindi") and is_plat("linux") then
        target:add("packages", "pkgconfig::libindi")
        target:add("packages", "pkgconfig::libnova")
        target:add("defines", "LIBNOVA")
    else
        -- Build INDI from source or use external project
        target:add("deps", "indi_client")
        target:add("defines", "LIBNOVA")
    end
end

-- Function to configure Google Test (global scope for use in targets)
configure_gtest = function(target)
    target:add("packages", "gtest")
end

-- Function to build libusb from source (non-Windows) (global scope)
build_libusb_target = function()
    if is_plat("windows") then
        return
    end

    target("usb_openphd")
        set_kind(is_plat("macosx") and "shared" or "static")
        add_files("thirdparty/libusb-1.0.21/libusb/core.c")
        add_files("thirdparty/libusb-1.0.21/libusb/descriptor.c")
        add_files("thirdparty/libusb-1.0.21/libusb/hotplug.c")
        add_files("thirdparty/libusb-1.0.21/libusb/io.c")
        add_files("thirdparty/libusb-1.0.21/libusb/sync.c")

        add_headerfiles("thirdparty/libusb-1.0.21/libusb/libusb.h")
        add_includedirs("thirdparty/libusb-1.0.21/libusb", {public = true})

        if is_plat("macosx") then
            add_files("thirdparty/libusb-1.0.21/libusb/os/darwin_usb.c")
            add_files("thirdparty/libusb-1.0.21/libusb/os/threads_posix.c")
            add_files("thirdparty/libusb-1.0.21/libusb/os/poll_posix.c")
            add_defines("OS_DARWIN=1")
            add_includedirs("thirdparty/include/libusb-1.0.21")
            add_frameworks("CoreFoundation", "IOKit")
            if is_kind("shared") then
                add_ldflags("-compatibility_version 2 -current_version 2")
            end
        elseif is_plat("linux") then
            add_files("thirdparty/libusb-1.0.21/libusb/os/linux_usbfs.c")
            add_files("thirdparty/libusb-1.0.21/libusb/os/linux_netlink.c")
            add_files("thirdparty/libusb-1.0.21/libusb/os/threads_posix.c")
            add_files("thirdparty/libusb-1.0.21/libusb/os/poll_posix.c")
            add_defines("OS_LINUX=1")
            add_includedirs("thirdparty/include/libusb-1.0.21")
        end

        add_defines("LIBUSB_DESCRIBE=\"\"")
        if not is_plat("windows") then
            add_defines("_CRT_SECURE_NO_WARNINGS")
        end
        set_group("Thirdparty")
end

-- Function to build OpenSSAG library (global scope)
build_openssag_target = function()
    if is_plat("windows") then
        return
    end

    target("OpenSSAG")
        set_kind("static")
        add_files("thirdparty/openssag/src/loader.cpp")
        add_files("thirdparty/openssag/src/openssag.cpp")
        add_headerfiles("thirdparty/openssag/src/*.h")
        add_includedirs("thirdparty/openssag/src", {public = true})
        add_defines("HAVE_OPENSSAG_CAMERA=1")
        set_group("Thirdparty")
end

-- Function to build VidCapture library (Windows x86 only) (global scope)
build_vidcapture_target = function()
    if not (is_plat("windows") and is_arch("x86")) then
        return
    end

    target("VidCapture")
        set_kind("static")
        add_files("thirdparty/VidCapture/Source/VidCapture/CVImage.cpp")
        add_files("thirdparty/VidCapture/Source/VidCapture/CVImageGrey.cpp")
        add_files("thirdparty/VidCapture/Source/VidCapture/CVImageRGB24.cpp")
        add_files("thirdparty/VidCapture/Source/VidCapture/CVImageRGBFloat.cpp")
        add_files("thirdparty/VidCapture/Source/VidCapture/CVVidCapture.cpp")
        add_files("thirdparty/VidCapture/Source/VidCapture/CVVidCaptureDSWin32.cpp")
        add_files("thirdparty/VidCapture/Source/VidCapture/CVDShowUtil.cpp")
        add_files("thirdparty/VidCapture/Source/VidCapture/CVFile.cpp")
        add_files("thirdparty/VidCapture/Source/VidCapture/CVPlatformWin32.cpp")
        add_files("thirdparty/VidCapture/Source/VidCapture/CVTraceWin32.cpp")

        add_includedirs("thirdparty/VidCapture/Source/CVCommon", {public = true})
        add_includedirs("thirdparty/VidCapture/Source/VidCapture", {public = true})

        add_defines("FF_NO_UNISTD_H", "_CRT_SECURE_NO_WARNINGS", "_CRT_SECURE_NO_DEPRECATE")
        set_group("Thirdparty")
end

-- Function to configure INDI external project (when not using system) (global scope)
build_indi_target = function()
    if has_config("use_system_libindi") then
        return
    end

    -- This would typically use an external project or git submodule
    -- For now, we'll assume INDI is built separately and libraries are available
    target("indi_client")
        set_kind("phony") -- Placeholder for external INDI build
        add_includedirs("build/libindi/include", {public = true})
        if is_plat("windows") then
            add_linkdirs("build/libindi/lib")
            add_links("indiclient")
        else
            add_linkdirs("build/libindi/lib")
            add_links("indiclient")
            if is_plat("macosx") then
                add_links("nova") -- Static libnova on macOS
            else
                add_links("nova", "z")
            end
        end
        add_defines("LIBNOVA")
        set_group("External")
end

-- Build libusb target if not using system libusb
if not has_config("use_system_libusb") and not is_plat("windows") then
    build_libusb_target()
end

-- Build OpenSSAG target if not opensource_only
if not has_config("opensource_only") and not is_plat("windows") then
    build_openssag_target()
end

-- Build VidCapture target for Windows x86
if is_plat("windows") and is_arch("x86") then
    build_vidcapture_target()
end

-- Build or configure INDI
if not has_config("use_system_libindi") then
    build_indi_target()
end
if not has_config("opensource_only") then
    build_openssag_target()
end

-- Build INDI target if not using system libindi
if not has_config("use_system_libindi") then
    build_indi_target()
end
