
#include <WS2tcpip.h>

#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <functiondiscoverykeys_devpkey.h>


#include "WaveFile.h"
#include "SocketHelper.h"
#include "TcpServer.h"


HRESULT CaptureAudio() {
	WaveFile waveFile;
	TcpServer tcpServer(8848);
	//SocketHelper socketHelper;
	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = 10000000;  // 1秒的缓冲区
	UINT32 bufferFrameCount;
	BYTE* pData;
	DWORD flags;
	WAVEFORMATEX* pwfx = nullptr;
	UINT32 packetLength = 0;
	UINT32 numFramesAvailable;
	UINT32 sumFramesAvailable = 0;
	IMMDeviceEnumerator* pEnumerator = nullptr;
	IMMDevice* pDevice = nullptr;
	IAudioClient* pAudioClient = nullptr;
	IAudioCaptureClient* pCaptureClient = nullptr;
	DWORD totalCaptureTime = 6000;  // 捕获4秒的音频
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

			std::wstring deviceName(varName.pwszVal);

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

	waveFile.printWaveFormatex(pwfx);

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
			//std::cout << "numFramesAvailable(50毫秒/单次采集的帧率): " << numFramesAvailable << std::endl;
			sumFramesAvailable += numFramesAvailable;
			UINT32 sampleRate = pwfx->nSamplesPerSec;
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
		std::cout << "sumFramesAvailable 总有效帧率: " << sumFramesAvailable << std::endl;
		std::cout << "计算出的有效大小: " << sumFramesAvailable * 2 * 4 << std::endl;
		sumFramesAvailable = 0;
		std::cout << "audioData.size() 总音频的数据大小: " << audioData.size() << std::endl;

		if (tcpServer.start()) {
			//tcpServer.receiveData();
			tcpServer.sendData(audioData);
		}

		//waveFile.SaveAsWave(audioData.data(), audioData.size(), pwfx);


		////开始传输数据
		//bool helpResult = socketHelper.initSocket(2, 2, "192.168.0.100", 8848);// "192.168.137.226"

		//if (!helpResult) {
		//	goto Exit;
		//}

		////const char* sendData = u8"这是来自客户端的消息";
		//socketHelper.sendDatas(audioData);
		//socketHelper.closeSocket();

		return hr;
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
