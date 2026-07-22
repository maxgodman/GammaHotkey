// Copyright (c) 2025 Max Godman

//{{NO_DEPENDENCIES}}

#define IDS_APP_TITLE			103
#define IDI_GAMMAHOTKEY			104
#define IDI_SMALL				105
#define IDC_GAMMAHOTKEY			106
#define IDI_ON					107
#define IDI_OFF					108

// About dialog strings (string table; loaded at runtime via LoadStringW). Their values are
// composed from the VER_* macros below in the .rc, so product name/version/description/
// copyright are never duplicated here.
#define IDS_ABOUT_TITLE			200
#define IDS_ABOUT_VERSION		201
#define IDS_ABOUT_DESCRIPTION	202
#define IDS_ABOUT_COPYRIGHT		203
#define IDS_ABOUT_OK			204

#ifndef IDC_STATIC
#define IDC_STATIC				-1
#endif // IDC_STATIC

#define VER_FILEVERSION			1,0,0,0
#define VER_FILEVERSION_STR		"1.0.0"
#define VER_PRODUCTVERSION		1,0,0,0
#define VER_PRODUCTVERSION_STR	"1.0.0"

#define VER_COMPANYNAME			"Max Godman"
#define VER_FILEDESCRIPTION		"Display gamma adjustment using hotkeys"
#define VER_INTERNALNAME		"GammaHotkey"
#define VER_LEGALCOPYRIGHT		"Copyright (c) 2025 Max Godman"
#define VER_ORIGINALFILENAME	"GammaHotkey.exe"
#define VER_PRODUCTNAME			"GammaHotkey"
#define VER_PRODUCTNAME_W		L"GammaHotkey"

// Next default values for new objects.
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS

#define _APS_NO_MFC					130
#define _APS_NEXT_RESOURCE_VALUE	129
#define _APS_NEXT_COMMAND_VALUE		32771
#define _APS_NEXT_CONTROL_VALUE		1000
#define _APS_NEXT_SYMED_VALUE		110
#endif // APSTUDIO_READONLY_SYMBOLS
#endif // APSTUDIO_INVOKED
