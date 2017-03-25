//
// MainPage.xaml.h
// MainPage クラスの宣言。
//

#pragma once

#include "MainPage.g.h"

#define USES_WRL_COM_PTR_FOR_MY_HELPER
#include "MyAudioPlayer.h"
#include "MyAudioSet1.h"


namespace MyWin81DXTKAudioTest1
{
	/// <summary>
	/// Frame 内へナビゲートするために利用する空欄ページ。
	/// </summary>
	public ref class MainPage sealed
	{
	private:
		Windows::UI::Xaml::DispatcherTimer^ m_dispatcherTimer;
		std::unique_ptr<MyAudioSet1> m_audioSet;

		static const int FramesPerSecond = 30;
	public:
		MainPage();

	private:
		void buttonPlay_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void buttonPause_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void buttonStop_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void buttonSE0_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void buttonSE1_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void buttonSE2_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void buttonSE3_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnTick(Platform::Object ^sender, Platform::Object ^args);
	};
}
