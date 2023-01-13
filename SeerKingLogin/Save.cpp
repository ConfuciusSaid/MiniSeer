#include "stdafx.h"
#include "Save.h"

BOOL SaveChangeToFile(void*pBuf, unsigned long ulBufLen)
{
	HMODULE hMod = GetModuleHandle(0);
	DWORD dwRVA = (DWORD)pBuf - (DWORD)hMod;

	HANDLE hFile;
	BYTE *arBuf = new BYTE[2048];
	char *szPath = new char[MAX_PATH];

	GetModuleFileNameA(hMod, szPath, MAX_PATH);
	hFile = CreateFileA(szPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == 0 || hFile == INVALID_HANDLE_VALUE){
		return FALSE;
	}

	ReadFile(hFile, arBuf, 2048, 0, 0);
	IMAGE_SECTION_HEADER *p = (IMAGE_SECTION_HEADER*)arBuf;
	p = (IMAGE_SECTION_HEADER*)((DWORD)p + ((PIMAGE_DOS_HEADER)p)->e_lfanew);
	int iSectionNum = ((PIMAGE_NT_HEADERS)p)->FileHeader.NumberOfSections;
	p = (IMAGE_SECTION_HEADER*)((DWORD)p + sizeof(IMAGE_NT_HEADERS));
	for (int i = 0; i < iSectionNum; i++){
		if (dwRVA >= p[i].VirtualAddress&&dwRVA <= p[i].VirtualAddress + p[i].Misc.VirtualSize-ulBufLen){
			DWORD dwOffset = dwRVA - p[i].Misc.VirtualSize + p[i].PointerToRawData;
			if (dwOffset >= p[i].PointerToRawData&&dwOffset <= p[i].PointerToRawData + p[i].SizeOfRawData - ulBufLen){
				SetFilePointer(hFile, dwOffset, 0, FILE_BEGIN);
				return WriteFile(hFile, pBuf, ulBufLen, 0, 0);
			}
			else{
				return FALSE;
			}
		}
	}
	return FALSE;
}