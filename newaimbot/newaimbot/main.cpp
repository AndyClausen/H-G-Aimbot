#include "selfdel.h"
#include "summoner.h"
#include <vector>
#include <math.h>
#include <cstdio>

#define PI 3.14159265
#define KEY_AIM 0x43																							// Key to activate aimbot in hex
#define ST_LIE 1
#define ST_START 2
#define ST_SPRINT 3
#define ST_CROUCH 4

HANDLE handle;																									// hng process handle, to edit, read, prot mem
DWORD hng;																										// hng base address
HWND hw_hng;																									// hng window handle, to draw points

struct enemyStruct {																							// struct that we use to fill vector
	float x, y, z;
	DWORD dword;
	double degree, degreeY;
	double degreeToAim, degreeToAimY;
	double faraway;
	int hp;
	int team;
	BYTE status;																								// 1 - lie, 2 - stay, 3 - sprint, 4 - crouch
	bool visible;
};
std::vector<enemyStruct> assEnemies;																			// vector where's enemies and local player
enemyStruct local_player;																						// local player info
int radarPos[2] = {200, 100};																					// radar position on window

HDC hdc_hng;																									// hng hdc from window handle
HBRUSH blueBrush;																								// some colors +
HBRUSH greenBrush;																								//             |
HBRUSH redBrush;																								//             |
HBRUSH blackBrush;																								//             |
HBRUSH whiteBrush;																								// ------------+

void draw_player(LONG x, LONG z, HBRUSH brush) {																// function to draw point to window, w: 3, h: 3
	RECT rect = { x, z, x +3, z +3 };																			// making rect
	FillRect(hdc_hng, &rect, brush);																			// drawing rect to screen
}

void draw_text(int x, int y, char *txt) {																		// function to draw text to window
	RECT rc = { x, y, x + 200, y + 20 };																		// making rectangle

	SetBkMode(hdc_hng, TRANSPARENT);																			// setting rect background mode
	SetBkColor(hdc_hng, ARGB(0, 255, 0, 0));																	// setting rect background color
	SetTextColor(hdc_hng, RGB(255, 0, 0));																		// setting text color
	DrawText(hdc_hng, txt, -1, &rc, DT_CENTER | DT_VCENTER | DT_EXPANDTABS);									// drawing text to screen
}

int aimLineLen = 17;																							// how far we're drawing looking coord

BYTE turnToAddr[24];																							// copied function from hng
DWORD dwTurnToAddr;																								// copied function address
_declspec(naked) ULONG __stdcall callTurnToAddr() { __asm jmp dwTurnToAddr }									// for calling copied function

BYTE getStatus(int v) {																							// function to get if player if crouching, lying, standing
	if (v==25182240||v==25182368||v==27279392||v==27279393||v==27279395||v==27279520) return 1;					// player is lying
	else if (v==81959456||v==41959584||v==44056608||v==44056736||v==81959424) return 4;							// player is crouching
	else if (v==8405536) return 3;																				// player is sprinting
	return 2;																									// player is standing
}

// 28	-	x foat
// 2C	-	y float
// 30	-	z float
// 5C	-	hp int
// 40	-	aim x
// 48	-	aim y
// C0	-	team vlue
// 10C	-	stand, lie, crouch check:
//				lie:	25182240(idle, ready to shot), 25182368(aiming), 27279392(turned), 27279393(turning left, moving), 27279395(turning right), 27279520(blocked & aiming)
//				stay:	8405024(idle, ready to shot), 8405152(aiming), 10502176(blocked), 10502304(blocked & aiming), 8405536(sprint)
//				jump:	8404992(idle, blocked), 81959424(crouch in air)
//				crouch:	81959456(idle), 41959584(aiming), 44056608(blocked), 44056736(blocked & aiming)

DWORD __stdcall _aimbotThread(LPVOID) {																			// |------------------------- Aimbot thread -------------------------|
	// Precision/aiming vars
	float precX = 0.3f;						// Just get that x-axis right... It's not much off, but a little left (I think)
	float duckY = 0.091f;					// Regulate y-axis if enemy is ducking (remember to only focus on closest when using this var)
	double lieY = 0;
	double normalYH = 0.1030081081081081;
	double normalYL = 0.1129081081081081;
	float runningX = 0;						// Regulate x-axis if enemy is running
											// any other regulations...? breathing perhaps?

	RECT client_rect;																							// holding window inner rect

	DWORD aiming_player = 0;																					// using to hold currently aiming player id(dword, player struct addr)
	DWORD someConst = 0xE, _ecx;																				// some variables use to get values like hng does

	unsigned long playerUL;																						// <------------------------------------------+
	ZwReadVirtualMemory(handle, (LPVOID)(hng + 0x63057C), &playerUL, 4, 0);										// reading some global struct start addr to --+
	MOD_ProtectVirtualMemory(handle, (void **)(playerUL + 0x4c8), (DWORD *)8, PAGE_READWRITE, NULL);			// making address available so ZwWriteVirtualmemory can edit mem

	for (;; Sleep(10)) {																						// infinite loop that sleeps 10 mili sec, not to use critically resource
		GetClientRect(hw_hng, &client_rect);																	// filling client_rect with hng window inner rectangle

		float my_aim_x;																							//       <--------------------------+
		ZwReadVirtualMemory(handle, (LPVOID)(playerUL + 0x4c8), &my_aim_x, 4, 0);								// reading local player aiming to --+

		unsigned long start_of_struct;																			//     <--------------------------------+
		ZwReadVirtualMemory(handle, (LPVOID)(playerUL + 0x28), &start_of_struct, 4, 0);							// reading struct addr holder start to -+

		unsigned int array_size = 1;																			// array size is 1 by default to do one loop at least, can crash 0.1%
		for (unsigned int i = 0; i < array_size; ) {
			// {{{ ----- getting player struct addr by loop index
			someConst = 0xE;																					// push E \n call -----------------+
			someConst = someConst * 0x2A8;																		// imul eax, [ebp + 8], 2A8 <------+------|
			ZwReadVirtualMemory(handle, (LPVOID)(someConst + start_of_struct + 0xBD0), &_ecx, 4, 0);			//                                        |
			ZwReadVirtualMemory(handle, (LPVOID)(_ecx + i *4), &_ecx, 4, 0);									//                                        |
			ZwReadVirtualMemory(handle, (LPVOID)(_ecx + 0x14), &_ecx, 4, 0);									//                                        |
			__asm {																								// little asm code too                    |
				mov eax, _ecx																					// moving variable _ecx to eax register   |
				call callTurnToAddr																				// call ing wierd func that uses eax      |
				mov _ecx, eax																					// and finaly returning eax to var _ecx   |
			}																									// ---------------------------------------+
			ZwReadVirtualMemory(handle, (LPVOID)(_ecx + 0x3ccc), &_ecx, 4, 0);									// getting player struct addr
			// ----- }}}

			BYTE buffer[0x110];																					// byte array where we read player struct
			ZwReadVirtualMemory(handle, (LPVOID)(_ecx), &buffer, 0x110, 0);										// slow function :P, so trying to not use it much

			float	x		= *(float*)(buffer + 0x28),															// reading x pos
					y		= *(float*)(buffer + 0x28 +4),														// reading y pos from byte array and type casting it with float
					z		= *(float*)(buffer + 0x28 +8);														// and reading z pos
			int		hp		= *(int*)(buffer + 0x5C);															// reading hp from byte array
			float	aim_x	= *(float*)(buffer + 0x40),															// reading x aiming(sloppy, transfering to degree after)
					aim_y	= *(float*)(buffer + 0x48);															// reading y aming
			int		team	= *(int*)(buffer + 0xC0);															// reading team vlue
			int		status	= *(int*)(buffer + 0x10C);															// and finally reading is this thing crouching, standing, ...

			if (x != 0.f && hp > 255 && aim_x != 0.f) {
				bool visible = false;
				// {{{ ----- searching colled array similarities to normal one
				/*for (DWORD _edi = 0; _edi < 0x1C; _edi++) {
					DWORD _edx, _esi;
					float colled_x;
					BYTE vis;

					ZwReadVirtualMemory(handle, (LPVOID)(hng + 0xB1CE84), &_edx, 4, 0);							// just getting edx
					ZwReadVirtualMemory(handle, (LPVOID)(_edx + _edi *4), &_esi, 4, 0);							// mov esi, [edx + edi *4]
					ZwReadVirtualMemory(handle, (LPVOID)(_esi + 0x30), &colled_x, 4, 0);
					ZwReadVirtualMemory(handle, (LPVOID)(_esi + 0x60), &vis, 1, 0);

					char normal[12], colled[12];
					memset(&normal, 0, 12); memset(&colled, 0, 12);

					sprintf_s(normal, "%.4f", x);
					sprintf_s(colled, "%.4f", colled_x);

					if (strcmp(normal, colled) == 0) {
						//printf("%x\n", _esi);
						visible = (vis == 0 ? false : true);
					}
				}*/
				// ----- }}}

				enemyStruct use;																				// getting struct to variable
				use.dword = _ecx;																				// filling variable with dowrd (this can be local player or enemy)
				use.x = x;																						// it's position --+
				use.y = y;																						//                 |
				use.z = z;																						// ----------------+
				use.degree = hngAimToDegree(getOldDaysAim(aim_x));												// getting it's degree where its looking, transfering hng x to degree
				use.degreeY = hngAimToDegreeY(aim_y);															// and so do y axis
				use.hp = hp;																					// getting player hp
				use.team = team;																				// and it's team value
				use.status = getStatus(status);																	// getting is he/she sitting, couching, ...
				use.visible = visible;																			// is he/she visible to us
				assEnemies.push_back(use);																		// and finally adding to vector(array that resizes)

				if (my_aim_x == aim_x) {																		// if got player is local player then
					local_player.dword = _ecx;																	// filling dword because its id here
					local_player.x = x;																			// it's x
					local_player.y = y;																			// it's y pos
					local_player.z = z;																			// and fillanly z
					local_player.degree = use.degree;															// getting its degree where we looking
					local_player.degreeY = use.degreeY;															// so do y axis to get nearest enemy to crosshair
					local_player.team = team;																	// getting team
					local_player.status = use.status;															// and getting are we sitting, couching, ...
				}
			}

			i++;																								// increasing array size counter

			// {{{ ----- getting array size																		// hng logic to get players array size
			someConst = 0xE;																					// push E \n call -----------------+
			someConst = someConst * 0x2A8;																		// imul eax, [ebp + 8], 2A8   <----+
			ZwReadVirtualMemory(handle, (LPVOID)(someConst + start_of_struct + 0xBD4), &_ecx, 4, 0);			// reading some data from hng      |
			ZwReadVirtualMemory(handle, (LPVOID)(someConst + start_of_struct + 0xBD0), &someConst, 4, 0);		// reading someconst from hng  <---|------+
			__asm {																								// --------------------------------+---+  |
				mov eax, _ecx																					// dealing in inline asm with eax only |  |
				sub eax, someConst																				// substracting eax by (someconst)-----|--+
				sar eax, 02																						// don't know sar in c                 |
				mov array_size, eax																				// moving eax to array_size            |
			}																									// ------------------------------------+
			// ----- }}}
		}

		int closest = -1;																						// by default its -1 so no one is selected
		double closestDist = 180;

		// drawing radar, and filling vector
		for (unsigned short i = 0; i < assEnemies.size(); i++) {												// loop that checks got players(enemies and local player)
			if (assEnemies[i].dword == local_player.dword) {													// if got player is local player then
				//draw_player(radarPos[0], radarPos[1], greenBrush);											// drawing local player

				// {{{ ----- drawing point where local player is looking
				//float adjance = (float)std::cos(std::abs(assEnemies[i].degree - 360) * PI / 180.0) * aimLineLen;
				//float opposite = (float)std::sin(std::abs(assEnemies[i].degree - 360) * PI / 180.0) * aimLineLen;

				//draw_player((LONG)(radarPos[0] - opposite), (LONG)(radarPos[1] + adjance), blueBrush);		// drawing looging coordinates
				// ----- }}}
			} else if (assEnemies[i].team != local_player.team) {												// if player in array(vector) isn't local player then
				//draw_player((LONG)radarPos[0] + (LONG)assEnemies[i].x - (LONG)local_player.x,					// drawing enemy x around local player
				//			(LONG)radarPos[1] + (LONG)assEnemies[i].z - (LONG)local_player.z,					// drawing enemy y around local player
				//			(assEnemies[i].visible == true ? redBrush : whiteBrush));							// choosing golor by enemy visiblity
				
				double adjance = std::abs(local_player.z - assEnemies[i].z);
				double opposite = std::abs(local_player.x - assEnemies[i].x);
				double adjanceY = std::abs(local_player.y - assEnemies[i].y);
				assEnemies[i].faraway = std::sqrt((adjance * adjance) + (opposite * opposite));

				if (assEnemies[i].x < local_player.x && assEnemies[i].z > local_player.z) {						// 270 to 360
					assEnemies[i].degreeToAim = 360 - std::atan2(opposite, adjance) / PI * 180.0;				// getting x degree from 0 point, from 4-th sector
				} else if (assEnemies[i].x < local_player.x && assEnemies[i].z < local_player.z) {				// 180 to 270
					assEnemies[i].degreeToAim = 270 - std::atan2(adjance, opposite) / PI * 180.0;				// getting x degree from 0 point, from 3-rd sector
				} else if (assEnemies[i].x > local_player.x && assEnemies[i].z < local_player.z) {				// 90 to 180
					assEnemies[i].degreeToAim = 90 + std::atan2(adjance, opposite) / PI * 180.0;				// getting x degree from 0 point, from 2-nd sector
				} else if (assEnemies[i].x > local_player.x && assEnemies[i].z > local_player.z) {				// 0 to 90
					assEnemies[i].degreeToAim = std::atan2(opposite, adjance) / PI * 180.0;						// getting x degree from 0 point, from 1-st sector
				}

				if (assEnemies[i].y < local_player.y) {																	// when enemy is lower than me(in y axis) then
					double faraway = assEnemies[i].faraway - assEnemies[i].faraway * normalYL;							// same as ----------------------+
					assEnemies[i].degreeToAimY = 180 - std::atan2(faraway, adjanceY) / PI * 180.0;						// but opposite                  |
																														//                               |
					assEnemies[i].faraway -= assEnemies[i].faraway * (normalYL - assEnemies[i].degreeToAimY / 8800);	//                               |
					assEnemies[i].degreeToAimY = 180 - std::atan2(assEnemies[i].faraway, adjanceY) / PI * 180.0;		//                               |
				} else {																								// or enemy is higher than me  --+-----------------------------------|
					double faraway = assEnemies[i].faraway - assEnemies[i].faraway * normalYH;							// getting y degree and fixing y aiming by how far is enemy
					assEnemies[i].degreeToAimY = std::atan2(faraway, adjanceY) / PI * 180.0;							// getting degree
																														// that degree is going off when is lower then 20 or bigger then 160
					assEnemies[i].faraway -= assEnemies[i].faraway * (normalYH + assEnemies[i].degreeToAimY / 8800);	// so doing littlebit fixing more by degree
					assEnemies[i].degreeToAimY = std::atan2(assEnemies[i].faraway, adjanceY) / PI * 180.0;				// getting degree again
				}

				if (assEnemies[i].status == ST_CROUCH)																	// when enemy is crouching then
					assEnemies[i].degreeToAimY += ((0.936 / assEnemies[i].faraway) * 100) /2;							// aiming littlebit lower than normaly
				else if (assEnemies[i].status == ST_LIE)																// or enemy lying then
					assEnemies[i].degreeToAimY += ((0.730 / assEnemies[i].faraway) * 100);								// moving y aim point down by enemy len(in y axis)

				assEnemies[i].degreeToAim += precX / (assEnemies[i].faraway / 32);										// moving aiming x axis litlebit left to fit nicely

				if (aiming_player == assEnemies[i].dword) {																// checking if we're already got enemy to aim then
					closest = i;																						// making it to closest
					break;																								// and ending this loop, to get ahead aiming process
				}

				//double dereeToPixels = client_rect.right / 180;
				//draw_text(	client_rect.right / 2 + (local_player.degree - assEnemies[i].degreeToAim) / 90 * (client_rect.right*.9) 
							//, client_rect.bottom / 2 - (local_player.degreeY - assEnemies[i].degreeToAimY) / 70 * (client_rect.bottom*.9), "x");


				// {{{ ----- finding closest one													// ask from andy i have no idea anymore ^^ // lol - Andy
				double newAim = 180 - local_player.degree;											// We pretend player aim = 180°			<------------------+
				double curAngle = assEnemies[i].degreeToAim;										// Current enemy's degree from ply pos					   |
																									//														   |
				double curDist = curAngle + newAim;													// We start checking distance here but first adding that 180°
				if (curDist > 360) curDist -= 360;													// We then make sure the degree isn't more than 360°

				if (curDist > 180) curDist -= 180;													// Then set the distance compared to which side the enemy
				else curDist = 180 - std::abs(curDist);												// is from player's aim, which we still act like is 180°

				curDist += std::abs(local_player.degreeY - assEnemies[i].degreeToAimY);				// Here, we put compare it with Y-axis too to get the best
																									// results

				if (curDist < closestDist) {														// Here, we check if the current enemy is the closest so far
					closest = i;																	// Sets closest var to current enemy's ID in array (kinda)
					closestDist = curDist;															// Aaand, we need to tell the program that there's a new
				}																					// closest enemy
				// ----- }}}													// Honestly, this is so much easier to understand if you actually draw it in paint
			}
		}

		// {{{ ----- doing aiming
		if ((GetAsyncKeyState(KEY_AIM) & 0x8000) && closest != -1) {
			float writeX = degreeToHngAim((float)assEnemies[closest].degreeToAim);
			float writeY = degreeToHngAimY((float)assEnemies[closest].degreeToAimY);

			ZwWriteVirtualMemory(handle, (LPVOID)(playerUL + 0x4c8), &writeX, 4, 0);							// writing x aiming
			ZwWriteVirtualMemory(handle, (LPVOID)(playerUL + 0x4cc), &writeY, 4, 0);							// writing where to aim in y axis

			if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {														// while holding left mouse button
				PostMessage(hw_hng, WM_KEYDOWN, VK_LBUTTON, MapVirtualKey(VK_LBUTTON, MAPVK_VK_TO_VSC));		// sending mouse down to hng
				Sleep(2);																						// sleeping too littlebit ^^
				PostMessage(hw_hng, WM_KEYUP, VK_LBUTTON, MapVirtualKey(VK_LBUTTON, MAPVK_VK_TO_VSC));			// and finally sending mouse up
			}

			aiming_player = assEnemies[closest].dword;									// saving got enemy address(struct start addr)
		} else if (aiming_player != 0) aiming_player = 0;								// if not aiming enymore then clearing saved enemy
		// ----- }}}

		assEnemies.clear();																// clearing signed enemyes to get new ones to deal with
	}																					//                         <-----------------+
}																						// no need to return because it's inf loop --+

int cppMain() {																			// |-------------- main function where this gets started --------------|
	hw_hng = FindWindowA(0, "H&G");

	if (hw_hng) {
		hdc_hng = GetDC(hw_hng);																				// getting hdc to draw this window
		blueBrush = CreateSolidBrush(RGB(0, 0, 255));															// getting some colors -+
		greenBrush = CreateSolidBrush(RGB(0, 255, 0));															//                      |
		redBrush = CreateSolidBrush(RGB(255, 0, 0));															//                      |
		blackBrush = CreateSolidBrush(RGB(0, 0, 0));															//                      |
		whiteBrush = CreateSolidBrush(RGB(255, 255, 255));														// ---------------------+

		unsigned long pid;																						// variable   <---------------------------+
		GetWindowThreadProcessId(hw_hng, &pid);																	// getting process id and writing it to --+

		hng = get_module(pid, "_hng.exe");																		// searching for _hng.exe(modded one)
		if (hng == NULL) get_module(pid, "hng.exe");															// searching for hng.exe(original one)

		init_global_function();																					// initializing low lvl functions
		handle = VZwOpenProcess(pid);																			// using one of low lvl function to open process

		if (handle) {
			ZwReadVirtualMemory(handle, (LPVOID)(hng + 0x174CA2), &turnToAddr, 24, 0);									// filling turnToAddr function
			dwTurnToAddr = (unsigned long)turnToAddr;
			MOD_ProtectVirtualMemory(GetCurrentProcess(), (void **)(turnToAddr), (DWORD *)24, PAGE_EXECUTE, NULL);		// making function turnToAddr available

			CreateThread(0, 0, _aimbotThread, 0, 0, 0);															// creating thread thats going through players

			for (; FindWindowA(0, "H&G") != NULL; Sleep(100));									// exiting this process when hng does not exist enymore in mem
		}
	}
	
	return 0;																				// if hng (not found)/(no privs)/(hng end) then ending this function
}