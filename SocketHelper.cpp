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


bool SocketHelper::sendDatas(const std::vector<BYTE>& audioData) {
	size_t totalSize = audioData.size();
	std::cout << "totalSize(�������ݰ���������С): " << totalSize << std::endl;
	size_t numPackets = totalSize / PACKET_SIZE + (totalSize % PACKET_SIZE != 0 ? 1 : 0);
	std::cout << "numPackets(�������ݰ���������ÿ�����ݰ���СΪ4096): " << numPackets << std::endl;
	int all = 0;
	for (size_t i = 0; i < numPackets; ++i) {
		Sleep(50);
		size_t offset = i * PACKET_SIZE;
		size_t bytesToSend = std::min<size_t>(PACKET_SIZE, totalSize - offset);
		const char* info = reinterpret_cast<const char*>(audioData.data() + offset);

		int bytesSent = sendto(this->socketT, info, bytesToSend, 0, (sockaddr*)&(this->addr), sizeof((this->addr)));

		if (bytesSent == SOCKET_ERROR) {
			std::cerr << "sendto failed with error: " << WSAGetLastError() << std::endl;
			closeSocket();
			return false;
		}
		std::cout << "Sent packet " << i + 1 << "/" << numPackets << " (" << bytesSent << " bytes)" << std::endl;
		all += bytesSent;
	}
	std::cout << "�ܴ������ݴ�С�� " << " (" << all << " bytes)" << std::endl;
	return true;
}
bool SocketHelper::sendData(const BYTE* audioData) {
	return true;
}

bool SocketHelper::closeSocket() const {
	closesocket(this->socketT);
	WSACleanup();
	return true;
}