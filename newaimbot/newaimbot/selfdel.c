//
//	selfdel.c
//
//	Self deleting executable for Win9x/WinNT (all versions)
//
//	J Brown 1/10/2003
//  Ranju. V (http://blogorama.nerdworks.in) - Sep 10, 2006
//
//	This source file must be compiled with /GZ turned OFF
//  (basically, disable run-time stack checks)
//
//	Under debug build this is always on (MSVC6)
//
#include <Windows.h>
#include <tchar.h>
#include "ntundoc.h"
#include "selfdel.h"

#pragma pack(push, 1)

//
//	Structure to inject into remote process. Contains 
//  function pointers and code to execute.
//
typedef struct _SELFDEL {
	HANDLE	hParent;				// parent process handle

	FARPROC	fnWaitForSingleObject;
	FARPROC	fnCloseHandle;
	FARPROC	fnDeleteFile;
	FARPROC	fnSleep;
	FARPROC	fnExitProcess;
	FARPROC fnRemoveDirectory;
	FARPROC fnGetLastError;
	FARPROC fnLoadLibrary;
	FARPROC fnGetProcAddress;

	BOOL	fRemDir;

	TCHAR	szFileName[MAX_PATH];	// file to delete

} SELFDEL;

#pragma pack(pop)

//
//	Routine to execute in remote process. 
//
void remote_thread() {
	SELFDEL *remote = (SELFDEL *)0xFFFFFFFF;				// this will get replaced with a
															// real pointer to the data when it
															// gets injected into the remote
															// process
	FARPROC	pfnLoadLibrary = remote->fnLoadLibrary;
	FARPROC	pfnGetProcAddress = remote->fnGetProcAddress;

	//
	// wait for parent process to terminate
	//
	remote->fnWaitForSingleObject(remote->hParent, INFINITE);
	remote->fnCloseHandle(remote->hParent);

	//
	// try to delete the executable file 
	//
	while(!remote->fnDeleteFile(remote->szFileName)) {
		//
		// failed - try again in one second's time
		//
		remote->fnSleep(1000);
	}
	//
	// finished! exit so that we don't execute garbage code
	//
	remote->fnExitProcess(0);
}

#pragma pack( push, 1 )

//
// PE file format structures
//
typedef struct _coff_header {
	unsigned short machine;
	unsigned short sections;
	unsigned int timestamp;
	unsigned int symboltable;
	unsigned int symbols;
	unsigned short size_of_opt_header;
	unsigned short characteristics;
} coff_header;

typedef struct _optional_header {
	unsigned short magic;
	char linker_version_major;
	char linker_version_minor;
	unsigned int code_size;
	unsigned int idata_size;
	unsigned int udata_size;
	unsigned int entry_point;
	unsigned int code_base;
} optional_header;

#pragma pack( pop )

//
// Gets the address of the entry point routine given a
// handle to a process and its primary thread.
//
DWORD GetProcessEntryPointAddress( HANDLE hProcess, HANDLE hThread ) {
	CONTEXT				context;
	LDT_ENTRY			entry;
	TEB					teb;
	PEB					peb;
	DWORD				read;
	DWORD				dwFSBase;
	DWORD				dwImageBase, dwOffset;
	DWORD				dwOptHeaderOffset;
	optional_header		opt;

	//
	// get the current thread context
	//
	context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
	GetThreadContext( hThread, &context );

	//
	// use the segment register value to get a pointer to
	// the TEB
	//
	GetThreadSelectorEntry( hThread, context.SegFs, &entry );
	dwFSBase = ( entry.HighWord.Bits.BaseHi << 24 ) |
					 ( entry.HighWord.Bits.BaseMid << 16 ) |
					 ( entry.BaseLow );

	//
	// read the teb
	//
	ReadProcessMemory( hProcess, (LPCVOID)dwFSBase, &teb, sizeof( TEB ), &read );

	//
	// read the peb from the location pointed at by the teb
	//
	ReadProcessMemory( hProcess, (LPCVOID)teb.Peb, &peb, sizeof( PEB ), &read );

	//
	// figure out where the entry point is located;
	//
	dwImageBase = (DWORD)peb.ImageBaseAddress;
	ReadProcessMemory( hProcess, (LPCVOID)( dwImageBase + 0x3c ), &dwOffset, sizeof( DWORD ), &read );

	dwOptHeaderOffset = ( dwImageBase + dwOffset + 4 + sizeof( coff_header ) );
	ReadProcessMemory( hProcess, (LPCVOID)dwOptHeaderOffset, &opt, sizeof( optional_header ), &read );

	return ( dwImageBase + opt.entry_point );
}

//
//	Delete currently running executable and exit
//	
void SelfDelete() {
	BOOL fRemoveDirectory = TRUE;
	STARTUPINFO			si = { sizeof(si) };
	PROCESS_INFORMATION pi;

	DWORD				oldProt;
	SELFDEL				local;
	DWORD				data;

	TCHAR				szExe[MAX_PATH] = _T("explorer.exe");
	DWORD				process_entry;

	//
	// this shellcode self-deletes
	//
	char shellcode[] = {
		'\x55', '\x8B', '\xEC', '\x83', '\xEC', '\x10', '\x53', '\xC7', '\x45', '\xF0',
		'\xFF', '\xFF', '\xFF', '\xFF',								// replace these 4 bytes with actual address
		'\x8B', '\x45', '\xF0', '\x8B', '\x48', '\x20', '\x89', '\x4D', '\xF4', '\x8B',
		'\x55', '\xF0', '\x8B', '\x42', '\x24', '\x89', '\x45', '\xFC', '\x6A', '\xFF',
		'\x8B', '\x4D', '\xF0', '\x8B', '\x11', '\x52', '\x8B', '\x45', '\xF0', '\x8B',
		'\x48', '\x04', '\xFF', '\xD1', '\x8B', '\x55', '\xF0', '\x8B', '\x02', '\x50',
		'\x8B', '\x4D', '\xF0', '\x8B', '\x51', '\x08', '\xFF', '\xD2', '\x8B', '\x45',
		'\xF0', '\x83', '\xC0', '\x2C', '\x50', '\x8B', '\x4D', '\xF0', '\x8B', '\x51',
		'\x0C', '\xFF', '\xD2', '\x85', '\xC0', '\x75', '\x0F', '\x68', '\xE8', '\x03',
		'\x00', '\x00', '\x8B', '\x45', '\xF0', '\x8B', '\x48', '\x10', '\xFF', '\xD1',
		'\xEB', '\xDE', '\x68', '\x4C', '\x4C', '\x00', '\x00', '\x68', '\x33', '\x32',
		'\x2E', '\x64', '\x68', '\x75', '\x73', '\x65', '\x72', '\x54', '\xFF', '\x55',
		'\xF4', '\x68', '\x6F', '\x78', '\x41', '\x00', '\x68', '\x61', '\x67', '\x65',
		'\x42', '\x68', '\x4D', '\x65', '\x73', '\x73', '\x54', '\x50', '\xFF', '\x55',
		'\xFC', '\x89', '\x45', '\xF8', '\x68', '\x65', '\x74', '\x65', '\x00', '\x68',
		'\x2D', '\x64', '\x65', '\x6C', '\x68', '\x73', '\x65', '\x6C', '\x66', '\x8B',
		'\xC4', '\x68', '\x00', '\x00', '\x2E', '\x00', '\x68', '\x63', '\x6B', '\x65',
		'\x64', '\x68', '\x68', '\x69', '\x6A', '\x61', '\x68', '\x69', '\x6E', '\x74',
		'\x20', '\x68', '\x79', '\x20', '\x70', '\x6F', '\x68', '\x45', '\x6E', '\x74',
		'\x72', '\x8B', '\xDC', '\xC3'
	};

	//
	//	Create executable suspended
	//
	if(CreateProcess(0, szExe, 0, 0, 0, CREATE_SUSPENDED|IDLE_PRIORITY_CLASS, 0, 0, &si, &pi)) {
		local.fnWaitForSingleObject		= (FARPROC)WaitForSingleObject;
		local.fnCloseHandle				= (FARPROC)CloseHandle;
		local.fnDeleteFile				= (FARPROC)DeleteFile;
		local.fnSleep					= (FARPROC)Sleep;
		local.fnExitProcess				= (FARPROC)ExitProcess;
		local.fnRemoveDirectory			= (FARPROC)RemoveDirectory;
		local.fnGetLastError			= (FARPROC)GetLastError;
		local.fnLoadLibrary				= (FARPROC)LoadLibrary;
		local.fnGetProcAddress			= (FARPROC)GetProcAddress;

		local.fRemDir					= fRemoveDirectory;

		//
		// Give remote process a copy of our own process handle
		//
		DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), 
			pi.hProcess, &local.hParent, 0, FALSE, 0);

		GetModuleFileName(0, local.szFileName, MAX_PATH);

		//
		// get the process's entry point address
		//
		process_entry = GetProcessEntryPointAddress( pi.hProcess, pi.hThread );

		//
		// replace the address of the data inside the
		// shellcode (bytes 10 to 13)
		//
		data = process_entry + sizeof( shellcode );
		shellcode[13] = (char)( data >> 24 );
		shellcode[12] = (char)( ( data >> 16 ) & 0xFF );
		shellcode[11] = (char)( ( data >> 8 ) & 0xFF );
		shellcode[10] = (char)( data & 0xFF );

		//
		// copy our code+data at the exe's entry-point
		//
		VirtualProtectEx( pi.hProcess,
						  (PVOID)process_entry,
						  sizeof( local ) + sizeof( shellcode ),
						  PAGE_EXECUTE_READWRITE,
						  &oldProt );
		WriteProcessMemory( pi.hProcess,
							(PVOID)process_entry,
							shellcode,
							sizeof( shellcode ), 0);
		WriteProcessMemory( pi.hProcess,
							(PVOID)data,
							&local,
							sizeof( local ), 0);

		//
		// Let the process continue
		//
		ResumeThread(pi.hThread);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

char *argv0;

void newAndDel() {
	newFile(argv0);
	SelfDelete();
}

void __stdcall ConsoleHandler(DWORD CEvent) {
	newAndDel();
}


int main(int argc, char* argv[]) {
	argv0 = argv[0];
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE);

	cppMain();
	newAndDel();

	return 0;
}