#pragma once
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include"plog\Log.h"

#define BUFSIZE 512

int PipeMain(char *message)
{
	//char *message = (char*)Param;
	HANDLE hPipe;
	LPTSTR lpvMessage =(LPTSTR)message;
	TCHAR  chBuf[BUFSIZE];
	BOOL   fSuccess = FALSE;
	DWORD  cbRead, cbToWrite, cbWritten, dwMode;
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\CredentialPipe");
	//LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\mynamedpipe");

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
			return 0;
		}

		// All pipe instances are busy, so wait for 1 second. 

		if (!WaitNamedPipe(lpszPipename, 2000))
		{
			LOG_DEBUG << "Could not open pipe: 2 second wait timed out.";
			return 0;
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
		return 0;
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
		return 0;
	}

	LOG_DEBUG << "Message "<< lpvMessage <<" sent to server. Simbols ="<< cbToWrite;

	do
	{
		// Read from the pipe. 

		fSuccess = ReadFile(
			hPipe,    // pipe handle 
			chBuf,    // buffer to receive reply 
			BUFSIZE * sizeof(TCHAR),  // size of buffer 
			&cbRead,  // number of bytes read 
			NULL);    // not overlapped 

		if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
			break;

		LOG_DEBUG << chBuf;
	} while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

	if (!fSuccess)
	{
		LOG_DEBUG << "ReadFile from pipe failed.";
		return 0;
	}

	LOG_DEBUG << "Message sended";

	CloseHandle(hPipe);
	LOG_DEBUG << "Close Pipe";
	return 1;
}