// Copyright (c) 2025 Max Godman

// Gamma ramp manipulation for Windows displays.

/**
 * INFORMATION:
 * The most important part of this application is the call to SetDeviceGammaRamp() found here.
 * SetDeviceGammaRamp() is a Win32 API call that has been available since Windows 95.
 * Microsoft strongly recommends to avoid using this, however for our purposes it is convenient and seems to work fine.
 * Alternative applications are using this API call too, either as stated in documentation/source code,
 * or as evidenced by each of these apps being able to fight over control of the gamma when used alongside one another.
 * Building and applying the gamma ramp is practically free and universally compatible on Windows systems.
 * 
 * WHAT IS A GAMMA RAMP:
 * A gamma ramp is a lookup table that maps input pixel values to output pixel values.
 * It's an array of 256 values (one per possible 8-bit input) for each color channel (R, G, B).
 * Windows allows applications to modify this ramp via SetDeviceGammaRamp().
 * This is hardware-accelerated and works for the entire screen, including games, videos, etc.
 *
 * MATHEMATICAL MODEL:
 * We apply three adjustments in order, this is generally the industry standard:
 * 1. Brightness: Linear offset (-50 to +50), shifts all values up/down.
 * 2. Contrast: Multiplier around midpoint (0.5 to 1.5), expands/compresses range.
 * 3. Gamma: Power curve (0.1 to 3.0), non-linear adjustment.
 */

#pragma once

#include "GammaHotkeyTypes.h"

namespace GammaManager
{
    /**
     * @brief Apply gamma settings from a profile to a specific display.
     * @param[in] profile Profile containing brightness, contrast, and gamma settings.
     * @param[in] displayIndex Index into App::displays vector.
     */
    void ApplyProfile(const Profile& profile, const int displayIndex);
    
    /**
     * @brief Reset gamma to default (linear) on a specific display.
     * @param[in] displayIndex Index into App::displays vector.
     */
    void ResetDisplay(const int displayIndex);
    
    /**
     * @brief Build a gamma ramp from profile settings.
     * @param[in] profile Profile containing brightness, contrast, and gamma values.
     * @param[out] ramp Output array [3][256] for R, G, B channels.
     */
    void BuildGammaRamp(const Profile& profile, WORD ramp[3][256]);
}
