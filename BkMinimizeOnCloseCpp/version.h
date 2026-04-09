#pragma once

#pragma code_page(932)
// Version
//
// 数値としての定義（FILEVERSION, PRODUCTVERSION 用）
#define VER_MAJOR			1
#define VER_MINOR			6
#define VER_PATCH			0
#define VER_BUILD			0

// マクロの引数を展開してから文字列にする
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// SemVer (3桁: "x.y.z")
#define SZ_PIN_SEMVERSION	STR(VER_MAJOR) "." STR(VER_MINOR) "." STR(VER_PATCH)
// システム用フルバージョン (4桁: "x.y.z.b")
#define SZ_PIN_VERSION		STR(VER_MAJOR) "." STR(VER_MINOR) "." STR(VER_PATCH) "." STR(VER_BUILD)

// Copyright
//
#define SZ_VENDOR_NAME		"Sou Niyari"
#define SZ_RELEASE_YEAR		"2026"
#define SZ_COPYRIGHT		"Copyright (C) " SZ_RELEASE_YEAR " " SZ_VENDOR_NAME " / MIT"
#define SZ_SITEURL			"https://github.com/niyari/BkMinimizeOnClose"
