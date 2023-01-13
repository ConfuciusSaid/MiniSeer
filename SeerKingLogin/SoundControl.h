#include <mmsystem.h>
#include <dsound.h>

#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"dsound.lib")

void SetSoundHook();
void EnableSound(BOOL b);
BOOL GetSound();