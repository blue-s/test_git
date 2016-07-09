#pragma once

#include <windows.h>
#include <tchar.h>

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
/*
#ifndef _NTDEF_
typedef __success(return >= 0) LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;
#endif
*/

#define FILE_CHANGE_FLAGS FILE_NOTIFY_CHANGE_FILE_NAME |\
					 FILE_NOTIFY_CHANGE_DIR_NAME |\
					 FILE_NOTIFY_CHANGE_ATTRIBUTES |\
					 FILE_NOTIFY_CHANGE_SIZE |\
					 FILE_NOTIFY_CHANGE_CREATION |\
					 FILE_NOTIFY_CHANGE_SECURITY

#define REG_CHANGE_FLAGS REG_NOTIFY_CHANGE_NAME |\
					 REG_NOTIFY_CHANGE_LAST_SET

void Output(USHORT Color, LPTSTR format, ... );
void StartFileMonitor(void);
void StartRegistryMonitor(void);

extern HANDLE  g_hStopEvent;
extern HANDLE  g_hFile;
extern HANDLE  g_hRegWatch[2];

// whitelisted filenames or paths
// whiteliste : 화이티리스트란 '안전'이 증명된 것만을 허용하는 것이다.
static LPTSTR g_szAllow[] = {
	_T("WINDOWS\\system32\\config\\"),
			// 윈도우 NT 기반의 운영 체제는
			// 자동으로 각 하이브의 백업본 (.BAK)을 %Windir%\System32\config 폴더에 만든다.
	_T("\\ntuser.dat.LOG"),
			// %UserProfile%\Ntuser.dat – HKEY_USERS\<사용자 SID> (HKEY_CURRENT_USER로 연결)
	_T("UsrClass.dat.LOG"),
			// %UserProfile%\Local Settings\Application Data\Microsoft\Windows\Usrclass.dat
			// (경로는 운영 체제의 언어에 따라 지역화되어 있다)
			// HKEY_USERS\<User SID>_Classes (HKEY_CURRENT_USER\Software\Classes)
	_T("RegFsNotify.txt"),
	_T("_restore"),
	_T("CatRoot2"),
	_T("\\Microsoft\\Cryptography\\RNG"),
	_T("\\Microsoft\\WBEM"),
};

// return true if szFile is in the g_szAllow list
// 함수 : _tcsstr(검색대상, 검색어) = wcsstr()
// 반환 : 성공한 경우 주소값을 반환 / 검색된 부분부터 끝까지 문자열 반환
// szFile에서 g_szAllow(화이트리스트)를 검색한 결과 반환값이 NULL이 아닐경우
// szFile에 화이트리스트의 단어와 동일한 단어가 존재하는 것으로 판단하여 TRUE를 반환
// 검색 결과가 없을 경우 FALSE를 반환
static BOOL IsWhitelisted(LPTSTR szFile)
{
	for(int i=0; i<sizeof(g_szAllow)/sizeof(LPTSTR); i++)
	{
		if (_tcsstr(szFile, g_szAllow[i]) != NULL) 
			return TRUE;
	}
	return FALSE;
}