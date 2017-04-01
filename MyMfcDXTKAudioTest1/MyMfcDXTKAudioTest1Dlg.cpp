
// MyMfcDXTKAudioTest1Dlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "MyMfcDXTKAudioTest1.h"
#include "MyMfcDXTKAudioTest1Dlg.h"
#include <afxdialogex.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMyMfcDXTKAudioTest1Dlg ダイアログ



CMyMfcDXTKAudioTest1Dlg::CMyMfcDXTKAudioTest1Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MYMFCDXTKAUDIOTEST1_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_audioSet = std::make_unique<MyAudioSet1>();
}

void CMyMfcDXTKAudioTest1Dlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_LOOP, m_ddxcCheckLoop);
}

BEGIN_MESSAGE_MAP(CMyMfcDXTKAudioTest1Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, &CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonPause)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_SE0, &CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonSe0)
	ON_BN_CLICKED(IDC_BUTTON_SE1, &CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonSe1)
	ON_BN_CLICKED(IDC_BUTTON_SE2, &CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonSe2)
	ON_BN_CLICKED(IDC_BUTTON_SE3, &CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonSe3)
END_MESSAGE_MAP()


// CMyMfcDXTKAudioTest1Dlg メッセージ ハンドラー

BOOL CMyMfcDXTKAudioTest1Dlg::OnInitDialog()
{
	__super::OnInitDialog();

	// "バージョン情報..." メニューをシステム メニューに追加します。

	// IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);  // 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE); // 小さいアイコンの設定

	// TODO: 初期化をここに追加します。

	// XACT オリジナルの XWB フォーマットは本来 AIFF/XMA の圧縮ファイルにも対応しているが、
	// DirectXTK Audio は圧縮フォーマットをサポートしていない模様。

	// テストとして、
	// BGM には Media Foundation + XAudio2 を直接使い、
	// SE には DXTK Audio ラッパーを使ってみる。
	// 複数のマスタリングボイスが同一アプリ上で共存することになるが、うまくミキシングしてくれる模様。

	// Windows の休止状態と復帰を繰り返すと、
	// BGM も SE も、最初の発音までに若干時間がかかるようになる模様。
	// 環境起因？　オーディオデバイスがカニ（Realtek）だからかもしれない。

	const CStringW userMusicFolderPath = MyMfcHelpers::GetKnownFolderPath(FOLDERID_Music);
	const CStringW userDownloadFolderPath = MyMfcHelpers::GetKnownFolderPath(FOLDERID_Downloads);

#if 0
	CPathW assetsDirPath(userDownloadFolderPath);
	assetsDirPath += LR"(free_assets_maoudamashii)";
#else
	WCHAR moduleFilePath[MAX_PATH] = {};
	::GetModuleFileNameW(nullptr, moduleFilePath, ARRAYSIZE(moduleFilePath));
	CPathW assetsDirPath(moduleFilePath);
	assetsDirPath += L"..";
	assetsDirPath += L"..";
	assetsDirPath += L"..";
	assetsDirPath += L"..";
	assetsDirPath += L"free_audio_assets";
#endif

	const LPCWSTR seAudioFileNames[MyAudioSet1::SoundEffectsSlotCount] =
	{
		L"se_maoudamashii_magical01.wav",
		L"se_maoudamashii_magical02.wav",
		L"se_maoudamashii_magical03.wav",
		L"se_maoudamashii_magical07.wav",
	};

#if 0
	CPathW bgmFilePath(userMusicFolderPath);
	bgmFilePath += LR"(other\mhr_eyec.wav)";
#else
	CPathW bgmFilePath(assetsDirPath);
	bgmFilePath += L"bgm_maoudamashii_healing01.mp3";
#endif

	// WASAPI コンポーネントによる foobar2000 の再生中あるいは一時停止した状態で、
	// DirectXTK の AudioEngine を初期化しようとしたら IXAudio2::CreateMasteringVoice() で 0x8889000a のエラーが発生する模様。
	// 当該エラーコードは AUDCLNT_E_DEVICE_IN_USE らしいので、WASAPI 排他モードでデバイスを占有したままになっている、
	// という状態を示している模様。
	// https://msdn.microsoft.com/ja-jp/library/ee416798.aspx
	// 排他モードでオーディオを聴きながらゲームをプレイする（ゲームサウンドは無視）、というのは状況としてありえなくはない。
	// オーディオデバイスの初期化に失敗しても続行するか、それとも中断するか。
	// 無理に続行して隠ぺいしてしまうと、エラーの原因を特定するのが難しくなる弊害もある。
	// 初期化エラーが発生した場合、強制続行する前に、排他モードのアプリを使用していないかどうか、
	// 確認項目として事例を提示するメッセージを出すようにしたほうがよさげ。

	try
	{
		m_audioSet->CreateAudioEngineSE();

		for (int index = 0; index < MyAudioSet1::SoundEffectsSlotCount; ++index)
		{
			CPathW seFilePath(assetsDirPath);
			seFilePath += seAudioFileNames[index];
			m_audioSet->m_soundEffects[index] = std::make_unique<DirectX::SoundEffect>(m_audioSet->m_audioEngineSE.get(), seFilePath);
			m_audioSet->m_soundEffectInstances[index] = m_audioSet->m_soundEffects[index]->CreateInstance();
		}

		m_audioSet->m_audioManager = std::make_unique<MyAudioHelpers::MyAudioManager>();
		m_audioSet->m_audioManager->Init();
		m_audioSet->m_audioManager->SetMasterVolume(0.1f);

		THROW_IF_FAILED(MyUtils::LoadBinaryFromFile(bgmFilePath, m_soundFileOnMemory) || m_soundFileOnMemory.empty() ? S_OK : E_FAIL, L"Failed to load the BGM file!!");

		{
			IStream* pStream = nullptr;
			THROW_IF_FAILED(MyComHelpers::CreateStreamOnMemory(&m_soundFileOnMemory[0], static_cast<UINT>(m_soundFileOnMemory.size()), pStream), L"CreateStreamOnMemory failure");
			m_audioSet->m_audioPlayerBGM = m_audioSet->m_audioManager->CreateAudioPlayerUniquePtr(
				MyAudioHelpers::MyAudioManager::CreateSourceReader(
					MyAudioHelpers::MyAudioManager::CreateByteStream(pStream)));
			const ULONG newRefCount = pStream->Release();
			pStream = nullptr;
		}
	}
	catch (const std::exception& ex)
	{
		CStringA strMsg;
		strMsg.Format("Exception = \"%s\", Message = \"%s\"", typeid(ex).name(), ex.what());
		AfxMessageBox(CString(strMsg));
		this->SendMessage(WM_CLOSE);
		return true;
	}
	catch (const MyComHelpers::MyComException& ex)
	{
		CStringW strMsg;
		strMsg.Format(L"Code = 0x%08lx, Message = \"%s\"", ex.GetResultCode(), ex.GetErrorMessage());
		AfxMessageBox(strMsg);
		this->SendMessage(WM_CLOSE);
		return true;
	}

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CMyMfcDXTKAudioTest1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		__super::OnSysCommand(nID, lParam);
	}
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CMyMfcDXTKAudioTest1Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		__super::OnPaint();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CMyMfcDXTKAudioTest1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMyMfcDXTKAudioTest1Dlg::OnDestroy()
{
	this->SafeStopTimer();

	m_audioSet.reset();

	// 明示的にすべて解放する。さもないと DirecXTK Audio 起因のメモリーリークが報告される。

	__super::OnDestroy();

	// TODO: ここにメッセージ ハンドラー コードを追加します。
}


void CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonPlay()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	if (m_audioSet->m_audioPlayerBGM)
	{
		m_audioSet->m_audioPlayerBGM->Play();
	}
	this->SafeStartTimer();
}


void CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonStop()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	this->SafeStopTimer();
	if (m_audioSet->m_audioPlayerBGM)
	{
		m_audioSet->m_audioPlayerBGM->Stop();
	}
}


void CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonPause()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	if (m_audioSet->m_audioPlayerBGM)
	{
		m_audioSet->m_audioPlayerBGM->Pause();
	}
}


void CMyMfcDXTKAudioTest1Dlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	// 簡単のため、不正確だがタイマーメッセージでポーリング。
	if (nIDEvent == m_streamEndCheckTimerID)
	{
		if (m_audioSet)
		{
			m_audioSet->UpdateSE();
			m_audioSet->UpdateBGM(m_ddxcCheckLoop.GetCheck() == BST_CHECKED);
		}
	}

	__super::OnTimer(nIDEvent);
}


void CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonSe0()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_audioSet->PlaySE(0);
}


void CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonSe1()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_audioSet->PlaySE(1);
}


void CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonSe2()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_audioSet->PlaySE(2);
}


void CMyMfcDXTKAudioTest1Dlg::OnBnClickedButtonSe3()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_audioSet->PlaySE(3);
}
