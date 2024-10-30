#pragma once
#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <vector>
#include <iostream>
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")


class TcpServer {
    public:
    TcpServer(int port);
    ~TcpServer();
    bool start();
    void stop();
    void receiveData();
    void sendData(const std::vector<BYTE>& audioData);

    private:
    SOCKET listenSock;
    SOCKET clientSock;
    sockaddr_in serverAddr;
    int port;
};



#endif // !TCPSERVER_H
