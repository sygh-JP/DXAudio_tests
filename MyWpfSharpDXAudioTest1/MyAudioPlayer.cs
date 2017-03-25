using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MyMiscHelpers
{
	public static class MyPInvokeHelper
	{
		[System.Runtime.InteropServices.DllImport("kernel32.dll")]
		public static extern void CopyMemory(IntPtr dst, IntPtr src, IntPtr size);

		[System.Runtime.InteropServices.DllImport("Kernel32.dll", EntryPoint = "RtlZeroMemory")]
		public static extern void ZeroMemory(IntPtr dest, IntPtr size);
	}

	public static class MyGenericsHelper
	{
		/// <summary>
		/// IDisposable を安全に Dispose する汎用メソッド。
		/// </summary>
		/// <typeparam name="Type">IDisposable</typeparam>
		/// <param name="obj"></param>
		public static void SafeDispose<Type>(ref Type obj)
			where Type : IDisposable
		{
			if (obj != null)
			{
				obj.Dispose();
				obj = default(Type); // null 非許容型への対応。
			}
		}

#if false
		public static void SafeRelease<Type>(ref Type obj)
			where Type : SharpDX.CppObject, SharpDX.IUnknown
		{
			if (obj != null)
			{
				System.Runtime.InteropServices.Marshal.Release(obj.NativePointer);
				obj.NativePointer = IntPtr.Zero; // ファイナライザによる Dispose 内での多重解放の防止。
				obj = null;
			}
		}
#endif
	}

	public static class MyIOHelper
	{
		public static byte[] LoadBinaryFromFile(string filePath)
		{
			using (var fs = new System.IO.FileStream(filePath, System.IO.FileMode.Open, System.IO.FileAccess.Read))
			{
				// 2GB 以上のファイルには対応しない。32bit 版の場合は実質 1GB 以上は対応不可能。
				if (fs.Length > Int32.MaxValue)
				{
					throw new NotSupportedException("The file is too huge!!");
				}
				var buffer = new byte[fs.Length];
				fs.Read(buffer, 0, (int)fs.Length);
				return buffer;
			}
		}
	}

	public class NativeBufferSet
	{
		public IntPtr Pointer { get; private set; }
		public int SizeInBytes { get; private set; }

		/// <summary>
		/// バッファの縮小はしない。CRT ヒープやマネージヒープと比べると、Win32 API による直接のメモリ確保・解放の速度はあまり速くないはず。
		/// </summary>
		/// <param name="newSizeInBytes"></param>
		public void ExpandNativeBuffer(int newSizeInBytes)
		{
			if (this.Pointer == IntPtr.Zero)
			{
				this.Pointer = System.Runtime.InteropServices.Marshal.AllocCoTaskMem(newSizeInBytes);
				this.SizeInBytes = newSizeInBytes;
			}
			else if (newSizeInBytes > this.SizeInBytes)
			{
				// 再確保に失敗した場合は OutOfMemoryException がスローされるので、以前のポインタを失なうことはない。
				this.Pointer = System.Runtime.InteropServices.Marshal.ReAllocCoTaskMem(this.Pointer, newSizeInBytes);
				this.SizeInBytes = newSizeInBytes;
			}
		}

		// HACK: IDisposable の実装。

		public void Destroy()
		{
			if (this.Pointer != IntPtr.Zero)
			{
				System.Runtime.InteropServices.Marshal.FreeCoTaskMem(this.Pointer);
				this.Pointer = IntPtr.Zero;
				this.SizeInBytes = 0;
			}
		}
	}

}

namespace MyAudioHelpers
{
	using MyMiscHelpers;
	using MF = SharpDX.MediaFoundation;
	using XA = SharpDX.XAudio2;

	public enum MyAudioPlayerState
	{
		Stopped,
		Playing,
		Pausing,
	};

	public class MyAudioPlayer
	{
		const int AudioRingBufferCount = 4;
		const int OneRingBufferInitialSizeInBytes = 32 * 1024;

		MF.SourceReader _mfSourceReader;
		XA.SourceVoice _xaSourceVoice;
		readonly NativeBufferSet[] _audioRingBuffers = new NativeBufferSet[AudioRingBufferCount];
		int _audioRingBufferIndex = 0;
		MyAudioPlayerState _playerState = MyAudioPlayerState.Stopped;
		bool _isEndOfStream = false;
		bool _isLoopMode = false;

		// 今回は C++ 版のクラス設計をほぼそのまま移植しているため、一部 C# らしくないコードになっているが意図的なもの。

		public MyAudioPlayer() { }

		public void Init(XA.XAudio2 xaudio, MF.SourceReader mfSourceReader)
		{
			IntPtr waveFormatPtr = IntPtr.Zero;

			for (int i = 0; i < this._audioRingBuffers.Count(); ++i)
			{
				this._audioRingBuffers[i] = new NativeBufferSet();
				this._audioRingBuffers[i].ExpandNativeBuffer(OneRingBufferInitialSizeInBytes);
			}

			try
			{
				this._mfSourceReader = mfSourceReader;
				using (var mfMediaType = this._mfSourceReader.GetCurrentMediaType(MF.SourceReaderIndex.FirstAudioStream))
				{
					int waveFormatLength;
					// ExtracttWaveFormat は多分スペルミス。SharpDX.ComObject.ComObject(object iunknowObject) にも引数名にスペルミスあり。
					var waveFormatWrapper = mfMediaType.ExtracttWaveFormat(out waveFormatLength, MF.WaveFormatExConvertFlags.ForceExtensible);
					waveFormatPtr = SharpDX.Multimedia.WaveFormat.MarshalToPtr(waveFormatWrapper);
					this._xaSourceVoice = new XA.SourceVoice(xaudio, waveFormatWrapper, XA.VoiceFlags.NoPitch, 1.0f, false);
					waveFormatWrapper = null;
				}
			}
			finally
			{
				// CoTaskMemFree() 呼び出し相当。
				System.Runtime.InteropServices.Marshal.FreeCoTaskMem(waveFormatPtr);

				// マネージスレッド ID はネイティブスレッド ID とは異なる。
				// もしイベントベースで実装し、XAudio2 が用意するコールバックスレッドと比較する場合は、ネイティブスレッドをチェックするようにしたほうがよい。
				System.Diagnostics.Debug.WriteLine("CurrentThreadID = {0}", System.Threading.Thread.CurrentThread.ManagedThreadId);
			}
		}

		/// <summary>
		/// 面倒なので IDisposable は実装せず、明示的な破棄を義務付ける。using ブロックで使うわけではないので、実装しようがすまいがあまり関係ない。
		/// </summary>
		public void Destroy()
		{
			// HACK: 複数の音声を再生している場合、停止から SourceReader 破棄までの間隔が短いと、Access Violation が発生する模様。具体的な原因は不明。
			// とりあえずスレッドを1ミリ秒待機させることで回避できるようだが、時間は環境に依存しそう。根本原因の調査が必要。
			// SharpDX を一切使わず、ネイティブ C++ で直接 Media Foundation を叩いて組んだ場合も発生するのか？
			// Windows 8.1 では発生するが、Windows 10 では発生しない？
			// SharpDX 3.0.2 では発生するが、SharpDX 3.1.1 では発生しない？
			this.Stop();

			//System.Threading.Thread.Sleep(1);

			MyGenericsHelper.SafeDispose(ref this._xaSourceVoice);

			if (this._mfSourceReader != null)
			{
				//this._mfSourceReader.Flush(MF.SourceReaderIndex.FirstAudioStream);
				//this._mfSourceReader.SetStreamSelection(MF.SourceReaderIndex.FirstAudioStream, new SharpDX.Mathematics.Interop.RawBool(false));
			}

			MyGenericsHelper.SafeDispose(ref this._mfSourceReader);

			foreach (var buffer in this._audioRingBuffers)
			{
				System.Diagnostics.Debug.Assert(buffer != null);
				buffer.Destroy();
			}
		}

		public static void SafeDestroy(ref MyAudioPlayer obj)
		{
			if (obj != null)
			{
				obj.Destroy();
				obj = null;
			}
		}

		public void Update()
		{
			if (this._playerState != MyAudioPlayerState.Playing)
			{
				return;
			}
			if (this.GetIsEndOfStream())
			{
				this.Stop();
				if (this._isLoopMode)
				{
					this.Play();
				}
			}
			else
			{
				if (this.GetIsSubmitRequired())
				{
					this.SubmitBuffer();
				}
			}
		}

		public void SetVolume(float value)
		{
			if (this._xaSourceVoice != null)
			{
				this._xaSourceVoice.SetVolume(value);
			}
		}

		public float GetVolume()
		{
			if (this._xaSourceVoice != null)
			{
				return this._xaSourceVoice.Volume;
			}
			return 0;
		}

		public void Pause()
		{
			if (this._xaSourceVoice != null && this._playerState == MyAudioPlayerState.Playing)
			{
				this._xaSourceVoice.Stop();
				this._playerState = MyAudioPlayerState.Pausing;
			}
		}

		public void Play()
		{
			if (this._xaSourceVoice != null && this._playerState != MyAudioPlayerState.Playing)
			{
				this.SubmitBuffer();
				this.SubmitBuffer();

				this._xaSourceVoice.Start();

				this._playerState = MyAudioPlayerState.Playing;
			}
		}

		public void Stop()
		{
			if (this._xaSourceVoice != null && this._playerState != MyAudioPlayerState.Stopped)
			{
				this._playerState = MyAudioPlayerState.Stopped;
				this._xaSourceVoice.Stop();
				this._xaSourceVoice.FlushSourceBuffers();
				this.RewindPositionToHead();
				this._isEndOfStream = false;
			}
		}

		public MyAudioPlayerState GetPlayerState() { return this._playerState; }

		public bool GetIsEndOfStream() { return this._isEndOfStream; }

		public bool GetIsLoopMode() { return this._isLoopMode; }
		public void SetIsLoopMode(bool value) { this._isLoopMode = value; }

		public int GetQueuedBuffersCount()
		{
			if (this._xaSourceVoice != null)
			{
				var stateInfo = this._xaSourceVoice.State;
				return stateInfo.BuffersQueued;
			}
			return 0;
		}

		public bool GetIsSubmitRequired()
		{ return (this.GetQueuedBuffersCount() < 3); }

		private void RewindPositionToHead()
		{
			if (this._mfSourceReader == null)
			{
				return;
			}
			this._mfSourceReader.SetCurrentPosition(0);
		}

		public void SubmitBuffer()
		{
			int audioBufferLength = 0;
			var pAudioBuffer = this.GetNextBlock(ref audioBufferLength);

			if (pAudioBuffer != IntPtr.Zero)
			{
				var buffer = new XA.AudioBuffer();
				//buffer.LoopCount = XA.AudioBuffer.LoopInfinite;
				buffer.AudioBytes = audioBufferLength;
				buffer.AudioDataPointer = pAudioBuffer;
				buffer.Context = pAudioBuffer;
				this._xaSourceVoice.SubmitSourceBuffer(buffer, null);
			}
			else
			{
			}
		}

		private IntPtr GetNextBlock(ref int audioBufferLength)
		{
			int actualStreamIndex = 0;
			MF.SourceReaderFlags flags;
			long timestamp = 0;
			using (var mfSample = this._mfSourceReader.ReadSample(MF.SourceReaderIndex.FirstAudioStream, MF.SourceReaderControlFlags.None, out actualStreamIndex, out flags, out timestamp))
			{

				if (flags.HasFlag(MF.SourceReaderFlags.Endofstream))
				{
					if (!this._isEndOfStream)
					{
						System.Diagnostics.Debug.WriteLine("End of audio stream file. CurrentThreadID = {0}", System.Threading.Thread.CurrentThread.ManagedThreadId);
						this._isEndOfStream = true;
					}
					audioBufferLength = 0;
					return IntPtr.Zero;
				}

				using (var mfMediaBuffer = mfSample.ConvertToContiguousBuffer())
				{
					IntPtr pAudioData = IntPtr.Zero;
					try
					{
						int audioDataLengthMax = 0;
						int audioDataLengthCurrent = 0;
						pAudioData = mfMediaBuffer.Lock(out audioDataLengthMax, out audioDataLengthCurrent);

						var audioBuffer = this._audioRingBuffers[this._audioRingBufferIndex];

						this._audioRingBufferIndex++;
						if (this._audioRingBufferIndex >= AudioRingBufferCount)
						{
							this._audioRingBufferIndex = 0;
						}
						audioBuffer.ExpandNativeBuffer(audioDataLengthCurrent);
						System.Diagnostics.Debug.Assert(audioBuffer.Pointer != IntPtr.Zero);
						System.Diagnostics.Debug.Assert(pAudioData != IntPtr.Zero);
						MyPInvokeHelper.CopyMemory(audioBuffer.Pointer, pAudioData, new IntPtr(audioDataLengthCurrent));
						audioBufferLength = audioDataLengthCurrent;
						return audioBuffer.Pointer;
					}
					finally
					{
						if (pAudioData != IntPtr.Zero)
						{
							mfMediaBuffer.Unlock();
						}
					}

				}
			}
		}
	}

	class MyAudioManager
	{
		XA.XAudio2 _xaudio;
		XA.MasteringVoice _xaMasteringVoice;

		public MyAudioManager()
		{
			// foobar2000 など、WASAPI 排他モードを使えるアプリケーションを起動しておくことで、XAudio2 の初期化エラーをエミュレート＆テストできるはず。

			this._xaudio = new XA.XAudio2();
			this._xaMasteringVoice = new XA.MasteringVoice(_xaudio);

			this.SetMasterVolume(0);
		}

		/// <summary>
		/// 面倒なので IDisposable は実装せず、明示的な破棄を義務付ける。using ブロックで使うわけではないので、実装しようがすまいがあまり関係ない。
		/// </summary>
		public void Destroy()
		{
			MyGenericsHelper.SafeDispose(ref this._xaudio);
			MyGenericsHelper.SafeDispose(ref this._xaMasteringVoice);
		}

		public static void SafeDestroy(ref MyAudioManager obj)
		{
			if (obj != null)
			{
				obj.Destroy();
				obj = null;
			}
		}

		public void SetMasterVolume(float value)
		{
			System.Diagnostics.Debug.Assert(this._xaMasteringVoice != null);
			this._xaMasteringVoice.SetVolume(value);
		}

		public static void Startup()
		{
			// Media Foundation の初期化を忘れると、SourceReader の生成時に ContextSwitchDeadlock 起因のタイムアウトエラーになってしまうので注意。
			// 未初期化を示すエラーを例外として即座に返してくれればいいのに……
			MF.MediaManager.Startup();
		}

		public static void Shutdown()
		{
			MF.MediaManager.Shutdown();
		}

		public static MF.SourceReader CreateSourceReader(MF.IByteStream mfByteStream)
		{
			throw new NotImplementedException();
		}

		public static MF.SourceReader CreateSourceReader(byte[] buffer)
		{
			// 列挙体・構造体・COM インターフェイスのマッピングは下記を参考にできそう。SharpDX はある程度までラップコードを自動生成している模様。
			// http://sharpdx.org/wiki/class-library-api/mediafoundation.html
			// GUID 定数のマッピングは下記を参考にできそう。
			// https://github.com/sharpdx/SharpDX/blob/master/Source/SharpDX.MediaFoundation/Mapping-core.xml

			// マネージ参照の代入は COM スマートポインタの代入とは別物。
			// マネージ参照を代入しただけでは、明示的な Dispose すなわち IUnknown::Release() による COM 参照カウント減少は不要。
			// 通例、他の COM オブジェクトのメソッド引数に渡すなどのタイミングで COM 参照カウントが増える。
			// フィールドで保持するルート オブジェクトなど、別の COM オブジェクトから参照されないものだけを最後に Dispose すればよいはず。
			// HACK: オブジェクト間の COM 参照関連付け前に途中で例外が発生したときはどうする？
			// Dispose を明示的に呼び出すタイミングを失い、GC 任せになるのは危険なので、どのみち例外 catch 時に明示的な破棄が必要になる。
			// だとすれば C# での記述は煩雑すぎる。using で Dispose すれば、C++ のスマートポインタのように例外発生時の COM 参照カウントを安全に制御できる？
			// C# の using による後始末は、C++ のコンストラクタ・デストラクタと違ってコードのネストが深くなりがちなのが欠点。
			// using を使わずに生成すると、戻り値として返却するまでに例外発生した場合の後始末が大変。
			// かといって、using ブロックで生成したオブジェクトをそのまま返すと、
			// 破棄後の無効なオブジェクトを返すことになってしまうので、COM 参照カウントを増やしてから返す。
			// → ルート オブジェクトには using は使えない模様。たとえ QueryInterface や AddReference していても、
			// 一度でも Dispose されたオブジェクトを使おうとすると、AccessViolationException が発生する。
			// QueryInterface はオブジェクトを複製するものではなく、また SharpDX.ComObject.Dispose() で Release した後、
			// 参照カウント残数にかかわらずネイティブポインタに NULL を代入する実装になっていることが原因。
			// したがって、ルート オブジェクトは Dispose すなわち Release を明示的に制御する必要がある。

			// MediaAttributes のコンストラクタは MFCreateAttributes() 相当らしいが、第2引数 cInitialSize の説明は
			// The initial number of elements allocated for the attribute store. The attribute store grows as needed.
			// とあるので、initialSizeInBytes という引数名は間違っているように思われる。
			// https://msdn.microsoft.com/en-us/library/windows/desktop/ms701878.aspx

			MF.SourceReader mfSourceReader = null;

			try
			{
				using (var mfAttributes = new MF.MediaAttributes(1))
				{
					mfAttributes.Set(MF.SinkWriterAttributeKeys.LowLatency, true);

					using (var mfMediaType = new MF.MediaType())
					{
						mfMediaType.Set(MF.MediaTypeAttributeKeys.MajorType, MF.MediaTypeGuids.Audio);
						//mfMediaType.Set(MF.MediaTypeAttributeKeys.Subtype, MF.AudioFormatGuids.Float); // MP3 では OK だが、WAV だと 0xC00D5212 で失敗する模様。
						mfMediaType.Set(MF.MediaTypeAttributeKeys.Subtype, MF.AudioFormatGuids.Pcm);
						mfSourceReader = new MF.SourceReader(buffer, mfAttributes);
						// オリジナルの IMFSourceReader::SetCurrentMediaType() の第2引数は NULL を指定する予約引数だが、SharpDX では隠ぺいされている。
						mfSourceReader.SetCurrentMediaType(MF.SourceReaderIndex.FirstAudioStream, mfMediaType);

						//return mfSourceReader.QueryInterface<MF.SourceReader>();
						//(mfSourceReader as SharpDX.IUnknown).AddReference();
						return mfSourceReader;
					}
				}
			}
			catch
			{
				MyGenericsHelper.SafeDispose(ref mfSourceReader);
				throw;
			}
		}

		public MyAudioPlayer CreateAudioPlayer(MF.SourceReader mfSourceReader)
		{
			var player = new MyAudioPlayer();
			player.Init(this._xaudio, mfSourceReader);
			return player;
		}
	}
}
