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

// maximum number of drives to monitor 
#define MAX_DRIVES 24

// global variables for change notifications
HANDLE  g_ChangeHandles[MAX_DRIVES];	// 변경된 핸들
HANDLE  g_DirHandles[MAX_DRIVES];		// 핸들 디렉토리
LPTSTR  g_szDrives[MAX_DRIVES];			// 드라이브 사이즈
DWORD   g_idx = 0;		// 인덱스

void ProcessChange(int idx)
{
	BYTE buf[32 * 1024];
	DWORD cb = 0;
	PFILE_NOTIFY_INFORMATION pNotify;
	int offset = 0;
	TCHAR szFile[MAX_PATH*2];

	memset(buf, 0, sizeof(buf));

	// find out what type of change triggered the notification
	if (ReadDirectoryChangesW(g_DirHandles[idx], buf, 
		sizeof(buf), TRUE, 
		FILE_CHANGE_FLAGS, &cb, NULL, NULL))
		/*
		#if(_WIN32_WINNT >= 0x0400)
		WINBASEAPI
		BOOL
		WINAPI
		ReadDirectoryChangesW(
			__in        HANDLE hDirectory,
			__out_bcount_part(nBufferLength, *lpBytesReturned) LPVOID lpBuffer,
			__in        DWORD nBufferLength,
			__in        BOOL bWatchSubtree,
			__in        DWORD dwNotifyFilter,
			__out_opt   LPDWORD lpBytesReturned,
			__inout_opt LPOVERLAPPED lpOverlapped,
			__in_opt    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
			);
		#endif  _WIN32_WINNT >= 0x0400 
		*/
	{
		// parse the array of file information structs
		do {
			pNotify = (PFILE_NOTIFY_INFORMATION) &buf[offset];
			offset += pNotify->NextEntryOffset;

			memset(szFile, 0, sizeof(szFile));

			memcpy(szFile, pNotify->FileName, 
				pNotify->FileNameLength);

			// if the file is whitelisted, go to the next
			if (IsWhitelisted(szFile)) {
				continue;
			}

			switch (pNotify->Action)
			{
			case FILE_ACTION_ADDED:
				Output(FOREGROUND_GREEN, 
					_T("[ADDED] %s%s\n"), g_szDrives[idx], szFile);
				break;
			case FILE_ACTION_REMOVED: 
				Output(FOREGROUND_RED, 
					_T("[REMOVED] %s%s\n"), g_szDrives[idx], szFile);
				break;
			case FILE_ACTION_MODIFIED: 
				Output(0, _T("[MODIFIED] %s%s\n"), 
					g_szDrives[idx], szFile);
				break;
			case FILE_ACTION_RENAMED_OLD_NAME:
				Output(0, _T("[RENAMED (OLD)] %s%s"), 
					g_szDrives[idx], szFile);
				break; 
			case FILE_ACTION_RENAMED_NEW_NAME:
				Output(0,_T("[RENAMED (NEW)] %s%s"), 
					g_szDrives[idx], szFile);
				break;
			default:
				Output(0,_T("[??] %s%s\n"), 
					g_szDrives[idx], szFile);
				break;
			};
		} while (pNotify->NextEntryOffset != 0);
	}
}

void StartFileMonitor(void)
{
	DWORD dwWaitStatus;
	BOOL  bOK = FALSE;
	TCHAR   pszList[1024];		// typedef WCHAR TCHAR, *PTCHAR;
	DWORD   ddType;
	LPTSTR  pStart = NULL;
	HANDLE  hChange, hDir;

	// get a list of logical drives
	memset(pszList, 0, sizeof(pszList));
	GetLogicalDriveStrings(sizeof(pszList), pszList);
	/*
	GetLogicalDriveStrings :
	Fills a buffer with strings that specify valid drives in the system.
	Each string in the buffer may be used wherever a root directory is required,
	such as for the GetDriveType and GetDiskFreeSpace functions.

	#define GetLogicalDriveStrings  GetLogicalDriveStringsW

	DWORD WINAPI GetLogicalDriveStrings(
		_In_  DWORD  nBufferLength,	
				// The maximum size of the buffer pointed to by lpBuffer, in TCHARs
		_Out_ LPTSTR lpBuffer
				// A pointer to a buffer that receives a series of null-terminated strings,
				// one for each valid drive in the system, plus with an additional null character.
	);
	*/

	// parse the list of null-terminated drive strings
	// (LPTSTR  pStart = NULL;)
	// Null로 종료되는 드라이브 문자열 목록을 구문 분석
	pStart = pszList;		// 배열의 시작 오프셋 저장
	while(_tcslen(pStart))	// = strlen
	{
		ddType = GetDriveType(pStart);
		/*
		GetDriveType :
		Determines whether a disk drive is a removable, fixed, CD-ROM, RAM disk, or network drive.
		To determine whether a drive is a USB-type drive,
		call SetupDiGetDeviceRegistryProperty and specify the SPDRP_REMOVAL_POLICY property.

		UINT WINAPI GetDriveType( _In_opt_ LPCTSTR lpRootPathName );
				// The root directory for the drive.
				// NULL : the function uses the root of the current directory.
		*/

		// only monitor local and removable (i.e. USB) drives
		if ((ddType == DRIVE_FIXED || ddType == DRIVE_REMOVABLE) && 
			_tcscmp(pStart, _T("A:\\")) != 0)
					/*
					// = strcmp : 스트링을 비교하는 함수
					
					DRIVE_REMOVABLE : The drive has removable media
					DRIVE_FIXED : The drive has fixed media
					
					모니터링하는 드라이브 조건 지정 : 로컬 및 이동식 드라이브
					드라이브의 종류가 하드디스크 또는 이동식 드라이버 또는 A:\\ 중 하나라도 참이면 다음 단계를 수행
					*/
		{
			hChange = FindFirstChangeNotification(pStart,
									TRUE, /* watch subtree */
								    FILE_CHANGE_FLAGS);
			/*
			*** FindFirstChangeNotification
			Creates a change notification handle and sets up initial change notification filter conditions.
			A wait on a notification handle succeeds
			when a change matching the filter conditions occurs in the specified directory or subtree.
			The function does not report changes to the specified directory itself.

			HANDLE WINAPI FindFirstChangeNotification(
				_In_ LPCTSTR lpPathName,
				_In_ BOOL    bWatchSubtree,
				_In_ DWORD   dwNotifyFilter
			);
			*/


			// #define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
			if (hChange == INVALID_HANDLE_VALUE)
				continue;

			hDir = CreateFile(pStart, 
				FILE_LIST_DIRECTORY, 
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				NULL, 
				OPEN_EXISTING, 
				FILE_FLAG_BACKUP_SEMANTICS, /* opens a directory */
				NULL);

			if (hDir == INVALID_HANDLE_VALUE) {
				FindCloseChangeNotification(hChange);	// WinBash.h
				/* 
				*** FindCloseChangeNotification
				Stops change notification handle monitoring.
				
				WINBASEAPI
				BOOL
				WINAPI
				FindCloseChangeNotification( __in HANDLE hChangeHandle );
				*/
				continue;
			}

			_tprintf(_T("Monitoring %s\n"), pStart);	// Start

			// save the handles and drive letter 
			g_szDrives[g_idx]      = _tcsdup(pStart);
			g_DirHandles[g_idx]    = hDir;
			g_ChangeHandles[g_idx] = hChange;
			g_idx++;
		}

		pStart += wcslen(pStart) + 1;
	}
 
	// wait for a notification to occur
	while(WaitForSingleObject(g_hStopEvent, 1) != WAIT_OBJECT_0) 
	{
		dwWaitStatus = WaitForMultipleObjects(
			g_idx,				// nCount
			g_ChangeHandles,	// *lpHandles
			FALSE, INFINITE);	// bWaitAll, dwMilliseconds
		/*
		*** WaitForSingleObject
		Waits until one or all of the specified objects are in the signaled state or the time-out interval elapses.
		To enter an alertable wait state, use the WaitForMultipleObjectsEx function.

		WINBASEAPI
		DWORD
		WINAPI
		WaitForMultipleObjects(
			__in DWORD nCount,
			__in_ecount(nCount) CONST HANDLE *lpHandles,
			__in BOOL bWaitAll,
			__in DWORD dwMilliseconds
			);
		*/


		bOK = FALSE;

		// if the wait suceeded, for which handle did it succeed?
		for(int i=0; i < g_idx; i++)
		{
			if (dwWaitStatus == WAIT_OBJECT_0 + i) 
			{
				bOK = TRUE;
				ProcessChange(i);
				
				if (!FindNextChangeNotification(g_ChangeHandles[i])) 
					/*
					*** FindNextChangeNotification
					https://msdn.microsoft.com/ko-kr/library/windows/desktop/aa364427(v=vs.85).aspx
					
					Requests that the operating system signal a change notification handle the next time it detects an appropriate change.
					After the FindNextChangeNotification function returns successfully, the application can wait for notification that a change has occurred by using the wait functions.
					*/
				{
					SetEvent(g_hStopEvent);
					break;
				}
				break;
			}
		}

		// the wait failed or timed out
		if (!bOK) break;
	}
	_tprintf(_T("Got stop event...\n"));

	for(int i=0; i < g_idx; i++)
	{
		_tprintf(_T("Closing handle for %s\n"), g_szDrives[i]);
		CloseHandle(g_DirHandles[i]);
		FindCloseChangeNotification(g_ChangeHandles[i]);
	}
}
