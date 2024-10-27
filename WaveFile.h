#pragma once
#ifndef WAVEFILE_H
#define WAVEFILE_H

#include <Windows.h>
#include <iostream>
#include <Audioclient.h>

struct WAVEHEADER
{
	DWORD   dwRiff;
	DWORD   dwSize;
	DWORD   dwWave;
	DWORD   dwFmt;
	DWORD   dwFmtSize;
};

const BYTE WaveFileHeader[] =
{
	'R',   'I',   'F',   'F',  0x00,  0x00,  0x00,  0x00, 'W',   'A',   'V',   'E',   'f',   'm',   't',   ' ', 0x00, 0x00, 0x00, 0x00
};

const BYTE WaveData[] = { 'd', 'a', 't', 'a' };



class WaveFile
{
	public :
	bool SaveAsWave(BYTE* buffer, size_t bufferSize, WAVEFORMATEX* waveFormate);

	private:

	bool createHandle(BYTE* buffer, size_t bufferSize, WAVEFORMATEX* waveFormate);
	bool writeToWaveFile(HANDLE FileHandle, const BYTE* Buffer, const size_t BufferSize, const WAVEFORMATEX* WaveFormat);

};


#endif // !WAVEFILE_H

