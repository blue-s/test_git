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

 HANDLE g_hFile;
 HANDLE g_hStopEvent;
 HANDLE g_hRegWatch[2];

USHORT GetConsoleTextAttribute(HANDLE hConsole)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
	/*
	*** GetConsoleScreenBufferInfo
	Retrieves information about the specified console screen buffer.

	WINBASEAPI
	BOOL
	WINAPI
	GetConsoleScreenBufferInfo(
		__in HANDLE hConsoleOutput,
			// A handle to the console screen buffer.
		__out PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo
			// A pointer to a CONSOLE_SCREEN_BUFFER_INFO structure
			// that receives the console screen buffer information.
		);
	*/
    return(csbi.wAttributes);
}

// ����� ��� �Լ�
void Output(USHORT Color, LPTSTR format, ... )
{
	va_list args;		// typedef char *  va_list;
	int len;
	DWORD cb;
	LPTSTR buffer;		// typedef LPWSTR PTSTR, LPTSTR;

	va_start(args, format);
	/* 
	#define va_start _crt_va_start
	http://psychoria.tistory.com/entry/%EA%B0%80%EB%B3%80-%EC%9D%B8%EC%9E%90Variable-Arguments-%EB%82%B4%EB%B6%80-%EA%B5%AC%EC%A1%B0
	���� ���� �Լ��� printf ���� ������ �����̳� ���� �������� ���� ������ �Լ���.
	���� ���� ��ũ�δ� stdarg.h�� ���ǵǾ� �ִ�.

	#define _crt_va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )

	ap�� v�� ���� �޾Ƽ� v�� �ּҿ� _INTSIZEOF(v)�� ���ؼ� ���� ���� ���ؼ� ap�� �����մϴ�.
	ap�� va_list�� ������ va_list�� �����δ� ���� char*�Դϴ�.
	��, v�� �ּҿ� _INTSIZEOF(v)�� ������ �ּҸ� char*�� �� �����Ϳ� �־��ִ� ���Դϴ�.

	//va_start : va_list�� ������� �����Ϳ��� ���� ���� �� ù��° ������ �ּҸ� ��������
	//va_start(va_list�� ���� ������, ������ ���� �μ�)

	Error : Only Win32 target supported!
	*/
	
	
	len = _vsctprintf(format, args) + sizeof(TCHAR);
			// _vsctprintf : ���յ� ���ڿ��� ���� Ȯ��, ���ڿ��� ���̰� ũ�� �������� �޸� Ȯ���Ͽ� printf
	buffer = new TCHAR[len * sizeof(TCHAR)];	// ���� �޸� ���� - ����

	if (!buffer) { 
		return;
	}

	_vstprintf_s(buffer, len, format, args);

	if (g_hFile != INVALID_HANDLE_VALUE) {		// #define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#ifdef _UNICODE
		LPSTR str = new CHAR[len + 1];
		if (str) { 
			memset(str, 0, len + 1);
			/*
			*** memset
			Sets the first num bytes of the block of memory pointed
			by ptr to the specified value (interpreted as an unsigned char).
			str�� 0���� len+1���� ä��

			void * memset ( void * ptr, int value, size_t num );
				ptr : Pointer to the block of memory to fill.
				value : Value to be set. The value is passed as an int
					but the function fills the block of memory using the unsigned char conversion of this value.
				num : Number of bytes to be set to the value.
					size_t is an unsigned integral type.

			void *  __cdecl memset(_Out_opt_bytecapcount_(_Size) void * _Dst, _In_ int _Val, _In_ size_t _Size);
			*/
			

			WideCharToMultiByte(CP_ACP, 0, 
				buffer, -1, str, len, NULL, NULL);
			/*
			*** WideCharToMultiByte

			WINBASEAPI
			int
			WINAPI
			WideCharToMultiByte(
				__in UINT     CodePage,
						// Code page to use in performing the conversion.
						// This parameter can be set to the value of any code page
						// that is installed or available in the operating system.
						// CP_ACP : The system default Windows ANSI code page.
						//  Code Page Default Values : 0
				__in DWORD    dwFlags,
				__in_ecount(cchWideChar) LPCWSTR  lpWideCharStr,
				__in int      cchWideChar,
				__out_bcount_opt(cbMultiByte) __transfer(lpWideCharStr) LPSTR   lpMultiByteStr,
				__in int      cbMultiByte,
				__in_opt LPCSTR   lpDefaultChar,
				__out_opt LPBOOL  lpUsedDefaultChar);
			*/


			WriteFile(g_hFile, str, strlen(str), &cb, NULL);
			/*
			*** WriteFile
			Writes data to the specified file or input/output (I/O) device.
			
			WINBASEAPI
			BOOL
			WINAPI
			WriteFile(
				__in        HANDLE hFile,
				__in_bcount_opt(nNumberOfBytesToWrite) LPCVOID lpBuffer,
				__in        DWORD nNumberOfBytesToWrite,
				__out_opt   LPDWORD lpNumberOfBytesWritten,
				__inout_opt LPOVERLAPPED lpOverlapped
				);
			*/


			delete[] str;
		}
#else 
	WriteFile(g_hFile, buffer, strlen(buffer), &cb, NULL);
#endif
	} 

	HANDLE Handle = GetStdHandle(STD_OUTPUT_HANDLE);
	/*
	*** GetStdHandle
	Retrieves a handle to the specified standard device
	(standard input, standard output, or standard error).
	ǥ������� �� �����ڵ��� ����
	�ֿܼ��� stdout, stdin, stderr �� �ڵ��� �������� �Լ�

	WINBASEAPI
	HANDLE
	WINAPI
	GetStdHandle( __in DWORD nStdHandle );
		//STD_INPUT_HANDLE : ǥ�� �Է� �ڵ�
		//STD_OUTPUT_HANDLE : ǥ�� ��� �ڵ�
		//STD_ERROR_HANDLE : ǥ�� ���� �ڵ�

	#define STD_OUTPUT_HANDLE   ((DWORD)-11)
	*/

	if (Color) 
	{ 
		SetConsoleTextAttribute(
			Handle, Color | FOREGROUND_INTENSITY);
			// #define FOREGROUND_INTENSITY 0x0008
			// text color is intensified.
		/*
		*** SetConsoleTextAttribute
		WINBASEAPI
		BOOL
		WINAPI
		SetConsoleTextAttribute(
			__in HANDLE hConsoleOutput,
			__in WORD wAttributes
			);
		*/
	} 

	_tprintf(buffer);

	SetConsoleTextAttribute(Handle, 
		FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);

   delete[] buffer;
}
 
BOOL CtrlHandler(DWORD fdwCtrlType) 
{ 
	switch(fdwCtrlType) 
	{ 
    case CTRL_C_EVENT:		// 0
		SetEvent(g_hStopEvent);
		return TRUE;
		/*
		*** SetEvent
		Sets the specified event object to the signaled state.

		WINBASEAPI
		BOOL
		WINAPI
		SetEvent( __in HANDLE hEvent );
		*/
    case CTRL_CLOSE_EVENT:	// 2
		SetEvent(g_hStopEvent);
		return TRUE; 

    case CTRL_BREAK_EVENT:	// 1
		return FALSE; 
    case CTRL_LOGOFF_EVENT:	// 5
		return FALSE; 
    case CTRL_SHUTDOWN_EVENT: // 6
		return FALSE; 
    default: 
		return FALSE; 
	} 
} 

void _tmain(int argc, TCHAR *argv[])
{
	g_hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);		// WinBash.h
	/*
	CreateEventW (Unicode)
	Creates or opens a named or unnamed event object.

	HANDLE WINAPI CreateEvent(
		_In_opt_ LPSECURITY_ATTRIBUTES lpEventAttributes,
		_In_     BOOL                  bManualReset,
		_In_     BOOL                  bInitialState,
		_In_opt_ LPCTSTR               lpName
	);

	*** !LPSECURITY_ATTRIBUTES

	The SECURITY_ATTRIBUTES structure contains the security descriptor for an object and specifies
	whether the handle retrieved by specifying this structure is inheritable.
	This structure provides security settings for objects created by various functions,
	such as CreateFile, CreatePipe, CreateProcess, RegCreateKeyEx, or RegSaveKeyEx.
	
	typedef struct _SECURITY_ATTRIBUTES {
	  DWORD  nLength;
	  LPVOID lpSecurityDescriptor;
	  BOOL   bInheritHandle;
	} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

		*** LPVOID
		http://kspil.tistory.com/6
		Microsoft Playform SDK�� ��ġ�ϰ� �� ���� ���丮���� WinDef.h ������ ã����
		�Ʒ��� ���� ġȯ����� ��� ���ǵǾ� �ֽ��ϴ�.
		
		#define WINAPI __stdcall
		typedef unsigned char BYTE;
		typedef unsigned long * PDWORD;
		typedef void far * LPVOID;

			*** far * http://egloos.zum.com/jhjang/v/2369128
			far ������ �� �Լ��� unix�� ansi c������ ������ ���� �ʰ� ������ �����쿡���� ����Ͽ�����
			�ֱٿ��� WIN32�� ȯ���� �ٲ�鼭 ������� �ʰ� �Ǿ���.
			���� ���� ���ϸ� near�����Ϳ� far�����ʹ� windef.h���Ͽ� ���ǵǴµ� win32�� �ٲ�鼭 �� ���忭�� ó���ϵ��� ����Ǿ���.

		�ܼ��� Microsoft���� �� �� �����ϱ� ����鼭 ª�� Ÿ�Ը��� ������ ���̴�.

		�̸����� ������ ������ �ִ�.

		1. ������ Ÿ��(*)�� ��쿡�� P�� �����ϸ� - PINT, PDWORD ��
		2. far ������(far *)�� ��쿡�� L�� �����մϴ� - LPVOID ��
		3. unsigned Ÿ���� ��� U�� �����մϴ�. - UINT, ULONG ��
		4. BYTE, WORD, DWORD ���� ��������� ���̴� ������ ������ �̸��� ���Խ��ϴ�.
	
	*** LPCTSTR
	http://egloos.zum.com/pelican7/v/1768951

	LP�� long pointer�� ��Ÿ���� ���μ� 16bit������ �������� �����̴�.
	���� LP(long pointer)�� .Net������ 64bit pointer��,
	VC++6.0�� �� ���� ���������� 32bit pointer�� ��Ÿ����.

	C�� constant, �� �Լ��� ���ο��� ���ڰ��� �������� ����� ���̴�. 
	STR�� ���״�� string�ڷᰡ �ɰ��̶�� ������ ���������δ� char�� �迭�� null�� ���Ḧ �ǹ��Ѵ�.

	LPSTR = long pointer string = char * 
	LPCTSTR = long pointer constant t_string = const tchar * 

	�����Ϸ��� precompile option�� ���� ȯ�濡 �°� �����ϴ� �ڵ带 �ۼ��� �� �ִ� ���ο� ���� ����� Macro�� �����ϰ� �Ǿ���. 
	�װ��� �ٷ� TCHAR, t_char��� ������. 
	��, �ڽ��� �ü���� multi-byteȯ���̸�, char������, 
	unicodeȯ���̸�, w_char, wide char������ type casting�ȴ�.
	*/

	

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
	/*
	SetConsoleCtrlHandler :
	Adds or removes an application-defined HandlerRoutine function
	from the list of handler functions for the calling process.
	If no handler function is specified,
	the function sets an inheritable attribute that determines
	whether the calling process ignores CTRL+C signals.
	
	SetConsoleCtrlHandler(
		__in_opt PHANDLER_ROUTINE HandlerRoutine,
		__in BOOL Add);	

	*** PHANDLER_ROUTINE

	// typedef for ctrl-c handler routines
	typedef
	BOOL
	(WINAPI *PHANDLER_ROUTINE)( __in DWORD CtrlType	);
	*/


	g_hFile = CreateFile(_T("RegFsNotify.txt"), 
		GENERIC_WRITE,				// (0x40000000L)
		FILE_SHARE_READ, 0,			// 0x00000001, 0
		CREATE_ALWAYS, 0, NULL);	// 2, 0, NULL
	/*
	https://msdn.microsoft.com/en-us/library/windows/desktop/aa363858(v=vs.85).aspx
	WINBASEAPI
	__out
	HANDLE
	WINAPI
	CreateFileW(
		__in     LPCWSTR lpFileName,
		__in     DWORD dwDesiredAccess,
					// The requested access to the file or device,
					// which can be summarized as read, write, both or neither zero
		__in     DWORD dwShareMode,
					// The requested sharing mode of the file or device,
					// which can be read, write, both, delete, all of these, or none
		__in_opt LPSECURITY_ATTRIBUTES lpSecurityAttributes,
					// A pointer to a SECURITY_ATTRIBUTES structure
					// that contains two separate but related data members
					// INULL : the handle returned
					// by CreateFile cannot be inherited by any child processes the application
		__in     DWORD dwCreationDisposition,
		__in     DWORD dwFlagsAndAttributes,
					// The file or device attributes and flags,
					// FILE_ATTRIBUTE_NORMAL being the most common default value for files.
		__in_opt HANDLE hTemplateFile
		);
	*/


	HANDLE hThread[2];

	hThread[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartFileMonitor, NULL, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartRegistryMonitor, NULL, 0, NULL);
	/*
	*** CreateThread
	Creates a thread to execute within the virtual address space of the calling process.
	To create a thread that runs in the virtual address space of another process,
	use the CreateRemoteThread function.

	WINBASEAPI
	__out_opt
	HANDLE
	WINAPI
	CreateThread(
		__in_opt  LPSECURITY_ATTRIBUTES lpThreadAttributes,
						// A pointer to a SECURITY_ATTRIBUTES structure that determines 
						// whether the returned handle can be inherited by child processes.
						// NULL : the handle cannot be inherited.
		__in      SIZE_T dwStackSize,
						// The initial size of the stack, in bytes.
						// 0 : the new thread uses the default size for the executable.
		__in      LPTHREAD_START_ROUTINE lpStartAddress,
		__in_opt __deref __drv_aliasesMem LPVOID lpParameter,
						// A pointer to a variable to be passed to the thread.
		__in      DWORD dwCreationFlags,
						// The flags that control the creation of the thread.
						// 0 : The thread runs immediately after creation.
		__out_opt LPDWORD lpThreadId
						// A pointer to a variable that receives the thread identifier.
						// NULL : the thread identifier is not returned.
		);
	*/


	WaitForMultipleObjects(2, (const HANDLE*)&hThread, TRUE, INFINITE);		// WinBash.h
	/*
	*** WaitForMultipleObjects
	Waits until one or all of the specified objects are
	in the signaled state or the time-out interval elapses.

	WINBASEAPI
	DWORD
	WINAPI
	WaitForMultipleObjects(
		__in DWORD nCount,
		__in_ecount(nCount) CONST HANDLE *lpHandles,
		__in BOOL bWaitAll,
					// TRUE : the function returns when the state of all objects
					// in the lpHandles array is signaled.
		__in DWORD dwMilliseconds
					// The time-out interval, in milliseconds.
					// INFINITE : the function will return only when the specified objects are signaled.
		);
	*/


	TerminateThread(g_hRegWatch[0], 0);
	TerminateThread(g_hRegWatch[1], 0);
	/*
	*** TerminateThread
	WINBASEAPI
	BOOL
	WINAPI
	TerminateThread(
		__in HANDLE hThread,
		__in DWORD dwExitCode
		);

	TerminatedThread�� ����ϸ� �����尡 ����� �������� �ʴ´�.
	��κ� ������ ��쿡�� ����ؾ� �ϴ� ������ �Լ���.
	�����ϰ� �����ϱ� ���� ��� :
		1. CreateEvent �Լ��� ����Ͽ� �̺�Ʈ ��ü�� �����.
		2. �����带 �����.
		3. �� ������� WaitForSingleObject �Լ��� ȣ���Ͽ� �̺�Ʈ ���¸� ����͸��Ѵ�.
		4. �� ������� �̺�Ʈ�� ��ȣ�� �޴� ���� (WaitForSingleObjtect)�� WAIT_OBJECT_0�� ��ȯ)�� �����Ǿ��� �� ������ �����Ѵ�.
	*/


	CloseHandle(g_hStopEvent);
	CloseHandle(g_hFile);
	/*
	WINBASEAPI
	BOOL
	WINAPI
	CloseHandle( __in HANDLE hObject );
	*/

	_tprintf(_T("Program terminating.\n"));
}