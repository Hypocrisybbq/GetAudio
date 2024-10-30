#include "TcpServer.h"

TcpServer::TcpServer(int port) : port(port), listenSock(INVALID_SOCKET), clientSock(INVALID_SOCKET) {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		throw std::runtime_error("WSAStartup failed");
	}
}
TcpServer::~TcpServer() {
	stop();
	WSACleanup();
}


bool TcpServer::start() {
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET) {
		std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
		return false;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listenSock);
		return false;
	}

	if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listenSock);
		return false;
	}

	std::cout << "Waiting for connection..." << std::endl;

	clientSock = accept(listenSock, NULL, NULL);
	if (clientSock == INVALID_SOCKET) {
		std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listenSock);
		return false;
	}

	std::cout << "Connection accepted!" << std::endl;
	return true;
}

void TcpServer::stop() {
	if (clientSock != INVALID_SOCKET) {
		closesocket(clientSock);
		clientSock = INVALID_SOCKET;
	}
	if (listenSock != INVALID_SOCKET) {
		closesocket(listenSock);
		listenSock = INVALID_SOCKET;
	}
}

void TcpServer::receiveData() {
	char buffer[512];
	int bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);
	if (bytesReceived == SOCKET_ERROR) {
		std::cerr << "recv failed with error: " << WSAGetLastError() << std::endl;
	}
	else {
		buffer[bytesReceived] = '\0';
		std::cout << "Received message: " << buffer << std::endl;
	}
}


void TcpServer::sendData(const std::vector<BYTE>& audioData) {
	size_t totalSize = audioData.size();
	std::cout << "totalSize(发送数据包的完整大小): " << totalSize << std::endl;
	const char* data = reinterpret_cast<const char*>(audioData.data());

	size_t bytesSentTotal = 0;
	while (bytesSentTotal < totalSize) {
		int bytesSent = send(clientSock, data + bytesSentTotal, totalSize - bytesSentTotal, 0);
		if (bytesSent == SOCKET_ERROR) {
			std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
			return;
		}
		bytesSentTotal += bytesSent;
	}
	std::cout << "Sent total bytes: " << bytesSentTotal << std::endl;
	stop();
	
}
