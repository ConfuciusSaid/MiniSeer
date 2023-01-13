#include <Windows.h>
#include <process.h>
#include <mmsystem.h>
#pragma comment (lib,"winmm.lib")

BOOL WINAPI AddHook();
void SetCriticalSection(LPCRITICAL_SECTION lpcs);
void SetRange(float f);

void EnableCountDownChange(BOOL bChange);