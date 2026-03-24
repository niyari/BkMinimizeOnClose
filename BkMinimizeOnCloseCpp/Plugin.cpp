#include <windows.h>
#include <string.h>
#include ".\BeckyAPI.h"
#include "resource.h"
// BKC_OnPlugInInfoに日本語が含まれるため、ソースファイルを Shift-JIS (CP932) で保存してください

// Becky! API インスタンス
CBeckyAPI bka;

HINSTANCE g_hInstance = NULL;
WNDPROC g_oldWndProc = NULL;

/////////////////////////////////////////////////////////////////////////////
// サブクラス化プロシージャ
/////////////////////////////////////////////////////////////////////////////
static LRESULT CALLBACK PluginWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // SC_CLOSE (終了命令) かつ Altキーが押されていない場合のみフック
    if (msg == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_CLOSE)
    {
        if (GetKeyState(VK_MENU) >= 0) // Altが押されていない
        {
            PostMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, lParam);
            return 0;
        }
    }
    return CallWindowProc(g_oldWndProc, hWnd, msg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// フック適用ロジック
/////////////////////////////////////////////////////////////////////////////
static void ApplyHook()
{
    if (g_oldWndProc != NULL) return;

    HWND hMain, hTree, hList, hView;
    if (bka.GetWindowHandles(&hMain, &hTree, &hList, &hView))
    {
        if (hMain != NULL)
        {
            g_oldWndProc = (WNDPROC)SetWindowLongPtr(hMain, GWLP_WNDPROC, (LONG_PTR)PluginWndProc);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    g_hInstance = (HINSTANCE)hModule;
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        if (!bka.InitAPI()) return FALSE;
    }
    return TRUE;
}

#ifdef __cplusplus
extern "C" {
#endif


////////////////////////////////////////////////////////////////////////
// Called when plug-in information is being retrieved.
	typedef struct tagBKPLUGININFO
	{
		char szPlugInName[80];   // Name of the plug-in
		char szVendor[80];       // Name of the vendor
		char szVersion[80];      // Version string
		char szDescription[256]; // Short description about this plugin
	} BKPLUGININFO, * LPBKPLUGININFO;

    // Becky! Callbacks
    int WINAPI BKC_OnStart() { ApplyHook(); return 0; }
    int WINAPI BKC_OnCommand(HWND hWnd, int nID) { ApplyHook(); return 0; }
    int WINAPI BKC_OnExit() { return 0; }

    int WINAPI BKC_OnPlugInInfo(LPBKPLUGININFO lp)
    {
        if (lp == NULL) return 0;
        char sz[256];
        LoadString(g_hInstance, IDS_PIN_NAME, sz, 80);
        strcpy_s(lp->szPlugInName, sz);
        LoadString(g_hInstance, IDS_PIN_VENDOR, sz, 80);
        strcpy_s(lp->szVendor, sz);
        LoadString(g_hInstance, IDS_PIN_VERSION, sz, 80);
        strcpy_s(lp->szVersion, sz);
        LoadString(g_hInstance, IDS_PIN_DESCRIPTION, sz, 80);
        strcpy_s(lp->szDescription, sz);
        return 0;
    }

#ifdef __cplusplus
}
#endif
