#include <Windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <functiondiscoverykeys_devpkey.h>

#include "WaveFile.h"



HRESULT CaptureAudio() {
	WaveFile waveFile;
	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = 10000000;  // 1秒的缓冲区
	UINT32 bufferFrameCount;
	BYTE* pData;
	DWORD flags;
	WAVEFORMATEX* pwfx = nullptr;
	UINT32 packetLength = 0;
	UINT32 numFramesAvailable;
	IMMDeviceEnumerator* pEnumerator = nullptr;
	IMMDevice* pDevice = nullptr;
	IAudioClient* pAudioClient = nullptr;
	IAudioCaptureClient* pCaptureClient = nullptr;
	DWORD totalCaptureTime = 10000;  // 捕获10秒的音频
	DWORD startTime = GetTickCount64();
	std::vector<BYTE> audioData;
	IPropertyStore* pPropertyStore;

	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr)) return hr;

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
	if (FAILED(hr)) goto Exit;

	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
	if (FAILED(hr)) goto Exit;

	hr = pDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
	if (SUCCEEDED(hr)) {
		PROPVARIANT varName;
		PropVariantInit(&varName);

		hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &varName);
		if (SUCCEEDED(hr)) {
			// 将设备名称转换为 std::wstring
			std::wstring deviceName(varName.pwszVal);

			// 将 std::wstring 转换为 std::string
			std::string deviceNameStr(deviceName.begin(), deviceName.end());

			for (char& ch : deviceNameStr) {
				if (static_cast<unsigned char>(ch) == 7) {
					ch = ' '; // 替换为空格
				}
			}
			// 打印设备名称
			std::cout << "Device Name: " << deviceNameStr << std::endl;

			PropVariantClear(&varName);
		}
		pPropertyStore->Release();
	}

	hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
	if (FAILED(hr)) goto Exit;

	hr = pAudioClient->GetMixFormat(&pwfx);
	if (FAILED(hr)) goto Exit;

	hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hnsRequestedDuration, 0, pwfx, nullptr);
	if (FAILED(hr)) goto Exit;

	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	if (FAILED(hr)) goto Exit;

	hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
	if (FAILED(hr)) goto Exit;

	hr = pAudioClient->Start();
	if (FAILED(hr)) goto Exit;


	while (GetTickCount64() - startTime < totalCaptureTime) {
		hr = pCaptureClient->GetNextPacketSize(&packetLength);
		if (FAILED(hr)) goto Exit;

		while (packetLength != 0) {
			hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
			if (FAILED(hr)) goto Exit;

			UINT32 numChannels = pwfx->nChannels;
			UINT32 bitsPerSample = pwfx->wBitsPerSample;
			UINT32 bytesPerFrame = (numChannels * bitsPerSample) / 8;
			UINT32 dataSize = numFramesAvailable * bytesPerFrame;

			if (dataSize > 0) {
				audioData.insert(audioData.end(), pData, pData + dataSize);
			}

			hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
			if (FAILED(hr)) goto Exit;

			hr = pCaptureClient->GetNextPacketSize(&packetLength);
			if (FAILED(hr)) goto Exit;
		}

		Sleep(50);  // 每50毫秒检查一次
	}

	hr = pAudioClient->Stop();
	if (FAILED(hr)) goto Exit;

	if (!audioData.empty()) {
		waveFile.SaveAsWave(audioData.data(), audioData.size(), pwfx);
	}
	else {
		std::cerr << "没有捕获到音频数据" << std::endl;
	}

Exit:
	CoTaskMemFree(pwfx);
	if (pEnumerator) pEnumerator->Release();
	if (pDevice) pDevice->Release();
	if (pAudioClient) pAudioClient->Release();
	if (pCaptureClient) pCaptureClient->Release();
	CoUninitialize();
	return hr;
}

int main() {
	HRESULT hr = CaptureAudio();
	if (FAILED(hr)) {
		std::cerr << "CaptureAudio failed with error: " << hr << std::endl;
	}
	return 0;
}
