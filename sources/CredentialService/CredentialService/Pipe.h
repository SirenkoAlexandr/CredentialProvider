#pragma once
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include"plog\Log.h"

#include"DefenitionConstants.h"

int PipeMain(char *message)
{
	LOG_DEBUG << "Start Pipe whith message : "<< message;
	HANDLE hPipe;
	LPTSTR lpvMessage =(LPTSTR)message;
	wchar_t ProviderAnswer[128];
	BOOL   fSuccess = FALSE;
	DWORD  cbRead, cbToWrite, cbWritten, dwMode;
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\CredentialPipe");

	// Try to open a named pipe; wait for it, if necessary. 
	while (1)
	{
		hPipe = CreateFile(
			lpszPipename,   // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

							// Break if the pipe handle is valid. 

		if (hPipe != INVALID_HANDLE_VALUE)
			break;

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 

		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			LOG_DEBUG << "Could not open pipe.";
			return PIPE_ERROR;
		}

		// All pipe instances are busy, so wait for 1 second. 

		if (!WaitNamedPipe(lpszPipename, 2000))
		{
			LOG_DEBUG << "Could not open pipe: 2 second wait timed out.";
			return PIPE_ERROR;
		}
	}

	// The pipe connected; change to message-read mode. 
	dwMode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
		hPipe,    // pipe handle 
		&dwMode,  // new pipe mode 
		NULL,     // don't set maximum bytes 
		NULL);    // don't set maximum time 
	if (!fSuccess)
	{
		LOG_DEBUG << "SetNamedPipeHandleState failed.";
		return PIPE_ERROR;
	}

	// Send a message to the pipe server. 
	cbToWrite = (lstrlen(lpvMessage) + 1) * sizeof(TCHAR);
	LOG_DEBUG << "Sending message... ";

	fSuccess = WriteFile(
		hPipe,                  // pipe handle 
		lpvMessage,             // message 
		cbToWrite,              // message length 
		&cbWritten,             // bytes written 
		NULL);                  // not overlapped 

	if (!fSuccess)
	{
		LOG_DEBUG << "WriteFile to pipe failed.";
		return PIPE_ERROR;
	}

	LOG_DEBUG << "Message "<< lpvMessage <<" sent to server. Simbols ="<< cbToWrite;
	do
	{
		// Read from the pipe. 
		fSuccess = ReadFile(
			hPipe,    // pipe handle 
			ProviderAnswer,    // buffer to receive reply 
			BUFSIZE * sizeof(TCHAR),  // size of buffer 
			&cbRead,  // number of bytes read 
			NULL);    // not overlapped 
		if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
			break;
		LOG_DEBUG << "message from credential :"<< ProviderAnswer <<". bites :"<< cbRead;
	} while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

	if (!fSuccess)
	{
		LOG_DEBUG << "ReadFile from pipe failed.";
		return PIPE_ERROR;
	}
	else
	{
		if (!wcscmp(ProviderAnswer, L"Fail"))
		{
			CloseHandle(hPipe);
			LOG_DEBUG << "Wrong creds";
			return WRONG_CREDS;
		}
		if (!wcscmp(ProviderAnswer, L"Success"))
		{
			CloseHandle(hPipe);
			LOG_DEBUG << "good creds";
			return GOOD_CREDS;
		}

	}
	CloseHandle(hPipe);
	LOG_DEBUG << "something wrong";
	return PIPE_ERROR;
}