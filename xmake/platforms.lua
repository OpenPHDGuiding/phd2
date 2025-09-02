-- PHD2 Platform-specific Configuration for xmake
-- Handles Windows, macOS, and Linux specific build settings

-- Windows-specific configuration
function configure_windows_platform(target)
    -- Windows version targeting
    add_defines("WINVER=0x0601", "_WIN32_WINNT=0x0601") -- Windows 7+
    
    -- Visual Studio specific settings
    if is_toolchain("msvc") then
        add_cxflags("/MP") -- Multi-processor compilation
        add_defines("_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE")
        
        -- Runtime library selection
        if is_mode("debug") then
            set_runtimes("MDd")
        else
            set_runtimes("MD")
        end
        
        -- Exception handling
        add_cxflags("/EHsc")
        
        -- Warning level
        add_cxflags("/W3")
        
        -- Disable specific warnings
        add_cxflags("/wd4996") -- Deprecated functions
        add_cxflags("/wd4244") -- Conversion warnings
        add_cxflags("/wd4305") -- Truncation warnings
    end
    
    -- Windows system libraries
    add_syslinks("kernel32", "user32", "gdi32", "winspool", "comdlg32")
    add_syslinks("advapi32", "shell32", "ole32", "oleaut32", "uuid")
    add_syslinks("odbc32", "odbccp32", "winmm", "version", "comctl32")
    add_syslinks("rpcrt4", "wsock32", "ws2_32")
    
    -- DirectShow libraries for video capture
    add_syslinks("strmiids", "quartz", "vfw32")
    
    -- Windows-specific defines
    add_defines("WIN32", "_WINDOWS", "HAVE_WINDOWS_H")
    
    if is_arch("x64") then
        add_defines("WIN64", "_WIN64")
    end
    
    -- Resource compilation
    if target:kind() == "binary" then
        add_rules("win.sdk.resource")
    end
end

-- macOS-specific configuration
function configure_macos_platform(target)
    -- macOS deployment target
    set_policy("build.macosx.deployment_target", "10.14")
    
    -- Compiler settings
    add_cxflags("-Wall", "-Wno-unused-parameter", "-Wno-sign-compare")
    add_cxflags("-Wno-unused-variable", "-Wno-unused-function")
    
    -- macOS system frameworks
    add_frameworks("Carbon", "Cocoa", "IOKit", "QuickTime", "System")
    add_frameworks("WebKit", "AudioToolbox", "OpenGL", "CoreFoundation")
    add_frameworks("Security", "SystemConfiguration")
    
    -- macOS-specific defines
    add_defines("__WXMAC__", "__WXOSX__", "__WXOSX_COCOA__")
    add_defines("HAVE_UNISTD_H", "HAVE_SYS_TYPES_H")
    
    -- Bundle configuration for applications
    if target:kind() == "binary" then
        -- Set bundle identifier
        set_values("xcode.bundle_identifier", "org.openphdguiding.phd2")
        
        -- Bundle version
        set_values("xcode.bundle_version", "$(version)")
        set_values("xcode.bundle_short_version_string", "$(version)")
        
        -- Bundle display name
        set_values("xcode.bundle_display_name", "PHD2")
        
        -- Bundle executable name
        set_values("xcode.bundle_executable", "PHD2")
        
        -- Bundle icon
        set_values("xcode.bundle_icon_file", "PHD_OSX_icon.icns")
        
        -- Bundle type
        set_values("xcode.bundle_package_type", "APPL")
        
        -- Bundle signature
        set_values("xcode.bundle_signature", "????")
        
        -- Minimum system version
        set_values("xcode.bundle_minimum_system_version", "10.14")
        
        -- High resolution capable
        set_values("xcode.bundle_high_resolution_capable", true)
        
        -- Document types (if needed)
        -- set_values("xcode.bundle_document_types", {...})
    end
    
    -- Linker settings
    add_ldflags("-Wl,-rpath,@executable_path/../Frameworks")
    add_ldflags("-Wl,-rpath,@loader_path/../Frameworks")
end

-- Linux-specific configuration
function configure_linux_platform(target)
    -- Compiler settings
    add_cxflags("-Wall", "-Wno-unused-parameter", "-Wno-sign-compare")
    add_cxflags("-Wno-unused-variable", "-Wno-unused-function")
    add_cxflags("-Wno-strict-aliasing")
    
    -- Position independent code
    add_cxflags("-fPIC")
    
    -- Linux system libraries
    add_syslinks("pthread", "dl", "rt")
    
    -- Math library (should be last)
    add_syslinks("m")
    
    -- Linux-specific defines
    add_defines("__WXGTK__", "WXUSINGDLL")
    add_defines("_FILE_OFFSET_BITS=64", "_LARGEFILE_SOURCE")
    add_defines("HAVE_UNISTD_H", "HAVE_SYS_TYPES_H")
    
    -- GTK-specific settings
    add_defines("__WXGTK3__")
    
    -- Installation paths
    set_configvar("CMAKE_INSTALL_PREFIX", "/usr/local")
    set_configvar("CMAKE_INSTALL_BINDIR", "bin")
    set_configvar("CMAKE_INSTALL_DATADIR", "share")
    set_configvar("CMAKE_INSTALL_DOCDIR", "share/doc/phd2")
    
    -- Desktop integration
    if target:kind() == "binary" then
        -- Install desktop file
        add_installfiles("phd2.desktop", {prefixdir = "share/applications"})
        
        -- Install icon
        add_installfiles("icons/phd2.png", {prefixdir = "share/pixmaps"})
        
        -- Install man page
        add_installfiles("phd2.1", {prefixdir = "share/man/man1"})
    end
end

-- Generic Unix configuration (for other Unix-like systems)
function configure_unix_platform(target)
    -- Compiler settings
    add_cxflags("-Wall", "-Wno-unused-parameter")
    
    -- System libraries
    add_syslinks("pthread", "m")
    
    -- Unix-specific defines
    add_defines("HAVE_UNISTD_H", "HAVE_SYS_TYPES_H")
    add_defines("_FILE_OFFSET_BITS=64")
end

-- Function to configure platform-specific settings
function configure_platform_specific(target)
    if is_plat("windows") then
        configure_windows_platform(target)
    elseif is_plat("macosx") then
        configure_macos_platform(target)
    elseif is_plat("linux") then
        configure_linux_platform(target)
    else
        -- Generic Unix fallback
        configure_unix_platform(target)
    end
end

-- Function to get platform-specific installation directories
function get_install_dirs()
    if is_plat("windows") then
        return {
            bindir = ".",
            datadir = ".",
            docdir = "doc",
            localedir = "locale"
        }
    elseif is_plat("macosx") then
        return {
            bindir = "Contents/MacOS",
            datadir = "Contents/Resources",
            docdir = "Contents/Resources/doc",
            localedir = "Contents/Resources/locale"
        }
    else -- Linux and other Unix
        return {
            bindir = "bin",
            datadir = "share/phd2",
            docdir = "share/doc/phd2",
            localedir = "share/locale"
        }
    end
end

-- Function to configure debug tools
function configure_debug_tools(target)
    if is_mode("debug") then
        if is_plat("windows") then
            -- Visual Leak Detector
            local vld_dir = os.getenv("VLD_DIR")
            if vld_dir and os.isdir(path.join(vld_dir, "include")) then
                target:add("defines", "HAVE_VLD=1")
                target:add("includedirs", path.join(vld_dir, "include"))
                if is_arch("x64") then
                    target:add("linkdirs", path.join(vld_dir, "lib/Win64"))
                else
                    target:add("linkdirs", path.join(vld_dir, "lib/Win32"))
                end
            end
        elseif is_plat("linux") then
            -- Valgrind support
            target:add("defines", "HAVE_VALGRIND=1")
        end
    end
end

-- Function to configure optimization settings
function configure_optimization(target)
    if is_mode("release") then
        if is_plat("windows") then
            add_cxflags("/O2", "/Ob2", "/Oi", "/Ot", "/Oy", "/GL")
            add_ldflags("/LTCG", "/OPT:REF", "/OPT:ICF")
        else
            add_cxflags("-O3", "-ffast-math", "-funroll-loops")
            add_ldflags("-Wl,--gc-sections")
        end
    elseif is_mode("debug") then
        if is_plat("windows") then
            add_cxflags("/Od", "/RTC1")
        else
            add_cxflags("-O0", "-g3")
        end
    end
end
