#include "summoner.h"
#include "lowlvl.h"
#include <TlHelp32.h>

unsigned long get_module(unsigned long pid, char *module_name) {
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	MODULEENTRY32 me32;
	me32.dwSize = sizeof(MODULEENTRY32);

	while (Module32Next(snapshot, &me32)) {
		if (strcmp(me32.szModule, module_name) == 0) {
			return (unsigned long)me32.modBaseAddr;
		}
	} return NULL;
}

unsigned long ARGB(int iR, int iG, int iB, int iA) {
	return ((iA * 256 + iR) * 256 + iG) * 256 + iB;
}

float maxHngAimX = 6.283081667f;

float getOldDaysAim(float newAim) {
	if (newAim < 0) {
		while (newAim < maxHngAimX *-1) newAim += maxHngAimX;
		return maxHngAimX - newAim;
	} else {
		while (newAim > maxHngAimX) newAim -= maxHngAimX;
		return maxHngAimX - newAim;
	}
}

double hngAimToDegree(float aim) {
	return 360 - (360 / maxHngAimX * aim);
}

double hngAimToDegreeY(float aim) {
	return 180 / (1.39626348f *2) * aim + 90;
}

float degreeToHngAim(float aim) {
	return maxHngAimX / 360 * aim;
}

float degreeToHngAimY(float aim) {
	return (1.39626348f *2) / 180 * aim - 1.39626348f;
}

// {{{ ----- low lvl opening, reading writing virtual memory
DWORD dwWrite, dwOpen, dwRead, dwGPA, dwPVM;
BYTE b_ZwReadVirtualMemory[33], b_ZwWriteVirtualMemory[33], b_ZwOpenProcess[33], b_MOD_GetProcAddress[45], b_MOD_ProtectVirtualMemory[45];

_declspec(naked) ULONG __stdcall ZwReadVirtualMemory(
                     HANDLE		ProcessHandle,
                     PVOID		BaseAddress,
                     PVOID		Buffer,
                     ULONG		BufferSize,
                     PULONG		NumberOfBytesWritten
                     ) {
	__asm jmp dwRead
}

_declspec(naked) ULONG __stdcall ZwWriteVirtualMemory(
                     HANDLE		ProcessHandle,
                     PVOID		BaseAddress,
                     PVOID		Buffer,
                     ULONG		BufferSize,
                     PULONG		NumberOfBytesWritten
                     ) {
	__asm jmp dwWrite
}

_declspec(naked) ULONG __stdcall ZwOpenProcess(
					_Out_		PHANDLE				ProcessHandle,
					_In_		ACCESS_MASK			DesiredAccess,
					_In_		POBJECT_ATTRIBUTES	ObjectAttributes,
					_In_opt_	PCLIENT_ID			ClientId
					) {
	__asm jmp dwOpen
}

_declspec(naked) ULONG __stdcall MOD_GetProcAddress(
					HMODULE		modlue,
					char		*functionName
					) {
	__asm jmp dwGPA
}

_declspec(naked) ULONG __stdcall MOD_ProtectVirtualMemory(
					HANDLE	ProcessHandle,
					void	**BaseAddress,
					DWORD	*NumberOfBytesToProtect,
					DWORD	NewAccessProtection,
					DWORD	*OldAccessProtection
					) {
	__asm jmp dwPVM
}

void *VZwOpenProcess(unsigned long pid) {
	void *handle;

	OBJECT_ATTRIBUTES objAttr;
	CLIENT_ID cID;

	InitializeObjectAttributes(&objAttr, NULL, 0, NULL, NULL);

	cID.UniqueProcess = (PVOID)pid;
	cID.UniqueThread = 0;

	ZwOpenProcess(&handle, PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, &objAttr, &cID);

	return handle;
}

void init_global_function() {
	unsigned long oldProtect = 0;
	VirtualProtectEx(GetCurrentProcess(), (void *)(b_ZwReadVirtualMemory), 33 + 33 + 33 + 45 + 45, PAGE_EXECUTE_READWRITE, &oldProtect);

	HMODULE ntdll = GetModuleHandleA("ntdll.dll");

	memcpy(b_MOD_GetProcAddress, (void*)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "GetProcAddress"), 45);
	dwGPA = (unsigned long)b_MOD_GetProcAddress;

	memcpy(b_ZwReadVirtualMemory, (void*)MOD_GetProcAddress(ntdll, "ZwReadVirtualMemory"), 33);
	dwRead = (unsigned long)b_ZwReadVirtualMemory;

	memcpy(b_ZwWriteVirtualMemory, (void*)MOD_GetProcAddress(ntdll, "ZwWriteVirtualMemory"), 33);
	dwWrite = (unsigned long)b_ZwWriteVirtualMemory;

	memcpy(b_ZwOpenProcess, (void*)MOD_GetProcAddress(ntdll, "ZwOpenProcess"), 33);
	dwOpen = (unsigned long)b_ZwOpenProcess;

	memcpy(b_MOD_ProtectVirtualMemory, (void*)MOD_GetProcAddress(ntdll, "NtProtectVirtualMemory"), 45);
	dwPVM = (unsigned long)b_MOD_ProtectVirtualMemory;
}
/// ----- }}}