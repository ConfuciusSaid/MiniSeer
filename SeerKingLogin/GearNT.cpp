#include "stdafx.h"
#include "GearNT.h"

float fRange = 1.0f;

LPCRITICAL_SECTION g_lpcs;

namespace hook_timeGetTime{
	void hook_timeGetTime(void);
	DWORD dwStartReal = 0;
	DWORD dwStartFake = 0;
	DWORD dwTmp = 0;
	BOOL bChangeSpeed = TRUE;
}

namespace hook_GetTickCount{
	void hook_GetTickCount(void);
	DWORD dwStartReal = 0;
	DWORD dwStartFake = 0;
	DWORD dwTmp = 0;
}
namespace hook_QueryPerformanceCounter{
	BOOL WINAPI hook_QueryPerformanceCounter(PLARGE_INTEGER pli);
	LARGE_INTEGER liStartReal = { 0, 0 };
	LARGE_INTEGER liStartFake = { 0, 0 };
	LARGE_INTEGER liTmp = { 0, 0 };
}



void pfnRealGetTickCount(void){
	__asm{
		push ecx
		call MessageBoxA
		pop ecx
		retn
		nop
	}
}
void pfnRtlQueryPerformanceCounter(void){
	__asm{
		mov edi, edi
		push ebp
		mov ebp, esp
		pop ebp
		jmp MessageBoxA
		nop
	}
}
void pfnRealtimeGetTime(void){
	__asm{
		push    0
			push    0
			push    0x76CDE3D0
			push    0x76D708CC
			call    MessageBoxA
			test    eax, eax
			je      LABEL2
			mov     edx, 0x7FFE000C
			push    ebx
			push    edi
			lea     edi, dword ptr[edx - 4]
			lea     ebx, dword ptr[edx + 4]
		LABEL1:
			mov     eax, dword ptr[edx]
				mov     ecx, dword ptr[edi]
				cmp     eax, dword ptr[ebx]
				jnz     LABEL1
				sub     ecx, dword ptr[0x76D70720]
				push    0
				sbb     eax, dword ptr[0x76D70724]
				push    0x2710
				push    eax
				push    ecx
				call    MessageBoxA
				add     eax, dword ptr[0x76D70728]
				pop     edi
				pop     ebx
				retn
			LABEL2 : 
			jmp     MessageBoxA
	}
}


BYTE arBytes[8] = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0, 0x90 };

void InitTimeValue(){
	hook_GetTickCount::dwStartReal = GetTickCount();
	hook_GetTickCount::dwStartFake = hook_GetTickCount::dwStartReal;
	QueryPerformanceCounter(&hook_QueryPerformanceCounter::liStartReal);
	hook_QueryPerformanceCounter::liStartFake = hook_QueryPerformanceCounter::liStartReal;
	hook_timeGetTime::dwStartReal = timeGetTime();
	hook_timeGetTime::dwStartFake = hook_timeGetTime::dwStartReal;
}
BOOL WINAPI AddHook(){

	LPVOID lpAddr = 0;
	

	EnterCriticalSection(g_lpcs);

	InitTimeValue();

	DWORD dwProtect;/*
	VirtualProtect(pfnRealGetTickCount, 9, PAGE_EXECUTE_READWRITE, 0);
	VirtualProtect(pfnRtlQueryPerformanceCounter, 12, PAGE_EXECUTE_READWRITE, 0);*/


	HMODULE hMod = GetModuleHandleA("kernel32.dll");
	//hook in kernel32.dll
	lpAddr = GetProcAddress(hMod, "GetTickCount");
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)pfnRealGetTickCount, lpAddr, 9, 0);
	*(DWORD*)&arBytes[1] = (DWORD)hook_GetTickCount::hook_GetTickCount;

	WriteProcessMemory(GetCurrentProcess(), (LPVOID)lpAddr, arBytes, 7, 0);

	lpAddr = GetProcAddress(hMod, "QueryPerformanceCounter");
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)pfnRtlQueryPerformanceCounter, lpAddr, 12, 0);
	*(DWORD*)&arBytes[1] = (DWORD)hook_QueryPerformanceCounter::hook_QueryPerformanceCounter;

	WriteProcessMemory(GetCurrentProcess(), (LPVOID)lpAddr, arBytes, 7, 0);
	
	hMod = GetModuleHandleA("winmm.dll");
	//hook in winmm.dll
	lpAddr = GetProcAddress(hMod, "timeGetTime");
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)pfnRealtimeGetTime, lpAddr, 86, 0);
	*(DWORD*)&arBytes[1] = (DWORD)hook_timeGetTime::hook_timeGetTime;

	WriteProcessMemory(GetCurrentProcess(), (LPVOID)lpAddr, arBytes, 7, 0);

	LeaveCriticalSection(g_lpcs);

	return TRUE;
}

void SetCriticalSection(LPCRITICAL_SECTION lpcs){
	g_lpcs = lpcs;
}
void SetRange(float f){
	
	if (f == fRange)return;

	DWORD dwStartReal2 = ((PROC)pfnRealGetTickCount)();
	DWORD dwStartFake2 = GetTickCount();

	DWORD dwStartReal3 = ((PROC)pfnRealtimeGetTime)();
	DWORD dwStartFake3 = timeGetTime();

	LARGE_INTEGER liStartReal2;
	LARGE_INTEGER liStartFake2;
	PLARGE_INTEGER pli = &liStartReal2;
	__asm{
		push pli
		call pfnRtlQueryPerformanceCounter
	}
	QueryPerformanceCounter(&liStartFake2);

	EnterCriticalSection(g_lpcs);
	hook_GetTickCount::dwStartReal = dwStartReal2;
	hook_GetTickCount::dwStartFake = dwStartFake2;
	hook_QueryPerformanceCounter::liStartReal = liStartReal2;
	hook_QueryPerformanceCounter::liStartFake = liStartFake2;
	hook_timeGetTime::dwStartReal = dwStartReal3;
	hook_timeGetTime::dwStartFake = dwStartFake3;

	fRange = f;
	LeaveCriticalSection(g_lpcs);
}

void hook_GetTickCount::hook_GetTickCount(void){
	__asm{
		push g_lpcs
			call EnterCriticalSection

			call pfnRealGetTickCount
			sub eax, dwStartReal
			mov dwTmp, eax
			fild dwTmp
			fmul fRange
			fistp dwTmp
			wait
			mov eax, dwTmp
			add eax, dwStartFake
			push eax

			push g_lpcs
			call LeaveCriticalSection

			pop eax
	}
}

BOOL WINAPI hook_QueryPerformanceCounter::hook_QueryPerformanceCounter(PLARGE_INTEGER pli){
	__asm{
		push g_lpcs
		call EnterCriticalSection

		push pli
		call pfnRtlQueryPerformanceCounter
	}
	*(PLONGLONG)((LPVOID)pli) -= *(PLONGLONG)((LPVOID)&liStartReal);
	*(PLONGLONG)((LPVOID)pli) *= fRange;
	*(PLONGLONG)((LPVOID)pli) += *(PLONGLONG)((LPVOID)&liStartFake);
	__asm{
		push g_lpcs
		call LeaveCriticalSection
	}
	return TRUE;
}


void EnableCountDownChange(BOOL bChange){
	EnterCriticalSection(g_lpcs);
	hook_timeGetTime::bChangeSpeed = bChange;
	LeaveCriticalSection(g_lpcs);
}
void hook_timeGetTime::hook_timeGetTime(void){
	__asm{
		push g_lpcs
			call EnterCriticalSection

			call pfnRealtimeGetTime

			sub eax, dwStartReal

			push ecx
			mov ecx, bChangeSpeed
			test ecx, ecx
			je LABEL

			mov dwTmp, eax
			fild dwTmp
			fmul fRange
			fistp dwTmp
			wait
			mov eax, dwTmp
		LABEL :
		pop ecx
			add eax, dwStartFake

			push eax

			push g_lpcs
			call LeaveCriticalSection

			pop eax
	}
}