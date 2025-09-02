-- PHD2 Localization and Documentation Configuration for xmake
-- Handles gettext internationalization and help system

-- Import platform utilities (will be available when called from xmake context)
local plat

-- Available languages for PHD2
local languages = {
    "ca", "cs", "da", "de", "el", "es", "fr", "it", "ja", "ko", "nl", "pl", "pt", "ru", "sv", "zh_CN", "zh_TW"
}

-- Function to find gettext tools
function find_gettext_tools()
    local tools = {}

    -- Simple tool detection - assume tools are in PATH or common locations
    if is_plat("windows") then
        local paths = {
            "C:/Program Files/gettext-iconv/bin/msgfmt.exe",
            "C:/Program Files (x86)/gettext-iconv/bin/msgfmt.exe",
            "C:/msys64/usr/bin/msgfmt.exe",
            "C:/tools/msys64/usr/bin/msgfmt.exe"
        }
        for _, p in ipairs(paths) do
            if os.isfile(p) then
                tools.msgfmt = p
                break
            end
        end

        local xgettext_paths = {
            "C:/Program Files/gettext-iconv/bin/xgettext.exe",
            "C:/Program Files (x86)/gettext-iconv/bin/xgettext.exe",
            "C:/msys64/usr/bin/xgettext.exe",
            "C:/tools/msys64/usr/bin/xgettext.exe"
        }
        for _, p in ipairs(xgettext_paths) do
            if os.isfile(p) then
                tools.xgettext = p
                break
            end
        end
    else
        -- Unix-like systems - check common paths
        local msgfmt_paths = {"/usr/bin/msgfmt", "/usr/local/bin/msgfmt"}
        for _, p in ipairs(msgfmt_paths) do
            if os.isfile(p) then
                tools.msgfmt = p
                break
            end
        end

        local xgettext_paths = {"/usr/bin/xgettext", "/usr/local/bin/xgettext"}
        for _, p in ipairs(xgettext_paths) do
            if os.isfile(p) then
                tools.xgettext = p
                break
            end
        end
    end

    -- Fallback to assuming tools are in PATH
    if not tools.msgfmt then
        tools.msgfmt = "msgfmt"
    end
    if not tools.xgettext then
        tools.xgettext = "xgettext"
    end

    return tools
end

-- Function to create locale targets
function create_locale_targets()
    local tools = find_gettext_tools()
    
    if not tools.msgfmt then
        print("Warning: msgfmt not found. Localization targets will be disabled.")
        return
    end
    
    -- Create individual locale targets
    for _, lang in ipairs(languages) do
        target("locale_" .. lang)
            set_kind("phony")
            set_group("Localization")
            
            -- Input and output files
            local po_file = path.join("locale", lang .. ".po")
            local mo_file = path.join("$(buildir)", "locale", lang, "LC_MESSAGES", "phd2.mo")
            
            -- Custom build rule
            add_rules("utils.bin2c", {extensions = {".po"}})
            
            -- Build .mo file from .po file
            on_build(function (target)
                local po_path = path.join(os.projectdir(), "locale", lang .. ".po")
                local mo_dir = path.join(target:targetdir(), "locale", lang, "LC_MESSAGES")
                local mo_path = path.join(mo_dir, "phd2.mo")
                
                if os.isfile(po_path) then
                    os.mkdir(mo_dir)
                    os.execv(tools.msgfmt, {"-o", mo_path, po_path})
                    print("Generated " .. mo_path)
                else
                    print("Warning: " .. po_path .. " not found")
                end
            end)
            
            -- Install rule
            on_install(function (target)
                -- Use standard install directories
                local install_base = "/usr/local/share/locale"
                if is_plat("windows") then
                    install_base = "locale"
                end

                local mo_file = path.join(target:targetdir(), "locale", lang, "LC_MESSAGES", "phd2.mo")
                local install_path = path.join(install_base, lang, "LC_MESSAGES", "phd2.mo")

                if os.isfile(mo_file) then
                    os.cp(mo_file, install_path)
                    print("Installed " .. install_path)
                end
            end)
    end
    
    -- Create master locale target
    target("locales")
        set_kind("phony")
        set_group("Localization")
        
        -- Depend on all individual locale targets
        for _, lang in ipairs(languages) do
            add_deps("locale_" .. lang)
        end
        
        -- Update .pot file from source code
        if tools.xgettext then
            on_build(function (target)
                local pot_file = path.join(os.projectdir(), "locale", "messages.pot")
                local source_files = {}
                
                -- Collect all C++ source files
                for _, file in ipairs(os.files("src/**.cpp")) do
                    table.insert(source_files, file)
                end
                for _, file in ipairs(os.files("src/**.h")) do
                    table.insert(source_files, file)
                end
                
                -- Extract translatable strings
                local args = {
                    "--language=C++",
                    "--keyword=_",
                    "--keyword=wxTRANSLATE",
                    "--keyword=wxPLURAL:1,2",
                    "--add-comments=TRANSLATORS:",
                    "--from-code=UTF-8",
                    "--package-name=PHD2",
                    "--package-version=" .. get_config("version") or "2.6.13",
                    "--msgid-bugs-address=openphdguiding-devel@lists.sourceforge.net",
                    "-o", pot_file
                }
                
                for _, file in ipairs(source_files) do
                    table.insert(args, file)
                end
                
                os.execv(tools.xgettext, args)
                print("Updated " .. pot_file)
            end)
        end
end

-- Function to create additional resource targets
function create_resource_targets()
    -- Dark theme files
    target("dark_theme")
        set_kind("phony")
        set_group("Resources")

        on_build(function (target)
            local theme_dir = path.join(target:targetdir(), "themes")
            os.mkdir(theme_dir)

            if os.isdir("themes") then
                os.cp("themes/*", theme_dir)
                print("Copied theme files to " .. theme_dir)
            end
        end)

        on_install(function (target)
            local install_base = "/usr/local/share/phd2"
            if is_plat("windows") then
                install_base = "share/phd2"
            end
            local theme_install_dir = path.join(install_base, "themes")

            if os.isdir(path.join(target:targetdir(), "themes")) then
                os.cp(path.join(target:targetdir(), "themes", "*"), theme_install_dir)
                print("Installed theme files to " .. theme_install_dir)
            end
        end)

    -- User manual (basic documentation files)
    target("user_manual")
        set_kind("phony")
        set_group("Resources")

        on_build(function (target)
            local doc_dir = path.join(target:targetdir(), "doc")
            os.mkdir(doc_dir)

            -- Copy documentation files
            local doc_files = {"README.md", "COPYING", "CHANGELOG.md"}
            for _, file in ipairs(doc_files) do
                if os.isfile(file) then
                    os.cp(file, doc_dir)
                end
            end

            print("Copied documentation files to " .. doc_dir)
        end)

        on_install(function (target)
            local install_base = "/usr/local/share/doc/phd2"
            if is_plat("windows") then
                install_base = "doc"
            end

            if os.isdir(path.join(target:targetdir(), "doc")) then
                os.cp(path.join(target:targetdir(), "doc", "*"), install_base)
                print("Installed documentation to " .. install_base)
            end
        end)
end

-- Function to configure localization for a target
function configure_localization(target)
    -- Add locale directory to search path
    if is_plat("windows") then
        target:add("defines", 'PHD_LOCALE_DIR="locale"')
    elseif is_plat("macosx") then
        target:add("defines", 'PHD_LOCALE_DIR="Contents/Resources/locale"')
    else
        target:add("defines", 'PHD_LOCALE_DIR="/usr/local/share/locale"')
    end
    
    -- Enable gettext support
    target:add("defines", "HAVE_GETTEXT=1")
    
    -- Link with gettext library if needed
    if is_plat("windows") then
        -- Windows typically uses static linking or bundled gettext
        local gettext_lib = find_library("intl")
        if gettext_lib then
            target:add("links", "intl")
        end
    elseif is_plat("macosx") then
        -- macOS may need explicit gettext linking
        local gettext_lib = find_library("intl")
        if gettext_lib then
            target:add("links", "intl")
        end
    else
        -- Linux typically has gettext built into glibc
        -- No additional linking needed
    end
end

-- Main function to set up localization and resources
function setup_localization_and_resources()
    create_locale_targets()
    create_resource_targets()

    -- Create master resources target (basic resources, not HTML documentation)
    target("resources")
        set_kind("phony")
        set_group("Resources")
        add_deps("locales", "dark_theme", "user_manual")
end

-- Call setup function when module is loaded
setup_localization_and_resources()
