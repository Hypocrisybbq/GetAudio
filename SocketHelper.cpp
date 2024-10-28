#include "SocketHelper.h"


bool SocketHelper::initSocket(int major, int minor, const char* ip, u_short host) {
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(major, minor), &wsaData);
	if (result != 0) {
		std::cerr << "�޷���ʼ��WSA:" << WSAGetLastError() << std::endl;
		return false;
	}


	//AF_INET :Address Family : Internet (�����ַ��,һ������IPv4)
	//SOCK_DGRAM :socket datagram(�׽��� ���ݱ�)
	//IPPROTO_UDP: ip protocol udp(ʹ�� udp Э��)
	sockaddr_in desktopAddr;
	desktopAddr.sin_family = AF_INET;
	desktopAddr.sin_port = htons(host);
	inet_pton(AF_INET, ip, &desktopAddr.sin_addr.s_addr);


	this->socketT = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	this->addr = desktopAddr;

	return true;
}


bool SocketHelper::sendData(const std::vector<BYTE>& audioData) {
	size_t totalSize = audioData.size();
	std::cout << "totalSize(�������ݰ���������С): " << totalSize << std::endl;
	size_t numPackets = totalSize / PACKET_SIZE + (totalSize % PACKET_SIZE != 0 ? 1 : 0);
	std::cout << "numPackets(�������ݰ���������ÿ�����ݰ���СΪ1024): " << numPackets << std::endl;
	for (size_t i = 0; i < numPackets; ++i) {
		if (i % 6 == 1) {
			Sleep(50);
		}
		size_t offset = i * PACKET_SIZE;
		size_t bytesToSend = std::min<size_t>(PACKET_SIZE, totalSize - offset);
		const char* info = reinterpret_cast<const char*>(audioData.data() + offset);

		//const char* sendData = u8"�������Կͻ��˵���Ϣ";


		std::stringstream ss;
		ss << i;
		std::string numStr = ss.str();

		std::string aaa = numStr + ":" + info;
		info = aaa.c_str();
		int bytesSent = sendto(this->socketT, info, strlen(info), 0, (sockaddr*)&(this->addr), sizeof((this->addr)));

		//int bytesSent = sendto(sock, info, bytesToSend, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
		if (bytesSent == SOCKET_ERROR) {
			std::cerr << "sendto failed with error: " << WSAGetLastError() << std::endl;
			closeSocket();
			return false;
		}
		std::cout << "Sent packet " << i + 1 << "/" << numPackets << " (" << bytesSent << " bytes)" << std::endl;
	}
	return true;
}

bool SocketHelper::closeSocket() const {
	closesocket(this->socketT);
	WSACleanup();
	return true;
}