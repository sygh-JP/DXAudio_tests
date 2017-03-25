//
// App.xaml.cpp
// App クラスの実装。
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace MyWin81DXTKAudioTest1;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// 空のアプリケーション テンプレートについては、http://go.microsoft.com/fwlink/?LinkId=234227 を参照してください

/// <summary>
/// 単一アプリケーション オブジェクトを初期化します。これは、実行される作成したコードの
/// 最初の行であり、main() または WinMain() と論理的に等価です。
/// </summary>
App::App()
{
	InitializeComponent();
	Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);

	// ハンドルされていない例外を統一的に処理する。Windows 8.1 以降で使える機能らしい。WinForms や WPF などの .NET ではおなじみだが……
	//Windows::ApplicationModel::Core::CoreApplication::UnhandledErrorDetected += ref new EventHandler<Windows::ApplicationModel::Core::UnhandledErrorDetectedEventArgs^>(this, &App::OnUnhandledException);
}

/// <summary>
/// アプリケーションがエンド ユーザーによって正常に起動されたときに呼び出されます。	他のエントリ ポイントは、
/// アプリケーションが特定のファイルを開くために呼び出されたときなどに使用されます。
/// </summary>
/// <param name="e">起動要求とプロセスの詳細を表示します。</param>
void App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e)
{

#if _DEBUG
		// デバッグ中にグラフィックスのプロファイル情報を表示します。
		if (IsDebuggerPresent())
		{
			// 現在のフレーム レート カウンターを表示します
			 DebugSettings->EnableFrameRateCounter = true;
		}
#endif

	auto rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

	// ウィンドウに既にコンテンツが表示されている場合は、アプリケーションの初期化を繰り返さずに、
	// ウィンドウがアクティブであることだけを確認してください
	if (rootFrame == nullptr)
	{
		// ナビゲーション コンテキストとして動作するフレームを作成し、
		// SuspensionManager キーに関連付けます
		rootFrame = ref new Frame();

		// 既定の言語を設定します
		rootFrame->Language = Windows::Globalization::ApplicationLanguages::Languages->GetAt(0);

		rootFrame->NavigationFailed += ref new Windows::UI::Xaml::Navigation::NavigationFailedEventHandler(this, &App::OnNavigationFailed);

		if (e->PreviousExecutionState == ApplicationExecutionState::Terminated)
		{
			// TODO: 必要な場合のみ、保存されたセッション状態を復元し、
			// 復元完了後の最後の起動手順をスケジュールします

		}

		if (rootFrame->Content == nullptr)
		{
			// ナビゲーションの履歴スタックが復元されていない場合、最初のページに移動します。
			// このとき、必要な情報をナビゲーション パラメーターとして渡して、新しいページを
			// 作成します
			rootFrame->Navigate(TypeName(MainPage::typeid), e->Arguments);
		}
		// フレームを現在のウィンドウに配置します
		Window::Current->Content = rootFrame;
		// 現在のウィンドウがアクティブであることを確認します
		Window::Current->Activate();
	}
	else
	{
		if (rootFrame->Content == nullptr)
		{
			// ナビゲーションの履歴スタックが復元されていない場合、最初のページに移動します。
			// このとき、必要な情報をナビゲーション パラメーターとして渡して、新しいページを
			// 作成します
			rootFrame->Navigate(TypeName(MainPage::typeid), e->Arguments);
		}
		// 現在のウィンドウがアクティブであることを確認します
		Window::Current->Activate();
	}
}

/// <summary>
/// アプリケーションの実行が中断されたときに呼び出されます。	アプリケーションの状態は、
/// アプリケーションが終了されるのか、メモリの内容がそのままで再開されるのか
/// わからない状態で保存されます。
/// </summary>
/// <param name="sender">中断要求の送信元。</param>
/// <param name="e">中断要求の詳細。</param>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
	(void) sender;	// 未使用のパラメーター
	(void) e;	// 未使用のパラメーター

	//TODO: アプリケーションの状態を保存してバックグラウンドの動作があれば停止します
}

/// <summary>
/// 特定のページへの移動が失敗したときに呼び出されます
/// </summary>
/// <param name="sender">移動に失敗したフレーム</param>
/// <param name="e">ナビゲーション エラーの詳細</param>
void App::OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e)
{
	throw ref new FailureException("Failed to load Page " + e->SourcePageType.Name);
}

void App::OnUnhandledException(Platform::Object^ sender, Windows::ApplicationModel::Core::UnhandledErrorDetectedEventArgs^ e)
{
	auto err = e->UnhandledError;

	if (!err->Handled) // Propagate has not been called on it yet.
	{
		try
		{
			err->Propagate();
		}
		// Catch any specific exception types if you know how to handle them
		catch (AccessDeniedException^ ex)
		{
			// TODO: Log error and either take action to recover
			// or else re-throw exception to continue fail-fast
		}
	}
}
