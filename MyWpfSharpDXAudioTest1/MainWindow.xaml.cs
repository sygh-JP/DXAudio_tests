using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace MyWpfSharpDXAudioTest1
{
	/// <summary>
	/// MainWindow.xaml の相互作用ロジック
	/// </summary>
	public partial class MainWindow : Window
	{
		//const string FreeAudioAssetsDirRel = @"Visual Studio 2015\Projects\MyDXTKAudioTest1\free_audio_assets";
		const string FreeAudioAssetsDirRel = @"..\..\..\free_audio_assets";

		const string BgmFileName = "bgm_maoudamashii_healing01.mp3";

		const int FramesPerSecond = 60;

		readonly string _freeAudioAssetsDirAbs;

		readonly string[] _seAudioFileNames =
		{
			"se_maoudamashii_magical01.wav",
			"se_maoudamashii_magical02.wav",
			"se_maoudamashii_magical03.wav",
			"se_maoudamashii_magical07.wav",
		};

		MyAudioSet1 _audioSet;

		//byte[][] _seFileOnMemory = new byte[MyAudioSet1.SoundEffectsSlotCount][];
		//byte[] _bgmFileOnMemory;

		System.Windows.Threading.DispatcherTimer _dispatcherTimer;

		public MainWindow()
		{
			InitializeComponent();

			//this._freeAudioAssetsDirAbs = System.IO.Path.Combine(System.Environment.GetFolderPath(Environment.SpecialFolder.Personal), FreeAudioAssetsDirRel);
			this._freeAudioAssetsDirAbs = System.IO.Path.GetFullPath(FreeAudioAssetsDirRel);

			this.Closed += MainWindow_Closed;

			this._dispatcherTimer = new System.Windows.Threading.DispatcherTimer();
			this._dispatcherTimer.Interval = new TimeSpan(1000 * 1000 * 10 / FramesPerSecond); // 単位は100[ns]。
			this._dispatcherTimer.Tick += _dispatcherTimer_Tick;

			try
			{
				// SharpDX の XAudio2 は、デスクトップ版の場合 v2.7 (XAudio2_7.dll) を使う模様。つまり DirectX エンドユーザーランタイムのインストールが必須。
				// おそらく Windows 8.x ストア アプリ版の場合は v2.8 を、Windows 10 UWP アプリ版の場合は v2.9 をそれぞれ使ってくれるはず。
				// SharpDX には XAudio2 のほか Media Foundation のサポートもあるようなので、
				// マネージ アプリケーションで WAV/WMA/MP3 の高パフォーマンス ストリーム再生やミキシングをするのに使えそう……と思ったが、
				// C++ と比べてリソース管理や例外発生時の処理が煩雑すぎ。逆に不安感がある。
				// メモリの確保・解放など、ローレベルな部分は結局 SharpDX のソースコードを追って、実装を具体的にイメージしておかなければならない。
				// C++/CLI や C++/CX で直接ラッパーライブラリを書いて、高レベルな I/F の形で公開し、C# の WPF/WinRT フロントエンドで使うほうがよさそう。
				// もし Windows Vista/7 もターゲットにする場合は DirectX SDK June 2010 とエンドユーザーランタイムが必要になるが、
				// Windows 8 以降をターゲットにする場合は Windows SDK 8.0 以降があれば開発でき、また追加ランタイムのインストールも不要。

				MyAudioHelpers.MyAudioManager.Startup();

				this._audioSet = new MyAudioSet1();
				this._audioSet._audioManager.SetMasterVolume(0.1f);

				for (int i = 0; i < MyAudioSet1.SoundEffectsSlotCount; ++i)
				{
					var fileFullPath = System.IO.Path.Combine(this._freeAudioAssetsDirAbs, this._seAudioFileNames[i]);
					var buffer = MyMiscHelpers.MyIOHelper.LoadBinaryFromFile(fileFullPath);
					// ソース配列のマネージ参照は SharpDX 内部で使われているストリームオブジェクト内で保持しているらしいので、
					// ホスト側では寿命管理する必要はないらしい。
					//this._seFileOnMemory[i] = buffer;
					this._audioSet._audioPlayerSEs[i] = this._audioSet.CreateAudioPlayer(buffer);
				}

				{
					var fileFullPath = System.IO.Path.Combine(this._freeAudioAssetsDirAbs, BgmFileName);
					var buffer = MyMiscHelpers.MyIOHelper.LoadBinaryFromFile(fileFullPath);
					//this._bgmFileOnMemory = buffer;
					this._audioSet._audioPlayerBGM = this._audioSet.CreateAudioPlayer(buffer);
				}

				this._dispatcherTimer.Start();
			}
			catch (Exception ex)
			{
				MessageBox.Show(ex.Message);
			}
		}

		private void MainWindow_Closed(object sender, EventArgs e)
		{
			this._dispatcherTimer.Stop();

			MyAudioSet1.SafeDestroy(ref this._audioSet);

			MyAudioHelpers.MyAudioManager.Shutdown();
		}

		private void _dispatcherTimer_Tick(object sender, EventArgs e)
		{
			if (this._audioSet != null)
			{
				for (int i = 0; i < MyAudioSet1.SoundEffectsSlotCount; ++i)
				{
					// SE はとりあえずすべてループなし。
					this._audioSet.UpdateSE(i);
				}
				this._audioSet.UpdateBGM(this.checkLoop.IsChecked == true);
			}
		}

		private void buttonPlay_Click(object sender, RoutedEventArgs e)
		{
			if (this._audioSet._audioPlayerBGM != null)
			{
				this._audioSet._audioPlayerBGM.Play();
			}
		}

		private void buttonPause_Click(object sender, RoutedEventArgs e)
		{
			if (this._audioSet._audioPlayerBGM != null)
			{
				this._audioSet._audioPlayerBGM.Pause();
			}
		}

		private void buttonStop_Click(object sender, RoutedEventArgs e)
		{
			if (this._audioSet._audioPlayerBGM != null)
			{
				this._audioSet._audioPlayerBGM.Stop();
			}
		}

		private void buttonSE0_Click(object sender, RoutedEventArgs e)
		{
			this._audioSet.PlaySE(0);
		}

		private void buttonSE1_Click(object sender, RoutedEventArgs e)
		{
			this._audioSet.PlaySE(1);
		}

		private void buttonSE2_Click(object sender, RoutedEventArgs e)
		{
			this._audioSet.PlaySE(2);
		}

		private void buttonSE3_Click(object sender, RoutedEventArgs e)
		{
			this._audioSet.PlaySE(3);
		}
	}
}
