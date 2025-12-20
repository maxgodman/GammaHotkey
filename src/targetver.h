// Copyright (c) 2025 Max Godman

// Defines the minimum required platform.
// This app requires at least Windows 10 (0x0A00).
// We could probably support earlier versions, but at this point Windows 10 is already out of support, so 10 is the minimum.

#pragma once

#ifndef WINVER
#define WINVER 0x0A00	// Windows 10.
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00	// Windows 10.
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0A00	// IE version (matches _WIN32_WINNT).
#endif

#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000000	// Windows 10.
#endif
