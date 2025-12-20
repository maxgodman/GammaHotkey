// Copyright (c) 2025 Max Godman

#include "framework.h"
#include "DisplayManager.h"
#include "AppGlobals.h"

namespace DisplayManager
{
    void EnumerateDisplays()
    {
        App::displays.clear();
        
        DISPLAY_DEVICE ddAdapter = {};
        ddAdapter.cb = sizeof(ddAdapter);
        DISPLAY_DEVICE ddDisplay = {};
        ddDisplay.cb = sizeof(ddDisplay);

        for (DWORD adapterIndex = 0; EnumDisplayDevices(NULL, adapterIndex, &ddAdapter, 0); ++adapterIndex)
        {
            if (!(ddAdapter.StateFlags & DISPLAY_DEVICE_ACTIVE))
                continue;

            for (DWORD displayIndex = 0; EnumDisplayDevices(ddAdapter.DeviceName, displayIndex, &ddDisplay, 0); ++displayIndex)
            {
                if (!(ddDisplay.StateFlags & DISPLAY_DEVICE_ACTIVE))
                    continue;

                DisplayEntry entry;
                entry.deviceName = ddAdapter.DeviceName;
                // Display first, then GPU, separated by |
                entry.friendlyName = std::wstring(ddDisplay.DeviceString) + L" | " +
                                    std::wstring(ddAdapter.DeviceString);
                App::displays.push_back(entry);
            }
        }
    }
}
