
// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Windows ヘッダーから使用されていない部分を除外します。
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 一部の CString コンストラクターは明示的です。

// 一般的で無視しても安全な MFC の警告メッセージの一部の非表示を解除します。
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC のコアおよび標準コンポーネント
#include <afxext.h>         // MFC の拡張部分


#include <afxdisp.h>        // MFC オートメーション クラス



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC の Internet Explorer 4 コモン コントロール サポート
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC の Windows コモン コントロール サポート
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC におけるリボンとコントロール バーのサポート

//#include <fstream>
#include <vector>
#include <array>

#include <wrl/client.h>

#include <atlpath.h>

// DirectXTK
#include <Audio.h>

// Media Foundation
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")

// DirectXTKAudio_Desktop_2015_Win8.vcxproj では、"_WIN32_WINNT=0x0602" が定義されている。
// これにより、Windows SDK 8.x 付属の XAudio2 v2.8 が使用されるようになる。
// Windows 8 以降には XAudio2 v2.8 (XAudio2_8.dll) が標準インストールされており、
// Windows 10 には XAudio2 v2.9 (XAudio2_9.dll) が標準インストールされているが、
// Windows 7 以前では XAudio2 v2.8 以降を利用できない。
#ifdef _DEBUG
#if defined(_M_IX86)
#pragma comment(lib, "Desktop_2015\\Win32\\Debug\\DirectXTKAudioWin8.lib")
#elif defined(_M_X64)
#pragma comment(lib, "Desktop_2015\\x64\\Debug\\DirectXTKAudioWin8.lib")
#endif
#else
#if defined(_M_IX86)
#pragma comment(lib, "Desktop_2015\\Win32\\Release\\DirectXTKAudioWin8.lib")
#elif defined(_M_X64)
#pragma comment(lib, "Desktop_2015\\x64\\Release\\DirectXTKAudioWin8.lib")
#endif
#endif





#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


