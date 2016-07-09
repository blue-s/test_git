/*
# Copyright (C) 2010 Michael Ligh
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "mon.h"

// global variable for time keeping 
ULARGE_INTEGER g_tmStart;	// WinNT.h

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

// associate HKEYs with key names
typedef struct REGMON { 
	HKEY   hMainKey;
	LPTSTR szSubkey;
} REGMON, *PREGMON;

//////////////////////////////////////////////////
// these are for calling ZwQueryKey which we use to 
// resolve a key name from a given HKEY 

typedef struct _KEY_NAME_INFORMATION {
  ULONG NameLength;
  WCHAR Name[4096];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

typedef enum _KEY_INFORMATION_CLASS {
  KeyBasicInformation            = 0,
  KeyNodeInformation             = 1,
  KeyFullInformation             = 2,
  KeyNameInformation             = 3,
  KeyCachedInformation           = 4,
  KeyFlagsInformation            = 5,
  KeyVirtualizationInformation   = 6,
  KeyHandleTagsInformation       = 7,
  MaxKeyInfoClass                = 8 
} KEY_INFORMATION_CLASS;

typedef NTSTATUS (WINAPI *ZWQUERYKEY)(
	HANDLE, 
	KEY_INFORMATION_CLASS, 
	PVOID, 
	ULONG, 
	PULONG);

ZWQUERYKEY ZwQueryKey;	// 레지스트리 키에대한 정보

void GetKeyName(HKEY hKey, LPWSTR szName)
{
	KEY_NAME_INFORMATION info;
	DWORD dwLen;
	NTSTATUS n;

	memset(&info, 0, sizeof(info));
	if (ZwQueryKey != NULL) {
		n = ZwQueryKey(hKey, KeyNameInformation,
		    &info, sizeof(info), &dwLen);
		if (n == STATUS_SUCCESS &&
			info.NameLength > 0 &&
			info.NameLength < MAX_KEY_LENGTH) 
		{ 
			wcscpy(szName, info.Name);
			szName[info.NameLength-1] = L'\x00';
		}
	}
}
//////////////////////////////////////////////////

void GetRegistryChanges(HKEY hKey) 
{ 
    TCHAR    szKey[MAX_KEY_LENGTH];  
    DWORD    cbName;                  
    DWORD    cSubKeys=0;                    
    FILETIME ftWrite;      
    DWORD    i, ret; 
	HKEY     hNewKey;
	ULARGE_INTEGER tmWrite;
	TCHAR    szName[MAX_KEY_LENGTH];
 
    // get the number of subkeys 
    ret = RegQueryInfoKey(		// WinReg.h
        hKey,                   
        NULL, NULL, NULL,               
        &cSubKeys,              
        NULL, NULL, NULL, NULL, 
		NULL, NULL, NULL);      
    
    // for each subkey, see if it changed based on its
    // last write timestamp
    for (i=0; i<cSubKeys; i++) 
    { 
        cbName = MAX_KEY_LENGTH;

        ret = RegEnumKeyEx(		// WinReg.h
					hKey, i, szKey, &cbName, 
					NULL, NULL, NULL, &ftWrite); 

        if (ret == ERROR_SUCCESS) 
        {
			tmWrite.HighPart = ftWrite.dwHighDateTime;
			tmWrite.LowPart  = ftWrite.dwLowDateTime;

            // it changed if the last write is greater than 
            // our start time
			if (tmWrite.QuadPart > g_tmStart.QuadPart)
			{
				memset(szName, 0, sizeof(szName));
				GetKeyName(hKey, szName);

				_tcscat(szName, _T("\\"));	// 문자열 추가 strcat_s
				_tcscat(szName, szKey);
				
				if (!IsWhitelisted(szName)) { 
					Output(FOREGROUND_BLUE, _T("[REGISTRY] %s\n"), szName);
				}
			}

			ret = RegOpenKeyEx(hKey, szKey, 0, KEY_READ, &hNewKey);

			if (ret == ERROR_SUCCESS) 
			{ 
				GetRegistryChanges(hNewKey);
				RegCloseKey(hNewKey);
			}
		}
    }
}

// call this once after each change to update the start time
// otherwise we'll print duplicates over and over
void UpdateTime(void)
{
	SYSTEMTIME st;
	FILETIME   ft;

	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);

	g_tmStart.HighPart = ft.dwHighDateTime;
	g_tmStart.LowPart  = ft.dwLowDateTime;	
}

// main thread for monitoring keys 
DWORD WatchKey(PREGMON p)
{
	HANDLE hEvent;
	HKEY   hKey;
	LONG   ret;

	Output(0, _T("Monitoring HKEY %x\\%s\n"), 
		p->hMainKey, p->szSubkey);

	/*
		WinReg.h
	RegOpenKeyEx :
	Opens the specified registry key.
	Note that key names are not case sensitive.
	To perform transacted registry operations on a key,
	call the RegOpenKeyTransacted function.

	RegOpenKeyTransacted :
	Opens the specified registry key and associates it with a transaction.
	Note that key names are not case sensitive.
	*/
	ret = RegOpenKeyEx(
		p->hMainKey, 
		p->szSubkey, 
		0, 
		KEY_READ | KEY_NOTIFY, 
		&hKey);

	if (ret != ERROR_SUCCESS)
	{
		return -1;
	}

	// create an event that will get signaled by the system
	// when a change is made to the monitored key
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent == NULL)
	{
		return -1;
	}

    // this event gets signaled if a user enters CTRL+C to stop
	while(WaitForSingleObject(g_hStopEvent, 1) != WAIT_OBJECT_0)
	{
		UpdateTime();
		
		// register to receive change notification 
		ret = RegNotifyChangeKeyValue(hKey, 
									  TRUE, 
									  REG_CHANGE_FLAGS, 
									  hEvent, 
									  TRUE);
		if (ret != ERROR_SUCCESS)
		{
			break;
		}

		if (WaitForSingleObject(hEvent, INFINITE) == WAIT_FAILED)
		{
			break;
		}

		GetRegistryChanges(hKey);
	}

	Output(0, _T("Closing HKEY %x\\%s\n"), 
		p->hMainKey, p->szSubkey);

	RegCloseKey(hKey);
	CloseHandle(hEvent);
	return 0;
}

void StartRegistryMonitor(void)
{
	HMODULE hNtdll = GetModuleHandle(_T("ntdll.dll"));
	/* 
	http://tip.daum.net/question/2920822
	일반적으로 컴퓨터는 유저모드 (사용자 공간)와 커널모드 (기계공간)으로 존재한다.
	ntdll.dll 파일은 유저모드에서 장치드라이버에 동작을 요청 시 거치게 된다.
	드라이버는 커널 모드에 존재하므로 커널 모드로 진입하게 해주는 라이브러리다.
	*/
	ZwQueryKey = (ZWQUERYKEY)GetProcAddress(hNtdll, "NtQueryKey");
	/*
	NtQueryKey :
	NtQueryKey and ZwQueryKey are two versions of the same Windows Native System Services routine.
	The NtQueryKey routine in the Windows kernel is not directly accessible to kernel-mode drives.
	However, kernel-mode drivers can access this routine indirectly by calling the ZwQueryKey routine.

	ZwQueryKey :
	The ZwQueryKey routine provides information about the class of a registry key, and the number and sizes of its subkeys.
	*/

	PREGMON p[2];		// HKEY 관리할 배열
	p[0] = new REGMON;
	p[1] = new REGMON;

	p[0]->hMainKey = HKEY_LOCAL_MACHINE;	// WinReg.h
	p[0]->szSubkey = _T("Software");
	
	// one thread for HKLM\\Software
	g_hRegWatch[0] = CreateThread(NULL, 0, 
	    (LPTHREAD_START_ROUTINE)WatchKey, p[0], 0, NULL);

	p[1]->hMainKey = HKEY_CURRENT_USER;		// WinReg.h
	p[1]->szSubkey = _T("Software");
	
	// one thread for HKCU\\Software
	g_hRegWatch[1] = CreateThread(NULL, 0, 
	    (LPTHREAD_START_ROUTINE)WatchKey, p[1], 0, NULL);
}