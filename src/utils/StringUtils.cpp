// Copyright (c) 2025 Max Godman

#include "framework.h"
#include "StringUtils.h"
#include <sstream>

namespace StringUtils
{
    std::string WideToUTF8(const std::wstring& wstr)
    {
        if (wstr.empty()) return std::string();
        
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), 
                                       nullptr, 0, nullptr, nullptr);
        std::string result(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), 
                           &result[0], size, nullptr, nullptr);
        return result;
    }
    
    std::wstring UTF8ToWide(const std::string& str)
    {
        if (str.empty()) return std::wstring();
        
        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), 
                                       nullptr, 0);
        std::wstring result(size, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), 
                           &result[0], size);
        return result;
    }
    
    std::wstring VkToName(const UINT vk)
    {
        if (vk == 0)
            return L"None";
        
        // Function keys.
        if (vk >= VK_F1 && vk <= VK_F24)
        {
            std::wstringstream ss;
            ss << L"F" << (vk - VK_F1 + 1);
            return ss.str();
        }
        
        // Numpad keys.
        if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
        {
            std::wstringstream ss;
            ss << L"Numpad " << (vk - VK_NUMPAD0);
            return ss.str();
        }
        
        // Special keys.
        switch (vk)
        {
        case VK_BACK: return L"Backspace";
        case VK_TAB: return L"Tab";
        case VK_RETURN: return L"Enter";
        case VK_SHIFT: return L"Shift";
        case VK_CONTROL: return L"Ctrl";
        case VK_MENU: return L"Alt";
        case VK_PAUSE: return L"Pause";
        case VK_CAPITAL: return L"Caps Lock";
        case VK_ESCAPE: return L"Esc";
        case VK_SPACE: return L"Space";
        case VK_PRIOR: return L"Page Up";
        case VK_NEXT: return L"Page Down";
        case VK_END: return L"End";
        case VK_HOME: return L"Home";
        case VK_LEFT: return L"Left";
        case VK_UP: return L"Up";
        case VK_RIGHT: return L"Right";
        case VK_DOWN: return L"Down";
        case VK_SNAPSHOT: return L"Print Screen";
        case VK_INSERT: return L"Insert";
        case VK_DELETE: return L"Delete";
        case VK_LWIN: return L"Left Win";
        case VK_RWIN: return L"Right Win";
        case VK_MULTIPLY: return L"Numpad *";
        case VK_ADD: return L"Numpad +";
        case VK_SUBTRACT: return L"Numpad -";
        case VK_DECIMAL: return L"Numpad .";
        case VK_DIVIDE: return L"Numpad /";
        case VK_NUMLOCK: return L"Num Lock";
        case VK_SCROLL: return L"Scroll Lock";
        case VK_OEM_1: return L";";
        case VK_OEM_PLUS: return L"=";
        case VK_OEM_COMMA: return L",";
        case VK_OEM_MINUS: return L"-";
        case VK_OEM_PERIOD: return L".";
        case VK_OEM_2: return L"/";
        case VK_OEM_3: return L"`";
        case VK_OEM_4: return L"[";
        case VK_OEM_5: return L"\\";
        case VK_OEM_6: return L"]";
        case VK_OEM_7: return L"'";
        }
        
        // Letter or number keys.
        if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z'))
        {
            return std::wstring(1, (wchar_t)vk);
        }
        
        // Unknown key.
        std::wstringstream ss;
        ss << L"Key " << vk;
        return ss.str();
    }

    void Trim(std::wstring& s)
    {
        size_t a = s.find_first_not_of(L" \t\r\n");
        size_t b = s.find_last_not_of(L" \t\r\n");
        if (a == std::wstring::npos)
        {
            s.clear();
            return;
        }
        s = s.substr(a, b - a + 1);
    }
}
