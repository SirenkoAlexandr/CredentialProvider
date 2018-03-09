#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include"Pipe.h"
#include <thread>
#include"plog\Log.h"
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

DWORD WINAPI InstanceServer(void* socet);

struct SocketTransport
{
	SOCKET socket;
};

DWORD WINAPI server(CONST LPVOID lpParam)
{
	LOG_DEBUG << "start server";
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	/*int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;*/

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		LOG_DEBUG << "WSAStartup failed";
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		LOG_DEBUG << "getaddrinfo failed";
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		LOG_DEBUG << "socket failed";
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		LOG_DEBUG << "bind failed";
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	LOG_DEBUG << "listing";
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		LOG_DEBUG << "listen failed";
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}


	for (;;)
	{
		
		LOG_DEBUG << "accepting";
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		LOG_DEBUG << "new ClientSocket =" << ClientSocket;
		SocketTransport *trans=new SocketTransport;
		trans->socket = ClientSocket;
 		if (ClientSocket == INVALID_SOCKET)
		{
			LOG_DEBUG << "accept failed";
			closesocket(ListenSocket);
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
		LOG_DEBUG << "try create InstantServer";
		HANDLE hThreadPipe = CreateThread(NULL, 0, InstanceServer, trans, 0, NULL);

	}
	// No longer need server socket
	//closesocket(ListenSocket);

	// Receive until the peer shuts down the connection
	

	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		LOG_DEBUG << "shutdown failed";
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}

	DWORD WINAPI InstanceServer(void* param)
	{
		SocketTransport *trans=(SocketTransport*)param;
		int iSendResult;
		char recvbuf[DEFAULT_BUFLEN];
		int recvbuflen = DEFAULT_BUFLEN;
		int iResult;
		
		SOCKET ClientSocket = trans->socket;
		LOG_DEBUG << "start new instantServer with SOCKET ="<<ClientSocket;

		do {

			iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0) 
			{
				LOG_DEBUG << "Bytes received " << recvbuflen;
				LOG_DEBUG << "Message: " << recvbuf;



				if (!PipeMain(recvbuf))
				{
					LOG_DEBUG << "Connection Error";
					strcpy_s(recvbuf, strlen("Connection Error")+1,"Connection Error");
				}
				else
				{
					LOG_DEBUG << "Authorisation is successfully";
					strcpy_s(recvbuf, strlen("Authorisation is successfully") + 1, "Authorisation is successfully");
				}
				

				LOG_DEBUG << "Send answer :" << recvbuf;
				// Echo the buffer back to the sender
				iSendResult = send(ClientSocket, recvbuf, strlen(recvbuf), 0);
				if (iSendResult == SOCKET_ERROR) 
				{
					LOG_DEBUG << "send failed";
					closesocket(ClientSocket);
					WSACleanup();
					return 1;
				}
				LOG_DEBUG << "Bytes sent" << strlen(recvbuf);
			}
			else if (iResult == 0)
			{
				closesocket(ClientSocket);
				LOG_DEBUG << "Connection closing...";
			}
			else {
				LOG_DEBUG << "recv failed";
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}

		} while (iResult > 0);
		return 0;
	}

