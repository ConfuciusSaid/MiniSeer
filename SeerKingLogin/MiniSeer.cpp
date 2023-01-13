// SeerKingLogin.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
CComModule _Module;
#include <atlwin.h>
#include <comutil.h>
#include <Wininet.h>
#include <psapi.h>
#include <shlguid.h>
#include <commdlg.h>

#include "SoundControl.h"
#include "GearNT.h"
#include "MiniSeer.h"
#pragma comment(lib,"wininet.lib")
#pragma comment(lib,"psapi.lib")
#pragma comment(lib,"comdlg32.lib")

#define WIDTH (960)
#define HEIGHT (565)
#define OFFSETX (16)
#define OFFSETY (60)

#define WM_SHOWPIC (WM_USER+1)
#define WM_SHOWROUND (WM_USER+2)

char szAbout[] = "作者：King丶秽翼\n版本：1.2\nb站：潜水人员_";
//char szHelp[] = "刷新快捷键：Ctrl+R\n变速快捷键：(重复还原)\nCtrl+↑\t：2倍速\nCtrl+→\t：4倍速\nCtrl+↓\t：1/2倍速\nCtrl+←\t：1/4倍速";
char szHelp[] = "刷新快捷键：Ctrl + R\n变速快捷键：(重复按可还原速度)\n见变速页面的自定义";

CRITICAL_SECTION g_cs;
IWebBrowser2 *g_pWebBrowser;
HDC g_hMainDC, g_hMdc;
HBRUSH hBkBrush;

HWND g_hWndWeb = 0;
HWND g_hWndMain;
HWND g_hDlgGearNT = 0;
HWND g_hDlgSetKey = 0;
HWND g_hDlgScript = 0;
HWND g_hWndPic = 0;
HWND g_hDlgRound = 0;

HINSTANCE g_hInst;
int ScreenWidth = 944, ScreenHeight = 522;
int arKey[4] = { 1, 2, -1, -2 };

BOOL RegisterMyWindowClass(HINSTANCE hInst);
HWND CreateMyWindow(HINSTANCE hInst);
HWND Init(HINSTANCE hInst);
int Run();

float fLastRange = 1;

//线程
//DWORD WINAPI ThreadRoundCount(LPVOID lParam);

//回合计数相关
HANDLE g_hRoundCount = 0;
BOOL bFighting = FALSE;
BOOL bOnCount = FALSE;
BOOL bOpposite = FALSE;
int iRound = 0;


//主窗口回调
LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//主窗口功能实现
void CreateWebActive(HWND hWndParent);
void Command(HWND hWnd, WPARAM wParam);
BOOL DeleteCache();
BOOL ClearMemory();
//缩放
void SetWebRange(float fRange);

//变速窗口回调
LRESULT WINAPI GearNTProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//变速功能实现
void InitDlgGearNTActive(HWND hWndParent);

//设置变速快捷键对话框回调
LRESULT WINAPI SetKeyProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//初始化控件
void InitDlgSetKeyActive(HWND hWndParent);

//脚本自定义的窗口回调
LRESULT WINAPI ScriptProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//所需变量
BOOL bGetPic = FALSE;
POINT PicPos = { 0, 0 };
POINT PicSize = { 0, 0 };
HBITMAP bitmapPicGet = 0;
//脚本功能的实现
BOOL OnGetPic(MSG msg);
void ShowPic(HWND hWnd);
void ShowPos(HWND hWnd, LPARAM lParam);
void SavePic(HWND hWnd);
void RecordPos(HWND hWnd);

//回合计数的窗口回调
LRESULT WINAPI RoundProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OnTimer(HWND hWnd);

//主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int){;
	InitializeCriticalSection(&g_cs);
	SetCriticalSection(&g_cs);
	AddHook();
	
	HWND hWnd = Init(hInstance);
	if (hWnd == 0)return 1;
	
	//初始化DC
	g_hMainDC = GetDC(hWnd);
	g_hMdc = CreateCompatibleDC(g_hMainDC);
	hBkBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
	
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	return Run();
}

//计数线程
//DWORD WINAPI ThreadRoundCount(LPVOID lParam){
//	BOOL bOpposite = FALSE;
//	iRound = 0;
//	bFighting = FALSE;
//	while (bOnCount){
//		Sleep(10);
//		if (g_hDlgRound){
//			PostMessage(g_hDlgRound, WM_SHOWROUND, 0, 0);
//		}
//		COLORREF color = GetPixel(g_hMainDC, 870, 485);
//		BOOL bLight = (color == 0x00D88003 || color == 0x000C8FFF || color == 0x00E5CBB2);
//		if (bLight){
//			if (!bFighting){
//				bFighting = TRUE;
//				bOpposite = TRUE;
//				iRound = 1;
//			}
//			else if (!bOpposite){
//				iRound++;
//				bOpposite = TRUE;
//			}
//			continue;
//		}
//		if (bOpposite){
//			BOOL bDark = (color == 0x00CFC8C0);
//			if (bDark){
//				bOpposite = FALSE;
//			}
//		}
//		color = GetPixel(g_hMainDC, 87, 7);
//		if (color != 0x00FFC200 && color != 0x000D0A0A){
//			bFighting = FALSE;
//		}
//	}
//	return 0;
//}

//窗口初始化及消息循环
BOOL RegisterMyWindowClass(HINSTANCE hInst)
{
	WNDCLASS wnd = { 0 };
	wnd.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wnd.hInstance = hInst;
	wnd.lpfnWndProc = WndProc;
	wnd.lpszClassName = L"MiniSeer";
	wnd.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON));
	wnd.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);

	return RegisterClass(&wnd);
}
HWND CreateMyWindow(HINSTANCE hInst)
{
	int x = GetSystemMetrics(SM_CXSCREEN) / 2 - WIDTH / 2 - 2;
	int y = GetSystemMetrics(SM_CYSCREEN) / 2 - HEIGHT / 2 - 23;
	g_hWndMain = CreateWindow(L"MiniSeer", L"MiniSeer", WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX , x, y, WIDTH+4 , HEIGHT + 42 +4, 0, 0, hInst, 0);
	return g_hWndMain;
}
HWND Init(HINSTANCE hInst)
{
	g_hInst = hInst;
	if (!RegisterMyWindowClass(hInst)){
		return 0;
	}
	return CreateMyWindow(hInst);
}
int Run(){
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT){
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)){/*
			if (g_hDlgScript != 0 && msg.message == WM_MOUSEMOVE){
				COLORREF color = GetPixel(g_hMainDC, 235, 475);
				char szText[9] = { 0 };
				sprintf(szText, "%08X", color);
				SetDlgItemTextA(g_hDlgScript, IDC_EDIT_COLOR, szText);
			}*/
			if (bOnCount&&msg.message == WM_LBUTTONDOWN){
				SendMessage(g_hDlgRound, WM_TIMER, 0, 0);
			}
			if (bGetPic && (msg.message == WM_LBUTTONDOWN || msg.message == WM_LBUTTONUP || msg.message == WM_MOUSEMOVE)){
				if (msg.message == WM_MOUSEMOVE)continue;
				OnGetPic(msg);
				continue;
			}
			if (msg.message == WM_MOUSEWHEEL){
				continue;
			}
			if (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP){
				if (msg.wParam == VK_RETURN&&g_hDlgScript != 0){
					SendMessage(g_hDlgScript, msg.message, msg.wParam, msg.lParam);
				}
				else{
					SendMessage(g_hWndMain, msg.message, msg.wParam, msg.lParam);
				}
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else{
			Sleep(0);
		}
		
	}
	return msg.wParam;
}

BOOL bControl = FALSE;
//主窗口回调及功能实现
LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg){
	case WM_KEYDOWN:
		if (bControl){
			if (wParam == 'r' || wParam == 'R'){
				g_pWebBrowser->Refresh();
			}
			if (g_hDlgGearNT != 0){

				int nPos = 0;
				bool bSet = true;
				switch (wParam){
				case VK_UP:nPos = arKey[0]; break;
				case VK_RIGHT:nPos = arKey[1]; break;
				case VK_DOWN:nPos = arKey[2]; break;
				case VK_LEFT:nPos = arKey[3]; break;
				default:
					bSet = false;
					break;
				}

				if (bSet){
					HWND hWndSlider = GetDlgItem(g_hDlgGearNT, IDC_SLIDERRANGE);
					if (nPos == SendMessage(hWndSlider, TBM_GETPOS, 0, 0)){
						nPos = 0;
					}
					SendMessage(hWndSlider, TBM_SETPOS, 1, nPos);
					SendMessage(g_hDlgGearNT, WM_HSCROLL, 0, 0);
				}
			}
		}
		if (wParam == VK_CONTROL){
			bControl = TRUE;
		}
		
		break;
	case WM_KEYUP:
		if (wParam == VK_CONTROL){
			bControl = FALSE;
		}
		break;
	case WM_CREATE:
		CreateWebActive(hWnd);
		//CreateThread(0, 0, ThreadClearMemory, 0, 0, 0);
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		Command(hWnd, wParam);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}
	return 0;
}
void CreateWebActive(HWND hWndParent)
{
	RECT rc;
	GetClientRect(hWndParent, &rc);
	VARIANT varMyURL;
	static CAxWindow WinContainer;
	LPOLESTR pszName = OLESTR("shell.Explorer.2");
	rc.right += 30;
	rc.bottom += 30;
	WinContainer.Create(hWndParent, rc, 0, WS_CHILD | WS_VISIBLE);
	WinContainer.CreateControl(pszName);
	WinContainer.QueryControl(__uuidof(IWebBrowser2), (void**)&g_pWebBrowser);
	VariantInit(&varMyURL);
	varMyURL.vt = VT_BSTR;
	
	varMyURL.bstrVal = SysAllocString(_T("http://seer.61.com"));
	g_pWebBrowser->Navigate2(&varMyURL, 0, 0, 0, 0);
	VariantClear(&varMyURL);
	g_pWebBrowser->put_Left(0 - OFFSETX);
	g_pWebBrowser->put_Top(0 - OFFSETY);
	g_pWebBrowser->Release();

}
BOOL bTop = FALSE;
void Command(HWND hWnd, WPARAM wParam)
{
	switch (wParam){
	case IDC_CENTER:
		if (1){
			RECT rt;
			GetWindowRect(hWnd, &rt);
			int x = GetSystemMetrics(SM_CXSCREEN) / 2 - (rt.right - rt.left) / 2;
			int y = GetSystemMetrics(SM_CYSCREEN) / 2 - (rt.bottom - rt.top) / 2;
			SetWindowPos(hWnd, HWND_TOP, x, y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
		}
		break;
	case IDC_HELP:
		MessageBoxA(hWnd, szHelp, "帮助", MB_OK);
		break;
	case IDC_ABOUT:
		MessageBoxA(hWnd, szAbout, "关于", MB_OK);
		break;
	case IDC_ROUNDCOUNT:
		if (bOnCount == FALSE){
			if (fLastRange != 1.0)SetWebRange(1.0);
			bOnCount = TRUE;
			CreateDialogW(g_hInst, MAKEINTRESOURCE(IDD_ROUNDCOUNT), 0, (DLGPROC)RoundProc);
			ShowWindow(g_hDlgRound, SW_SHOW);
			RECT rt;
			GetWindowRect(g_hWndMain, &rt);
			int x = rt.right + 10;
			int y = (rt.bottom - rt.top) / 2 + rt.top - 20;
			SetWindowPos(g_hDlgRound, HWND_TOP, x, y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
		}
		else{
			BringWindowToTop(g_hDlgGearNT);
			RECT rt;
			GetWindowRect(g_hWndMain, &rt);
			int x = rt.right + 10;
			int y = (rt.bottom - rt.top) / 2 + rt.top - 20;
			SetWindowPos(g_hDlgRound, HWND_TOP, x, y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
		}
		break;
	case IDC_TOPMOST:
		if (!bTop){
			SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
			bTop = TRUE;
		}
		else{
			SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
			SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
			bTop = FALSE;
		}
		break;
	
	/*case IDC_SCRIPT:
		if (g_hDlgScript == 0){
			if (fLastRange != 1.0)SetWebRange(1.0);
			CreateDialogW(g_hInst, MAKEINTRESOURCE(IDD_SCRIPT), 0, (DLGPROC)ScriptProc);
			ShowWindow(g_hDlgScript, SW_SHOW);
		}
		else{
			BringWindowToTop(g_hDlgScript);
		}
		break;*/
	case IDC_GEARNT:
		if (g_hDlgGearNT == 0){
			CreateDialogW(g_hInst, MAKEINTRESOURCE(IDD_GEARNT), 0, (DLGPROC)GearNTProc);
			ShowWindow(g_hDlgGearNT, SW_SHOW);
			RECT rt;
			GetWindowRect(g_hWndMain, &rt);
			int x = rt.left + (rt.right - rt.left) / 2 - 380 / 2;
			int y = rt.bottom + 10;
			SetWindowPos(g_hDlgGearNT, HWND_TOP, x, y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
		}
		else{
			BringWindowToTop(g_hDlgGearNT);
			RECT rt;
			GetWindowRect(g_hWndMain, &rt);
			int x = rt.left + (rt.right - rt.left) / 2 - 380 / 2;
			int y = rt.bottom + 10;
			SetWindowPos(g_hDlgGearNT, HWND_TOP, x, y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
		}
		break;
	case IDC_CLEARMEMORY:
		ClearMemory();
		break;
	case IDC_CLEARCOOKIES:
		DeleteCache();
		break;
	case IDC_REFRESH:
		g_pWebBrowser->Refresh();
		break;
	}
	if (!bOnCount){
		switch (wParam){
		case IDC_50PERCENT:
			SetWebRange(0.5);
			break;
		case IDC_75PERCENT:
			SetWebRange(0.75);
			break;
		case IDC_25PERCENT:
			SetWebRange(0.25);
			break;
		case IDC_100PERCENT:
			SetWebRange(1.0);
			break;
		}
	}
}
BOOL DeleteCache()
{

	DWORD dwNeeded = 0x1000;
	LPSTR szBuf = new char[0x1000];
	LPINTERNET_CACHE_ENTRY_INFOA lpicei = (LPINTERNET_CACHE_ENTRY_INFOA)szBuf;
	HANDLE hFind = FindFirstUrlCacheEntryA(0, lpicei, &dwNeeded);

	BOOL bRun = TRUE;
	do{
		dwNeeded = 0x1000;
		//string str(lpicei->lpszSourceUrlName);
		//listCache.push_back(str);
		if (lpicei->lpszSourceUrlName != (char*)0xcdcdcdcd){
			char szUrl[] = "http://seer.61.com";
			char cOld = lpicei->lpszSourceUrlName[strlen(szUrl)];
			lpicei->lpszSourceUrlName[strlen(szUrl)] = '\0';
			if (strcmp(szUrl, lpicei->lpszSourceUrlName) == 0){
				lpicei->lpszSourceUrlName[strlen(szUrl)] = cOld;
				OutputDebugStringA(lpicei->lpszSourceUrlName);
				if (DeleteUrlCacheEntryA(lpicei->lpszSourceUrlName) == 0) {
					OutputDebugStringA("DeleteUrlCacheEntryA - error\n");
				}
			}
		}
		if (!FindNextUrlCacheEntryA(hFind, lpicei, &dwNeeded)){
			bRun = FALSE;
		}
	} while (bRun);
	FindCloseUrlCache(hFind);
	delete szBuf;

	return 0;
}
BOOL ClearMemory()
{
	if (!SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1)){
		return FALSE;
	}
	if (!EmptyWorkingSet(GetCurrentProcess())){
		return FALSE;
	}
	return TRUE;
}

//变速窗口回调及功能实现
int MyPow(int nIndex){
	if (nIndex >= 0){
		return ((int)1 << nIndex);
	}
	else{
		return ((int)1 << (-1*nIndex));
	}
}
void ChangeTitle(HWND hWndSlider){


	
	int nPos;
	nPos = SendMessageW(hWndSlider, TBM_GETPOS, 0, 0);

	LPSTR szTitle = new char[64];
	char str[7] = { 0 };
	if (nPos > 0)memcpy(str, "加速", 4);
	else if (nPos < 0)memcpy(str, "减速1/", 6);
	else memcpy(str, "原速", 4);

	nPos = (nPos < 0) ? (-1 * nPos) : (nPos);

	sprintf(szTitle, "变速功能(%s%d倍)", str, MyPow(nPos));

	SetWindowTextA(g_hDlgGearNT, szTitle);
	delete szTitle;
}
LRESULT WINAPI GearNTProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch (uMsg){
	case WM_COMMAND:
		if (1){
			WORD wmId = LOWORD(wParam);
			WORD wmEvent = HIWORD(wParam);
			if (wmId == IDC_COUNTDOWNCHANGE){

				if (wmEvent == BN_CLICKED)
				{
					HWND child = GetDlgItem(hWnd, IDC_COUNTDOWNCHANGE);
					int ret = SendMessage(child, BM_GETCHECK, 0, 0);
					EnableCountDownChange(ret);
				}
			}if (wmId == IDC_BTN_DEFINEKEY){
				if (g_hDlgSetKey == 0){
					CreateDialogW(g_hInst, MAKEINTRESOURCE(IDD_SETKEY), 0, (DLGPROC)SetKeyProc);
					ShowWindow(g_hDlgSetKey, SW_SHOW);
				}
				else{
					BringWindowToTop(g_hDlgSetKey);
				}
			}
			
		}
	case WM_HSCROLL:
		if (1){
			HWND hWndSlider = GetDlgItem(hWnd, IDC_SLIDERRANGE);
			int nPos = SendMessageW(hWndSlider, TBM_GETPOS, 0, 0);
			float fRange = MyPow(nPos);
			if (nPos < 0){
				fRange = 1 / fRange;
			}
			ChangeTitle(hWndSlider);
			SetRange(fRange);
		}
		break;
	case WM_INITDIALOG:
		g_hDlgGearNT = hWnd;
		InitDlgGearNTActive(hWnd);
		break;
	case WM_CLOSE:
		EnableCountDownChange(1);
		SetRange(1);
		g_hDlgGearNT = 0;
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		EndDialog(hWnd, 0);
		break;
	}
	return 0;
}
void InitDlgGearNTActive(HWND hWndParent){
	HWND hWndSlider = GetDlgItem(hWndParent, IDC_SLIDERRANGE);
	SendMessageW(hWndSlider, TBM_SETRANGE, TRUE, (LPARAM)MAKELONG(-8, +8));
	SendMessageW(hWndSlider, TBM_SETPOS, TRUE, (LPARAM)0);

	ChangeTitle(hWndSlider);

	HWND hWndBox = GetDlgItem(hWndParent, IDC_COUNTDOWNCHANGE);
	SendMessage(hWndBox, BM_SETCHECK, BST_CHECKED, 0);
}
void SetWebRange(float fRange){
	int zoom = fRange * 100;
	CComVariant varZoom((int)zoom);
	g_pWebBrowser->ExecWB(OLECMDID_OPTICAL_ZOOM, OLECMDEXECOPT_DODEFAULT, &varZoom, NULL);

	/*g_pWebBrowser->put_Left(OFFSETX*fLastRange);*/
	g_pWebBrowser->put_Top(OFFSETY*fLastRange);
	g_pWebBrowser->put_Top(-1 * OFFSETY*fRange);
	g_pWebBrowser->put_Left(OFFSETX*fLastRange);
	g_pWebBrowser->put_Left(-1 * (OFFSETX*fRange));

	g_pWebBrowser->put_Left(OFFSETX*fLastRange + (WIDTH + OFFSETX * 2) * (1 - fLastRange));
	g_pWebBrowser->put_Left(-1 * (OFFSETX*fRange + (WIDTH + OFFSETX * 2) * (1 - fRange)));

	SetWindowPos(g_hWndMain, HWND_TOP, CW_USEDEFAULT, 0, fRange*WIDTH + 4, fRange*HEIGHT + 42 + 4, SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_DRAWFRAME);

	IDispatch *pDisp;
	HRESULT hr = g_pWebBrowser->get_Document(&pDisp);

	IHTMLDocument2* pDoc;
	hr = pDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc);

	IHTMLWindow2 *pWindow;
	pDoc->get_parentWindow(&pWindow);
	hr = pWindow->scrollBy(0, -5000);

	fLastRange = fRange;

}
LRESULT WINAPI SetKeyProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg){
	case WM_COMMAND:
		if (wParam == IDC_BTN_SETKEY){
			int arNew[4] = { 0 };
			arNew[0] = GetDlgItemInt(hWnd, IDC_EDIT_CONTROLUP, 0, TRUE);
			arNew[1] = GetDlgItemInt(hWnd, IDC_EDIT_CONTROLRIGHT, 0, TRUE);
			arNew[2] = GetDlgItemInt(hWnd, IDC_EDIT_CONTROLDOWN, 0, TRUE);
			arNew[3] = GetDlgItemInt(hWnd, IDC_EDIT_CONTROLLEFT, 0, TRUE);
			for (int i = 0; i < 4; i++){
				if (arNew[i]>8)arNew[i] = 8;
				if (arNew[i]<-8)arNew[i] = -8;
				arKey[i] = arNew[i];
			}
			PostMessage(hWnd, WM_CLOSE, 0, 0);
		}
		break;
	case WM_INITDIALOG:
		g_hDlgSetKey = hWnd;
		InitDlgSetKeyActive(hWnd);
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		g_hDlgSetKey = 0;
		EndDialog(hWnd, 0);
		break;
	}
	return 0;
}
void InitDlgSetKeyActive(HWND hWndParent){
	SetDlgItemInt(hWndParent, IDC_EDIT_CONTROLUP, arKey[0], TRUE);
	SetDlgItemInt(hWndParent, IDC_EDIT_CONTROLRIGHT, arKey[1], TRUE);
	SetDlgItemInt(hWndParent, IDC_EDIT_CONTROLDOWN, arKey[2], TRUE);
	SetDlgItemInt(hWndParent, IDC_EDIT_CONTROLLEFT, arKey[3], TRUE);
}

//自定义脚本窗口回调及功能实现
LRESULT WINAPI ScriptProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch (uMsg){
	case WM_KEYDOWN:
		if (wParam == VK_RETURN)
		{
			RecordPos(hWnd);
		}
		break;
	case WM_MOUSEMOVE:
		ShowPos(hWnd, lParam);
		break;
	case WM_SHOWPIC:
		ShowPic(hWnd);
		break;
	case WM_COMMAND:
		switch (wParam){
		case IDC_BTN_RUNSCRIPT:
			/*PostMessage(g_hWndPic, WM_LBUTTONDOWN, 0, MAKEWORD(472, 462));
			Sleep(1);
			PostMessage(g_hWndPic, WM_LBUTTONUP, 0, MAKEWORD(472, 462));*/
			break;
		case IDC_BTN_GETPIC:
			bGetPic = TRUE;
			break;
		case IDC_BTN_SAVEPIC:
			SavePic(hWnd);
			break;
		}
		break;
	case WM_INITDIALOG:
		g_hDlgScript = hWnd;
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		g_hDlgScript = 0;
		EndDialog(hWnd, 0);
		break;
	}
	return 0;
}
BOOL OnGetPic(MSG msg){
	TCHAR szClassName[256] = { 0 };
	::GetClassName(msg.hwnd, szClassName, 256);
	if (_tcsicmp(szClassName, _T("Internet Explorer_Server")) != 0)return FALSE;
	if (msg.message == WM_LBUTTONDOWN){
		PicPos.x = LOWORD(msg.lParam);
		PicPos.y = HIWORD(msg.lParam);
	}
	else{
		PicSize.x = LOWORD(msg.lParam) - PicPos.x;
		PicSize.y = HIWORD(msg.lParam) - PicPos.y;
		DWORD time = GetTickCount();
		bGetPic = FALSE;
		if (PicSize.x < 0){
			PicSize.x *= -1;
			PicPos.x = LOWORD(msg.lParam);
		}
		if (PicSize.y < 0){
			PicSize.y *= -1;
			PicPos.y = HIWORD(msg.lParam);
		}
		PicPos.x -= OFFSETX;
		PicPos.y -= OFFSETY;
		if (bitmapPicGet != 0){
			DeleteObject(bitmapPicGet);
		}
		bitmapPicGet = CreateCompatibleBitmap(g_hMainDC, PicSize.x, PicSize.y);
		SelectObject(g_hMdc, bitmapPicGet);
		BitBlt(g_hMdc, 0, 0, PicSize.x, PicSize.y, g_hMainDC, PicPos.x, PicPos.y, SRCCOPY);
		SendMessage(g_hDlgScript, WM_SHOWPIC, 0, 0);
	}
	return TRUE;

}
void ShowPic(HWND hWnd){
	HWND hWndPic = GetDlgItem(hWnd, IDC_GAMEPIC);
	HDC hdcPic = GetDC(hWndPic);
	RECT rt;
	GetWindowRect(hWndPic, &rt);
	rt.right = rt.right - rt.left;
	rt.bottom = rt.bottom - rt.top;
	rt.top = 0;
	rt.left = 0;
	HBRUSH brush = CreateSolidBrush(0x00F0F0F0);
	FillRect(hdcPic, &rt, brush);
	DeleteObject(brush);
	BitBlt(hdcPic, 0, 0, rt.right, rt.bottom, g_hMdc, 0, 0, SRCCOPY);
}
void ShowPos(HWND hWnd, LPARAM lParam){
	POINT pos = { LOWORD(lParam), HIWORD(lParam) };
	if (pos.x >= 5 && pos.x <= 221 && pos.y >= 5 && pos.y <= 145){
		SetDlgItemInt(hWnd, IDC_EDIT_POSX, pos.x - 5 + PicPos.x, FALSE);
		SetDlgItemInt(hWnd, IDC_EDIT_POSY, pos.y - 5 + PicPos.y, FALSE);
		HWND hWndPic = GetDlgItem(hWnd, IDC_GAMEPIC);
		HDC hdcPic = GetDC(hWndPic);
		COLORREF color = GetPixel(hdcPic, pos.x - 5, pos.y - 5);
		char szColor[9] = { 0 };
		sprintf(szColor, "%08X", color);
		SetDlgItemTextA(hWnd, IDC_EDIT_COLOR, szColor);
	}
}
BOOL SaveBitmapToFile(LPCTSTR lpszFilePath, HBITMAP hBm)
{
	//  定义位图文件表头
	BITMAPFILEHEADER bmfh;
	//  定义位图信息表头
	BITMAPINFOHEADER bmih;

	//  调色板长度
	int nColorLen = 0;
	//  调色表大小
	DWORD dwRgbQuadSize = 0;
	//  位图大小
	DWORD dwBmSize = 0;
	//  分配内存的指针
	HGLOBAL    hMem = NULL;

	LPBITMAPINFOHEADER     lpbi;

	BITMAP bm;

	HDC hDC;

	HANDLE hFile = NULL;

	DWORD dwWritten;

	GetObjectW(hBm, sizeof(BITMAP), &bm);

	bmih.biSize = sizeof(BITMAPINFOHEADER);    // 本结构所占的字节
	bmih.biWidth = bm.bmWidth;            // 位图宽
	bmih.biHeight = bm.bmHeight;            // 位图高
	bmih.biPlanes = 1;
	bmih.biBitCount = bm.bmBitsPixel;    // 每一图素的位数
	bmih.biCompression = BI_RGB;            // 不压缩
	bmih.biSizeImage = 0;  //  位图大小
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;

	//  计算位图图素数据区大小 
	dwBmSize = 4 * ((bm.bmWidth * bmih.biBitCount + 31) / 32) * bm.bmHeight;

	//  如果图素位 <= 8bit，则有调色板
	if (bmih.biBitCount <= 8)
	{
		nColorLen = (1 << bm.bmBitsPixel);
	}

	//  计算调色板大小
	dwRgbQuadSize = nColorLen * sizeof(RGBQUAD);

	//  分配内存
	hMem = GlobalAlloc(GHND, dwBmSize + dwRgbQuadSize + sizeof(BITMAPINFOHEADER));

	if (NULL == hMem)
	{
		return FALSE;
	}

	//  锁定内存
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hMem);

	//  将bmih中的内容写入分配的内存中
	*lpbi = bmih;


	hDC = GetDC(NULL);

	//  将位图中的数据以bits的形式放入lpData中。
	GetDIBits(hDC, hBm, 0, (DWORD)bmih.biHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER)+dwRgbQuadSize, (BITMAPINFO *)lpbi, (DWORD)DIB_RGB_COLORS);

	bmfh.bfType = 0x4D42;  // 位图文件类型：BM
	bmfh.bfSize = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+dwRgbQuadSize + dwBmSize;  // 位图大小
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+dwRgbQuadSize;  // 位图数据与文件头部的偏移量

	//  把上面的数据写入文件中

	hFile = CreateFile(lpszFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		return FALSE;
	}

	//  写入位图文件头
	WriteFile(hFile, (LPSTR)&bmfh, sizeof(BITMAPFILEHEADER), (DWORD *)&dwWritten, NULL);
	//  写入位图数据
	WriteFile(hFile, (LPBITMAPINFOHEADER)lpbi, bmfh.bfSize - sizeof(BITMAPFILEHEADER), (DWORD *)&dwWritten, NULL);

	GlobalFree(hMem);
	CloseHandle(hFile);

	return TRUE;
}
void SavePic(HWND hWnd){
	if (bitmapPicGet == 0)return;
	LPWSTR szPicPath = new WCHAR[MAX_PATH];
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = L"图片文件(.bmp)\0*.bmp";
	ofn.lpstrFile = szPicPath;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrInitialDir = 0;
	ofn.lpstrTitle = L"保存图片到";
	ofn.nMaxFileTitle = 11;
	ofn.Flags =  OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"bmp";
	ofn.nFilterIndex = 1;
	ofn.FlagsEx = 0;
	if (!GetSaveFileName(&ofn)){
		return;
	}

	SaveBitmapToFile(szPicPath, bitmapPicGet);
	delete szPicPath;
}
void RecordPos(HWND hWnd){
	char szColor[9] = { 0 };
	GetDlgItemTextA(hWnd, IDC_EDIT_COLOR, szColor, 9);
	SetDlgItemTextA(hWnd, IDC_EDIT_JUDGE_COLOR, szColor);
	SetDlgItemInt(hWnd, IDC_EDIT_JUDGE_X, GetDlgItemInt(hWnd, IDC_EDIT_POSX, 0, FALSE), FALSE);
	SetDlgItemInt(hWnd, IDC_EDIT_JUDGE_Y, GetDlgItemInt(hWnd, IDC_EDIT_POSY, 0, FALSE), FALSE);
}

LRESULT WINAPI RoundProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch (uMsg){
	case WM_TIMER:
		OnTimer(hWnd);
		break;
	case WM_INITDIALOG:
		bOpposite = FALSE;
		iRound = 0;
		bFighting = FALSE;
		g_hDlgRound = hWnd;
		SetTimer(hWnd, 1001, 2, 0);
		break;
	case WM_SHOWROUND:
		if (bFighting){
			
		}
		else{
			SetDlgItemInt(hWnd, IDC_EDIT_ROUND, (int)0, TRUE);
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		g_hDlgRound = 0;
		bOnCount = FALSE;
		
		EndDialog(hWnd, 0);
		break;
	}
	return 0;
}
void OnTimer(HWND hWnd){
	COLORREF color = GetPixel(g_hMainDC, 870, 485);
	BOOL bLight = (color == 0x00D88003 || color == 0x000C8FFF || color == 0x00E5CBB2);
	if (bLight){
		if (!bFighting){
			bFighting = TRUE;
			bOpposite = TRUE;
			iRound = 1;
			SetDlgItemInt(hWnd, IDC_EDIT_ROUND, iRound, TRUE);
		}
		else if (!bOpposite){
			iRound++;
			SetDlgItemInt(hWnd, IDC_EDIT_ROUND, iRound, TRUE);
			bOpposite = TRUE;
		}
		return;
	}
	if (bOpposite){
		BOOL bDark = (color == 0x00CFC8C0);
		if (bDark){
			bOpposite = FALSE;
		}
	}
	color = GetPixel(g_hMainDC, 87, 7);
	if (color != 0x00FFC200 && color != 0x000D0A0A){
		bFighting = FALSE;
		SetDlgItemInt(hWnd, IDC_EDIT_ROUND, 0, FALSE);
	}
}