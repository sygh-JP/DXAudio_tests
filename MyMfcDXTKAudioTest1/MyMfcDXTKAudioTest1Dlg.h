
// MyMfcDXTKAudioTest1Dlg.h : ヘッダー ファイル
//

#pragma once


#include <afxwin.h>

//#define USES_WRL_COM_PTR_FOR_MY_HELPER
#include "MyAudioPlayer.h"
#include "MyAudioSet1.h"


namespace MyMfcHelpers
{
	inline void SafeStartTimer(CWnd* pWnd, UINT_PTR& inoutTimerID, UINT_PTR requestedTimerID, UINT periodMillisec)
	{
		if (inoutTimerID == 0)
		{
			inoutTimerID = pWnd->SetTimer(requestedTimerID, periodMillisec, nullptr);
			ATLASSERT(inoutTimerID != 0);
		}
	}

	inline void SafeStopTimer(CWnd* pWnd, UINT_PTR& inoutTimerID)
	{
		if (inoutTimerID != 0)
		{
			ATLVERIFY(pWnd->KillTimer(inoutTimerID));
			inoutTimerID = 0;
		}
	}

	inline CStringW GetKnownFolderPath(const KNOWNFOLDERID& fid, DWORD flags = KF_FLAG_DEFAULT, HANDLE token = nullptr)
	{
		CStringW str;
		ATL::CComHeapPtr<wchar_t> folderPathPtr; // CoTaskMemFree() を自動で呼んでくれる。
		if (SUCCEEDED(::SHGetKnownFolderPath(fid, flags, token, &folderPathPtr)))
		{
			str = static_cast<LPCWSTR>(folderPathPtr);
		}
		return str;
	}
}


// CMyMfcDXTKAudioTest1Dlg ダイアログ
class CMyMfcDXTKAudioTest1Dlg : public CDialogEx
{
	// コンストラクション
public:
	explicit CMyMfcDXTKAudioTest1Dlg(CWnd* pParent = nullptr); // 標準コンストラクター

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MYMFCDXTKAUDIOTEST1_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV サポート

private:
	std::unique_ptr<MyAudioSet1> m_audioSet;
	std::vector<uint8_t> m_soundFileOnMemory;

private:
	static const UINT_PTR StreamEndCheckTimerID = 1;
	static const UINT FramesPerSecond = 30;
	static const UINT PeriodMsPerFrame = 1000 / FramesPerSecond;

	UINT_PTR m_streamEndCheckTimerID = 0;

private:
	void SafeStartTimer()
	{
		MyMfcHelpers::SafeStartTimer(this, m_streamEndCheckTimerID, StreamEndCheckTimerID, PeriodMsPerFrame);
	}
	void SafeStopTimer()
	{
		MyMfcHelpers::SafeStopTimer(this, m_streamEndCheckTimerID);
	}

	// 実装
protected:
	HICON m_hIcon = nullptr;

	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog() override;
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnBnClickedButtonPause();
private:
	CButton m_ddxcCheckLoop;
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtonSe0();
	afx_msg void OnBnClickedButtonSe1();
	afx_msg void OnBnClickedButtonSe2();
	afx_msg void OnBnClickedButtonSe3();
};
