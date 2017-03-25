
// MyMfcDXTKAudioTest1.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"		// メイン シンボル


// CMyMfcDXTKAudioTest1App:
// このクラスの実装については、MyMfcDXTKAudioTest1.cpp を参照してください。
//

class CMyMfcDXTKAudioTest1App : public CWinApp
{
public:
	CMyMfcDXTKAudioTest1App();

// オーバーライド
public:
	virtual BOOL InitInstance() override;

// 実装

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance() override;
};

extern CMyMfcDXTKAudioTest1App theApp;
