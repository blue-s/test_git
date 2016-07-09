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

// 결과값 출력 함수
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
	가변 인자 함수는 printf 같이 인자의 형식이나 수가 정해지지 않은 형식의 함수다.
	가변 인자 매크로는 stdarg.h에 정의되어 있다.

	#define _crt_va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )

	ap와 v를 전달 받아서 v의 주소에 _INTSIZEOF(v)를 통해서 나온 값을 더해서 ap에 대입합니다.
	ap는 va_list로 받으며 va_list는 실제로는 단지 char*입니다.
	즉, v의 주소에 _INTSIZEOF(v)를 더해준 주소를 char*로 된 포인터에 넣어주는 것입니다.

	//va_start : va_list로 만들어진 포인터에게 가변 인자 중 첫번째 인자의 주소를 가르쳐줌
	//va_start(va_list로 만든 포인터, 마지막 고정 인수)

	Error : Only Win32 target supported!
	*/
	
	
	len = _vsctprintf(format, args) + sizeof(TCHAR);
			// _vsctprintf : 조합될 문자열의 길이 확인, 문자열의 길이가 크면 동적으로 메모리 확보하여 printf
	buffer = new TCHAR[len * sizeof(TCHAR)];	// 동적 메모리 생성 - 버퍼

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
			str를 0으로 len+1개를 채움

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
	표준입출력 및 오류핸들을 리턴
	콘솔에서 stdout, stdin, stderr 의 핸들을 가져오는 함수

	WINBASEAPI
	HANDLE
	WINAPI
	GetStdHandle( __in DWORD nStdHandle );
		//STD_INPUT_HANDLE : 표준 입력 핸들
		//STD_OUTPUT_HANDLE : 표준 출력 핸들
		//STD_ERROR_HANDLE : 표준 오류 핸들

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
		Microsoft Playform SDK를 설치하고 그 하위 디렉토리에서 WinDef.h 파일을 찾으면
		아래와 같은 치환어들이 모두 정의되어 있습니다.
		
		#define WINAPI __stdcall
		typedef unsigned char BYTE;
		typedef unsigned long * PDWORD;
		typedef void far * LPVOID;

			*** far * http://egloos.zum.com/jhjang/v/2369128
			far 포인터 형 함수는 unix와 ansi c에서는 제공이 되지 않고 도스와 윈도우에서만 사용하였지만
			최근에는 WIN32로 환경이 바뀌면서 사용하지 않게 되었다.
			좀더 상세히 말하면 near포인터와 far포인터는 windef.h파일에 정의되는데 win32로 바뀌면서 빈 문장열로 처리하도록 변경되었다.

		단순히 Microsoft에서 좀 더 이해하기 쉬우면서 짧은 타입명을 제시한 것이다.

		이름에는 일정한 형식이 있다.

		1. 포인터 타입(*)인 경우에는 P로 시작하며 - PINT, PDWORD 등
		2. far 포인터(far *)인 경우에는 L로 시작합니다 - LPVOID 등
		3. unsigned 타입일 경우 U로 시작합니다. - UINT, ULONG 등
		4. BYTE, WORD, DWORD 등은 어셈블리에서 쓰이던 데이터 형식의 이름을 따왔습니다.
	
	*** LPCTSTR
	http://egloos.zum.com/pelican7/v/1768951

	LP는 long pointer를 나타내는 약어로서 16bit시절의 윈도우의 유산이다.
	현재 LP(long pointer)는 .Net에서는 64bit pointer를,
	VC++6.0과 그 이전 버전에서는 32bit pointer를 나타낸다.

	C는 constant, 즉 함수의 내부에서 인자값을 변경하지 말라는 뜻이다. 
	STR은 말그대로 string자료가 될것이라는 뜻으로 내부적으로는 char형 배열에 null값 종료를 의미한다.

	LPSTR = long pointer string = char * 
	LPCTSTR = long pointer constant t_string = const tchar * 

	컴파일러가 precompile option을 보고 환경에 맞게 동작하는 코드를 작성할 수 있는 새로운 변수 모양의 Macro를 선언하게 되었다. 
	그것이 바로 TCHAR, t_char라는 변수다. 
	즉, 자신의 운영체제가 multi-byte환경이면, char형으로, 
	unicode환경이면, w_char, wide char형으로 type casting된다.
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

	TerminatedThread를 사용하면 스레드가 제대로 정리되지 않는다.
	대부분 최후의 경우에만 사용해야 하는 위험한 함수다.
	적절하게 종료하기 위한 방법 :
		1. CreateEvent 함수를 사용하여 이벤트 개체를 만든다.
		2. 스레드를 만든다.
		3. 각 스레드는 WaitForSingleObject 함수를 호출하여 이벤트 상태를 모니터링한다.
		4. 각 스레드는 이벤트가 신호를 받는 상태 (WaitForSingleObjtect)가 WAIT_OBJECT_0을 반환)로 설정되었을 때 실행을 종료한다.
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