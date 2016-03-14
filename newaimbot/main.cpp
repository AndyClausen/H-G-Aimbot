#include "summoner.h"
#include "lowlvl.h"
#include <stdio.h>
#include <vector>
#include <math.h>

#define PI 3.14159265
#define KEY_AIM 0x43	// Key to activate aimbot in hex

HANDLE handle;
DWORD hng;
HWND hw_hng;

// enemies
struct enemyStruct {
	float x, y, z;
	//double aim_x;
	DWORD dword;
	double degree, degreeY;
	double degreeToAim, degreeToAimY;
	double faraway;
	int hp;
	int team;
};
DWORD unassEnemiesDword[800];
std::vector<enemyStruct> assEnemies;
enemyStruct local_player;
int radarPos[2] = {200, 100};

// Find is enemy  in our array
bool isThereEnemy(DWORD target_value, std::vector<enemyStruct> vector) {
	for (unsigned short i = 0; i < vector.size(); i++) {
		if (vector[i].dword == target_value) return true;
	} return false;
}
// little overflow
bool isThereEnemy(DWORD target_value, DWORD dwArray[500], int *pos) {
	unsigned short i;
	for (i = 0; dwArray[i] != 0; i++) {
		if (dwArray[i] == target_value) return true;
	} 
	*pos = i;
	return false;
}

HDC hdc_hng;
HBRUSH blueBrush;
HBRUSH greenBrush;
HBRUSH redBrush;
HBRUSH blackBrush;
HBRUSH whiteBrush;

void draw_player(LONG x, LONG z, HBRUSH brush) {
	RECT rect = { x, z, x +3, z +3 };
	FillRect(hdc_hng, &rect, brush);
}

float maxHngAimX = 6.283081667f;
int aimLineLen = 17;

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
	return 360 / maxHngAimX * aim;
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

BYTE turnToAddr[24];
DWORD dwTurnToAddr;

_declspec(naked) ULONG __stdcall callTurnToAddr() { __asm jmp dwTurnToAddr }

// 28 - x foat
// 2C - y float
// 30 - z float
// 5C - hp int
// 40 - aim x
// 48 - aim y
// C0 - team vlue

DWORD WINAPI _searchSim(LPVOID) {
	for (;; Sleep(10)) {
		/* fining all object htat has collision check
		unsigned short loopSize = 0;

		// getting for loop size
		DWORD pointed;
		ReadProcessMemory(handle, (LPVOID)(hng + 0xB1CAEC), &pointed, 4, 0);
		ReadProcessMemory(handle, (LPVOID)(pointed + 0x10008), &loopSize, 2, 0);

		for (unsigned short i = 0; i < loopSize +1; i++) {
			ReadProcessMemory(handle, (LPVOID)(hng + 0xB1CAEC), &pointed, 4, 0);
			pointed = pointed + i *8;
			ReadProcessMemory(handle, (LPVOID)(pointed + 0xC), &pointed, 4, 0);

			BYTE buffer[0x30];
			ReadProcessMemory(handle, (LPVOID)(pointed + 0x40), &buffer, 0x30, 0); // slow

			float	x = *(float*)(buffer),
					y = *(float*)(buffer +4),
					z = *(float*)(buffer +8);
			int		visibleValue = *(int*)(buffer + 0x28);

			//if (visibleValue == 6) printf("%x\n", pointed);
			//for (unsigned short j = 0; j < assEnemies.size(); j++) {
			//	if (x == assEnemies[j].x || y == assEnemies[j].y || z == assEnemies[j].z) printf("found\n");
			//}
		}
		*/
		/* finding all players
		DWORD start_of_struct;

		ReadProcessMemory(handle, (LPVOID)(hng + 0x63057C), &start_of_struct, 4, 0);
		ReadProcessMemory(handle, (LPVOID)(start_of_struct + 0x28), &start_of_struct, 4, 0);

		for (unsigned int i = 0; i < 0x1C; i++) {
			DWORD someConst = 0xE, _ecx;

			__asm {
				mov eax, 0x1C
				imul eax, someConst, 0x2A8
				mov someConst, eax
			}

			ReadProcessMemory(handle, (LPVOID)(someConst + start_of_struct + 0xBD0), &_ecx, 4, 0);
			ReadProcessMemory(handle, (LPVOID)(_ecx + i *4), &_ecx, 4, 0);
			ReadProcessMemory(handle, (LPVOID)(_ecx + 0x14), &_ecx, 4, 0);

			__asm {
				mov eax, _ecx
				call callTurnToAddr
				mov _ecx, eax
			}

			ReadProcessMemory(handle, (LPVOID)(_ecx + 0x3ccc), &_ecx, 4, 0);

			// checking hp
			ReadProcessMemory(handle, (LPVOID)(_ecx + 0x5C), &_ecx, 4, 0);
			if (_ecx != 0) printf("%i\n", _ecx);
		}*/
	}
}

// Aimbot thread
DWORD WINAPI _aimbotThread(LPVOID) {
	//Precision/aiming vars
	float precX = 0.3f;	// Just get that x-axis right... It's not much off, but a little left (I think)
	float duckY = 10;	// Regulate y-axis if enemy is ducking (remember to only focus on closest when using this var)
	float jumpY = 0;	// Not necessary... Would be cool to have too though.
	float runningX = 0;	// Regulate x-axis if enemy is running
						// any other regulations...? breathing perhaps?

	DWORD aiming_player = 0;

	for (;; Sleep(10)) {
		// clearing view
		//RECT rect = { 0, 0, 700, 400 };
		//FillRect(hdc_hng, &rect, blackBrush);

		DWORD orrandomdw;

		float my_aim_x;
		ReadProcessMemory(handle, (LPVOID)(hng + 0x63057C), &orrandomdw, 4, 0);
		ReadProcessMemory(handle, (LPVOID)(orrandomdw + 0x4c8), &my_aim_x, 4, 0);

		for (unsigned int i = 0; unassEnemiesDword[i] != 0; i++) {
			BYTE buffer[0xC0 +4];
			ReadProcessMemory(handle, (LPVOID)(unassEnemiesDword[i]), &buffer, 0xC0 +4, 0); // slow

			float	x = *(float*)(buffer + 0x28),
					y = *(float*)(buffer + 0x28 +4),
					z = *(float*)(buffer + 0x28 +8);
			int		hp = *(int*)(buffer + 0x5C);
			float	aim_x = *(float*)(buffer + 0x40),
					aim_y = *(float*)(buffer + 0x48);
			int		team = *(int*)(buffer + 0xC0);
			//int		stopCheck = *(int*)(buffer + 0x50);

			if (x != 0.f && hp > 255 && aim_x != 0.f) {
				//if (isThereEnemy(dw, assEnemies) == false) {
					enemyStruct use;
					use.dword = unassEnemiesDword[i];
					use.x = x;
					use.y = y;
					use.z = z;
					use.degree = hngAimToDegree(getOldDaysAim(aim_x));
					use.degreeY = hngAimToDegreeY(aim_y);
					use.hp = hp;
					use.team = team;
					assEnemies.push_back(use);

					if (my_aim_x == aim_x) {
						local_player.dword = unassEnemiesDword[i];
						local_player.x = x;
						local_player.y = y;
						local_player.z = z;
						local_player.degree = 360 - use.degree;
						local_player.degreeY = use.degreeY;
						local_player.team = team;
					}
				//}
			}
		}

		int closest = -1;
		double closestDist = 180;

		// drawing radar, and filling vector
		for (unsigned short i = 0; i < assEnemies.size(); i++) {
			if (assEnemies[i].dword == local_player.dword) {
				//draw_player(radarPos[0], radarPos[1], greenBrush);

				// {{{ ----- drawing point where local player is looking
				//float adjance = (float)std::cos(assEnemies[i].degree * PI / 180.0) * aimLineLen;
				//float opposite = (float)std::sin(assEnemies[i].degree * PI / 180.0) * aimLineLen;

				//draw_player(radarPos[0] - opposite, radarPos[1] + adjance, blueBrush);
				// ----- }}}
			} else if (assEnemies[i].team != local_player.team) {
				//draw_player(radarPos[0] + assEnemies[i].x - local_player.x,					// drawing enemy x around local player
				//			radarPos[1] + assEnemies[i].z - local_player.z, redBrush);		// drawing enemy y around local player
				
				double adjance = std::abs(local_player.z - assEnemies[i].z);
				double opposite = std::abs(local_player.x - assEnemies[i].x);
				double adjanceY = std::abs(local_player.y - assEnemies[i].y);
				assEnemies[i].faraway = std::sqrt((adjance * adjance) + (opposite * opposite));

				if (assEnemies[i].x < local_player.x && assEnemies[i].z > local_player.z) { // 270 to 360
					assEnemies[i].degreeToAim = 360 - std::atan2(opposite, adjance) / PI * 180.0;
				} else if (assEnemies[i].x < local_player.x && assEnemies[i].z < local_player.z) { // 180 to 270
					assEnemies[i].degreeToAim = 270 - std::atan2(adjance, opposite) / PI * 180.0;
				} else if (assEnemies[i].x > local_player.x && assEnemies[i].z < local_player.z) { // 90 to 180
					assEnemies[i].degreeToAim = 90 + std::atan2(adjance, opposite) / PI * 180.0;
				} else if (assEnemies[i].x > local_player.x && assEnemies[i].z > local_player.z) { // 0 to 90
					assEnemies[i].degreeToAim = std::atan2(opposite, adjance) / PI * 180.0;
				}

				if (assEnemies[i].y < local_player.y) {
					assEnemies[i].faraway -= assEnemies[i].faraway * 0.1115081081081081;
					assEnemies[i].degreeToAimY = 180 - std::atan2(assEnemies[i].faraway, adjanceY) / PI * 180.0;
				} else {
					assEnemies[i].faraway -= assEnemies[i].faraway * 0.1035081081081081;
					assEnemies[i].degreeToAimY = std::atan2(assEnemies[i].faraway, adjanceY) / PI * 180.0;
				}

				assEnemies[i].degreeToAim += precX / (assEnemies[i].faraway / 32);

				// checking if were already got enemy to aim and is it still in same pos in array
				if (aiming_player == assEnemies[i].dword && std::abs(assEnemies[i].degreeToAim - local_player.degree) < 2) {
					closest = i;
					break;
				}

				// {{{ ----- finding closest one
				double newAim = 180 - local_player.degree;
				double curAngle = assEnemies[i].degreeToAim;

				double curDist = curAngle + newAim;
				if (curDist > 360) curDist -= 360;

				if (curDist > 180) curDist -= 180;
				else curDist = 180 - std::abs(curDist);

				curDist += std::abs(local_player.degreeY - assEnemies[i].degreeToAimY);

				if (curDist < closestDist) {
					closest = i;
					closestDist = curDist;
				}
				// ----- }}}
			}
		}

		// doing aiming
		if ((GetAsyncKeyState(KEY_AIM) & 0x8009) && closest != -1) {
			float writeX = degreeToHngAim((float)assEnemies[closest].degreeToAim);
			float writeY = degreeToHngAimY((float)assEnemies[closest].degreeToAimY);

			ReadProcessMemory(handle, (LPVOID)(hng + 0x63057C), &orrandomdw, 4, 0);
			WriteProcessMemory(handle, (LPVOID)(orrandomdw + 0x4c8), &writeX, 4, 0);
			WriteProcessMemory(handle, (LPVOID)(orrandomdw + 0x4cc), &writeY, 4, 0);			

			//if (hw_hng == GetForegroundWindow()) {
				if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
					PostMessage(hw_hng, WM_KEYDOWN, VK_LBUTTON, MapVirtualKey(VK_LBUTTON, MAPVK_VK_TO_VSC));
					Sleep(2);
					PostMessage(hw_hng, WM_KEYUP, VK_LBUTTON, MapVirtualKey(VK_LBUTTON, MAPVK_VK_TO_VSC));
				}
			//}

			aiming_player = assEnemies[closest].dword;
			printf("%x\n", assEnemies[closest].dword);
			//printf("%i\n",  assEnemies[closest].hp);
		} else if (aiming_player != 0) {
			aiming_player = 0;
		}
		
		// clearing signed enemyes to get new ones to deal with
		assEnemies.clear();
	}

	return 0;
}

int main() {
	hw_hng = FindWindowA(0, "H&G");

	if (hw_hng) {
		hdc_hng = GetDC(GetConsoleWindow());
		blueBrush = CreateSolidBrush(RGB(0, 0, 255));
		greenBrush = CreateSolidBrush(RGB(0, 255, 0));
		redBrush = CreateSolidBrush(RGB(255, 0, 0));
		blackBrush = CreateSolidBrush(RGB(0, 0, 0));
		whiteBrush = CreateSolidBrush(RGB(255, 255, 255));

		DWORD pid;
		GetWindowThreadProcessId(hw_hng, &pid);

		hng = get_module(pid, "_hng.exe");
		if (hng == NULL) get_module(pid, "hng.exe");
		handle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, false, pid);

		if (handle) {
			DWORD oldProtect = 0;

			// filling turnToAddr function
			ReadProcessMemory(handle, (LPVOID)(hng + 0x174CA2), &turnToAddr, 24, 0);
			dwTurnToAddr = (DWORD)turnToAddr;
			// making function turnToAddr available
			VirtualProtectEx(GetCurrentProcess(), (LPVOID)(turnToAddr), 24, PAGE_EXECUTE_READWRITE, &oldProtect);

			// making gun no wobble
			VirtualProtectEx(handle, (LPVOID)(hng + 0x51C178), 4, PAGE_READWRITE, &oldProtect);
			WriteProcessMemory(handle, (LPVOID)(hng + 0x51C178), "\x00\x00\x00\x00", 4, 0);

			//DWORD target_addr = hng + 0x5B5084;

			// mov [someaddr], edi      write unto "useless" address
			//WriteProcessMemory(handle, (LPVOID)(hng + 0x1849A8), "\x89\x35", 2, 0);
			// writing dword so we get mov [target_addr], edi
			//WriteProcessMemory(handle, (LPVOID)(hng + 0x1849A8 + 2), &target_addr, 4, 0);
			//WriteProcessMemory(handle, (LPVOID)(hng + 0x1849A8 + 2 + 4), "\x90", 1, 0); // and finally 1 nop
			
			// creating thread thats going to fill enemy struct
			CreateThread(0, 0, _aimbotThread, 0, 0, 0);
			// creating thread to search similarities between col array and non col array
			//CreateThread(0, 0, _searchSim, 0, 0, 0);

			for (;; Sleep(10)) {
				DWORD start_of_struct;

				ReadProcessMemory(handle, (LPVOID)(hng + 0x63057C), &start_of_struct, 4, 0);
				ReadProcessMemory(handle, (LPVOID)(start_of_struct + 0x28), &start_of_struct, 4, 0);

				// getting target struct start value
				//DWORD target_value;
				int pos;

				//ReadProcessMemory(handle, (LPVOID)(target_addr), &target_value, 4, 0);
				//if (isThereEnemy(target_value, unassEnemiesDword, &pos) == false) {
				//	unassEnemiesDword[pos] = target_value;
				//}

				for (unsigned int i = 0; i < 0x1C; i++) {
					DWORD someConst = 0xE, _ecx;

					__asm {
						mov eax, 0x1C
						imul eax, someConst, 0x2A8
						mov someConst, eax
					}

					ReadProcessMemory(handle, (LPVOID)(someConst + start_of_struct + 0xBD0), &_ecx, 4, 0);
					ReadProcessMemory(handle, (LPVOID)(_ecx + i *4), &_ecx, 4, 0);
					ReadProcessMemory(handle, (LPVOID)(_ecx + 0x14), &_ecx, 4, 0);

					__asm {
						mov eax, _ecx
						call callTurnToAddr
						mov _ecx, eax
					}

					ReadProcessMemory(handle, (LPVOID)(_ecx + 0x3ccc), &_ecx, 4, 0);
					if (isThereEnemy(_ecx, unassEnemiesDword, &pos) == false) {
						unassEnemiesDword[pos] = _ecx;
					}
				}

				// check if hng is still alive
				if (FindWindowA(0, "H&G") == NULL) exit(0);
			}
		}
	}

	return 0;
}

