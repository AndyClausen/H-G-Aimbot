#pragma once
#include <Windows.h>

DWORD get_module(DWORD pid, char *module_name);
unsigned long ARGB(int iR, int iG, int iB, int iA);
float getOldDaysAim(float newAim);
double hngAimToDegree(float aim);
double hngAimToDegreeY(float aim);
float degreeToHngAim(float aim);
float degreeToHngAimY(float aim);

ULONG __stdcall ZwWriteVirtualMemory(
	HANDLE	ProcessHandle,
	PVOID	BaseAddress,
	PVOID	Buffer,
	ULONG	BufferSize,
	PULONG	NumberOfBytesWritten
);

ULONG __stdcall ZwReadVirtualMemory(
	HANDLE	ProcessHandle,
	PVOID	BaseAddress,
	PVOID	Buffer,
	ULONG	BufferSize,
	PULONG	NumberOfBytesWritten
);

ULONG __stdcall MOD_ProtectVirtualMemory(
	HANDLE	ProcessHandle,
	void	**BaseAddress,
	DWORD	*NumberOfBytesToProtect,
	DWORD	NewAccessProtection,
	DWORD	*OldAccessProtection
);

void *VZwOpenProcess(unsigned long pid);
void init_global_function();