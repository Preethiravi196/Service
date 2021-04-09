#include <windows.h>
#include <iostream>
#include <atlstr.h>
#include <strsafe.h>
#include "logger.h"

using namespace std;

class ServiceHandlers
{
private:
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

public:

	ServiceHandlers()
	{
		schSCManager = NULL;
		schService = NULL;
	}

	ServiceHandlers(LPWSTR SERVICE_NAME)
	{
		//LOGGER->SetFilename("ServiceHandlers.txt");
		// Get a handle to the database
		schSCManager = OpenSCManager(
			NULL,                   
			NULL,                    
			SC_MANAGER_ALL_ACCESS);  

		// Get a handle to the service.
		schService = OpenService(
			schSCManager,            
			SERVICE_NAME,               
			SC_MANAGER_ALL_ACCESS | SERVICE_USER_DEFINED_CONTROL );  

	}

	VOID Close_Handlers()
	{
		CloseServiceHandle(schSCManager);
		LOGGER->Log("Service Control Manager handle is closed");

		CloseServiceHandle(schService);
		LOGGER->Log("Service handle is closed");
	}

	BOOL check_handlers()
	{
		if (NULL == schSCManager)
		{
			LOGGER->Log("Open SCManager failed in check handler ");
			return FALSE;
		}
		else
		{
			LOGGER->Log("Open SCManager success in check handler");
		}
		if (schService == NULL)
		{
			LOGGER->Log("Open Service failed in check handler ");
			return FALSE;
		}
		else
		{
			LOGGER->Log("Open Service success in check handler");
		}
		return TRUE;
	}

	SC_HANDLE get_service_handle()
	{
		return schService;
	}
		
	SC_HANDLE get_scmanager_handle()
	{
		return schSCManager;
	}

	VOID DisplayMessage(BOOL res, string type, int error)
	{
		if (res)
		{
			cout << type << "success\n";
		}
		else
		{
			cout << type << "failure : " << error << endl;
		}
	}

	BOOL DoUpdateSvcDesc()
	{
		SERVICE_DESCRIPTION sd;

		string str = "This is a test description";
		WCHAR wide_str[sizeof(str) / sizeof(char)];
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wide_str, sizeof(str) / sizeof(char));
		LPTSTR szDesc = wide_str;

		sd.lpDescription = szDesc;

		if (!ChangeServiceConfig2(
			schService,                 // handle to service
			SERVICE_CONFIG_DESCRIPTION, // change: description
			&sd))                      // new description
		{
			LOGGER->Log("ChangeServiceConfig2 failed in update description ");
			return FALSE;
		}
		else
		{
			LOGGER->Log("Service description updated successfully");
			return TRUE;
		}
	}

	BOOL DoQuerySvc()
	{
		LPQUERY_SERVICE_CONFIG lpsc = NULL;
		LPSERVICE_DESCRIPTION lpsd = NULL;
		DWORD dwBytesNeeded, cbBufSize, dwError;
		
		// Get the configuration information.
		if (!QueryServiceConfig(
			schService,
			NULL,
			0,
			&dwBytesNeeded))
		{
			dwError = GetLastError();
			if (ERROR_INSUFFICIENT_BUFFER == dwError)
			{
				cbBufSize = dwBytesNeeded;
				lpsc = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_FIXED, cbBufSize);
			}
			else
			{
				LOGGER->Log("QueryServiceConfig of configuration information failed in query ");
				return FALSE;
			}
		}

		if (!QueryServiceConfig(
			schService,
			lpsc,
			cbBufSize,
			&dwBytesNeeded))
		{
			LOGGER->Log("QueryServiceConfig of configuration information failed in query");
			return FALSE;
		}

		//Get the descriptiion data
		if (!QueryServiceConfig2(
			schService,
			SERVICE_CONFIG_DESCRIPTION,
			NULL,
			0,
			&dwBytesNeeded))
		{
			dwError = GetLastError();
			if (ERROR_INSUFFICIENT_BUFFER == dwError)
			{
				cbBufSize = dwBytesNeeded;
				lpsd = (LPSERVICE_DESCRIPTION)LocalAlloc(LMEM_FIXED, cbBufSize);
			}
			else
			{
				LOGGER->Log("QueryServiceConfig of description failed in query");
				return FALSE;
			}
		}

		if (!QueryServiceConfig2(
			schService,
			SERVICE_CONFIG_DESCRIPTION,
			(LPBYTE)lpsd,
			cbBufSize,
			&dwBytesNeeded))
		{
			LOGGER->Log("QueryServiceConfig of description failed in query");
			return FALSE;
		}

		// Print the configuration information.

		_tprintf(TEXT("%s configuration: \n"), lpsc->lpDisplayName);
		_tprintf(TEXT("  Type: 0x%x\n"), lpsc->dwServiceType);
		_tprintf(TEXT("  Start Type: 0x%x\n"), lpsc->dwStartType);
		_tprintf(TEXT("  Error Control: 0x%x\n"), lpsc->dwErrorControl);
		_tprintf(TEXT("  Binary path: %s\n"), lpsc->lpBinaryPathName);
		_tprintf(TEXT("  Account: %s\n"), lpsc->lpServiceStartName);

		if (lpsd->lpDescription != NULL && lstrcmp(lpsd->lpDescription, TEXT("")) != 0)
		{
			_tprintf(TEXT("  Description: %s\n"), lpsd->lpDescription);
		}

		if (lpsc->lpLoadOrderGroup != NULL && lstrcmp(lpsc->lpLoadOrderGroup, TEXT("")) != 0)
		{
			_tprintf(TEXT("  Load order group: %s\n"), lpsc->lpLoadOrderGroup);
		}

		if (lpsc->dwTagId != 0)
		{
			_tprintf(TEXT("  Tag ID: %d\n"), lpsc->dwTagId);
		}

		if (lpsc->lpDependencies != NULL && lstrcmp(lpsc->lpDependencies, TEXT("")) != 0)
		{
			_tprintf(TEXT("  Dependencies: %s\n"), lpsc->lpDependencies);
		}
		LOGGER->Log("DoQuerySvc success");
		return TRUE;
		LocalFree(lpsc);
		LocalFree(lpsd);
	}

};

int _tmain(int argc, TCHAR* argv[])
{
	if (argc > 1)
	{
		SERVICE_STATUS status;
		BOOL result;

		TCHAR szCommand[10];
		TCHAR szSvcName[80];
		StringCchCopy(szCommand, 10, argv[1]);
		StringCchCopy(szSvcName, 80, argv[2]);

		ServiceHandlers object(szSvcName);

		if (object.check_handlers())
		{
			if (lstrcmpi(szCommand, TEXT("query")) == 0)
			{
				result = object.DoQuerySvc();
				object.DisplayMessage(result, "Query ", GetLastError());
			}
			else if (lstrcmpi(argv[1], TEXT("update")) == 0)
			{
				result = object.DoUpdateSvcDesc();
				object.DisplayMessage(result, "Update description ", GetLastError());
			}
			else if (lstrcmpi(argv[1], TEXT("stop")) == 0)
			{
				result = ControlService(object.get_service_handle(), SERVICE_CONTROL_STOP, &status);
				object.DisplayMessage(result, "Stop ", GetLastError());
			}
			else if (lstrcmpi(argv[1], TEXT("user")) == 0)
			{
				result = ControlService(object.get_service_handle(), SERVICE_CONTROL_CUSTOM_MESSAGE, &status);
				object.DisplayMessage(result, "User Control Service ", GetLastError());
			}
			else
			{
				cout << "Invalid command!!";
			}
		}

		object.Close_Handlers();	
	}
	return 0;
}
