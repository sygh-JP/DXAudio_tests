#pragma once


class MyAudioSet1
{
public:
	static const int SoundEffectsSlotCount = 4;
	std::unique_ptr<DirectX::AudioEngine> m_audioEngineSE;
	std::unique_ptr<DirectX::SoundEffect> m_soundEffects[SoundEffectsSlotCount];
	std::unique_ptr<DirectX::SoundEffectInstance> m_soundEffectInstances[SoundEffectsSlotCount];
	// WaveBank を使う場合は、0から始まるインデックス番号か、名前の文字列を指定する。
	//std::unique_ptr<DirectX::WaveBank> m_waveBank;
	std::unique_ptr<MyAudioHelpers::MyAudioManager> m_audioManager;
	std::unique_ptr<MyAudioHelpers::MyAudioPlayer> m_audioPlayerBGM;
public:
	MyAudioSet1() {}
	virtual ~MyAudioSet1()
	{
		for (int index = 0; index < SoundEffectsSlotCount; ++index)
		{
			if (m_soundEffectInstances[index])
			{
				m_soundEffectInstances[index]->Stop(true);
			}
		}
	}
	void CreateAudioEngineSE()
	{
		DirectX::AUDIO_ENGINE_FLAGS eflags = DirectX::AudioEngine_Default;
#ifdef _DEBUG
		eflags = eflags | DirectX::AudioEngine_Debug;
#endif
		m_audioEngineSE = std::make_unique<DirectX::AudioEngine>(eflags);

		m_audioEngineSE->Update();
	}
	void PlaySE(int index)
	{
		_ASSERTE(0 <= index && index < SoundEffectsSlotCount);
#if 0
		if (m_soundEffects[index])
		{
			m_soundEffects[index]->Stop();
			m_soundEffects[index]->Play();
		}
#else
		if (m_soundEffectInstances[index])
		{
			m_soundEffectInstances[index]->Stop();
			// ループはしない。
			// ちなみに DirectXTK Audio は IXAudio2VoiceCallback::OnBufferEnd() を利用しているらしい。
			// つまりイベント モードを使っているらしい。
			// ロックフリーでよろしくやっているとのことだが、注意深く使わないとハマりそう。
			// https://directxtk.codeplex.com/wikipage?title=Audio&referringTitle=Home
			m_soundEffectInstances[index]->Play(false);
		}
#endif
	}
	void UpdateSE()
	{
		if (m_audioEngineSE)
		{
			m_audioEngineSE->Update();
		}
	}
	void UpdateBGM(bool loop)
	{
		if (m_audioPlayerBGM)
		{
			m_audioPlayerBGM->SetIsLoopMode(loop);
			m_audioPlayerBGM->Update();
		}
	}
};
