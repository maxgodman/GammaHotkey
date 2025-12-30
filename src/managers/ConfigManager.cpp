// Copyright (c) 2025 Max Godman

// Configuration file loading and saving implementation.

#include "framework.h"
#include "ConfigManager.h"
#include "AppGlobals.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include <fstream>
#include <filesystem>
#include <map>
#include <functional>
#include <mutex>
#include <algorithm>

namespace ConfigManager
{
    // Thread safety mutex.
    static std::mutex configMutex;

    // Config file section types.
    enum class ConfigSection
    {
        None,
        GlobalHotkeys,
        SimpleProfile,
        Profile,
    };

    // Config file key names.
    namespace Keys
    {
        // Sections.
        static constexpr const wchar_t* SECTION_GLOBALHOTKEYS = L"GlobalHotkeys";
        static constexpr const wchar_t* SECTION_SIMPLEPROFILE = L"SimpleProfile";
        static constexpr const wchar_t* SECTION_PROFILE = L"Profile";
        
        // Profile fields.
        static constexpr const wchar_t* PROFILE_NAME = L"Name";
        static constexpr const wchar_t* PROFILE_BRIGHTNESS = L"Brightness";
        static constexpr const wchar_t* PROFILE_CONTRAST = L"Contrast";
        static constexpr const wchar_t* PROFILE_GAMMA = L"Gamma";
        static constexpr const wchar_t* PROFILE_HOTKEY = L"Hotkey";
        
        // Global settings.
        static constexpr const wchar_t* TOGGLE_HOTKEY = L"ToggleHotkey";
        static constexpr const wchar_t* NEXTPROFILE_HOTKEY = L"NextProfileHotkey";
        static constexpr const wchar_t* PREVIOUSPROFILE_HOTKEY = L"PreviousProfileHotkey";
        static constexpr const wchar_t* LOOP_PROFILES = L"LoopProfiles";
        static constexpr const wchar_t* START_MINIMIZED = L"StartMinimized";
        static constexpr const wchar_t* MINIMIZE_TO_TRAY = L"MinimizeToTray";
        static constexpr const wchar_t* LAUNCH_ON_STARTUP = L"LaunchOnStartup";
        static constexpr const wchar_t* SELECTED_DISPLAY = L"SelectedDisplay";
        static constexpr const wchar_t* APPLY_ON_LAUNCH = L"ApplyProfileOnLaunch";
        static constexpr const wchar_t* SELECTED_PROFILE_INDEX = L"SelectedProfileIndex";
        static constexpr const wchar_t* ADVANCED_MODE = L"AdvancedMode";
    }
    
    // Case-insensitive comparator for wide strings.
    struct CaseInsensitiveCompare
    {
        bool operator()(const std::wstring& a, const std::wstring& b) const
        {
            return _wcsicmp(a.c_str(), b.c_str()) < 0;
        }
    };
    
    // Helper to compare keys, case-insensitive.
    static bool KeyEquals(const std::wstring& key, const wchar_t* target)
    {
        return _wcsicmp(key.c_str(), target) == 0;
    }
    
    // Safe string to integer conversion with default value.
    static int ParseInt(const std::wstring& str, const int defaultValue = 0)
    {
        try
        {
            return std::stoi(str);
        }
        catch (...)
        {
            return defaultValue;
        }
    }
    
    // Safe string to float conversion with default value.
    static float ParseFloat(const std::wstring& str, const float defaultValue = 1.0f)
    {
        try
        {
            return std::stof(str);
        }
        catch (...)
        {
            return defaultValue;
        }
    }
    
    // Sanitize profile name by removing problematic characters.
    static std::wstring SanitizeProfileName(const std::wstring& name)
    {
        std::wstring sanitized = name;
        
        // Remove or replace characters that might cause issues in config files.
        // Remove leading/trailing whitespace.
        StringUtils::Trim(sanitized);
        
        // Replace problematic characters with underscore.
        const std::wstring problematicChars = L"\r\n\t[]=#;";
        for (wchar_t& ch : sanitized)
        {
            if (problematicChars.find(ch) != std::wstring::npos)
            {
                ch = L'_';
            }
        }
        
        // Ensure name is not empty after sanitization.
        if (sanitized.empty())
        {
            sanitized = L"Unnamed Profile";
        }
        
        return sanitized;
    }
    
    // Check if a profile with the given name already exists (case-insensitive).
    static bool ProfileExists(const std::wstring& name)
    {
        return std::find_if(App::profiles.begin(), App::profiles.end(),
            [&name](const Profile& p)
            {
                return _wcsicmp(p.name.c_str(), name.c_str()) == 0;
            }) != App::profiles.end();
    }

    // Finalize and add a completed profile to the profiles list.
    // This is called when we encounter a new section or reach end of file,
    // indicating that the current profile definition is complete.
    static void FinalizeProfile(Profile& profile)
    {
        if (!profile.name.empty() && !ProfileExists(profile.name))
        {
            App::profiles.push_back(profile);
        }

        // Reset profile for potential reuse.
        profile = Profile();
    }
    
    bool Load()
    {
        std::lock_guard<std::mutex> lock(configMutex);
        
        App::profiles.clear();
        const std::wstring path = PathUtils::GetConfigPath();
        std::wifstream ifs(path);
        
        if (!ifs)
        {
            return false; // Config file doesn't exist or can't be opened.
        }

        std::wstring line;
        Profile currentProfile;
        ConfigSection currentSection = ConfigSection::None;

        // Iterate through each line of the config.
        while (std::getline(ifs, line))
        {
            StringUtils::Trim(line);
            if (line.empty() || line[0] == L';' || line[0] == L'#')
                continue;

            // Handle section headers, now entered a new section.
            if (line.front() == L'[' && line.back() == L']')
            {
                std::wstring section = line.substr(1, line.size() - 2);
                StringUtils::Trim(section);

                // Handle switching sections, finalize any profile we were building.
                if (currentSection == ConfigSection::Profile)
                {
                    FinalizeProfile(currentProfile);
                }

                if (KeyEquals(section, Keys::SECTION_PROFILE))
                {
                    currentSection = ConfigSection::Profile;
                }
                else if (KeyEquals(section, Keys::SECTION_SIMPLEPROFILE))
                {
                    currentSection = ConfigSection::SimpleProfile;
                }
                else if (KeyEquals(section, Keys::SECTION_GLOBALHOTKEYS))
                {
                    currentSection = ConfigSection::GlobalHotkeys;
                }
                else
                {
                    currentSection = ConfigSection::None; // Unknown section.
                }

                continue;
            }

            // Key=Value pairs.
            const auto eq = line.find(L'=');
            if (eq == std::wstring::npos)
                continue;
            
            std::wstring key = line.substr(0, eq);
            std::wstring val = line.substr(eq + 1);
            StringUtils::Trim(key);
            StringUtils::Trim(val);

            switch (currentSection)
            {
            case ConfigSection::GlobalHotkeys:
            {
                // Global settings, using dispatch table with lambdas.
                using HandlerFunc = std::function<void(const std::wstring&)>;

                // Static handler map with case-insensitive comparator.
                static const std::map<std::wstring, HandlerFunc, CaseInsensitiveCompare> handlers = {
                    {Keys::TOGGLE_HOTKEY, [](const std::wstring& v) { App::toggleHotkey = static_cast<UINT>(ParseInt(v, 0)); }},
                    {Keys::NEXTPROFILE_HOTKEY, [](const std::wstring& v) { App::nextProfileHotkey = static_cast<UINT>(ParseInt(v, 0)); }},
                    {Keys::PREVIOUSPROFILE_HOTKEY, [](const std::wstring& v) { App::previousProfileHotkey = static_cast<UINT>(ParseInt(v, 0)); }},
                    {Keys::LOOP_PROFILES, [](const std::wstring& v) { App::loopProfiles = (ParseInt(v, 0) != 0); }},
                    {Keys::START_MINIMIZED, [](const std::wstring& v) { App::startMinimized = (ParseInt(v, 0) != 0); }},
                    {Keys::MINIMIZE_TO_TRAY, [](const std::wstring& v) { App::minimizeToTray = (ParseInt(v, 0) != 0); }},
                    {Keys::LAUNCH_ON_STARTUP, [](const std::wstring& v) { App::launchOnStartup = (ParseInt(v, 0) != 0); }},
                    {Keys::SELECTED_DISPLAY, [](const std::wstring& v) { App::selectedDisplayIndex = ParseInt(v, 0); }},
                    {Keys::APPLY_ON_LAUNCH, [](const std::wstring& v) { App::applyProfileOnLaunch = (ParseInt(v, 0) != 0); }},
                    {Keys::SELECTED_PROFILE_INDEX, [](const std::wstring& v) { App::selectedProfileIndex = ParseInt(v, 0); }},
                    {Keys::ADVANCED_MODE, [](const std::wstring& v) { App::state.SetAdvancedModeEnabled((ParseInt(v, 0) != 0)); }}
                };

                // Lookup using case-insensitive comparator.
                const auto it = handlers.find(key);
                if (it != handlers.end())
                {
                    it->second(val);
                }

                break;
            }
            case ConfigSection::SimpleProfile:
            {
                // Simplified profile settings.
                if (KeyEquals(key, Keys::PROFILE_BRIGHTNESS))
                {
                    App::simpleProfile.brightness = ParseInt(val, 0);
                }
                else if (KeyEquals(key, Keys::PROFILE_CONTRAST))
                {
                    App::simpleProfile.contrast = ParseFloat(val, 1.0f);
                }
                else if (KeyEquals(key, Keys::PROFILE_GAMMA))
                {
                    App::simpleProfile.gamma = ParseFloat(val, 1.0f);
                }

                break;
            }
                
            case ConfigSection::Profile:
            {
                // Profile settings.
                if (KeyEquals(key, Keys::PROFILE_NAME))
                {
                    currentProfile.name = SanitizeProfileName(val);
                }
                else if (KeyEquals(key, Keys::PROFILE_BRIGHTNESS))
                {
                    currentProfile.brightness = ParseInt(val, 0);
                }
                else if (KeyEquals(key, Keys::PROFILE_CONTRAST))
                {
                    currentProfile.contrast = ParseFloat(val, 1.0f);
                }
                else if (KeyEquals(key, Keys::PROFILE_GAMMA))
                {
                    currentProfile.gamma = ParseFloat(val, 1.0f);
                }
                else if (KeyEquals(key, Keys::PROFILE_HOTKEY))
                {
                    currentProfile.hotkey = static_cast<UINT>(ParseInt(val, 0));
                }

                break;
            }

            case ConfigSection::None:
            default:
                // Ignore key-value pairs outside of known sections.
                break;
            }
        }

        // Finalize the last profile if we ended the file while in a profile section.
        if (currentSection == ConfigSection::Profile)
        {
            FinalizeProfile(currentProfile);
        }
        
        return true;
    }
    
    bool Save()
    {
        std::lock_guard<std::mutex> lock(configMutex);
        
        const std::filesystem::path finalPath = PathUtils::GetConfigPath();
        const std::filesystem::path tempPath = std::filesystem::path(finalPath).concat(L".tmp");
        
        // Write to temporary file first.
        std::wofstream ofs(tempPath, std::ios::trunc);
        if (!ofs)
        {
            return false;
        }

        // Write configuration file header.
        ofs << L"; Configuration file for GammaHotkey application.\n";
        ofs << L"; Hotkey values are virtual-key codes (0 = none).\n\n";

        // Save global hotkeys and settings.
        ofs << L"[" << Keys::SECTION_GLOBALHOTKEYS << L"]\n";
        ofs << Keys::TOGGLE_HOTKEY << L"=" << App::toggleHotkey << L"\n";
        ofs << Keys::NEXTPROFILE_HOTKEY << L"=" << App::nextProfileHotkey << L"\n";
        ofs << Keys::PREVIOUSPROFILE_HOTKEY << L"=" << App::previousProfileHotkey << L"\n";
        ofs << Keys::LOOP_PROFILES << L"=" << (App::loopProfiles ? 1 : 0) << L"\n";
        ofs << Keys::START_MINIMIZED << L"=" << (App::startMinimized ? 1 : 0) << L"\n";
        ofs << Keys::MINIMIZE_TO_TRAY << L"=" << (App::minimizeToTray ? 1 : 0) << L"\n";
        ofs << Keys::LAUNCH_ON_STARTUP << L"=" << (App::launchOnStartup ? 1 : 0) << L"\n";
        ofs << Keys::SELECTED_DISPLAY << L"=" << App::selectedDisplayIndex << L"\n";
        ofs << Keys::APPLY_ON_LAUNCH << L"=" << (App::applyProfileOnLaunch ? 1 : 0) << L"\n";
        ofs << Keys::SELECTED_PROFILE_INDEX << L"=" << App::selectedProfileIndex << L"\n";
        ofs << Keys::ADVANCED_MODE << L"=" << (App::state.IsAdvancedModeEnabled() ? 1 : 0) << L"\n\n";

        // Save simple profile.
        ofs << L"[" << Keys::SECTION_SIMPLEPROFILE << L"]\n";
        ofs << Keys::PROFILE_BRIGHTNESS << L"=" << App::simpleProfile.brightness << L"\n";
        ofs << Keys::PROFILE_CONTRAST << L"=" << App::simpleProfile.contrast << L"\n";
        ofs << Keys::PROFILE_GAMMA << L"=" << App::simpleProfile.gamma << L"\n\n";

        // Save profiles.
        for (const auto& profile : App::profiles)
        {
            ofs << L"[" << Keys::SECTION_PROFILE << L"]\n";
            ofs << Keys::PROFILE_NAME << L"=" << profile.name << L"\n";
            ofs << Keys::PROFILE_BRIGHTNESS << L"=" << profile.brightness << L"\n";
            ofs << Keys::PROFILE_CONTRAST << L"=" << profile.contrast << L"\n";
            ofs << Keys::PROFILE_GAMMA << L"=" << profile.gamma << L"\n";
            ofs << Keys::PROFILE_HOTKEY << L"=" << profile.hotkey << L"\n\n";
        }
        
        // Close the file before attempting to rename.
        ofs.close();
        
        // Check if write was successful.
        if (ofs.fail())
        {
            std::filesystem::remove(tempPath);
            return false;
        }
        
        // Atomic replacement, rename temp file to final file.
        try
        {
            // Remove existing config file first, if it exists.
            if (std::filesystem::exists(finalPath))
            {
                std::filesystem::remove(finalPath);
            }

            // Rename the temp file, replacing the existing config file.
            std::filesystem::rename(tempPath, finalPath);
        }
        catch (const std::filesystem::filesystem_error&)
        {
            // Failed to rename.
            // Leave the temp file so the user can recover their settings from it.
            return false;
        }
        
        return true;
    }
}
