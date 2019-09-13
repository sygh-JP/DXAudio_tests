# DXAudio_tests
Tests of DirectX Audio by sygh.

## Development Environment (開発環境)
* Microsoft Visual Studio 2013 Update 5
* Microsoft Visual Studio 2015 Update 3
* MFC 14.0
* .NET Framework 4.5.2 (WPF)
* DirectXTK 2016-04-26
* [SharpDX](http://sharpdx.org/) 4.2.0 (the final version)

## Target Environment (ターゲット環境)
* Windows Vista/Windows 7/Windows 8.1/Windows 10 (Desktop, WPF)
* Windows 8.1/Windows 10 (Desktop, MFC)
* Windows 8.1/Windows 10 (WinRT for Win8.1, Store)
* Windows 10 (WinRT for Win10, UWP)
* Audio device compatible with XAudio2
* Audio driver compatible with XAudio2
* Latest DirectX End-User Runtime (for XAudio2 v2.7 in SharpDX)

## How to Build (ビルド方法)
1. Build DirectXTK (GitHub/CodePlex) and append the global include and library directory paths to it
1. Download *.mp3 and *.wav files from "https://maoudamashii.jokersounds.com/" and copy to the directory "free_audio_assets"
1. Build each solution file

2019-09-14, sygh.
