using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MyWpfSharpDXAudioTest1
{
	class MyAudioSet1
	{
		public const int SoundEffectsSlotCount = 4;
		public MyAudioHelpers.MyAudioManager _audioManager;
		public MyAudioHelpers.MyAudioPlayer _audioPlayerBGM;
		public MyAudioHelpers.MyAudioPlayer[] _audioPlayerSEs = new MyAudioHelpers.MyAudioPlayer[SoundEffectsSlotCount];

		public MyAudioSet1()
		{
			this._audioManager = new MyAudioHelpers.MyAudioManager();
		}

		public void Destroy()
		{
			for (int i = 0; i < _audioPlayerSEs.Count(); ++i)
			{
				MyAudioHelpers.MyAudioPlayer.SafeDestroy(ref this._audioPlayerSEs[i]);
			}
			MyAudioHelpers.MyAudioPlayer.SafeDestroy(ref this._audioPlayerBGM);
			MyAudioHelpers.MyAudioManager.SafeDestroy(ref this._audioManager);
		}

		public static void SafeDestroy(ref MyAudioSet1 obj)
		{
			if (obj != null)
			{
				obj.Destroy();
				obj = null;
			}
		}

		public void PlaySE(int index)
		{
			System.Diagnostics.Debug.Assert(0 <= index && index < SoundEffectsSlotCount);

			if (this._audioPlayerSEs[index] != null)
			{
				this._audioPlayerSEs[index].Stop();
				this._audioPlayerSEs[index].Play();
			}
		}

		public void UpdateSE(int index, bool loop = false)
		{
			System.Diagnostics.Debug.Assert(0 <= index && index < SoundEffectsSlotCount);

			if (this._audioPlayerSEs[index] != null)
			{
				this._audioPlayerSEs[index].SetIsLoopMode(loop);
				this._audioPlayerSEs[index].Update();
			}
		}

		public void UpdateBGM(bool loop = false)
		{
			if (this._audioPlayerBGM != null)
			{
				this._audioPlayerBGM.SetIsLoopMode(loop);
				this._audioPlayerBGM.Update();
			}
		}

		public MyAudioHelpers.MyAudioPlayer CreateAudioPlayer(byte[] buffer)
		{
			System.Diagnostics.Debug.Assert(this._audioManager != null);
			return this._audioManager.CreateAudioPlayer(MyAudioHelpers.MyAudioManager.CreateSourceReader(buffer));
		}
	}
}
