#include "stdafx.h"
#include "SoundControl.h"

BOOL bEnableSound = FALSE;

void RealwaveOutWrite(void){
	__asm{
		mov edi, edi
			push ebp
			mov ebp, esp
			jmp MessageBoxA
			nop
	}
}
MMRESULT WINAPI HookwaveOutWrite(HWAVEOUT hwo,
	LPWAVEHDR pwh,
	UINT cbwh
	)
{
	if (bEnableSound)
	{
		__asm{
			push cbwh
				push pwh
				push hwo
				call RealwaveOutWrite
		}
		return;
	}

	return MMSYSERR_NOERROR;
}
void RealmidiStreamOut(void){
	__asm{
		mov edi, edi
			push ebp
			mov ebp, esp
			jmp MessageBoxExA
			nop
	}
}
MMRESULT WINAPI HookmidiStreamOut(HMIDISTRM hMidiStream,
	LPMIDIHDR lpMidiHdr,
	UINT      cbMidiHdr
	)
{
	if (bEnableSound)
	{
		__asm{
			push cbMidiHdr
				push lpMidiHdr
				push hMidiStream
				call RealmidiStreamOut
		}
		return;
	}

	return MMSYSERR_NOERROR;
}
void RealDirectSoundCreate(void){
	__asm{
		mov edi, edi
			push ebp
			mov ebp, esp
			jmp MessageBoxW
			nop
	}
}
HRESULT WINAPI HookDirectSoundCreate(PVOID pv1,
	PVOID pv2,
	PVOID pv3
	)
{
	if (bEnableSound)
	{
		__asm{
			push pv3
				push pv2
				push pv1
				call RealDirectSoundCreate
		}
		return;
	}

	return DSERR_NODRIVER;
}
void RealDirectSoundCreate8(void){
	__asm{
		mov edi, edi
			push ebp
			mov ebp, esp
			jmp MessageBoxExW
			nop
	}
}
HRESULT WINAPI HookDirectSoundCreate8(PVOID pv1,
	PVOID pv2,
	PVOID pv3
	)
{
	if (bEnableSound)
	{
		__asm{
			push pv3
				push pv2
				push pv1
				call RealDirectSoundCreate8
		}
		return;
	}

	return DSERR_NODRIVER;
}

void HookSoundFunc(char*szDllName, char*szFuncName, PROC pfnOld, PROC pfnNew){
	HMODULE hMod = LoadLibraryA(szDllName);
	if (hMod == 0)return;
	PROC pfnOrig = (PROC)GetProcAddress(hMod, szFuncName);
	if (pfnOrig == 0)return;

	if (!WriteProcessMemory(GetCurrentProcess(), (LPVOID)pfnOld, (LPVOID)pfnOrig, 5, 0))return;
	BYTE byJmp[6];
	byJmp[0] = 0xE9;
	byJmp[5] = 0x90;
	*(DWORD*)&byJmp[1] = (DWORD)pfnOrig - (DWORD)pfnOld - 5;
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)((DWORD)pfnOld + 5), byJmp, 6, 0);
	*(DWORD*)&byJmp[1] = (DWORD)pfnNew - (DWORD)pfnOrig - 5;
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)pfnOrig, byJmp, 5, 0);
}
void EnableSound(BOOL b){
	bEnableSound = b;
}
BOOL GetSound(){
	return bEnableSound;
}
void SetSoundHook(){
	HookSoundFunc("winmm.dll", "waveOutWrite", (PROC)RealwaveOutWrite, (PROC)HookwaveOutWrite);
	HookSoundFunc("winmm.dll", "midiStreamOut", (PROC)RealmidiStreamOut, (PROC)HookmidiStreamOut);
	HookSoundFunc("dsound.dll", "DirectSoundCreate", (PROC)RealDirectSoundCreate, (PROC)HookDirectSoundCreate);
	HookSoundFunc("dsound.dll", "DirectSoundCreate8", (PROC)RealDirectSoundCreate8, (PROC)HookDirectSoundCreate8);
}