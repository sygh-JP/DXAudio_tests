#pragma once

namespace MyUtils
{
#ifdef __cplusplus_cli
	// T*& を受け取る関数テンプレートは、C++/CLI マネージ クラスのフィールドに対しては使えない。
	// 一見同じポインタ型に見えても、実態は異なる。トラッキング参照を使わなければならない。

	template<typename T> void SafeDelete(T*% p)
	{
		delete p;
		p = nullptr;
	}

	template<typename T> void SafeRelease(T*% p)
	{
		if (p)
		{
			p->Release();
			p = nullptr;
		}
	}

	template<typename T> void SafeDelete(T^% p)
	{
		delete p; // IDisposable であれば Dispose() が呼ばれる。
		p = nullptr;
	}
#endif

	template<typename T> void SafeDelete(T*& p)
	{
		delete p;
		p = nullptr;
	}

	template<typename T> void SafeRelease(T*& p)
	{
		if (p)
		{
			p->Release();
			p = nullptr;
		}
	}
}

namespace MyUtils
{
	template<typename T> void LoadBinaryFromFileImpl(LPCWSTR pFilePath, std::vector<T>& outBuffer)
	{
		outBuffer.clear();

		struct _stat64 fileStats = {};
		const auto getFileStatFunc = _wstat64;

		if (getFileStatFunc(pFilePath, &fileStats) != 0 || fileStats.st_size < 0)
		{
			throw std::exception("Cannot get the file stats for the file!!");
		}

		if (fileStats.st_size % sizeof(T) != 0)
		{
			throw std::exception("The file size is not a multiple of the expected size of element!!");
		}

		const auto fileSizeInBytes = static_cast<uint64_t>(fileStats.st_size);

		if (sizeof(size_t) < 8 && (std::numeric_limits<size_t>::max)() < fileSizeInBytes)
		{
			throw std::exception("The file size is over the capacity on this platform!!");
		}

		if (fileStats.st_size == 0)
		{
			return;
		}

		const auto numElementsInFile = static_cast<size_t>(fileStats.st_size / sizeof(T));

		outBuffer.resize(numElementsInFile);

		FILE* pFile = nullptr;
		const auto retCode = _wfopen_s(&pFile, pFilePath, L"rb");
		if (retCode != 0 || pFile == nullptr)
		{
			throw std::exception("Cannot open the file!!");
		}
		fread_s(&outBuffer[0], numElementsInFile * sizeof(T), sizeof(T), numElementsInFile, pFile);
		fclose(pFile);
		pFile = nullptr;
	}

	template<typename T> bool LoadBinaryFromFileImpl2(LPCWSTR pFileName, std::vector<T>& buffer)
	{
		try
		{
			LoadBinaryFromFileImpl(pFileName, buffer);
			return true;
		}
		catch (const std::exception& ex)
		{
			const CStringW strMsg(ex.what());
			ATLTRACE(L"%s <%s>\n", strMsg.GetString(), pFileName);
			return false;
		}
	}

	inline bool LoadBinaryFromFile(LPCWSTR pFileName, std::vector<uint8_t>& buffer)
	{
		return LoadBinaryFromFileImpl2(pFileName, buffer);
	}

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	// Windows 8 以降であればデスクトップからでも一応 WinRT を使用できるが、対応しない。
#else

	// DirectX ストア アプリのテンプレートに付属していたコードから引用。

	inline Concurrency::task<Platform::Array<byte>^> CreateByteArrayFromLocalFileAsync(Platform::String^ filename)
	{
		using namespace Windows::Storage;
		using namespace Concurrency;

		auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;

		return create_task(folder->GetFileAsync(filename)).then([](StorageFile^ file)
		{
			return FileIO::ReadBufferAsync(file);
		}).then([](Streams::IBuffer^ fileBuffer)
		{
			auto fileData = ref new Platform::Array<byte>(fileBuffer->Length);
			Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(fileData);
			return fileData;
		});
	}

	inline Concurrency::task<Windows::Storage::Streams::InMemoryRandomAccessStream^> CreateMemoryStreamFromLocalFileAsync(Platform::String^ filename)
	{
		using namespace Windows::Storage;
		using namespace Concurrency;
		using namespace Windows::Storage::Streams;

		// 微妙な違いはあるが、どちらでも OK らしい。
#if 1
		auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;
		return create_task(folder->GetFileAsync(filename)).then([](StorageFile^ file)
		{
			return FileIO::ReadBufferAsync(file);
		}).then([](Streams::IBuffer^ fileBuffer)
		{
			auto stream = ref new InMemoryRandomAccessStream();
			auto task = create_task(stream->WriteAsync(fileBuffer));
			task.wait();
			stream->Seek(0);
			return stream;
		});
#else
		auto stream = ref new InMemoryRandomAccessStream();
		return create_task([filename]()
		{
			auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;
			return folder->GetFileAsync(filename);
		}).then([](StorageFile^ file)
		{
			return FileIO::ReadBufferAsync(file);
		}).then([stream](Streams::IBuffer^ fileBuffer)
		{
			return stream->WriteAsync(fileBuffer);
		}).then([stream](uint32_t)
		{
			// IAsyncOperationWithProgress を普通に then でチェーンしてよいものかどうか不明。
			// C# の async/await と違って、C++/CX では PPL を直接使用しないといけないのでかなり扱いづらい。resumable/await が欲しい。
			stream->Seek(0);
			return stream;
		});
#endif
	}
#endif
}

namespace MyComHelpers
{
	// WRL の ComPtr は Visual C++ 2012 (Windows SDK 8.0) 以降で標準的に使用できる、新たな COM スマート ポインタのクラス テンプレートだが、
	// ATL の CComPtr と比較して余計な複雑さをはらんでいるという批判もある。
	// https://msdn.microsoft.com/ja-jp/magazine/dn904668.aspx
	// また、WRL は C++/CLI ("/clr") のコードから使用できないのが一番痛い。
	// ATL であれば C++/CLI でも C++/CX でも使える。
	// ATL/WRL に依存しないスマート ポインタとして、boost::intrusive_ptr を使う方法もあるが、
	// boost::intrusive_ptr はポインタを管理する private 内部フィールドへのアドレスを取得する operator& オーバーロードやメソッドが存在しないので、
	// void**, IUnknown** もしくは派生インターフェイスへのダブルポインタを受け取る COM ファクトリ関数に
	// 直接スマートポインタオブジェクトの内部フィールドへのアドレスを渡せないのが不便。
	// オブジェクト指向のカプセル化原則に忠実に従った実装の結果なのだろうが、かえって混乱するだけ。
	// 他に、_com_ptr_t を使う方法もあるが、テンプレート実体化の宣言に手間がかかるのがネック。
	// ATL は Express エディションの Visual C++ では使えないという制約もあるが、
	// 総合的に考えて、やはり ATL を使うべき。

	// ATL/WRL の吸収層。
	// ATL に最初から用意されている演算子オーバーロードやラッパーメソッドを素直に使えば、より簡潔に記述できるが、
	// WRL 互換の明示的な名前を持つラッパー関数を使ったほうがコードの意図を理解しやすくなる。grep もしやすい。
	// WRL のクラスを引数として受け取るグローバル関数オーバーロードを別途定義できるように、インターフェイスを共通化しておけば、
	// 今後 ATL から WRL あるいは他のライブラリに置き換えることも容易になる。
	// C++ に拡張メソッドの言語機能があればよいのだが……

#ifdef USES_WRL_COM_PTR_FOR_MY_HELPER
	template<typename T> T* Get(const Microsoft::WRL::ComPtr<T>& ptr)
	{
		return ptr.Get();
	}

	template<typename T> ULONG Reset(Microsoft::WRL::ComPtr<T>& ptr)
	{
		return ptr.Reset();
	}

	template<typename T, typename U> HRESULT As(const Microsoft::WRL::ComPtr<T>& pIn, Microsoft::WRL::ComPtr<U>& pOut)
	{
		return pIn.As(&pOut);
	}

	template<typename T> T** GetAddressOf(Microsoft::WRL::ComPtr<T>& ptr)
	{
		return ptr.GetAddressOf();
	}

	template<typename T> void** GetAddressAsVoidOf(Microsoft::WRL::ComPtr<T>& ptr)
	{
		return reinterpret_cast<void**>(ptr.GetAddressOf());
	}

	template<typename T> T*const* GetAddressOf(const Microsoft::WRL::ComPtr<T>& ptr)
	{
		return ptr.GetAddressOf();
	}

	template<typename T> T** ReleaseAndGetAddressOf(Microsoft::WRL::ComPtr<T>& ptr)
	{
		return ptr.ReleaseAndGetAddressOf();
	}

	template<typename T> void** ReleaseAndGetAddressAsVoidOf(Microsoft::WRL::ComPtr<T>& ptr)
	{
		return reinterpret_cast<void**>(ptr.ReleaseAndGetAddressOf());
	}

#else

	template<typename T> T* Get(const ATL::CComPtr<T>& ptr)
	{
		return static_cast<T*>(ptr);
	}

	template<typename T> ULONG Reset(ATL::CComPtr<T>& ptr)
	{
		auto temp = ptr.Detach();
		if (temp)
		{
			// ATL::CComPtr::Release は戻り値を返さないのがネック。IUnknown::Release を直接使う。
			return temp->Release();
		}
		else
		{
			return 0;
		}
	}

	template<typename T, typename U> HRESULT As(const ATL::CComPtr<T>& pIn, ATL::CComPtr<U>& pOut)
	{
		//return pIn->QueryInterface(__uuidof(U), ReleaseAndGetAddressAsVoidOf(pOut));
		return pIn.QueryInterface(ReleaseAndGetAddressOf(pOut));
	}

	template<typename T> T** GetAddressOf(ATL::CComPtr<T>& ptr)
	{
		return &ptr;
	}

	template<typename T> void** GetAddressAsVoidOf(ATL::CComPtr<T>& ptr)
	{
		return reinterpret_cast<void**>(&ptr);
	}

	template<typename T> T*const* GetAddressOf(const ATL::CComPtr<T>& ptr)
	{
		return &ptr;
	}

	template<typename T> T** ReleaseAndGetAddressOf(ATL::CComPtr<T>& ptr)
	{
		ptr.Release();
		return &ptr;
	}

	template<typename T> void** ReleaseAndGetAddressAsVoidOf(ATL::CComPtr<T>& ptr)
	{
		return reinterpret_cast<void**>(ReleaseAndGetAddressOf(ptr));
	}
#endif
}

namespace MyComHelpers
{
	// コードを共通化するため、独自のネイティブ C++ 例外クラスを定義する。_com_error や Platform::COMException は使わない。
	// __FILEW__ と __LINE__ を使ったマクロでラップして、スローした位置を記録できるようにする。
	// __FILE__ は使わない。
	// CStringW を使えば ANSI 文字列も受け取って変換できる。std::string/std::wstring と違い、nullptr が渡されたときの自前対処も要らない。
	// ただし ATL が必要になる。とはいえ、COM スマートポインタに ATL を使うと割り切っていれば大した問題ではない。
	class MyComException
	{
		HRESULT m_hresult = S_OK;
		CStringW m_message;
		CStringW m_filePath;
		int m_lineNumber = -1;
	private:
		MyComException() = delete;
	public:
		MyComException(HRESULT hr, const CStringW& message, const CStringW& filePath, int lineNumber)
			: m_hresult(hr), m_message(message), m_filePath(filePath), m_lineNumber(lineNumber)
		{}
		HRESULT GetResultCode() const { return m_hresult; }
		CStringW GetErrorMessage() const { return m_message; }
		CStringW GetFilePath() const { return m_filePath; }
		int GetLineNumber() const { return m_lineNumber; }
	};

	inline void ThrowIfFailed(HRESULT hr, CStringW message, CStringW filePath, int lineNumber)
	{
		if (FAILED(hr))
		{
			throw MyComException(hr, message, filePath, lineNumber);
		}
	}

#ifdef __cplusplus_cli
	inline System::Runtime::InteropServices::COMException^ ConvertToManagedCOMException(const MyComException& ex)
	{
		return gcnew System::Runtime::InteropServices::COMException(gcnew System::String(ex.GetErrorMessage()), ex.GetResultCode());
	}
#endif

	// IStream のファクトリー関数はことごとくデスクトップ専用らしい。IStream 自体がデスクトップ専用とみなしたほうがよい。
	// とりあえず関数インターフェイスを統一しておく。
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	inline HRESULT CreateStreamOnFile(LPCWSTR filePath, IStream*& outStream)
	{
		return ::SHCreateStreamOnFileW(filePath, STGM_READ, &outStream);
	}

	inline HRESULT CreateStreamOnFile(LPCWSTR filePath, IStream** ppOutStream)
	{
		_ASSERTE(ppOutStream != nullptr);
		return ::SHCreateStreamOnFileW(filePath, STGM_READ, ppOutStream);
	}

	inline HRESULT CreateStreamOnMemory(const BYTE* pInit, UINT sizeInBytes, IStream*& outStream)
	{
		// Windows 8 以降はスレッドセーフだが、それ以前はスレッドセーフではないらしい。
		outStream = ::SHCreateMemStream(pInit, sizeInBytes);
		return outStream ? S_OK : E_FAIL;
	}

	inline HRESULT CreateStreamOnMemory(const BYTE* pInit, UINT sizeInBytes, IStream** ppOutStream)
	{
		_ASSERTE(ppOutStream != nullptr);
		*ppOutStream = ::SHCreateMemStream(pInit, sizeInBytes);
		return *ppOutStream ? S_OK : E_FAIL;
	}
#endif
}

#define THROW_IF_FAILED(hr, message)  MyComHelpers::ThrowIfFailed((hr), (message), __FILEW__, __LINE__)

#if 0
// boost::intrusive_ptr 用の特殊化。
inline void intrusive_ptr_add_ref(IUnknown* p) { p->AddRef(); }
inline void intrusive_ptr_release(IUnknown* p) { p->Release(); }
#endif

namespace MyAudioHelpers
{
	// https://msdn.microsoft.com/en-us/magazine/dn166936.aspx
	// MSDN Magazine に掲載されていたストア アプリ向けのコードを改良。

	// C++11 のエイリアステンプレートを使っておけば、ATL から WRL あるいは他のライブラリに置き換えることも容易になる。
#ifdef USES_WRL_COM_PTR_FOR_MY_HELPER
	template<typename T> using MyComPtr = Microsoft::WRL::ComPtr<T>;
#else
	template<typename T> using MyComPtr = ATL::CComPtr<T>;
#endif

	enum class MyAudioPlayerState
	{
		Stopped,
		Playing,
		Pausing,
	};

	// Media Foundation のデコーダーを使えば、WAV だけでなく MP3, WMA, AAC などにも対応できる。
	// 再生前にすべてデコードしてしまい、RIFF の生データをメモリに載せておく方法もあるが、
	// 今回は少しずつキューイング用のデータ分だけデコードしていく。

	// XAudio2 でバッファアンダーランが起こると、
	// "XAUDIO2: WARNING: Glitch at output sample X"
	// というエラーメッセージが IDE 出力ウィンドウに出力される模様。
	// オーディオデータのバッファアンダーランはノイズの原因になる。
	// ただし、XAudio2 の更新間隔は30ミリ秒程度で十分らしい。
	// https://msdn.microsoft.com/ja-jp/library/ee415765.aspx

	// NOTE: ホスト（EXE）側を WPF アプリケーションで開発している場合、
	// Visual C# プロジェクトにて「ネイティブ コードのデバッグを有効にする」にチェックを入れていると、
	// マネージデバッグ実行時、Visual Studio の出力ウィンドウにテキストが出力されるたび、ノイズがひどく乗る模様。
	// デバッグビルドであっても、デバッグなしで実行すれば（デバッグセッションでなければ）ノイズは乗らない。
	// System.Diagnostics.Debug.WriteLine() によるユーザー出力だけでなく、C++ 例外発生時などでもノイズが乗る模様。
	// Visual C++ ネイティブコードのみで開発している場合は、このような現象は発生しない。
	// XAUDIO2_DEBUG_ENGINE のせいというわけでもないらしい。
	// UI スレッドではなく専用のサブスレッドを起動してオーディオデータを更新したとしても、おそらく改善されないと思う。
	// ちなみに、ネイティブコードのデバッグを有効にしていると、wil::ResultException が頻繁に報告されてうっとうしい。

	// MSDN オリジナル コードでは、XAudio2 イベント モードを使うために IXAudio2VoiceCallback インターフェイスを実装していたが、
	// ポーリング モードであれば別に実装の必要はない。
	class MyAudioPlayer : public IXAudio2VoiceCallback
	{
	private:
		// シングルバッファだとひどいノイズが乗る。とりあえず適当な数でリングバッファを作成してみる。
		static const int AudioRingBufferCount = 4;
		static const size_t OneRingBufferInitialSizeInBytes = 32 * 1024;
	private:
		MyComPtr<IMFSourceReader> m_pMFSourceReader;
		IXAudio2SourceVoice* m_pXASourceVoice = nullptr; // IUnknown 実装ではなく、Release() を持たない。
		std::array<std::vector<BYTE>, AudioRingBufferCount> m_audioRingBuffers;
		int m_audioRingBufferIndex = 0;
		MyAudioPlayerState m_playerState = MyAudioPlayerState::Stopped;
		bool m_isEndOfStream = false;
		bool m_isLoopMode = false;
		// ループフラグはフィールドで持たせるか、Update() メソッド引数で逐一指定するか、悩みどころ。
		// ただ、少なくともループするか否かはいつでも変更できるべき。再生開始時にしか指定できないというのは NG。

	private:
		MyAudioPlayer(const MyAudioPlayer&) = delete;
		MyAudioPlayer& operator=(const MyAudioPlayer&) = delete;

	public:
		MyAudioPlayer() {}

		void Init(MyComPtr<IXAudio2> pXAudio, MyComPtr<IMFSourceReader> pMFSourceReader)
		{
			// 例外がスローされてもオブジェクト本体がリークしないよう、コンストラクタではなく2段階初期化にする。

			this->m_pMFSourceReader = pMFSourceReader;

			for (auto& buffer : m_audioRingBuffers)
			{
				buffer.resize(OneRingBufferInitialSizeInBytes);
			}

			// Get the Media Foundation media type
			MyComPtr<IMFMediaType> mfMediaType;
			HRESULT hresult = m_pMFSourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, MyComHelpers::GetAddressOf(mfMediaType));
			THROW_IF_FAILED(hresult, L"IMFSourceReader::GetCurrentMediaType failure");

			// Create a WAVEFORMATEX from the media type
			WAVEFORMATEX* pWaveFormat = nullptr;
			UINT32 waveFormatLength = 0;
			hresult = ::MFCreateWaveFormatExFromMFMediaType(MyComHelpers::Get(mfMediaType), &pWaveFormat, &waveFormatLength);
			THROW_IF_FAILED(hresult, L"MFCreateWaveFormatExFromMFMediaType failure");

			// IXAudio2SourceVoice::GetState()を使い、キューの状態を監視しながら SubmitSourceBuffer() することで、
			// イベント モードでなくポーリングでも簡単にバッファをキューイングできる。
			// やはりゲームでストリーム再生を使うのであれば、イベント モードではなくポーリングを使ったほうがよさそう。
			// ポーリングの場合、低負荷のゲームであればたぶん UI スレッドにバッファの Submit をやらせても普通にいけると思うが、
			// グラフィックス負荷が変動した場合にも対応できるように、オーディオ専用スレッドは立てておいたほうがよさそう。
			// イベント モードは勝手にバッファ状態監視用のバックグラウンド スレッドを起動するっぽいので、
			// UI スレッドで排他制御やロックフリーなどの考えなしに開始直後のキック用 Submit をやらせるのは危険な香りがする。
			// であればいっそオーディオ監視のためのポーリング スレッドを明示的に起動して、同期をとるかロックフリーコードを書いたほうがいい。
			// マルチスレッド プログラムは一見正常に動作しているように見えるときが一番危なくて、真面目に設計されていないと
			// ちょっと変更を加えたり呼び出しのタイミングを変えたりしたときに不可解なバグが発生して原因追及がすさまじく困難になる。

			_ASSERTE(m_pXASourceVoice == nullptr);
			// Create the XAudio2 source voice
			//IXAudio2VoiceCallback* pCallback = this; // イベント モード。
			IXAudio2VoiceCallback* pCallback = nullptr; // コールバックは使わず、ポーリングする。
			hresult = pXAudio->CreateSourceVoice(&m_pXASourceVoice, pWaveFormat,
				XAUDIO2_VOICE_NOPITCH, 1.0f, pCallback);

			// Free the memory allocated by function
			::CoTaskMemFree(pWaveFormat);
			pWaveFormat = nullptr;

			THROW_IF_FAILED(hresult, L"CreateSourceVoice failure");

			ATLTRACE(_T("CurrentThreadID = 0x%lx\n"), ::GetCurrentThreadId());

			//this->Play();
		}

		virtual ~MyAudioPlayer()
		{
			this->Stop();
			if (m_pXASourceVoice)
			{
				m_pXASourceVoice->DestroyVoice();
				m_pXASourceVoice = nullptr;
			}
		}

		void Update()
		{
			if (m_playerState != MyAudioPlayerState::Playing)
			{
				return;
			}
			if (this->GetIsEndOfStream())
			{
				// まず停止。
				this->Stop();
				if (m_isLoopMode)
				{
					// HACK: 単純ループだけでなく、ループ開始ポイントも指定できるとよい。
					this->Play();
				}
			}
			else
			{
				if (this->GetIsSubmitRequired())
				{
					this->SubmitBuffer();
				}
			}
		}

		void SetVolume(float value)
		{
			if (m_pXASourceVoice)
			{
				HRESULT hresult = m_pXASourceVoice->SetVolume(value);
				THROW_IF_FAILED(hresult, L"IXAudio2SourceVoice::SetVolume failure");
			}
		}

		float GetVolume()
		{
			if (m_pXASourceVoice)
			{
				float value = 0;
				m_pXASourceVoice->GetVolume(&value);
				return value;
			}
			return 0;
		}

		void Pause()
		{
			if (m_pXASourceVoice && m_playerState == MyAudioPlayerState::Playing)
			{
				THROW_IF_FAILED(m_pXASourceVoice->Stop(), L"Stop");
				m_playerState = MyAudioPlayerState::Pausing;
			}
		}

		void Play()
		{
			// HACK: 一時停止からの再開が若干ぎこちない。
			if (m_pXASourceVoice && m_playerState != MyAudioPlayerState::Playing)
			{
				// Submit two buffers
				this->SubmitBuffer();
				this->SubmitBuffer();

				// Start the voice playing
				THROW_IF_FAILED(m_pXASourceVoice->Start(), L"Start");

				m_playerState = MyAudioPlayerState::Playing;
			}
		}

		void Stop()
		{
			// XAudio の Source Voice を Stop して、Source Buffers を Flush して、
			// Media Foundation のストリーム シーク位置を先頭に巻き戻しておく。
			if (m_pXASourceVoice && m_playerState != MyAudioPlayerState::Stopped)
			{
				// イベント モードで FlushSourceBuffers() を呼び出すと、OnBufferEnd() がコールバックされる。
				// OnBufferEnd() で SubmitSourceBuffer() を呼び出して無限ループに陥らないようにするため、状態フラグを使う。
				m_playerState = MyAudioPlayerState::Stopped;
				THROW_IF_FAILED(m_pXASourceVoice->Stop(), L"Stop");
				THROW_IF_FAILED(m_pXASourceVoice->FlushSourceBuffers(), L"FlushSourceBuffers");
				this->RewindPositionToHead();
				m_isEndOfStream = false;
			}
		}

		MyAudioPlayerState GetPlayerState() const { return m_playerState; }

		bool GetIsEndOfStream() const { return m_isEndOfStream; }

		bool GetIsLoopMode() const { return m_isLoopMode; }
		void SetIsLoopMode(bool value) { m_isLoopMode = value; }

		uint32_t GetQueuedBuffersCount()
		{
			if (m_pXASourceVoice)
			{
				XAUDIO2_VOICE_STATE stateInfo = {};
				m_pXASourceVoice->GetState(&stateInfo);
				return stateInfo.BuffersQueued;
			}
			return 0;
		}

		// キューイングされたバッファの数が規定値より小さい場合、新たな Submit が必要と判断する。
		bool GetIsSubmitRequired()
		{ return (this->GetQueuedBuffersCount() < 3); }

	private:
		void RewindPositionToHead()
		{
			if (!m_pMFSourceReader)
			{
				return;
			}
			PROPVARIANT varPosition = {};
			varPosition.vt = VT_I8;
			varPosition.hVal.QuadPart = 0;
			//::InitPropVariantFromInt64(0, &varPosition); // <Propvarutil.h> で定義されているらしいが、デスクトップ専用らしい？
			//::PropVariantClear(&varPosition); // <combaseapi.h>, <Propidl.h> で定義されている。こちらは WinRT アプリでも使える模様。
			THROW_IF_FAILED(m_pMFSourceReader->SetCurrentPosition(GUID_NULL, varPosition), L"SetCurrentPosition");
			//THROW_IF_FAILED(m_pMFSourceReader->Flush(MF_SOURCE_READER_FIRST_AUDIO_STREAM), L"Flush");
		}

	public:
		void SubmitBuffer()
		{
			// Get the next block of audio data
			int audioBufferLength = 0;
			auto pAudioBuffer = this->GetNextBlock(&audioBufferLength);

			if (pAudioBuffer != nullptr)
			{
				// Create an XAUDIO2_BUFFER for submitting audio data
				XAUDIO2_BUFFER buffer = {};
				//buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
				buffer.AudioBytes = audioBufferLength;
				buffer.pAudioData = pAudioBuffer;
				buffer.pContext = pAudioBuffer; // 各種コールバック関数の引数として渡されるらしい。
				HRESULT hresult = m_pXASourceVoice->SubmitSourceBuffer(&buffer);
				THROW_IF_FAILED(hresult, L"IXAudio2SourceVoice::SubmitSourceBuffer failure");
				// UNDONE: 圧縮フォーマットを再生する場合、XAUDIO2_BUFFER_WMA を使えばよい？
			}
			else
			{
				// ループ再生は任意のタイミングで ON/OFF できるようにするため、
				// XAUDIO2_LOOP_INFINITE は使わない。
				//this->RewindPositionToHead();
				// 再帰。ファイル末尾に到達したら先頭に戻り、永久にループする。
				// HACK: 万が一 GetNextBlock() が nullptr を返し続けた場合、無限再帰でスタックオーバーフローしそう。
				// タイマーでポーリングして、ストリームが終了しているか否かをチェックして、
				// ループする場合は明示的に停止と再生を実行するようにしたほうがいいかも。
				//this->SubmitBuffer();
			}
		}

	private:
		BYTE* GetNextBlock(int* pOutAudioBufferLength)
		{
			// Get an IMFSample object
			MyComPtr<IMFSample> mfSample;
			DWORD flags = 0;
			HRESULT hresult = m_pMFSourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM,
				0, nullptr, &flags, nullptr, MyComHelpers::GetAddressOf(mfSample));
			THROW_IF_FAILED(hresult, L"IMFSourceReader::ReadSample failure");

			// Check if we're at the end of the file
			if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
			{
				if (!m_isEndOfStream)
				{
					ATLTRACE(_T("End of audio stream file. CurrentThreadID = 0x%lx\n"), ::GetCurrentThreadId());
					m_isEndOfStream = true;
				}
				*pOutAudioBufferLength = 0;
				return nullptr;
			}

			// If not, convert the data to a contiguous buffer
			MyComPtr<IMFMediaBuffer> mfMediaBuffer;
			hresult = mfSample->ConvertToContiguousBuffer(MyComHelpers::GetAddressOf(mfMediaBuffer));
			THROW_IF_FAILED(hresult, L"IMFSample::ConvertToContiguousBuffer failure");

			// Lock the audio buffer and copy the samples to local memory
			BYTE* pAudioData = nullptr;
			DWORD currentAudioDataLength = 0;
			hresult = mfMediaBuffer->Lock(&pAudioData, nullptr, &currentAudioDataLength);
			THROW_IF_FAILED(hresult, L"IMFMediaBuffer::Lock failure");

			auto& audioBuffer = m_audioRingBuffers[m_audioRingBufferIndex];

			// 排他処理はしない。このメソッドが常に単一のスレッドからコールバックされるという前提。
			// イベント モードの場合、OnBufferEnd() をコールバックするのは UI スレッドではないらしい。
			m_audioRingBufferIndex++;
			if (m_audioRingBufferIndex >= AudioRingBufferCount)
			{
				m_audioRingBufferIndex = 0;
			}

			// オリジナル コードでは、OnBufferEnd() でオーディオバッファ用のメモリの動的確保（new[]）と解放（delete[]）を実行していたが、
			// アプリを終了するときに delete[] が呼ばれずにメモリリークが報告される。
			// 明示的な解放を不要にするため、そして動的確保と解放の負荷を減らすため、
			// 複数の std::vector によるリングバッファを使う。
			// resize() は確保した領域のシュリンクは行なわないので、負荷は小さい。
			audioBuffer.resize(currentAudioDataLength);
			//auto pAudioBuffer = new byte[currentAudioDataLength]();
			//::CopyMemory(pAudioBuffer, pAudioData, currentAudioDataLength);
			memcpy(&audioBuffer[0], pAudioData, currentAudioDataLength);
			hresult = mfMediaBuffer->Unlock();
			THROW_IF_FAILED(hresult, L"IMFMediaBuffer::Unlock failure");

			*pOutAudioBufferLength = currentAudioDataLength;
			//return pAudioBuffer;
			return &audioBuffer[0];
		}

#pragma region // Callbacks required for IXAudio2VoiceCallback
		void _stdcall OnVoiceProcessingPassStart(UINT32 BytesRequired) {}
		void _stdcall OnVoiceProcessingPassEnd() {}
		void _stdcall OnStreamEnd() {}
		void _stdcall OnBufferStart(void* pBufferContext) {}
		void _stdcall OnBufferEnd(void* pBufferContext)
		{
			if (m_playerState == MyAudioPlayerState::Playing && !m_isEndOfStream)
			{
				this->SubmitBuffer();
			}
#if 0
			// Remember to free the audio buffer!
			//delete[] pBufferContext;
			//pBufferContext = nullptr;

			// Either submit a new buffer or clean up
			if (!m_isEndOfStream)
			{
				this->SubmitBuffer();
			}
			else
			{
				ATLTRACE(_T("End of audio stream file.\n"));
			}
#endif
		}

		void _stdcall OnLoopEnd(void* pBufferContext) {}
		void _stdcall OnVoiceError(void* pBufferContext, HRESULT Error) {}
#pragma endregion
	};


	class MyAudioManager
	{
		MyComPtr<IXAudio2> m_pXAudio;
		IXAudio2MasteringVoice* m_pXAMasteringVoice = nullptr; // IUnknown 実装ではなく、Release() を持たない。

	private:
		MyAudioManager(const MyAudioManager&) = delete;
		MyAudioManager& operator=(const MyAudioManager&) = delete;

	public:
		MyAudioManager() {}

		void Init(bool enablesDebugMode = false)
		{
			MyComPtr<IUnknown> pUnknown;

			ATLTRACE("Test of QueryInterface (before): 0x%p, 0x%p\n", MyComHelpers::Get(m_pXAudio), MyComHelpers::Get(pUnknown));

			UINT32 xaCreationFlags = 0;
#if defined(_DEBUG) && defined(XAUDIO2_DEBUG_ENGINE)
			// XAUDIO2_DEBUG_ENGINE は、Windows SDK 8.x には存在しない。SDK 10 では復活している。
			if (enablesDebugMode)
			{
				xaCreationFlags |= XAUDIO2_DEBUG_ENGINE;
			}
#endif

			// Create an IXAudio2 object
			// COM の初期化を忘れていたときなどに失敗する。
			// 単純なコンソール アプリケーションなどでは、XAudio2 を使う前にユーザーコードによる明示的な COM の事前初期化（と終了）が必要となるが、
			// Direct3D などの COM コンポーネントが要素技術として標準的に使われている WPF アプリケーションでは不要。
			// COM の初期化（と終了）はライブラリ内部で行なわず、アプリケーション側にゆだねる。
			HRESULT hresult = ::XAudio2Create(MyComHelpers::GetAddressOf(m_pXAudio), xaCreationFlags);
			THROW_IF_FAILED(hresult, L"XAudio2Create failure");

			// QueryInterface のテスト。
			_ASSERTE(SUCCEEDED(MyComHelpers::As(m_pXAudio, pUnknown)));
			ATLTRACE("Test of QueryInterface (after): 0x%p, 0x%p\n", MyComHelpers::Get(m_pXAudio), MyComHelpers::Get(pUnknown));

			_ASSERTE(m_pXAMasteringVoice == nullptr);
			// Create a mastering voice
			hresult = m_pXAudio->CreateMasteringVoice(&m_pXAMasteringVoice);
			THROW_IF_FAILED(hresult, L"IXAudio2::CreateMasteringVoice failure");

			// Initialize the volume
			hresult = m_pXAMasteringVoice->SetVolume(0);
			THROW_IF_FAILED(hresult, L"IXAudio2MasteringVoice::SetVolume failure");

			// いったんマスターボイスをゼロにしておく。
			// 使用時は明示的にマスターボイスを事前設定すること。
		}

		virtual ~MyAudioManager()
		{
			if (m_pXAMasteringVoice)
			{
				m_pXAMasteringVoice->DestroyVoice();
				m_pXAMasteringVoice = nullptr;
			}
		}

		void SetMasterVolume(float value)
		{
			_ASSERTE(m_pXAMasteringVoice);
			HRESULT hresult = m_pXAMasteringVoice->SetVolume(value);
			THROW_IF_FAILED(hresult, L"IXAudio2MasteringVoice::SetVolume failure");
		}

		float GetMasterVolume()
		{
			_ASSERTE(m_pXAMasteringVoice);
			float value = 0;
			m_pXAMasteringVoice->GetVolume(&value);
			return value;
		}

		static void Startup()
		{
			// Start up the Media Foundation
			HRESULT hresult = ::MFStartup(MF_VERSION);
			THROW_IF_FAILED(hresult, L"MFStartup failure");
		}

		static void Shutdown()
		{
			HRESULT hresult = ::MFShutdown();
			THROW_IF_FAILED(hresult, L"MFShutdown failure");
		}

		// 例えば独自アーカイブファイルからの読み込みにも対応できるように、このクラスにはファイルパスを直接指定するインターフェイスは用意しない。
		// もし std::vector<uint8_t> などの配列（メモリブロック）にストリーム インターフェイスを被せる場合、
		// ストリーム インターフェイスのターゲットとなる配列は、インターフェイスの使用が終了するまで保持しておかなければならないはず。
		// 他にメモリブロックを直接管理する COM インターフェイスは ID3DBlob くらいしかなさそう。
		// 代替物を作るとすれば std::shared_ptr<std::vector<uint8_t>> などが候補。
		// カプセル化的にはあまりよろしくないが、ストリームの元となるメモリブロックの寿命管理は、このクラスとは別々に行なう。
		// もし C++/CLI でマネージラッパーを作る場合は、System.Runtime.InteropServices.ComTypes.IStream を使ってもよいが、
		// マネージライブラリとしてはファイルパス文字列を渡すような高レベル API を公開するようにしたほうがよさげ。
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		// Create an IMFByteStream to wrap the IStream
		static MyComPtr<IMFByteStream> CreateByteStream(IStream* randomAccessStream)
		{
			MyComPtr<IMFByteStream> mfByteStream;
			// IStream はデスクトップ専用ではないが、MFCreateMFByteStreamOnStream() はデスクトップ専用。
			// 呼び出しが成功し、Media Foundation のストリームに変換した段階で
			// IStream の参照カウントが増えるらしく、呼び出し元で使用していた IStream は解放してもよいらしい。
			HRESULT hresult = ::MFCreateMFByteStreamOnStream(randomAccessStream, MyComHelpers::GetAddressOf(mfByteStream));
			THROW_IF_FAILED(hresult, L"MFCreateMFByteStreamOnStream failure");
			return mfByteStream;
		}
#else
		// Create an IMFByteStream to wrap the IRandomAccessStream
		static MyComPtr<IMFByteStream> CreateByteStream(Windows::Storage::Streams::IRandomAccessStream^ randomAccessStream)
		{
			MyComPtr<IMFByteStream> mfByteStream;
			// WinRT の Windows::Storage::Streams::IRandomAccessStream を受け取るバージョン。
			// Windows::Storage::Streams::FileRandomAccessStream や
			// Windows::Storage::Streams::InMemoryRandomAccessStream なども渡せる。
			// 呼び出しが成功し、Media Foundation のストリームに変換した段階で
			// IRandomAccessStream の参照カウントが増えるらしく、
			// 呼び出し元で保持しておく必要はないらしい。
			HRESULT hresult = MFCreateMFByteStreamOnStreamEx(reinterpret_cast<IUnknown*>(randomAccessStream), MyComHelpers::GetAddressOf(mfByteStream));
			THROW_IF_FAILED(hresult, L"MFCreateMFByteStreamOnStreamEx failure");
			return mfByteStream;
		}
#endif

		// Create an IMFSourceReader
		static MyComPtr<IMFSourceReader> CreateSourceReader(MyComPtr<IMFByteStream> mfByteStream)
		{
			// Create an attribute for low latency operation
			MyComPtr<IMFAttributes> mfAttributes;
			HRESULT hresult = ::MFCreateAttributes(MyComHelpers::GetAddressOf(mfAttributes), 1);
			THROW_IF_FAILED(hresult, L"MFCreateAttributes failure");

#if (WINVER >= _WIN32_WINNT_WIN8)
			hresult = mfAttributes->SetUINT32(MF_LOW_LATENCY, TRUE);
			THROW_IF_FAILED(hresult, L"IMFAttributes::SetUINT32 failure");
#endif

			// Create the IMFSourceReader
			MyComPtr<IMFSourceReader> mfSourceReader;
			hresult = ::MFCreateSourceReaderFromByteStream(MyComHelpers::Get(mfByteStream), MyComHelpers::Get(mfAttributes), MyComHelpers::GetAddressOf(mfSourceReader));
			THROW_IF_FAILED(hresult, L"MFCreateSourceReaderFromByteStream failure");

			// Create an IMFMediaType for setting the desired format
			MyComPtr<IMFMediaType> mfMediaType;
			hresult = ::MFCreateMediaType(MyComHelpers::GetAddressOf(mfMediaType));
			THROW_IF_FAILED(hresult, L"MFCreateMediaType failure");

			hresult = mfMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
			THROW_IF_FAILED(hresult, L"IMFMediaType::SetGUID failure");

#if 0
			// MP3 では OK だが、WAV だと IMFSourceReader::SetCurrentMediaType() が 0xC00D5212 すなわち MF_E_TOPO_CODEC_NOT_FOUND で失敗する模様。
			hresult = mfMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
#else
			hresult = mfMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
#endif
			THROW_IF_FAILED(hresult, L"IMFMediaType::SetGUID failure");

			// Set the media type in the source reader
			hresult = mfSourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM,
				nullptr, MyComHelpers::Get(mfMediaType));
			THROW_IF_FAILED(hresult, L"IMFSourceReader::SetCurrentMediaType failure");

			return mfSourceReader;
		}

#pragma region // Create/Init an IXAudio2SourceVoice wrapper

		std::unique_ptr<MyAudioPlayer> CreateAudioPlayerUniquePtr(MyComPtr<IMFSourceReader> pMFSourceReader) const
		{
			auto pOut = std::make_unique<MyAudioPlayer>();
			pOut->Init(m_pXAudio, pMFSourceReader);
			return pOut;
		}

		std::shared_ptr<MyAudioPlayer> CreateAudioPlayerSharedPtr(MyComPtr<IMFSourceReader> pMFSourceReader) const
		{
			auto pOut = std::make_shared<MyAudioPlayer>();
			pOut->Init(m_pXAudio, pMFSourceReader);
			return pOut;
		}

		void InitAudioPlayer(MyAudioPlayer* pAudioPlayer, MyComPtr<IMFSourceReader> pMFSourceReader) const
		{
			_ASSERTE(pAudioPlayer != nullptr);
			_ASSERTE(pMFSourceReader != nullptr);
			pAudioPlayer->Init(m_pXAudio, pMFSourceReader);
		}
#pragma endregion
	};
}
