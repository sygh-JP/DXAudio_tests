//
// MainPage.xaml.cpp
// MainPage クラスの実装。
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace MyWin81DXTKAudioTest1;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Popups;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// 空白ページのアイテム テンプレートについては、http://go.microsoft.com/fwlink/?LinkId=234238 を参照してください

MainPage::MainPage()
{
	InitializeComponent();

	MyAudioHelpers::MyAudioManager::Startup();

	m_dispatcherTimer = ref new DispatcherTimer();
	TimeSpan span;
	span.Duration = 1000 * 1000 * 10 / FramesPerSecond; // 単位は100[ns]。
	m_dispatcherTimer->Interval = span;
	m_dispatcherTimer->Tick += ref new Windows::Foundation::EventHandler<Platform::Object ^>(this, &MyWin81DXTKAudioTest1::MainPage::OnTick);

	const LPCWSTR seAudioFileNames[MyAudioSet1::SoundEffectsSlotCount] =
	{
		L"se_maoudamashii_magical01.wav",
		L"se_maoudamashii_magical02.wav",
		L"se_maoudamashii_magical03.wav",
		L"se_maoudamashii_magical07.wav",
	};

	m_audioSet = std::make_unique<MyAudioSet1>();

	String^ outerErrMsg = nullptr;
	try
	{
		// DirectXTK Audio 用のファイルは、ストア アプリの場合どうやってロードする？
		// EXE のあるローカル フォルダーに出力されたファイルであれば、ファイル名（相対パス）のみを指定することでロードできる模様。
		// 内部では CreateFile2() を使っているだけらしい。
		// もし大量のファイルアクセスを行なう場合は、PPL で読み込み・生成処理をサブスレッドに逃がして非同期で実行する必要がありそう。
		// SoundEffect はファイルパスを渡すほか、メモリ上ファイルからも作れそうだが、
		// WaveBank はファイルパスしかなさそう。
		// SE も BGM も Media Foundation のデコーダーを使うようにして、統一してもいい。

		m_audioSet->CreateAudioEngineSE();

		for (int index = 0; index < MyAudioSet1::SoundEffectsSlotCount; ++index)
		{
			const LPCWSTR seFilePath = seAudioFileNames[index];
			m_audioSet->m_soundEffects[index] = std::make_unique<DirectX::SoundEffect>(m_audioSet->m_audioEngineSE.get(), seFilePath);
			m_audioSet->m_soundEffectInstances[index] = m_audioSet->m_soundEffects[index]->CreateInstance();
		}

		m_audioSet->m_audioManager = std::make_unique<MyAudioHelpers::MyAudioManager>();
		m_audioSet->m_audioManager->Init();
		m_audioSet->m_audioManager->SetMasterVolume(0.1f);

		String^ bgmAudioFileName = L"bgm_maoudamashii_healing01.mp3";

		// まずファイルからメモリストリームを生成し、
		// Media Foundation によるデコーダーを生成する。
		// サウンドファイルをすべてメモリ上に載せるため、ファイルから直接ストリームを生成しない。
		// Windows ファイルキャッシュ機能が効けば、たとえファイルストリームを使っていてもディスクアクセスは最初の1回だけで済みそうだが……

		auto loadBgmFileTask = MyUtils::CreateMemoryStreamFromLocalFileAsync(bgmAudioFileName);
		loadBgmFileTask.then([this](Windows::Storage::Streams::InMemoryRandomAccessStream^ memStream)
		{
			m_audioSet->m_audioPlayerBGM = m_audioSet->m_audioManager->CreateAudioPlayerUniquePtr(
				MyAudioHelpers::MyAudioManager::CreateSourceReader(
				MyAudioHelpers::MyAudioManager::CreateByteStream(memStream)));
		}).then([](concurrency::task<void> endTask)
		{
			String^ innerErrMsg = nullptr;
			// C++/CX の PPL における例外の捕捉はコツが要る。
			// https://msdn.microsoft.com/library/windows/apps/Hh780559.aspx
			try
			{
				endTask.get();
			}
			catch (const MyComHelpers::MyComException& ex)
			{
				innerErrMsg = ref new String(ex.GetErrorMessage());
			}
			catch (const std::exception& ex)
			{
				innerErrMsg = ref new String(CStringW(ex.what()));
			}
			catch (Platform::Exception^ ex)
			{
				innerErrMsg = ex->Message;
			}
			if (innerErrMsg != nullptr)
			{
				auto dialog = ref new MessageDialog(innerErrMsg, L"Error");
				concurrency::create_task(dialog->ShowAsync()).then([](IUICommand^){});
			}
		});
	}
	catch (const MyComHelpers::MyComException& ex)
	{
		outerErrMsg = ref new String(ex.GetErrorMessage());
	}
	catch (const std::exception& ex)
	{
		outerErrMsg = ref new String(CStringW(ex.what()));
	}
	catch (Platform::Exception^ ex)
	{
		outerErrMsg = ex->Message;
	}
	if (outerErrMsg != nullptr)
	{
		auto dialog = ref new MessageDialog(outerErrMsg, L"Error");
		concurrency::create_task(dialog->ShowAsync()).then([](IUICommand^){});
	}
}


void MyWin81DXTKAudioTest1::MainPage::buttonPlay_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (m_audioSet->m_audioPlayerBGM)
	{
		m_audioSet->m_audioPlayerBGM->Play();
	}
	m_dispatcherTimer->Start();
}


void MyWin81DXTKAudioTest1::MainPage::buttonPause_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (m_audioSet->m_audioPlayerBGM)
	{
		m_audioSet->m_audioPlayerBGM->Pause();
	}
}


void MyWin81DXTKAudioTest1::MainPage::buttonStop_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_dispatcherTimer->Stop();
	if (m_audioSet->m_audioPlayerBGM)
	{
		m_audioSet->m_audioPlayerBGM->Stop();
	}
}


void MyWin81DXTKAudioTest1::MainPage::buttonSE0_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_audioSet->PlaySE(0);
}


void MyWin81DXTKAudioTest1::MainPage::buttonSE1_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_audioSet->PlaySE(1);
}


void MyWin81DXTKAudioTest1::MainPage::buttonSE2_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_audioSet->PlaySE(2);
}


void MyWin81DXTKAudioTest1::MainPage::buttonSE3_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_audioSet->PlaySE(3);
}


void MyWin81DXTKAudioTest1::MainPage::OnTick(Platform::Object ^sender, Platform::Object ^args)
{
	//throw ref new Platform::NotImplementedException();
	if (m_audioSet)
	{
		m_audioSet->UpdateSE();
		m_audioSet->UpdateBGM(this->checkLoop->IsChecked->Value == true);
	}
}
