// Copyright (c) 2025 Max Godman

#include "framework.h"
#include "GammaManager.h"
#include "AppGlobals.h"
#include <algorithm>
#include <math.h>

// Windows.h defines these as macros, conflicts with std::max/min.
#undef max
#undef min

namespace GammaManager
{
    void BuildGammaRamp(const Profile& profile, WORD ramp[3][256])
    {
        // We should probably clamp to safer values here, but SetDeviceGammaRamp() has a bunch of safety
        // built into it to prevent the screen from becoming unreadable, so we will rely on that instead.
        const float gamma = profile.gamma;
        const float contrast = profile.contrast;
        const int brightness = profile.brightness;
        
        // Remap brightness from (-50 to +50) to (-0.25 to +0.25).
        const float brightnessOffset = brightness / 200.0f;

        // Build ramp for all 256 possible input values.
        for (int i = 0; i < GammaConstants::RAMP_SIZE; ++i)
        {
            // Start with normalized input (0.0 to 1.0).
            float v = i / 255.0f;
            
            // 1. Apply brightness (linear offset).
            v += brightnessOffset;
            
            // 2. Apply contrast (scale around midpoint 0.5).
            // Formula: output = (input - 0.5) * contrast + 0.5
            // This keeps midpoint unchanged while expanding/compressing range.
            v = (v - 0.5f) * contrast + 0.5f;
            
            // 3. Clamp to valid range [0, 1].
            v = std::max(0.0f, std::min(1.0f, v));
            
            // 4. Apply gamma curve (power function).
            v = powf(v, 1.0f / gamma);
            
            // Cache for comparison (avoids reapplying identical ramps).
            App::lastRamp[i] = v;
            
            // Convert to Windows gamma ramp format (16-bit integer, 0-65535).
            const WORD val = (WORD)(v * GammaConstants::RAMP_MAX + 0.5f);
            
            // Apply same value to all three color channels.
            // Could be extended to support per-channel adjustments for color tinting.
            ramp[0][i] = val;  // Red.
            ramp[1][i] = val;  // Green.
            ramp[2][i] = val;  // Blue.
        }
    }
    
    void ApplyProfile(const Profile& profile, const int displayIndex)
    {
        if (App::displays.empty()) return;
        
        if (displayIndex < 0 || displayIndex >= (int)App::displays.size())
            return; // Invalid displayIndex.
        
        // Create device context for the target display, for the SetDeviceGammaRamp() call.
        const HDC hdc = CreateDC(NULL, App::displays[displayIndex].deviceName.c_str(), NULL, NULL);
        if (!hdc) return;

        WORD ramp[3][GammaConstants::RAMP_SIZE];
        BuildGammaRamp(profile, ramp);

        const BOOL success = SetDeviceGammaRamp(hdc, ramp);
        App::gammaRampFailed = !success;
        DeleteDC(hdc);
    }
    
    void ResetDisplay(const int displayIndex)
    {
        if (displayIndex < 0 || displayIndex >= (int)App::displays.size())
            return; // Invalid displayIndex.
        
        WORD defaultRamp[3][GammaConstants::RAMP_SIZE];
        for (int i = 0; i < GammaConstants::RAMP_SIZE; ++i)
        {
            const WORD val = (WORD)(i * 257);
            defaultRamp[0][i] = val;
            defaultRamp[1][i] = val;
            defaultRamp[2][i] = val;
        }
        
        const HDC hDC = CreateDC(NULL, App::displays[displayIndex].deviceName.c_str(), NULL, NULL);
        if (hDC)
        {
            SetDeviceGammaRamp(hDC, defaultRamp);
            DeleteDC(hDC);
        }
    }
}
