#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <iostream>

#include <Windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <functiondiscoverykeys_devpkey.h>
#include <algorithm>

#include "WaveFile.h"

void printpwfx(WAVEFORMATEX* pwfx) {
	UINT32 sampleRate = pwfx->nSamplesPerSec;
	std::cout << "sampleRate(采样率): " << sampleRate << std::endl;

	UINT32 numChannels = pwfx->nChannels;
	std::cout << "numChannels(通道数): " << numChannels << std::endl;

	UINT32 bitsPerSample = pwfx->wBitsPerSample;
	std::cout << "bitsPerSample(位深): " << bitsPerSample << std::endl;

	UINT32 bytesPerFrame = (numChannels * bitsPerSample) / 8;
	std::cout << "bytesPerFrame(一帧的数据大小): " << bytesPerFrame << std::endl;
}

const int PACKET_SIZE = 1024; // 每个数据包的大小

void sendAudioData(const std::vector<BYTE>& audioData, SOCKET& sock, sockaddr_in& destAddr) {
	size_t totalSize = audioData.size();
	size_t numPackets = totalSize / PACKET_SIZE + (totalSize % PACKET_SIZE != 0 ? 1 : 0);

	for (size_t i = 0; i < numPackets; ++i) {
		size_t offset = i * PACKET_SIZE;
		size_t bytesToSend = std::min<size_t>(PACKET_SIZE, totalSize - offset);
		const char* info = reinterpret_cast<const char*>(audioData.data() + offset);
		
		int bytesSent = sendto(sock, info, bytesToSend, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
		if (bytesSent == SOCKET_ERROR) {
			std::cerr << "sendto failed with error: " << WSAGetLastError() << std::endl;
			return;
		}
		std::cout << "Sent packet " << i + 1 << "/" << numPackets << " (" << bytesSent << " bytes)" << std::endl;
	}
}



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
	UINT32 sumFramesAvailable = 0;
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
	printpwfx(pwfx);

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

		waveFile.SaveAsWave(audioData.data(), audioData.size(), pwfx);


		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);


		//AF_INET :Address Family : Internet (网络地址族,一般用作IPv4)
		//SOCK_DGRAM :socket datagram(套接字 数据报)
		//IPPROTO_UDP: ip protocol udp(使用 udp 协议)

		SOCKET desktopSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);


		// 目标地址和端口
		sockaddr_in desktopAddr;
		desktopAddr.sin_family = AF_INET;
		desktopAddr.sin_port = htons(8848); // 目标端口
		inet_pton(AF_INET, "192.168.0.103", &desktopAddr.sin_addr.s_addr); // 目标IP地址

		const char* sendData = u8"这是来自客户端的消息";
		sendto(desktopSocket, sendData, strlen(sendData), 0, (sockaddr*)&desktopAddr, sizeof(desktopAddr));

		sendAudioData(audioData, desktopSocket, desktopAddr);
		// 
		//sendto(desktopSocket,)
		//int sendResult = sendto(desktopSocket, reinterpret_cast<const char*>(audioData.data()), audioData.size(), 0, (sockaddr*)&desktopAddr, sizeof(desktopAddr));

		//if (sendResult == SOCKET_ERROR) {
		//	std::cerr << "sendto failed with error: " << WSAGetLastError() << std::endl;
		//}
		//else {
		//	std::cout << "Sent  success" << std::endl;
		//}
		closesocket(desktopSocket);
		WSACleanup();
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
