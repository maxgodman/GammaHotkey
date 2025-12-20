// Copyright (c) 2025 Max Godman

// String conversion and manipulation utilities.

#pragma once

#include <string>
#include <windows.h>

namespace StringUtils
{
    /**
     * @brief Convert wide string (UTF-16) to UTF-8 string.
     * @param[in] wstr Wide string to convert.
     * @return UTF-8 encoded string.
     */
    std::string WideToUTF8(const std::wstring& wstr);
    
    /**
     * @brief Convert UTF-8 string to wide string (UTF-16).
     * @param[in] str UTF-8 encoded string.
     * @return Wide string.
     */
    std::wstring UTF8ToWide(const std::string& str);
    
    /**
     * @brief Convert virtual key code to human-readable name.
     * @param[in] vk Virtual key code (VK_*).
     * @return String representation (e.g. "F1", "Numpad 5").
     */
    std::wstring VkToName(const UINT vk);

    /**
     * @brief Helper function to trim whitespace.
     */
    void Trim(std::wstring& s);
}
