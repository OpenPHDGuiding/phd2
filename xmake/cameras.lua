-- PHD2 Camera SDK Configuration for xmake
-- Handles platform-specific camera driver SDKs and libraries

-- Camera SDK detection and configuration functions

-- Function to detect and configure camera SDKs (global scope)
configure_camera_sdks = function(target)
    if is_plat("windows") then
        configure_windows_cameras(target)
    elseif is_plat("macosx") then
        configure_macos_cameras(target)
    elseif is_plat("linux") then
        configure_linux_cameras(target)
    end
end

-- Windows camera SDK configuration (global scope)
configure_windows_cameras = function(target)
    local arch = is_arch("x64") and "x64" or "x86"
    local camera_root = "cameras"
    
    -- ZWO ASI cameras
    local asi_lib = path.join(camera_root, "zwolibs/win", arch, "ASICamera2.lib")
    if os.isfile(asi_lib) then
        target:add("defines", "HAVE_ZWO_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "zwolibs/include"))
        target:add("links", asi_lib)
        target:add("installfiles", path.join(camera_root, "zwolibs/win", arch, "ASICamera2.dll"))
    end
    
    -- QHY cameras
    local qhy_lib = path.join(camera_root, "qhyccdlibs/win", arch, "qhyccd.lib")
    if os.isfile(qhy_lib) then
        target:add("defines", "HAVE_QHY_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "qhyccdlibs/include"))
        target:add("links", qhy_lib)
        target:add("installfiles", path.join(camera_root, "qhyccdlibs/win", arch, "qhyccd.dll"))
        target:add("installfiles", path.join(camera_root, "qhyccdlibs/win", arch, "tbb.dll"))
    end
    
    -- ToupTek cameras
    local toupcam_lib = path.join(camera_root, "toupcam/win", arch, "toupcam.lib")
    if os.isfile(toupcam_lib) then
        target:add("defines", "HAVE_TOUPTEK_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "toupcam/include"))
        target:add("links", toupcam_lib)
        target:add("installfiles", path.join(camera_root, "toupcam/win", arch, "toupcam.dll"))
    end
    
    -- SBIG cameras
    local sbig_lib = path.join(camera_root, "sbig/win", arch, "SBIGUDrv.lib")
    if os.isfile(sbig_lib) then
        target:add("defines", "HAVE_SBIG_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "sbig/include"))
        target:add("links", sbig_lib)
    end
    
    -- Altair cameras
    local altair_lib = path.join(camera_root, "altair/win", arch, "altaircam.lib")
    if os.isfile(altair_lib) then
        target:add("defines", "HAVE_ALTAIR_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "altair/include"))
        target:add("installfiles", path.join(camera_root, "altair/win", arch, "altaircam.dll"))
        target:add("installfiles", path.join(camera_root, "altair/win", arch, "AltairCam_legacy.dll"))
    end
    
    -- SVB cameras
    local svb_lib = path.join(camera_root, "svblibs/win", arch, "SVBCameraSDK.lib")
    if os.isfile(svb_lib) then
        target:add("defines", "HAVE_SVB_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "svblibs/include"))
        target:add("links", svb_lib)
        target:add("installfiles", path.join(camera_root, "svblibs/win", arch, "SVBCameraSDK.dll"))
    end
    
    -- PlayerOne cameras
    local playerone_lib = path.join(camera_root, "playerone/win", arch, "PlayerOneCamera.lib")
    if os.isfile(playerone_lib) then
        target:add("defines", "HAVE_PLAYERONE_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "playerone/include"))
        target:add("links", playerone_lib)
        target:add("installfiles", path.join(camera_root, "playerone/win", arch, "PlayerOneCamera.dll"))
    end
    
    -- Moravian cameras
    local moravian_lib = path.join(camera_root, "moravian/win", arch, "gXusb.lib")
    if os.isfile(moravian_lib) then
        target:add("defines", "HAVE_MORAVIAN_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "moravian/include"))
        target:add("links", moravian_lib)
        target:add("installfiles", path.join(camera_root, "moravian/win", arch, "gXusb.dll"))
    end
    
    -- Starlight Xpress
    local sxv_lib = path.join(camera_root, "sxv/win", arch, "SXUSB.lib")
    if os.isfile(sxv_lib) then
        target:add("defines", "HAVE_SXV_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "sxv/include"))
        target:add("links", sxv_lib)
        target:add("installfiles", path.join(camera_root, "sxv/win", arch, "SXUSB.dll"))
    end
    
    -- Additional Windows camera libraries
    configure_windows_additional_cameras(target, arch)
end

configure_windows_additional_cameras = function(target, arch)
    local camera_root = "cameras"
    
    -- INOVA PLC
    local inova_lib = path.join(camera_root, "inovaplc/win", arch, "DICAMSDK.lib")
    if os.isfile(inova_lib) then
        target:add("defines", "HAVE_INOVA_PLC_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "inovaplc/include"))
        target:add("links", inova_lib)
        target:add("installfiles", path.join(camera_root, "inovaplc/win", arch, "DICAMSDK.dll"))
    end
    
    -- QGuide
    local qguide_lib = path.join(camera_root, "qguide/win", arch, "CMOSDLL.lib")
    if os.isfile(qguide_lib) then
        target:add("defines", "HAVE_QGUIDE_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "qguide/include"))
        target:add("links", qguide_lib)
        target:add("installfiles", path.join(camera_root, "qguide/win", arch, "CMOSDLL.dll"))
    end
    
    -- SSPIAG
    local sspiag_lib = path.join(camera_root, "sspiag/win", arch, "astroDLLQHY5V.lib")
    if os.isfile(sspiag_lib) then
        target:add("defines", "HAVE_SSPIAG_CAMERA=1")
        target:add("installfiles", path.join(camera_root, "sspiag/win", arch, "astroDLLGeneric.dll"))
        target:add("installfiles", path.join(camera_root, "sspiag/win", arch, "astroDLLQHY5V.dll"))
        target:add("installfiles", path.join(camera_root, "sspiag/win", arch, "astroDLLsspiag.dll"))
        target:add("installfiles", path.join(camera_root, "sspiag/win", arch, "SSPIAGCAM.dll"))
        target:add("installfiles", path.join(camera_root, "sspiag/win", arch, "SSPIAGUSB_WIN.dll"))
    end
    
    -- Shoestring GPUSB
    local shoestring_lib = path.join(camera_root, "shoestring", arch, "ShoestringGPUSB_DLL.lib")
    if os.isfile(shoestring_lib) then
        target:add("defines", "HAVE_SHOESTRING=1")
        target:add("includedirs", path.join(camera_root, "shoestring/include"))
        target:add("links", shoestring_lib)
        target:add("links", path.join(camera_root, "shoestring", arch, "ShoestringLXUSB_DLL.lib"))
        target:add("installfiles", path.join(camera_root, "shoestring", arch, "ShoestringGPUSB_DLL.dll"))
        target:add("installfiles", path.join(camera_root, "shoestring", arch, "ShoestringLXUSB_DLL.dll"))
    end
    
    -- Windows system libraries for cameras
    target:add("syslinks", "vfw32", "Strmiids", "Quartz", "winmm")
    
    -- Runtime libraries
    if arch == "x86" then
        target:add("installfiles", "WinLibs/x86/msvcr120.dll")
        target:add("links", "WinLibs/x86/inpout32.lib")
        target:add("installfiles", "WinLibs/x86/inpout32.dll")
    end
    
    target:add("installfiles", "WinLibs/" .. arch .. "/msvcp140.dll")
    target:add("installfiles", "WinLibs/" .. arch .. "/vcomp140.dll")
    target:add("installfiles", "WinLibs/" .. arch .. "/vcruntime140.dll")
    target:add("installfiles", "WinLibs/" .. arch .. "/concrt140.dll")
end

-- macOS camera framework configuration (global scope)
configure_macos_cameras = function(target)
    local camera_root = "cameras"
    
    -- SBIG framework
    local sbig_framework = path.join("thirdparty/frameworks", "SBIGUDrv.framework")
    if os.isdir(sbig_framework) then
        target:add("defines", "HAVE_SBIG_CAMERA=1")
        target:add("frameworks", sbig_framework)
    end
    
    -- ZWO ASI cameras
    local asi_lib = path.join(camera_root, "zwolibs/mac", "libASICamera2.dylib")
    if os.isfile(asi_lib) then
        target:add("defines", "HAVE_ZWO_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "zwolibs/include"))
        target:add("links", asi_lib)
    end
    
    -- QHY cameras
    local qhy_lib = path.join(camera_root, "qhyccdlibs/mac/universal", "libqhyccd.dylib")
    if os.isfile(qhy_lib) then
        target:add("defines", "HAVE_QHY_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "qhyccdlibs/include"))
        target:add("links", qhy_lib)
    end
    
    -- ToupTek cameras
    local toupcam_lib = path.join(camera_root, "toupcam/mac", "libtoupcam.dylib")
    if os.isfile(toupcam_lib) then
        target:add("defines", "HAVE_TOUPTEK_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "toupcam/include"))
        target:add("links", toupcam_lib)
    end
    
    -- SVB cameras
    local svb_lib = path.join(camera_root, "svblibs/mac/x64", "libSVBCameraSDK.dylib")
    if os.isfile(svb_lib) then
        target:add("defines", "HAVE_SVB_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "svblibs/include"))
        target:add("links", svb_lib)
    end
    
    -- PlayerOne cameras
    local playerone_lib = path.join(camera_root, "playerone/mac", "libPlayerOneCamera.dylib")
    if os.isfile(playerone_lib) then
        target:add("defines", "HAVE_PLAYERONE_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "playerone/include"))
        target:add("links", playerone_lib)
    end
    
    -- Starlight Xpress (built-in support)
    target:add("defines", "HAVE_SXV_CAMERA=1")
    
    -- KWIQGuider (macOS only)
    target:add("defines", "HAVE_KWIQGUIDER_CAMERA=1")
    target:add("includedirs", path.join(camera_root, "KWIQGuider"))
    
    -- OpenSSAG
    target:add("defines", "HAVE_OPENSSAG_CAMERA=1")
end

-- Linux camera library configuration (global scope)
configure_linux_cameras = function(target)
    if has_config("opensource_only") then
        -- Only open source camera support
        target:add("defines", "HAVE_SXV_CAMERA=1")
        target:add("defines", "HAVE_OPENSSAG_CAMERA=1")
        return
    end
    
    local camera_root = "cameras"
    local arch = get_linux_camera_arch()
    
    -- ZWO ASI cameras
    local asi_lib = path.join(camera_root, "zwolibs/linux", arch, "libASICamera2.so")
    if os.isfile(asi_lib) then
        target:add("defines", "HAVE_ZWO_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "zwolibs/include"))
        target:add("links", asi_lib)
    end
    
    -- QHY cameras
    local qhy_lib = path.join(camera_root, "qhyccdlibs/linux", arch, "libqhyccd.so")
    if os.isfile(qhy_lib) then
        target:add("defines", "HAVE_QHY_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "qhyccdlibs/include"))
        target:add("links", qhy_lib)
    end
    
    -- ToupTek cameras
    local toupcam_arch = get_toupcam_arch()
    local toupcam_lib = path.join(camera_root, "toupcam/linux", toupcam_arch, "libtoupcam.so")
    if os.isfile(toupcam_lib) then
        target:add("defines", "HAVE_TOUPTEK_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "toupcam/include"))
        target:add("links", toupcam_lib)
    end
    
    -- SVB cameras
    local svb_lib = path.join(camera_root, "svblibs/linux", arch, "libSVBCameraSDK.so")
    if os.isfile(svb_lib) then
        target:add("defines", "HAVE_SVB_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "svblibs/include"))
        target:add("links", svb_lib)
    end
    
    -- PlayerOne cameras
    local playerone_lib = path.join(camera_root, "playerone/linux", arch, "libPlayerOneCamera.so")
    if os.isfile(playerone_lib) then
        target:add("defines", "HAVE_PLAYERONE_CAMERA=1")
        target:add("includedirs", path.join(camera_root, "playerone/include"))
        target:add("links", playerone_lib)
    end
    
    -- Always available on Linux
    target:add("defines", "HAVE_SXV_CAMERA=1")
    target:add("defines", "HAVE_OPENSSAG_CAMERA=1")
end

-- Helper functions for architecture detection (global scope)
get_linux_camera_arch = function()
    if is_arch("x86_64") then
        return "x64"
    elseif is_arch("i386") then
        return "x86"
    elseif is_arch("arm64", "aarch64") then
        return "armv8"
    elseif is_arch("armv7") then
        return "armv7"
    elseif is_arch("armv6") then
        return "armv6"
    else
        return "x64" -- default
    end
end

get_toupcam_arch = function()
    if is_arch("x86_64") then
        return "x64"
    elseif is_arch("i386") then
        return "x86"
    elseif is_arch("arm64", "aarch64") then
        return "arm64"
    elseif is_arch("armv7") then
        return "armhf"
    elseif is_arch("armv6") then
        return "armel"
    else
        return "x64" -- default
    end
end

-- Function to check if camera SDK is available (global scope)
check_camera_sdk = function(sdk_path, sdk_name)
    if os.isfile(sdk_path) then
        print("Found " .. sdk_name .. " SDK: " .. sdk_path)
        return true
    else
        print("Warning: " .. sdk_name .. " SDK not found at: " .. sdk_path)
        return false
    end
end

-- Function to add camera SDK with validation (global scope)
add_camera_sdk = function(target, sdk_name, lib_path, include_path, dll_path, defines)
    if check_camera_sdk(lib_path, sdk_name) then
        target:add("defines", defines)
        if include_path then
            target:add("includedirs", include_path)
        end
        target:add("links", lib_path)
        if dll_path and os.isfile(dll_path) then
            target:add("installfiles", dll_path)
        end
        return true
    end
    return false
end

-- Enhanced Windows camera configuration with better error handling (global scope)
configure_windows_cameras_enhanced = function(target)
    local arch = is_arch("x64") and "x64" or "x86"
    local camera_root = "cameras"
    local camera_count = 0

    print("Configuring Windows camera SDKs for architecture: " .. arch)

    -- ZWO ASI cameras
    local asi_lib = path.join(camera_root, "zwolibs/win", arch, "ASICamera2.lib")
    local asi_dll = path.join(camera_root, "zwolibs/win", arch, "ASICamera2.dll")
    local asi_include = path.join(camera_root, "zwolibs/include")
    if add_camera_sdk(target, "ZWO ASI", asi_lib, asi_include, asi_dll, "HAVE_ZWO_CAMERA=1") then
        camera_count = camera_count + 1
    end

    -- QHY cameras
    local qhy_lib = path.join(camera_root, "qhyccdlibs/win", arch, "qhyccd.lib")
    local qhy_dll = path.join(camera_root, "qhyccdlibs/win", arch, "qhyccd.dll")
    local qhy_include = path.join(camera_root, "qhyccdlibs/include")
    if add_camera_sdk(target, "QHY", qhy_lib, qhy_include, qhy_dll, "HAVE_QHY_CAMERA=1") then
        target:add("installfiles", path.join(camera_root, "qhyccdlibs/win", arch, "tbb.dll"))
        camera_count = camera_count + 1
    end

    -- ToupTek cameras
    local toupcam_lib = path.join(camera_root, "toupcam/win", arch, "toupcam.lib")
    local toupcam_dll = path.join(camera_root, "toupcam/win", arch, "toupcam.dll")
    local toupcam_include = path.join(camera_root, "toupcam/include")
    if add_camera_sdk(target, "ToupTek", toupcam_lib, toupcam_include, toupcam_dll, "HAVE_TOUPTEK_CAMERA=1") then
        camera_count = camera_count + 1
    end

    -- SBIG cameras
    local sbig_lib = path.join(camera_root, "sbig/win", arch, "SBIGUDrv.lib")
    local sbig_include = path.join(camera_root, "sbig/include")
    if add_camera_sdk(target, "SBIG", sbig_lib, sbig_include, nil, "HAVE_SBIG_CAMERA=1") then
        camera_count = camera_count + 1
    end

    -- Altair cameras
    local altair_lib = path.join(camera_root, "altair/win", arch, "altaircam.lib")
    local altair_dll = path.join(camera_root, "altair/win", arch, "altaircam.dll")
    local altair_include = path.join(camera_root, "altair/include")
    if add_camera_sdk(target, "Altair", altair_lib, altair_include, altair_dll, "HAVE_ALTAIR_CAMERA=1") then
        target:add("installfiles", path.join(camera_root, "altair/win", arch, "AltairCam_legacy.dll"))
        camera_count = camera_count + 1
    end

    -- SVB cameras
    local svb_lib = path.join(camera_root, "svblibs/win", arch, "SVBCameraSDK.lib")
    local svb_dll = path.join(camera_root, "svblibs/win", arch, "SVBCameraSDK.dll")
    local svb_include = path.join(camera_root, "svblibs/include")
    if add_camera_sdk(target, "SVB", svb_lib, svb_include, svb_dll, "HAVE_SVB_CAMERA=1") then
        camera_count = camera_count + 1
    end

    -- PlayerOne cameras
    local playerone_lib = path.join(camera_root, "playerone/win", arch, "PlayerOneCamera.lib")
    local playerone_dll = path.join(camera_root, "playerone/win", arch, "PlayerOneCamera.dll")
    local playerone_include = path.join(camera_root, "playerone/include")
    if add_camera_sdk(target, "PlayerOne", playerone_lib, playerone_include, playerone_dll, "HAVE_PLAYERONE_CAMERA=1") then
        camera_count = camera_count + 1
    end

    -- Moravian cameras
    local moravian_lib = path.join(camera_root, "moravian/win", arch, "gXusb.lib")
    local moravian_dll = path.join(camera_root, "moravian/win", arch, "gXusb.dll")
    local moravian_include = path.join(camera_root, "moravian/include")
    if add_camera_sdk(target, "Moravian", moravian_lib, moravian_include, moravian_dll, "HAVE_MORAVIAN_CAMERA=1") then
        camera_count = camera_count + 1
    end

    -- Starlight Xpress
    local sxv_lib = path.join(camera_root, "sxv/win", arch, "SXUSB.lib")
    local sxv_dll = path.join(camera_root, "sxv/win", arch, "SXUSB.dll")
    local sxv_include = path.join(camera_root, "sxv/include")
    if add_camera_sdk(target, "Starlight Xpress", sxv_lib, sxv_include, sxv_dll, "HAVE_SXV_CAMERA=1") then
        camera_count = camera_count + 1
    end

    print("Configured " .. camera_count .. " camera SDKs for Windows")
    return camera_count
end
