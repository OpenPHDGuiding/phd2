-- PHD2 Targets Configuration for xmake
-- Defines all build targets including main executable and libraries

-- Configuration functions will be available in xmake context

-- Source file collections
local cam_sources = {
    "src/cameras/drivers/cam_altair.cpp",
    "src/cameras/drivers/cam_altair.h",
    "src/cameras/drivers/cam_ascom.cpp",
    "src/cameras/drivers/cam_ascom.h", 
    "src/cameras/drivers/cam_atik16.cpp",
    "src/cameras/drivers/cam_atik16.h",
    "src/cameras/drivers/cam_firewire_IC.cpp",
    "src/cameras/drivers/cam_firewire_IC.h",
    "src/cameras/drivers/cam_firewire_OSX.cpp",
    "src/cameras/drivers/cam_firewire_OSX.h",
    "src/cameras/drivers/cam_indi.cpp",
    "src/cameras/drivers/cam_indi.h",
    "src/cameras/drivers/cam_INovaPLC.cpp",
    "src/cameras/drivers/cam_INovaPLC.h",
    "src/cameras/drivers/cam_ioptron.cpp",
    "src/cameras/drivers/cam_ioptron.h",
    "src/cameras/drivers/cam_KWIQGuider.cpp",
    "src/cameras/drivers/cam_KWIQGuider.h",
    "src/cameras/drivers/cam_LEwebcam.cpp",
    "src/cameras/drivers/cam_LEwebcam.h",
    "src/cameras/drivers/cam_MeadeDSI.cpp",
    "src/cameras/drivers/cam_MeadeDSI.h",
    "src/cameras/drivers/cam_moravian.cpp",
    "src/cameras/drivers/cam_moravian.h",
    "src/cameras/drivers/cam_NebSBIG.cpp",
    "src/cameras/drivers/cam_NebSBIG.h",
    "src/cameras/drivers/cam_ogma.cpp",
    "src/cameras/drivers/cam_ogma.h",
    "src/cameras/drivers/cam_opencv.cpp",
    "src/cameras/drivers/cam_opencv.h",
    "src/cameras/drivers/cam_openssag.cpp",
    "src/cameras/drivers/cam_openssag.h",
    "src/cameras/drivers/cam_OSPL130.cpp",
    "src/cameras/drivers/cam_OSPL130.h",
    "src/cameras/drivers/cam_playerone.cpp",
    "src/cameras/drivers/cam_playerone.h",
    "src/cameras/drivers/cam_qguide.cpp",
    "src/cameras/drivers/cam_qguide.h",
    "src/cameras/drivers/cam_qhy.cpp",
    "src/cameras/drivers/cam_qhy.h",
    "src/cameras/drivers/cam_sbig.cpp",
    "src/cameras/drivers/cam_sbig.h",
    "src/cameras/drivers/cam_sbigrotator.cpp",
    "src/cameras/drivers/cam_sbigrotator.h",
    "src/cameras/drivers/cam_skyraider.cpp",
    "src/cameras/drivers/cam_skyraider.h",
    "src/cameras/drivers/cam_sspiag.cpp",
    "src/cameras/drivers/cam_sspiag.h",
    "src/cameras/drivers/cam_starfish.cpp",
    "src/cameras/drivers/cam_starfish.h",
    "src/cameras/drivers/cam_StarShootDSCI.cpp",
    "src/cameras/drivers/cam_StarShootDSCI.h",
    "src/cameras/drivers/cam_svb.cpp",
    "src/cameras/drivers/cam_svb.h",
    "src/cameras/drivers/cam_sxv.cpp",
    "src/cameras/drivers/cam_sxv.h",
    "src/cameras/drivers/cam_touptek.cpp",
    "src/cameras/drivers/cam_touptek.h",
    "src/cameras/drivers/cam_vfw.cpp",
    "src/cameras/drivers/cam_vfw.h",
    "src/cameras/drivers/cam_wdm_base.h",
    "src/cameras/drivers/cam_wdm.cpp",
    "src/cameras/drivers/cam_wdm.h",
    "src/cameras/drivers/cam_zwo.cpp",
    "src/cameras/drivers/cam_zwo.h",
    "src/cameras/camera.cpp",
    "src/cameras/camera.h",
    "src/cameras/cameras.h",
    "src/cameras/camcal_import_dialog.cpp",
    "src/cameras/camcal_import_dialog.h"
}

-- Windows-specific camera sources
if is_plat("windows") then
    table.insert(cam_sources, "src/cameras/drivers/cam_LEParallelWebcam.cpp")
    table.insert(cam_sources, "src/cameras/drivers/cam_LEParallelWebcam.h")
    table.insert(cam_sources, "src/cameras/drivers/cam_LESerialWebcam.cpp")
    table.insert(cam_sources, "src/cameras/drivers/cam_LESerialWebcam.h")
    table.insert(cam_sources, "src/cameras/drivers/cam_LELXUSBWebcam.cpp")
    table.insert(cam_sources, "src/cameras/drivers/cam_LELXUSBWebcam.h")
    table.insert(cam_sources, "cameras/ArtemisHSCAPI.h")
    table.insert(cam_sources, "cameras/ArtemisHSCAPI.cpp")
elseif is_plat("linux") then
    table.insert(cam_sources, "src/cameras/drivers/cam_qhy5.h")
    table.insert(cam_sources, "src/cameras/drivers/cam_qhy5.cpp")
end

local scope_sources = {
    "src/mounts/mount.cpp",
    "src/mounts/mount.h",
    "src/mounts/scope.cpp",
    "src/mounts/scope.h",
    "src/mounts/scopes.h",
    "src/mounts/drivers/scope_ascom.cpp",
    "src/mounts/drivers/scope_ascom.h",
    "src/mounts/drivers/scope_eqmac.cpp",
    "src/mounts/drivers/scope_eqmac.h",
    "src/mounts/drivers/scope_equinox.cpp",
    "src/mounts/drivers/scope_equinox.h",
    "src/mounts/drivers/scope_GC_USBST4.cpp",
    "src/mounts/drivers/scope_GC_USBST4.h",
    "src/mounts/drivers/scope_gpint.cpp",
    "src/mounts/drivers/scope_gpint.h",
    "src/mounts/drivers/scope_gpusb.cpp",
    "src/mounts/drivers/scope_gpusb.h",
    "src/mounts/drivers/scope_manual_pointing.cpp",
    "src/mounts/drivers/scope_manual_pointing.h",
    "src/mounts/drivers/scope_onboard_st4.cpp",
    "src/mounts/drivers/scope_onboard_st4.h",
    "src/mounts/drivers/scope_oncamera.cpp",
    "src/mounts/drivers/scope_oncamera.h",
    "src/mounts/drivers/scope_onstepguider.cpp",
    "src/mounts/drivers/scope_onstepguider.h",
    "src/mounts/drivers/scope_voyager.cpp",
    "src/mounts/drivers/scope_voyager.h",
    "src/mounts/drivers/scope_indi.cpp",
    "src/mounts/drivers/scope_indi.h",
    "src/stepguiders/stepguider_sxao.cpp",
    "src/stepguiders/stepguider_sxao.h",
    "src/stepguiders/stepguider_sxao_indi.cpp",
    "src/stepguiders/stepguider_sxao_indi.h",
    "src/stepguiders/stepguider_sbigao_indi.cpp",
    "src/stepguiders/stepguider_sbigao_indi.h",
    "src/stepguiders/stepguider.cpp",
    "src/stepguiders/stepguider.h",
    "src/stepguiders/stepguiders.h"
}

local guiding_sources = {
    "src/guiding/backlash_comp.cpp",
    "src/guiding/backlash_comp.h",
    "src/guiding/algorithms/guide_algorithm_hysteresis.cpp",
    "src/guiding/algorithms/guide_algorithm_hysteresis.h",
    "src/guiding/algorithms/guide_algorithm_gaussian_process.cpp",
    "src/guiding/algorithms/guide_algorithm_gaussian_process.h",
    "src/guiding/algorithms/guide_algorithm_identity.cpp",
    "src/guiding/algorithms/guide_algorithm_identity.h",
    "src/guiding/algorithms/guide_algorithm_lowpass.cpp",
    "src/guiding/algorithms/guide_algorithm_lowpass.h",
    "src/guiding/algorithms/guide_algorithm_lowpass2.cpp",
    "src/guiding/algorithms/guide_algorithm_lowpass2.h",
    "src/guiding/algorithms/guide_algorithm_resistswitch.cpp",
    "src/guiding/algorithms/guide_algorithm_resistswitch.h",
    "src/guiding/algorithms/guide_algorithm_zfilter.cpp",
    "src/guiding/algorithms/guide_algorithm_zfilter.h",
    "src/guiding/guide_algorithm.cpp",
    "src/guiding/guide_algorithm.h",
    "src/guiding/guide_algorithms.h",
    "src/guiding/guider_multistar.cpp",
    "src/guiding/guider_multistar.h",
    "src/guiding/guider.cpp",
    "src/guiding/guider.h",
    "src/guiding/guiders.h",
    "src/guiding/zfilterfactory.cpp",
    "src/guiding/zfilterfactory.h",
    "src/guiding/calibration_assistant.cpp",
    "src/guiding/calibration_assistant.h",
    "src/guiding/guiding_assistant.cpp",
    "src/guiding/guiding_assistant.h"
}

local core_sources = {
    "src/ui/dialogs/about_dialog.cpp",
    "src/ui/dialogs/about_dialog.h",
    "src/ui/dialogs/advanced_dialog.cpp",
    "src/ui/dialogs/advanced_dialog.h",
    "src/ui/aui_controls.cpp",
    "src/ui/aui_controls.h",
    "src/ui/dialogs/calreview_dialog.cpp",
    "src/ui/dialogs/calreview_dialog.h",
    "src/ui/dialogs/calstep_dialog.cpp",
    "src/ui/dialogs/calstep_dialog.h",
    "src/utilities/circbuf.h",
    "src/ui/tools/comet_tool.cpp",
    "src/ui/tools/comet_tool.h",
    "src/core/config_indi.cpp",
    "src/core/config_indi.h",
    "src/core/configdialog.cpp",
    "src/core/configdialog.h",
    "src/ui/dialogs/confirm_dialog.cpp",
    "src/ui/dialogs/confirm_dialog.h",
    "src/ui/dialogs/darks_dialog.cpp",
    "src/ui/dialogs/darks_dialog.h",
    "src/logging/debuglog.cpp",
    "src/logging/debuglog.h",
    "src/ui/tools/drift_tool.cpp",
    "src/ui/tools/drift_tool.h",
    "src/utilities/eegg.cpp",
    "src/communication/network/event_server.cpp",
    "src/communication/network/event_server.h",
    "src/utilities/fitsiowrap.cpp",
    "src/utilities/fitsiowrap.h",
    "src/ui/dialogs/gear_dialog.cpp",
    "src/ui/dialogs/gear_dialog.h",
    "src/ui/gear_simulator.cpp",
    "src/ui/gear_simulator.h",
    "src/ui/graph-stepguider.cpp",
    "src/ui/graph-stepguider.h",
    "src/ui/graph.cpp",
    "src/ui/graph.h",
    "src/logging/guidinglog.cpp",
    "src/logging/guidinglog.h",
    "src/logging/guiding_stats.cpp",
    "src/logging/guiding_stats.h",
    "src/core/image_math.cpp",
    "src/core/image_math.h",
    "src/logging/imagelogger.cpp",
    "src/logging/imagelogger.h",
    "src/core/indi_gui.cpp",
    "src/core/indi_gui.h",
    "src/utilities/json_parser.cpp",
    "src/utilities/json_parser.h",
    "src/logging/logger.cpp",
    "src/logging/logger.h",
    "src/logging/log_uploader.cpp",
    "src/logging/log_uploader.h",
    "src/ui/dialogs/manualcal_dialog.cpp",
    "src/ui/dialogs/manualcal_dialog.h",
    "src/utilities/messagebox_proxy.cpp",
    "src/utilities/messagebox_proxy.h",
    "src/ui/myframe.cpp",
    "src/ui/myframe.h",
    "src/ui/myframe_events.cpp",
    "src/utilities/nudge_lock.cpp",
    "src/utilities/nudge_lock.h",
    "src/communication/onboard_st4.cpp",
    "src/communication/onboard_st4.h",
    "src/ui/optionsbutton.cpp",
    "src/ui/optionsbutton.h",
    "src/core/phd.cpp",
    "src/core/phd.h",
    "src/core/phdconfig.cpp",
    "src/core/phdconfig.h",
    "src/core/phdcontrol.cpp",
    "src/core/phdcontrol.h",
    "src/core/phdupdate.cpp",
    "src/core/phdupdate.h",
    "src/ui/tools/pierflip_tool.cpp",
    "src/ui/tools/pierflip_tool.h",
    "src/ui/tools/polardrift_tool.h",
    "src/ui/tools/polardrift_toolwin.cpp",
    "src/ui/tools/polardrift_toolwin.h",
    "src/ui/profile_wizard.cpp",
    "src/ui/profile_wizard.h",
    "src/utilities/point.h",
    "src/utilities/Refine_DefMap.cpp",
    "src/utilities/Refine_DefMap.h",
    "src/rotators/rotators.h",
    "src/rotators/rotator.cpp",
    "src/rotators/rotator.h",
    "src/rotators/rotator_ascom.cpp",
    "src/rotators/rotator_ascom.h",
    "src/rotators/rotator_indi.cpp",
    "src/rotators/rotator_indi.h",
    "src/utilities/runinbg.cpp",
    "src/utilities/runinbg.h",
    "src/communication/serial/serialport.cpp",
    "src/communication/serial/serialport.h",
    "src/communication/serial/serialport_loopback.cpp",
    "src/communication/serial/serialport_loopback.h",
    "src/communication/serial/serialport_mac.cpp",
    "src/communication/serial/serialport_mac.h",
    "src/communication/serial/serialport_win32.cpp",
    "src/communication/serial/serialport_win32.h",
    "src/communication/serial/serialport_posix.cpp",
    "src/communication/serial/serialport_posix.h",
    "src/communication/serial/serialports.h",
    "src/utilities/sha1.cpp",
    "src/utilities/sha1.h",
    "src/communication/network/socket_server.cpp",
    "src/communication/network/socket_server.h",
    "src/core/starcross_test.cpp",
    "src/core/starcross_test.h",
    "src/ui/tools/staticpa_tool.h",
    "src/ui/tools/staticpa_toolwin.cpp",
    "src/ui/tools/staticpa_toolwin.h",
    "src/ui/statswindow.cpp",
    "src/ui/statswindow.h",
    "src/core/star.cpp",
    "src/core/star.h",
    "src/core/star_profile.cpp",
    "src/core/star_profile.h",
    "src/core/target.cpp",
    "src/core/target.h",
    "src/core/testguide.cpp",
    "src/core/testguide.h",
    "src/core/usImage.cpp",
    "src/core/usImage.h",
    "src/core/worker_thread.cpp",
    "src/core/worker_thread.h",
    "src/ui/wxled.cpp",
    "src/ui/wxled.h"
}

-- Windows-specific sources
if is_plat("windows") then
    table.insert(core_sources, "src/communication/parallel/parallelports.h")
    table.insert(core_sources, "src/communication/parallel/parallelport.h")
    table.insert(core_sources, "src/communication/parallel/parallelport.cpp")
    table.insert(core_sources, "src/communication/parallel/parallelport_win32.h")
    table.insert(core_sources, "src/communication/parallel/parallelport_win32.cpp")
    table.insert(core_sources, "src/communication/comdispatch.h")
    table.insert(core_sources, "src/communication/comdispatch.cpp")
end

-- Gaussian Process libraries (contributions)
target("MPIIS_GP_TOOLS")
    set_kind("static")
    add_files("contributions/MPI_IS_gaussian_process/tools/math_tools.cpp")
    add_headerfiles("contributions/MPI_IS_gaussian_process/tools/math_tools.h")
    add_includedirs("contributions/MPI_IS_gaussian_process/src", {public = true})
    add_includedirs("contributions/MPI_IS_gaussian_process/tools", {public = true})
    on_load(function (target)
        configure_eigen3(target)
    end)
    set_group("Contributions")

target("MPIIS_GP")
    set_kind("static")
    add_files("contributions/MPI_IS_gaussian_process/src/gaussian_process.cpp")
    add_files("contributions/MPI_IS_gaussian_process/src/covariance_functions.cpp")
    add_headerfiles("contributions/MPI_IS_gaussian_process/src/*.h")
    add_deps("MPIIS_GP_TOOLS")
    add_includedirs("contributions/MPI_IS_gaussian_process/src", {public = true})
    add_includedirs("contributions/MPI_IS_gaussian_process/tools", {public = true})
    on_load(function (target)
        configure_eigen3(target)
    end)
    set_group("Contributions")

target("GPGuider")
    set_kind("static")
    add_files("contributions/MPI_IS_gaussian_process/src/gaussian_process_guider.cpp")
    add_headerfiles("contributions/MPI_IS_gaussian_process/src/gaussian_process_guider.h")
    add_deps("MPIIS_GP_TOOLS", "MPIIS_GP")
    add_includedirs("contributions/MPI_IS_gaussian_process/src", {public = true})
    add_includedirs("contributions/MPI_IS_gaussian_process/tools", {public = true})
    add_includedirs("src", {public = true})
    on_load(function (target)
        configure_eigen3(target)
    end)
    set_group("Contributions")

-- Build third-party library targets using dependency functions
-- All targets are built automatically when their conditions are met
-- build_libusb_target() is called conditionally in dependencies.lua
-- build_openssag_target() is called conditionally in dependencies.lua
-- build_vidcapture_target() is called conditionally in dependencies.lua
-- build_indi_target() is called conditionally in dependencies.lua

-- Main PHD2 executable target
target("phd2")
    set_kind("binary")

    -- Add all source files
    for _, file in ipairs(cam_sources) do
        add_files(file)
    end
    for _, file in ipairs(scope_sources) do
        add_files(file)
    end
    for _, file in ipairs(guiding_sources) do
        add_files(file)
    end
    for _, file in ipairs(core_sources) do
        add_files(file)
    end

    -- Platform-specific additional sources
    if is_plat("unix", "linux") and not is_plat("macosx") then
        add_files("cameras/SXMacLib.c")
    end

    if is_plat("macosx") then
        add_files("cameras/SXMacLib.c")
        add_files("cameras/KWIQGuider/*.cpp")
    end

    -- Add dependencies on contribution libraries
    add_deps("MPIIS_GP", "GPGuider")

    -- Add dependencies on documentation and localization (matching CMake)
    add_deps("documentation", "locales")

    -- Add third-party library dependencies
    if not is_plat("windows") then
        if not has_config("use_system_libusb") then
            add_deps("usb_openphd")
        end
        if not has_config("opensource_only") then
            add_deps("OpenSSAG")
        end
    end

    if is_plat("windows") and is_arch("x86") then
        add_deps("VidCapture")
    end

    if not has_config("use_system_libindi") then
        add_deps("indi_client")
    end

    -- Configure external dependencies using on_load
    on_load(function (target)
        configure_wxwidgets(target)
        configure_cfitsio(target)
        configure_curl(target)
        configure_eigen3(target)
        configure_opencv(target)
        configure_libusb(target)
        configure_indi(target)
        
        -- Configure camera SDKs
        configure_camera_sdks(target)
        
        -- Configure platform-specific settings
        configure_platform_specific(target)
        configure_debug_tools(target)
        configure_optimization(target)
        
        -- Configure localization
        if configure_localization then
            configure_localization(target)
        end
    end)

    -- Platform-specific configuration
    if is_plat("windows") then
        add_files("phd.rc")
        set_filename("phd2.exe")

        -- Windows-specific linker flags
        if is_mode("debug") then
            add_ldflags("/NODEFAULTLIB:libcmtd.lib", "/NODEFAULTLIB:msvcrt.lib")
        else
            add_ldflags("/DELAYLOAD:sbigudrv.dll", "/NODEFAULTLIB:libcmt.lib")
        end

        -- Visual Leak Detector support
        local vld_include = os.getenv("VLD_DIR") or "C:/Program Files (x86)/Visual Leak Detector"
        if os.isdir(path.join(vld_include, "include")) then
            add_defines("HAVE_VLD=1")
            add_includedirs(path.join(vld_include, "include"))
        end

    elseif is_plat("macosx") then
        set_kind("binary")
        set_filename("PHD2")

        -- macOS bundle configuration
        add_files("icons/PHD_OSX_icon.icns")
        add_files("MacOSXBundleInfo.plist.in")

        -- Set bundle properties
        set_values("xcode.bundle_identifier", "org.openphdguiding.phd2")
        set_values("xcode.bundle_version", "$(version)")

    elseif is_plat("linux") then
        set_filename("phd2.bin")

        -- Linux installation script
        add_files("scripts/phd2.sh.in")

        -- Math library (should be last)
        add_syslinks("m")
    end

    -- Common defines for all platforms
    add_defines("HAVE_TYPE_TRAITS")

    -- Platform-specific defines
    if is_plat("unix", "linux") and not is_plat("macosx") then
        add_defines("HAVE_SXV_CAMERA=1")
        if not has_config("opensource_only") then
            add_defines("HAVE_OPENSSAG_CAMERA=1")
        end
    elseif is_plat("macosx") then
        add_defines("HAVE_SXV_CAMERA=1", "HAVE_KWIQGUIDER_CAMERA=1")
        if not has_config("opensource_only") then
            add_defines("HAVE_OPENSSAG_CAMERA=1")
        end
    end

-- Unit tests for Gaussian Process libraries
target("GaussianProcessTest")
    set_kind("binary")
    add_files("contributions/MPI_IS_gaussian_process/tests/gaussian_process/gaussian_process_test.cpp")
    add_deps("MPIIS_GP")
    add_includedirs("contributions/MPI_IS_gaussian_process/tools")
    on_load(function (target)
        configure_gtest(target)
    end)
    set_group("Tests")

target("MathToolboxTest")
    set_kind("binary")
    add_files("contributions/MPI_IS_gaussian_process/tests/gaussian_process/math_tools_test.cpp")
    add_deps("MPIIS_GP_TOOLS")
    add_includedirs("contributions/MPI_IS_gaussian_process/src")
    on_load(function (target)
        configure_eigen3(target)
        configure_gtest(target)
    end)
    set_group("Tests")

target("GPGuiderTest")
    set_kind("binary")
    add_files("contributions/MPI_IS_gaussian_process/tests/gaussian_process/gp_guider_test.cpp")
    add_deps("MPIIS_GP", "GPGuider")
    add_includedirs("contributions/MPI_IS_gaussian_process/tools")
    add_includedirs("src")
    on_load(function (target)
        configure_gtest(target)
    end)
    set_group("Tests")

target("GuidePerformanceTest")
    set_kind("binary")
    add_files("contributions/MPI_IS_gaussian_process/tests/gaussian_process/guide_performance_test.cpp")
    add_deps("MPIIS_GP", "GPGuider")
    add_includedirs("contributions/MPI_IS_gaussian_process/tools")
    add_includedirs("src")
    on_load(function (target)
        configure_gtest(target)
    end)
    set_group("Tests")

target("GuidePerformanceEval")
    set_kind("binary")
    add_files("contributions/MPI_IS_gaussian_process/tests/gaussian_process/evaluate_performance.cpp")
    add_deps("MPIIS_GP", "GPGuider")
    add_includedirs("contributions/MPI_IS_gaussian_process/tools")
    add_includedirs("src")
    on_load(function (target)
        configure_gtest(target)
    end)
    set_group("Tests")

-- Set up localization and documentation targets
-- These are now handled by the included modules automatically

-- Create installation target
target("install")
    set_kind("phony")
    add_deps("phd2", "documentation")
    set_group("Installation")

    on_install(function (target)
        local install_dirs = plat.get_install_dirs()

        -- Install main executable
        local exe_name = is_plat("windows") and "phd2.exe" or (is_plat("macosx") and "PHD2" or "phd2.bin")
        local exe_path = path.join("$(buildir)", exe_name)
        local install_exe_path = path.join(install_dirs.bindir, exe_name)

        if os.isfile(exe_path) then
            os.cp(exe_path, install_exe_path)
            print("Installed " .. install_exe_path)
        end

        -- Install additional files based on platform
        if is_plat("linux") then
            -- Install launcher script
            local script_path = path.join("scripts", "phd2.sh")
            if os.isfile(script_path) then
                os.cp(script_path, path.join(install_dirs.bindir, "phd2"))
                os.chmod(path.join(install_dirs.bindir, "phd2"), "755")
            end
        end
    end)

-- Create package target
target("package")
    set_kind("phony")
    add_deps("install")
    set_group("Packaging")

    on_build(function (target)
        if is_plat("windows") then
            print("Windows packaging would create installer using NSIS or similar")
        elseif is_plat("macosx") then
            print("macOS packaging would create .dmg bundle")
        elseif is_plat("linux") then
            print("Linux packaging would create .deb/.rpm packages")
        end
    end)
