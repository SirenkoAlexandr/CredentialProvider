//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//
/////////////
//for server
//#undef UNICODE

#define WIN32_LEAN_AND_MEAN

//#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

///////////////

#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <comdef.h>
#include <Wbemidl.h>
# pragma comment(lib, "wbemuuid.lib")
# pragma comment(lib, "credui.lib")
# pragma comment(lib, "comsuppw.lib")
#include <wincred.h>
#include <strsafe.h>

#include "CommandWindow.h"
#include <strsafe.h>

#include "consts.h"

// Custom messages for managing the behavior of the window thread.
#define WM_EXIT_THREAD              WM_USER + 1
#define WM_TOGGLE_CONNECTED_STATUS  WM_USER + 2

    static IEnumWbemClassObject* pEnumerator ;
    static IWbemLocator *pLoc ;
    static IWbemServices *pSvc;
    static IWbemClassObject *pclsObj;

const WCHAR c_szClassName[] = L"cheaterWindow";

CCommandWindow::CCommandWindow() : _hWnd(NULL), _hInst(NULL), _fConnected(FALSE), _pProvider(NULL)
{
	pEnumerator = NULL;
	pLoc = NULL;
	pSvc = NULL;
}

CCommandWindow::~CCommandWindow()
{
    if (_hWnd != NULL)
    {
        PostMessage(_hWnd, WM_EXIT_THREAD, 0, 0);
        _hWnd = NULL;
    }

    if (_pProvider != NULL)
    {
        _pProvider->Release();
        _pProvider = NULL;
    }

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    pclsObj->Release();
    CoUninitialize();
}

HRESULT CCommandWindow::Initialize(__in CSampleProvider *pProvider)
{
    HRESULT hr = S_OK;

    if (_pProvider != NULL)
    {
        _pProvider->Release();
    }
    _pProvider = pProvider;
    _pProvider->AddRef();
	
    HANDLE hThread = CreateThread(NULL, 0, _ThreadProc, this, 0, NULL);
	HANDLE hThreadServer = CreateThread(NULL, 0, MakingThread, this, 0, NULL);
	//_pProvider->SetCredentials(L"Home", L"12345");
	//_fConnected = true;
	//_pProvider->OnConnectStatusChanged();
    if (hThread == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

DWORD WINAPI CCommandWindow::MakingThread(__in LPVOID lpParameter1)
{
	HWND h;
	CCommandWindow *pCommandWindow = static_cast<CCommandWindow *>(lpParameter1);
	wchar_t* username=new wchar_t[256];
	wchar_t* password =new wchar_t[256];
	
	pCommandWindow->Server(username, password);
	//MessageBox(NULL, (LPCWSTR)"Before sending credentials", (LPCWSTR)"info", MB_ICONWARNING | MB_OK);
	pCommandWindow->_pProvider->SetCredentials(username, password);
	//pCommandWindow->_ProcessNextMessage();
	pCommandWindow->_fConnected = true;
	pCommandWindow->_pProvider->OnConnectStatusChanged();
	return 0;
}


void CCommandWindow::Server(wchar_t* UserName, wchar_t* Password)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	char username[256]="";
	char password[256]="";
	wchar_t buf[256];

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		//MessageBox(NULL, (LPCWSTR)"WSAStartup failed with error", (LPCWSTR)"info", MB_ICONWARNING | MB_OK);
		return ;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		//MessageBox(NULL, (LPCWSTR)"getaddrinfo failed with error", (LPCWSTR)"info", MB_ICONWARNING | MB_OK);
		WSACleanup();
		return ;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		//MessageBox(NULL, (LPCWSTR)"socket failed with error", (LPCWSTR)"info", MB_ICONWARNING | MB_OK);
		freeaddrinfo(result);
		WSACleanup();
		return;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		//MessageBox(NULL, (LPCWSTR)"bind failed with error", (LPCWSTR)"info", MB_ICONWARNING | MB_OK);
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		//MessageBox(NULL, (LPCWSTR)"listen failed with error", (LPCWSTR)"info", MB_ICONWARNING | MB_OK);
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		//MessageBox(NULL, (LPCWSTR)"accept failed with error:", (LPCWSTR)"info", MB_ICONWARNING | MB_OK);
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	// No longer need server socket
	closesocket(ListenSocket);

	// Receive until the peer shuts down the connection
	do {
		//MessageBox(NULL, (LPCWSTR)"in do while", (LPCWSTR)"info", MB_ICONWARNING | MB_OK);
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);
			char tmp[256]="";
		//	MessageBox(NULL, (LPCWSTR)"work whith strings", (LPCWSTR)"info", MB_ICONWARNING | MB_OK);
		
			
			strcpy(username, recvbuf);
			strtok(username, "&");
			strcat(tmp, &username[strlen(username) + 1]);
			strcpy(password, tmp);
			strtok(password, "&");
			strcpy(recvbuf, "");

			size_t len1 = strlen(username);
			size_t len2 = strlen(password);
			

			wchar_t b[256];
			wcscpy(b, L"");
			wchar_t b1[256];
			wcscpy(b1, L"");

			//mbstowcs(buf, username, len);
			
			wcscpy(UserName, L""); //strcpy(UserName, "");
			
			//mbstowcs(UserName, username, len1);
			mbstowcs_s(&len1, b, username, strlen(username)); //strcpy(UserName, username);
			wcscpy(UserName, b);
			wcscpy(Password, L"");//	strcpy(Password, "");
			//mbstowcs(Password, password, len2);
			mbstowcs_s(&len2, b1, password, strlen(password));//strcpy(Password, password);
			wcscpy(Password, b1);
			
			strcat(recvbuf, "log in is successful");
			// Echo the buffer back to the sender

			iSendResult = send(ClientSocket, recvbuf, iResult, 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				//MessageBox(NULL, (LPCWSTR)"getaddrinfo failed with error", (LPCWSTR)"info", MB_ICONWARNING | MB_OK);
				closesocket(ClientSocket);
				WSACleanup();
				return;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return;
		}

	} while (iResult > 0);

	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		//MessageBox(NULL, (LPCWSTR)"getaddrinfo failed with error", (LPCWSTR)"info", MB_ICONWARNING | MB_OK);
		closesocket(ClientSocket);
		WSACleanup();
		return;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

}

// Wraps our internal connected status so callers can easily access it.
BOOL CCommandWindow::GetConnectedStatus()
{
    return _fConnected;
}

//
//  FUNCTION: _MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
HRESULT CCommandWindow::_MyRegisterClass()
{
    WNDCLASSEX wcex = { sizeof(wcex) };
    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc      = _WndProc;
    wcex.hInstance        = _hInst;
    wcex.hIcon            = NULL;
    wcex.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName    = c_szClassName;

    return RegisterClassEx(&wcex) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

//
//   FUNCTION: _InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HRESULT CCommandWindow::_InitInstance()
{
    HRESULT hr = S_OK;

    _hWnd = CreateWindowEx(
        WS_EX_TOPMOST, 
        c_szClassName, 
		L"", 
        WS_DLGFRAME,
        200, 200, 200, 80, 
        NULL,
        NULL, _hInst, NULL);
    if (_hWnd == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
    {
		if (!ShowWindow(_hWnd, SW_HIDE))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }

            if (SUCCEEDED(hr))
            {
                if (!UpdateWindow(_hWnd))
                {
                   hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
    }

    return hr;
}

BOOL CCommandWindow::_ProcessNextMessage()
{
    MSG msg;
    GetMessage(&(msg), _hWnd, 0, 0);
    TranslateMessage(&(msg));
    DispatchMessage(&(msg));

    switch (msg.message)
    {
    case WM_EXIT_THREAD: return FALSE;
    case WM_TOGGLE_CONNECTED_STATUS:
		//_pProvider->SetCredentials(L"Home",L"12345");
		_fConnected=true;
		_pProvider->OnConnectStatusChanged();
		break;
	}
    return TRUE;

}

LRESULT CALLBACK CCommandWindow::_WndProc(__in HWND hWnd, __in UINT message, __in WPARAM wParam, __in LPARAM lParam)
{
    switch (message)
    {
 //   case WM_DEVICECHANGE:
	//	{
	//		HRESULT hres = pSvc->ExecQuery(
	//        bstr_t("WQL"), 
	//        bstr_t("SELECT * FROM Win32_DiskDrive"),
	//        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
	//        NULL,
	//        &pEnumerator);
	//    if (FAILED(hres))
	//    {
	//        pSvc->Release();
	//        pLoc->Release();
	//        CoUninitialize();
	//        return 1;               // Program has failed
	//	}

	//    ULONG uReturn = 0;
	//   
	//    while (pEnumerator)
	//    {
	//       HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
	//            &pclsObj, &uReturn);

	//        if(0 == uReturn)
	//        {
	//            break;
	//        }

	//        VARIANT vtProp;

	//       // hr = pclsObj->Get(L"PNPDeviceID", 0, &vtProp, 0, 0);
	//		//if (1)//wcscmp(vtProp.bstrVal,PNPID) == 0) 
	//		//{
	//		
	//	        PostMessage(hWnd, WM_TOGGLE_CONNECTED_STATUS, 0, 0);
	//		//}
	//        VariantClear(&vtProp);

	//        pclsObj->Release();
	//    }
	//}
 //       break;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        PostMessage(hWnd, WM_EXIT_THREAD, 0, 0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

DWORD WINAPI CCommandWindow::_ThreadProc(__in LPVOID lpParameter)
{
	CCommandWindow *pCommandWindow = static_cast<CCommandWindow *>(lpParameter);
    if (pCommandWindow == NULL)
    {
        return 0;
    }

	HRESULT hres;
    hres =  CoInitializeEx(0, COINIT_MULTITHREADED); 
    if (FAILED(hres))
        return 1;                  // Program has failed.

    hres = CoCreateInstance(
        CLSID_WbemLocator,             
        0, 
        CLSCTX_INPROC_SERVER, 
		IID_IWbemLocator, (LPVOID *) &(pLoc));
 
    if (FAILED(hres))
    {
        CoUninitialize();
        return 1;                 // Program has failed.
    }
    hres = pLoc->ConnectServer(
         _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
         NULL,                    // User name. NULL = current user
         NULL,                    // User password. NULL = current
         0,                       // Locale. NULL indicates current
         NULL,                    // Security flags.
         0,                       // Authority (e.g. Kerberos)
         0,                       // Context object 
         &pSvc                    // pointer to IWbemServices proxy
         );

    if (FAILED(hres))
    {
        pLoc->Release();     
        CoUninitialize();
        return 1;                // Program has failed.
    }

    hres = CoSetProxyBlanket(
       pSvc,                        // Indicates the proxy to set
       RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
       RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
       NULL,                        // Server principal name 
       RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
       RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
       NULL,                        // client identity
       EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();     
        CoUninitialize();
        return 1;               // Program has failed.
    }

    

    HRESULT hr = S_OK;

    // Create the window.
    pCommandWindow->_hInst = GetModuleHandle(NULL);
    if (pCommandWindow->_hInst != NULL)
    {            
        hr = pCommandWindow->_MyRegisterClass();
        if (SUCCEEDED(hr))
        {
            hr = pCommandWindow->_InitInstance();
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
	}
	ShowWindow(pCommandWindow->_hWnd, SW_HIDE);
	
    if (SUCCEEDED(hr))
    {        
        while (pCommandWindow->_ProcessNextMessage()) 
        {
        }
    }
    else
    {
        if (pCommandWindow->_hWnd != NULL)
        {
            pCommandWindow->_hWnd = NULL;
        }
    }

    return 0;
}

