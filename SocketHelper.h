#pragma once
#ifndef SOCKETHELPER_H
#define SOCKETHELPER_H

#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")


const int PACKET_SIZE = 4096; // 每个数据包的大小
class SocketHelper
{
	public:
	bool initSocket(int major, int minor, const char* ip, u_short host);
	bool sendDatas(const std::vector<BYTE>& audioData);
	bool sendData(const BYTE* audioData);
	bool closeSocket() const;

	private:
	sockaddr_in addr;
	SOCKET socketT;
};


#endif // !SOCKETHELPER_H
