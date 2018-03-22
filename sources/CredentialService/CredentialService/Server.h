#define WIN32_LEAN_AND_MEAN

// uncludes for get active session
#include <windows.h>
#include<WtsApi32.h>
#pragma comment(lib,"WtsApi32.lib")// 

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include"Pipe.h"
#include <thread>
#include"plog\Log.h"

#pragma comment (lib, "Ws2_32.lib")


//includes for crypt/decrypt
#include"openssl\aes.h"


#include"DefenitionConstants.h"

//prototypes
DWORD WINAPI InstanceServer(void* socet);
LPTSTR CheckActiveUser();

//structs
struct SocketTransport
{
	SOCKET socket;
};
struct Messages
{
	int ID;
	int LenData;
	int LenOriginal;
	char* data;
	void InputData(char*text)
	{
		data = new char[LenData + 1];
		//strncpy_s(result->data, text + 7, );
		strcpy_s(data, LenData + 1, text);
		data[LenData] = '\0';
		if (LenOriginal > LenData)
			ID = 0;
	}
	void InputMessageToStruct(char* text)
	{
		switch (text[0])
		{
		case '1': //request
		{
			ID = REQUEST;
			LenOriginal = 0;
			LenData = 0;
			data = NULL;
			break;
		}
		case '4': //creds
		{
			if (strlen(text) < 7)
			{
				ID = ERROR;
				return;
			}
			ID = CREDS;
			char lens_temp[4];
			strncpy_s(lens_temp, text + 1, 3);
			lens_temp[3] = '\0';
			LenData = atoi(lens_temp);
			strncpy_s(lens_temp, text + 4, 3);
			lens_temp[3] = '\0';
			LenOriginal = atoi(lens_temp);
			break;
		}
		default: // something wrong
		{
			ID = ERROR; // error
			break;
		}
		}
		return;
	}
	void ProcessMessage()
	{
		switch (ID)
		{
		case REQUEST://request
		{
			// check active session
			LPTSTR ActiveSessionUserName = CheckActiveUser();
			if (ActiveSessionUserName)
			{
				ID = BUSY;
				LenData = (int)strlen(ActiveSessionUserName);
				LenOriginal = LenData;
				data = new char[LenData + 1];
				strcpy_s(data, LenData + 1, ActiveSessionUserName);
			}
			else
			{
				ID = FREE;
			}
			break;
		}
		case 4://creds
		{
			// check active session
			LPTSTR ActiveSessionUserName = CheckActiveUser();
			if (ActiveSessionUserName)
			{
				ID = BUSY;
				LenData = (int)strlen(ActiveSessionUserName);
				LenOriginal = LenData;
				data = new char[LenData + 1];
				strcpy_s(data, LenData + 1, ActiveSessionUserName);
			}
			else
			{
				//decrypt
				Decrypt();
				LOG_DEBUG << "Send to pipeServer :" <<data;
				switch (PipeMain(data))
				{
					case GOOD_CREDS:// ok
					{
						LOG_DEBUG << "Authorisation is successfully";
						ID = SUCCESS;
						break;
					}
					case WRONG_CREDS: // wrong creds
					{
						LOG_DEBUG << "Incorrect username or password";
						ID = FAIL;
					}
					case PIPE_ERROR:// fail pipe
					{
						LOG_DEBUG << "Connection error.Pipe error.";
						ID = ERROR;
					}
				}
			}
			break;
		}
		default:
		{
			ID = 0; // error
			break;
		}
		}
	}
	char* FillAnswer()
	{
		char *answer = nullptr;
		switch (ID)
		{
		case BUSY://busy
		{
			answer = new char[LenData + 8];
			answer[0] = '2';
			answer[1] = '\0';
			char lens_temp[3];

			_itoa_s(LenData, lens_temp, 10);
			if (LenData < 10)
			{
				strcat_s(answer, LenData + 8, "00");
				answer[3] = '\0';
				strcat_s(answer, LenData + 8, lens_temp);
			}
			else
			{
				if (LenData < 100)
				{
					strcat_s(answer + 1, LenData + 8, "0");
					answer[2] = '\0';
					strcat_s(answer, LenData + 8, lens_temp);
				}
			}
			answer[4] = '\0';
			_itoa_s(LenOriginal, lens_temp, 10);
			if (LenData < 10)
			{
				strcat_s(answer, LenData + 8, "00");
				answer[6] = '\0';
				strcat_s(answer, LenData + 8, lens_temp);
			}
			else
			{
				if (LenData < 100)
				{
					strcat_s(answer + 1, LenData + 8, "0");
					answer[5] = '\0';
					strcat_s(answer, LenData + 8, lens_temp);
				}
			}
			answer[7] = '\0';
			strcat_s(answer, LenData + 8, data);
			answer[LenData + 8] = '\0';
			break;
		}
		case FREE://free
		{
			answer = new char[2];
			answer[0] = '3';
			answer[1] = '\0';
			break;
		}
		case FAIL://fail
		{
			answer = new char[2];
			answer[0] = '5';
			answer[1] = '\0';
			break;
		}
		case SUCCESS://success
		{
			answer = new char[2];
			answer[0] = '6';
			answer[1] = '\0';
			break;
		}
		case ERROR://error
		{
			answer = new char[2];
			answer[0] = '0';
			answer[1] = '\0';
			break;
		}
		}
		return answer;
	}
	void Decrypt()
	{
		unsigned char key[17] = DECRYPTION_KEY;
		unsigned char *dec_out = new unsigned char[LenData];
		AES_KEY dec_key;
		AES_set_decrypt_key(key, 128, &dec_key);
		AES_decrypt((unsigned char*)data, dec_out, &dec_key);
		dec_out[LenOriginal] = '\0';
		delete[]data;
		data = new char[LenData + 1];
		strcpy_s(data, LenOriginal + 1, (char*)dec_out);
		data[LenOriginal] = '\0';
		delete[]dec_out;
	}
	~Messages()
	{
		if(data)
			delete [] data;
	}
};

LPTSTR CheckActiveUser()
{
	LOG_DEBUG << "start checkActiveUser";
	DWORD k = 1, count = 0;
	PWTS_SESSION_INFO_1 session_info_1;
	LPTSTR ActiveSessionUserName;
	WTS_SESSION_INFO_1 s;
	WTSEnumerateSessionsEx(WTS_CURRENT_SERVER_HANDLE, &k, 0, &session_info_1, &count);
	for (DWORD i = 0; i < count; ++i)
	{
		s = session_info_1[i];
		if (s.State == WTSActive)
		{
			ActiveSessionUserName = s.pUserName;
			LOG_DEBUG << "in checkActiveUser User :" << ActiveSessionUserName;
			return ActiveSessionUserName;
		}

	}
	WTSFreeMemory(session_info_1);
	return nullptr;
}

DWORD WINAPI server(CONST LPVOID lpParam)
{
	LOG_DEBUG << "start server";
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

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
		SocketTransport *trans = new SocketTransport;
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
	SocketTransport *trans = (SocketTransport*)param;
	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	int iResult;

	SOCKET ClientSocket = trans->socket;
	LOG_DEBUG << "start new instantServer with SOCKET =" << ClientSocket;

	Messages mes;
	do {
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			recvbuf[iResult] = '\0';
			LOG_DEBUG << "Bytes received " << recvbuflen;
			LOG_DEBUG << "Message: " << recvbuf;
			
			if (strlen(recvbuf) > 7)
			{
				mes.InputData(recvbuf);
				mes.ProcessMessage();
				strcpy_s(recvbuf, 512, mes.FillAnswer());
				iSendResult = send(ClientSocket, recvbuf, (int)strlen(recvbuf), 0);
				if (iSendResult == SOCKET_ERROR)
				{
					LOG_DEBUG << "send failed with error";
					closesocket(ClientSocket);
					WSACleanup();
					return 1;
				}
				LOG_DEBUG << "Send answer :" << recvbuf;
			}
			else
			{
				mes.InputMessageToStruct(recvbuf);
				if (mes.ID != 4)
				{
					mes.ProcessMessage();
					strcpy_s(recvbuf, 512, mes.FillAnswer());
					iSendResult = send(ClientSocket, recvbuf, (int)strlen(recvbuf), 0);
					if (iSendResult == SOCKET_ERROR)
					{
						LOG_DEBUG << "send failed with error";
						closesocket(ClientSocket);
						WSACleanup();
						return 1;
					}
					LOG_DEBUG << "Send answer :" << recvbuf;
				}
			}
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





