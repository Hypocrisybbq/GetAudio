#include "WaveFile.h"


void WaveFile::printWaveFormatex(WAVEFORMATEX* pwfx) {
	UINT32 sampleRate = pwfx->nSamplesPerSec;
	std::cout << "sampleRate(采样率): " << sampleRate << std::endl;

	UINT32 numChannels = pwfx->nChannels;
	std::cout << "numChannels(通道数): " << numChannels << std::endl;

	UINT32 bitsPerSample = pwfx->wBitsPerSample;
	std::cout << "bitsPerSample(位深): " << bitsPerSample << std::endl;

	UINT32 bytesPerFrame = (numChannels * bitsPerSample) / 8;
	std::cout << "bytesPerFrame(一帧的数据大小): " << bytesPerFrame << std::endl;
}


bool WaveFile::SaveAsWave(BYTE* buffer, size_t bufferSize, WAVEFORMATEX* waveFormate) {

	if (!createHandle(buffer, bufferSize, waveFormate)) {
		std::wcerr << "Unable to create File_Handle !" << std::endl;
		return false;
	}
	return true;
}


bool WaveFile::createHandle(BYTE* buffer, size_t bufferSize, WAVEFORMATEX* waveFormate) {

	HRESULT hr = NOERROR;

	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	wchar_t waveFileName[_MAX_PATH];
	PWSTR desktopPath = NULL;
	HRESULT result = SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &desktopPath);
	swprintf_s(waveFileName, _MAX_PATH, L"%s\\WAV_%04d%02d%02d_%02d-%02d-%02d.wav", desktopPath, sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);

	HANDLE waveHandle = CreateFile(waveFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);
	if (waveHandle == INVALID_HANDLE_VALUE) {
		std::wcerr << "Unable to open output WAV file " << waveFileName << " : " << GetLastError() << std::endl;
		return false;
	}
	if (!writeToWaveFile(waveHandle, buffer, bufferSize, waveFormate)) {
		std::wcerr << "Unable to write wave file " << std::endl;
		CloseHandle(waveHandle);
		return false;
	}
	CloseHandle(waveHandle);
	std::wcerr << "Successfully wrote WAVE data to " << waveFileName << std::endl;

	return true;

}
bool WaveFile::writeToWaveFile(HANDLE FileHandle, const BYTE* Buffer, const size_t BufferSize, const WAVEFORMATEX* WaveFormat) {

	DWORD waveFileSize = sizeof(WAVEHEADER) + sizeof(WAVEFORMATEX) + WaveFormat->cbSize + sizeof(WaveData) + sizeof(DWORD) + static_cast<DWORD>(BufferSize);


	BYTE* waveFileData = new (std::nothrow) BYTE[waveFileSize];


	if (waveFileData == NULL)
	{
		printf("Unable to allocate %d bytes to hold output wave data\n", waveFileSize);
		return false;
	}

	BYTE* waveFilePointer = waveFileData;
	WAVEHEADER* waveHeader = reinterpret_cast<WAVEHEADER*>(waveFileData);

	if (waveFileSize < sizeof(WAVEHEADER)) {
		return false;
	}
	memcpy(waveFilePointer, WaveFileHeader, sizeof(WAVEHEADER));
	waveFilePointer += sizeof(WAVEHEADER);

	waveHeader->dwSize = waveFileSize - (2 * sizeof(DWORD));
	waveHeader->dwFmtSize = sizeof(WAVEFORMATEX) + WaveFormat->cbSize;

	memcpy(waveFilePointer, WaveFormat, sizeof(WAVEFORMATEX) + WaveFormat->cbSize);
	waveFilePointer += sizeof(WAVEFORMATEX) + WaveFormat->cbSize;


	memcpy(waveFilePointer, WaveData, sizeof(WaveData));
	waveFilePointer += sizeof(WaveData);

	DWORD bufferSize = static_cast<DWORD>(BufferSize);
	memcpy(waveFilePointer, &bufferSize, sizeof(DWORD));
	waveFilePointer += sizeof(DWORD);


	memcpy(waveFilePointer, Buffer, BufferSize);



	DWORD bytesWritten;
	if (!WriteFile(FileHandle, waveFileData, waveFileSize, &bytesWritten, NULL))
	{
		printf("Unable to write wave file: %d\n", GetLastError());
		delete[] waveFileData;
		return false;
	}

	if (bytesWritten != waveFileSize)
	{
		printf("Failed to write entire wave file\n");
		delete[] waveFileData;
		return false;
	}
	delete[] waveFileData;
	return true;
}