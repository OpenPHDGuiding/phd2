-- PHD2 Documentation Processing for xmake
-- Handles HTML help generation, multi-language documentation, and packaging
-- Provides equivalent functionality to cmake's PHD2BuildDoc.cmake

-- Required modules will be available in xmake context

-- Available languages for documentation (based on locale directory structure)
local doc_languages = {
    "en_EN", "fr_FR", "de_DE", "es_ES", "it_IT", "ja_JP", "ko_KR", 
    "zh_CN", "zh_TW", "ru_RU", "pl_PL", "cs_CZ", "da_DK", "nl_NL", "sv_SE"
}

-- Default locale
local default_locale = "en_EN"

-- Build folder names (matching cmake configuration)
local html_build_folder = "tmp_build_html"
local translation_build_folder = "tmp_build_translations"

-- Function to find required tools (global scope)
find_documentation_tools = function()
    local tools = {}

    -- Find zip command for packaging
    if is_plat("windows") then
        -- Try common Windows locations
        local paths = {
            "C:/Program Files/7-Zip/7z.exe",
            "C:/Program Files (x86)/7-Zip/7z.exe",
            "C:/tools/7zip/7z.exe"
        }
        for _, p in ipairs(paths) do
            if os.isfile(p) then
                tools.zip = p
                tools.zip_is_7z = true
                break
            end
        end

        -- Find HTML Help Compiler
        local hhc_paths = {
            "C:/Program Files (x86)/HTML Help Workshop/hhc.exe",
            "C:/Program Files/HTML Help Workshop/hhc.exe"
        }
        for _, p in ipairs(hhc_paths) do
            if os.isfile(p) then
                tools.hhc = p
                break
            end
        end
    else
        -- Unix-like systems
        local zip_paths = {"/usr/bin/zip", "/usr/local/bin/zip"}
        for _, p in ipairs(zip_paths) do
            if os.isfile(p) then
                tools.zip = p
                break
            end
        end
    end

    -- Fallback to assuming zip is in PATH
    if not tools.zip then
        tools.zip = "zip"
    end

    return tools
end

-- Function to parse .hhp file and extract HTML file list (global scope)
parse_hhp_file = function(hhp_path, base_folder)
    if not os.isfile(hhp_path) then
        return {}
    end
    
    local content = io.readfile(hhp_path)
    if not content then
        return {}
    end
    
    local files = {}
    local in_files_section = false
    
    for line in content:gmatch("[^\r\n]+") do
        line = line:trim()
        
        if line == "[FILES]" then
            in_files_section = true
        elseif line:startswith("[") and line:endswith("]") then
            in_files_section = false
        elseif in_files_section and line ~= "" then
            -- Check if it's an HTML file
            if line:match("%.html?$") then
                local file_path = path.join(base_folder, line)
                if os.isfile(file_path) then
                    table.insert(files, file_path)
                else
                    print("Warning: HTML file not found: " .. file_path)
                end
            end
        end
    end
    
    return files
end

-- Function to generate .hhk index file (global scope)
generate_hhk_file = function(html_files, output_file)
    local hhk_content = [[<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
<meta name="GENERATOR" content="xmake PHD2 Documentation Builder">
<!-- Sitemap 1.0 -->
</HEAD><BODY>
<UL>
]]
    
    -- Extract titles from HTML files and create index entries
    for _, html_file in ipairs(html_files) do
        local filename = path.filename(html_file)
        local title = filename:gsub("%.html?$", ""):gsub("_", " ")
        
        -- Try to extract actual title from HTML file
        if os.isfile(html_file) then
            local html_content = io.readfile(html_file)
            if html_content then
                local extracted_title = html_content:match("<title>(.-)</title>")
                if extracted_title then
                    title = extracted_title
                end
            end
        end
        
        hhk_content = hhk_content .. string.format([[
	<LI> <OBJECT type="text/sitemap">
		<param name="Name" value="%s">
		<param name="Local" value="%s">
		</OBJECT>
]], title, filename)
    end
    
    hhk_content = hhk_content .. [[
</UL>
</BODY></HTML>
]]
    
    -- Ensure output directory exists
    local output_dir = path.directory(output_file)
    if not os.isdir(output_dir) then
        os.mkdir(output_dir)
    end
    
    -- Write the .hhk file
    io.writefile(output_file, hhk_content)
    return true
end

-- Function to package documentation files
function package_documentation(input_folder, output_folder, locale, tools)
    local zip_file = path.join(output_folder, "PHD2GuideHelp.zip")
    
    -- Ensure output directory exists
    if not os.isdir(output_folder) then
        os.mkdir(output_folder)
    end
    
    -- Copy all documentation files to output folder first
    if os.isdir(input_folder) then
        os.cp(path.join(input_folder, "*"), output_folder)
    end
    
    -- Create zip file
    if tools.zip then
        local old_dir = os.curdir()
        os.cd(output_folder)
        
        local files_to_zip = {}
        for _, file in ipairs(os.files("*")) do
            if not file:endswith(".zip") then
                table.insert(files_to_zip, path.filename(file))
            end
        end
        
        if #files_to_zip > 0 then
            if tools.zip_is_7z then
                -- Use 7-Zip syntax
                os.execv(tools.zip, {"a", "-tzip", zip_file, table.unpack(files_to_zip)})
            else
                -- Use standard zip syntax
                os.execv(tools.zip, {"-r", zip_file, table.unpack(files_to_zip)})
            end
            print("Created documentation package: " .. zip_file)
        end
        
        os.cd(old_dir)
    else
        print("Warning: zip tool not found, skipping documentation packaging")
    end
    
    return zip_file
end

-- Function to detect available documentation languages
function detect_documentation_languages()
    local available_langs = {}

    -- Check for default English documentation
    local project_dir = os.projectdir()
    local help_dir = path.join(project_dir, "help")
    local hhp_file = path.join(help_dir, "PHD2GuideHelp.hhp")

    if os.isdir(help_dir) and os.isfile(hhp_file) then
        table.insert(available_langs, default_locale)
        print("Found English documentation in: " .. help_dir)
    else
        print("Debug: project_dir=" .. project_dir)
        print("Debug: help_dir=" .. help_dir .. ", exists=" .. tostring(os.isdir(help_dir)))
        print("Debug: hhp_file=" .. hhp_file .. ", exists=" .. tostring(os.isfile(hhp_file)))
    end

    -- Check for localized documentation
    local locale_dir = path.join(project_dir, "locale")
    if os.isdir(locale_dir) then
        for _, lang in ipairs(doc_languages) do
            local locale_help_dir = path.join(locale_dir, lang, "help")
            local locale_hhp = path.join(locale_help_dir, "PHD2GuideHelp.hhp")
            if os.isdir(locale_help_dir) and os.isfile(locale_hhp) then
                table.insert(available_langs, lang)
                print("Found " .. lang .. " documentation in: " .. locale_help_dir)
            end
        end
    end

    return available_langs
end

-- Function to create documentation target for a specific locale
function create_locale_documentation_target(locale)
    local target_name = locale .. "_html"

    target(target_name)
        set_kind("phony")
        set_group("Documentation")

        on_build(function (target)
            local tools = find_documentation_tools()

            -- Determine input and output folders
            local input_folder, hhp_file
            if locale == default_locale then
                input_folder = "help"
                hhp_file = path.join(input_folder, "PHD2GuideHelp.hhp")
            else
                input_folder = path.join("locale", locale, "help")
                hhp_file = path.join(input_folder, "PHD2GuideHelp.hhp")
            end

            local output_folder = path.join("$(buildir)", html_build_folder, locale)

            -- Check if documentation exists for this locale
            if not os.isfile(hhp_file) then
                print("Warning: No documentation found for locale " .. locale)
                return
            end

            print("Building documentation for locale: " .. locale)

            -- Parse .hhp file to get HTML file list
            local html_files = parse_hhp_file(hhp_file, input_folder)
            if #html_files == 0 then
                print("Warning: No HTML files found in " .. hhp_file)
                return
            end

            -- Generate .hhk index file
            local hhk_file = path.join(output_folder, "PHD2GuideHelp.hhk")
            if generate_hhk_file(html_files, hhk_file) then
                print("Generated index file: " .. hhk_file)
            end

            -- Package documentation
            package_documentation(input_folder, output_folder, locale, tools)
        end)
end

-- Function to create all documentation targets
function create_documentation_targets()
    local available_langs = detect_documentation_languages()

    if #available_langs == 0 then
        print("Warning: No documentation found in help/ or locale/*/help/ directories")
        return
    end

    print("Found documentation for languages: " .. table.concat(available_langs, ", "))

    -- Create individual locale documentation targets
    for _, locale in ipairs(available_langs) do
        create_locale_documentation_target(locale)
    end

    -- Create master documentation target
    target("documentation")
        set_kind("phony")
        set_group("Documentation")

        -- Depend on all locale documentation targets
        for _, locale in ipairs(available_langs) do
            add_deps(locale .. "_html")
        end

        on_build(function (target)
            print("All documentation targets completed")
        end)
end

-- Function to create translation targets (integrates with localization.lua)
function create_translation_targets()
    local available_langs = detect_documentation_languages()

    for _, locale in ipairs(available_langs) do
        if locale ~= default_locale then
            local target_name = locale .. "_translation"

            target(target_name)
                set_kind("phony")
                set_group("Documentation/Translations")

                on_build(function (target)
                    local po_file = path.join("locale", locale .. ".po")
                    local mo_dir = path.join("$(buildir)", translation_build_folder, locale, "LC_MESSAGES")
                    local mo_file = path.join(mo_dir, "phd2.mo")

                    if os.isfile(po_file) then
                        -- This will be handled by localization.lua
                        print("Translation target for " .. locale .. " (handled by localization module)")
                    else
                        print("Warning: Translation file not found: " .. po_file)
                    end
                end)
        end
    end
end

-- Main setup function
function setup_documentation()
    create_documentation_targets()
    create_translation_targets()

    -- Create a combined target for all documentation and translations
    target("docs_and_translations")
        set_kind("phony")
        set_group("Documentation")
        add_deps("documentation")

        -- Add translation dependencies if they exist
        local available_langs = detect_documentation_languages()
        for _, locale in ipairs(available_langs) do
            if locale ~= default_locale then
                add_deps(locale .. "_translation")
            end
        end
end

-- Call setup function when module is loaded (only once)
if not _DOCUMENTATION_SETUP_DONE then
    setup_documentation()
    _DOCUMENTATION_SETUP_DONE = true
end
