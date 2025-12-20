// Copyright (c) 2025 Max Godman

// Precompiled header for Windows C++ project.

#pragma once

// Define minimum Windows version.
#include "targetver.h"

// Exclude rarely-used Windows headers to speed up compilation.
#define WIN32_LEAN_AND_MEAN

// Windows headers.
#include <Windows.h>

// Standard C++ headers.
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <algorithm>
#include <utility>
#include <cassert>
