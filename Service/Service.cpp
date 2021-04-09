#include <windows.h>
#include <iostream>
#include <atlstr.h>
#include <wtsapi32.h>
#include "logger.h"

using namespace std;

#define SERVICE_NAME  _T("Sample")

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = NULL ;

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
	//  Periodically check if the service has been requested to stop
	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		//  Simulate some work by sleeping
		Sleep(3000);
	}
	return ERROR_SUCCESS;
}

VOID LaunchNotepad()
{
	//convert to method launch path
	LPCWSTR path = L"C:\\Windows\\System32\\notepad.exe";

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	HANDLE htoken;
	DWORD sessionId;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	sessionId = WTSGetActiveConsoleSessionId();
	WTSQueryUserToken(sessionId, &htoken);

	WTS_INFO_CLASS WTSInfoClass;

	ZeroMemory(&WTSInfoClass, sizeof(WTSInfoClass));

	DWORD bytesReturned = 0;
	LPSTR pData = NULL;

	WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE,sessionId, WTSInfoClass, &pData, &bytesReturned);

	//LOGGER->Log("Current Username: %s ", WTSInfoClass);
	si.wShowWindow = TRUE;

	if (CreateProcessAsUser(htoken, path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}

	CloseHandle(htoken);
}

VOID UserDefinedControl()
{
	LOGGER->Log("SERVICE_CONTROL_CUSTOM_MESSAGE is requested");

	CString ucpTitle = _T("Request"), ucpMessage = _T("Do you want to open notepad");
	DWORD ucpResponse;

	BOOL res = WTSSendMessage(NULL,
		WTSGetActiveConsoleSessionId(),
		ucpTitle.GetBuffer(),
		wcslen(ucpTitle) * sizeof(WCHAR),
		ucpMessage.GetBuffer(),
		wcslen(ucpMessage) * sizeof(WCHAR),
		MB_ICONQUESTION | MB_YESNO | MB_TOPMOST,
		10,
		&ucpResponse,
		TRUE);

	if (res)
	{
		if (ucpResponse == 6)
		{
			LaunchNotepad();
		}
		LOGGER->Log("WTSSendMessage success in user defined control ");
	}
	else
	{
		LOGGER->Log("WTSSendMessage failed in user defined control ");
	}
	
}				
			
VOID Stop()
{
	LOGGER->Log("SERVICE_CONTROL_STOP is triggered");

	if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
	{
		return;
	}
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 4;
	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		LOGGER->Log( " ServiceCtrlHandler: SetServiceStatus returned error in stop");
	}
	// This will signal the worker thread to start shutting down
	SetEvent(g_ServiceStopEvent);
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:
	{
		Stop();
		break;
	}
	case SERVICE_CONTROL_CUSTOM_MESSAGE:
	{
		UserDefinedControl();
		break;
	}
	case SERVICE_CONTROL_INTERROGATE:
	{
		LOGGER->Log("SERVICE_CONTROL_INTERROGATE is triggered");
		break;
	}
	case SERVICE_CONTROL_PRESHUTDOWN:
	{
		LOGGER->Log("SERVICE_CONTROL_PRESHUTDOWN is triggered");
		break;
	}
	case SERVICE_CONTROL_SHUTDOWN:
	{
		LOGGER->Log("SERVICE_CONTROL_SHUTDOWN is triggered");
		break;
	}
	case SERVICE_CONTROL_SESSIONCHANGE:
	{
		LOGGER->Log("SERVICE_CONTROL_SESSIONCHANGE is triggered");
		break;
	}
	case SERVICE_CONTROL_USERMODEREBOOT :
	{
		LOGGER->Log("SERVICE_CONTROL_USERMODEREBOOT is triggered");
		break;
	}
	case SERVICE_CONTROL_USER_LOGOFF:
	{
		LOGGER->Log("SERVICE_CONTROL_USER_LOGOFF is triggered");
		break;
	}
	default:
	{
		break;
	}
	}
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
	DWORD Status = NULL ;
	
	// Tell the service controller we are starting
	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

	if (g_StatusHandle == NULL)
	{
		return;
	}

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		LOGGER->Log("ServiceMain: SetServiceStatus returned error ");
	}

	// Create a service stop event to wait on later
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (g_ServiceStopEvent == NULL)
	{
		// Error creating event
		// Tell service controller we are stopped and exit
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			LOGGER->Log("ServiceMain: SetServiceStatus returned error");
		}
		return;
	}

	// Tell the service controller we are started
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		LOGGER->Log("ServiceMain: SetServiceStatus returned error") ;
	}

	HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

	// Wait until our worker thread exits signaling that the service needs to stop
	WaitForSingleObject(hThread, INFINITE);
	
	CloseHandle(hThread);
	CloseHandle(g_ServiceStopEvent);

	// Tell the service controller we are stopped
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		LOGGER->Log("ServiceMain: SetServiceStatus returned error");
	}
}

int main(int argc, TCHAR* argv[])
{	
	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{(LPWSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL, NULL}
	};

	if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
	{
		return GetLastError();
	}
	
	return 0;
}

