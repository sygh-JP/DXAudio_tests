//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <collection.h>
#include <ppltasks.h>

#include <vector>
#include <array>

#include <wrl/client.h>

#include <atlbase.h>
#include <atlstr.h>

// DirectXTK
#include <Audio.h>

// Media Foundation
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")


#ifdef _DEBUG
#if defined(_M_IX86)
#pragma comment(lib, "Windows10\\Win32\\Debug\\DirectXTK.lib")
#elif defined(_M_X64)
#pragma comment(lib, "Windows10\\x64\\Debug\\DirectXTK.lib")
#elif defined(_M_ARM)
#pragma comment(lib, "Windows10\\ARM\\Debug\\DirectXTK.lib")
#endif
#else
#if defined(_M_IX86)
#pragma comment(lib, "Windows10\\Win32\\Release\\DirectXTK.lib")
#elif defined(_M_X64)
#pragma comment(lib, "Windows10\\x64\\Release\\DirectXTK.lib")
#elif defined(_M_ARM)
#pragma comment(lib, "Windows10\\ARM\\Release\\DirectXTK.lib")
#endif
#endif

#include "App.xaml.h"
