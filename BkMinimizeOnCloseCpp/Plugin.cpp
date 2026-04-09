#include <windows.h>
#include <string.h>
#include "BeckyAPI.h"
#include "resource.h"
// BKC_OnPlugInInfoに日本語が含まれるため、ソースファイルを Shift-JIS (CP932) で保存してください

// Becky! API インスタンス
CBeckyAPI bka;

HINSTANCE g_hInstance = NULL;
WNDPROC g_oldWndProc = NULL;


////////////////////////////////////////////////////////////////////////
// コントロールIDと文字列リソースIDをペアにして管理する構造体
struct RESOURCE_BIND {
	int nCtrlID;
	int nStrID;
};

// バインド情報の静的定義
const RESOURCE_BIND g_ConfigBinds[] = {
	{ IDC_PIN_DESCRIPTION,	IDS_PIN_DESCRIPTION	},
	{ IDC_DIAG_SITEURL,		IDS_DIAG_SITEURL    },
	{ IDOK,					IDS_DIAG_BTN_OK     }
};

// 最小化回数
unsigned int g_nMinimizeCount = 0x0;

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
			g_nMinimizeCount++;
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

////////////////////////////////////////////////////////////////////////
// 現在の西暦を文字列（例: "2026"）として取得
static void GetCurrentYearString(char* szBuf, size_t nBufSize)
{
	SYSTEMTIME st;
	GetLocalTime(&st); // システムの現在時刻を取得
	wsprintf(szBuf, "%d", st.wYear); // 数値を文字列に変換
}


#ifdef __cplusplus
extern "C" {
#endif

	////////////////////////////////////////////////////////////////////////
	// プラグイン設定ダイアログの管理
	INT_PTR CALLBACK MyHelpDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_INITDIALOG:
		{
			char sz[512];
			char szYear[10];
			char szCountLabel[512];
			char szOutput[512];

			// ダイアログのタイトルをセット
			if (LoadString(g_hInstance, IDS_PIN_NAME, sz, sizeof(sz))) {
				SetWindowText(hDlg, sz);
			}

			// 定義に基づいて一括で文字列をセット
			for (int i = 0; i < _countof(g_ConfigBinds); i++) {
				if (LoadString(g_hInstance, g_ConfigBinds[i].nStrID, sz, sizeof(sz))) {
					SetDlgItemText(hDlg, g_ConfigBinds[i].nCtrlID, sz);
				}
			}

			// ラベル：ヘルプの案内テキストをセット
			if (LoadString(g_hInstance, IDS_PIN_DESCRIPTION, sz, sizeof(sz)) &&
				LoadString(g_hInstance, IDS_PIN_COUNT, szCountLabel, sizeof(szCountLabel))) {
				wsprintf(szOutput, "%s : %u\r\n%s", szCountLabel, g_nMinimizeCount, sz);
				SetDlgItemText(hDlg, IDC_PIN_DESCRIPTION, szOutput);
			}

			// ラベル：Copyright をセット
			if (LoadString(g_hInstance, IDS_PIN_VENDOR, sz, sizeof(sz))) {
				GetCurrentYearString(szYear, sizeof(szYear));
				wsprintf(szOutput, "Copyright (C) %s %s / MIT", szYear, sz);
				SetDlgItemText(hDlg, IDC_PIN_VENDOR, szOutput);
			}
		}
		return (INT_PTR)TRUE;

		case WM_COMMAND:
			// OKボタン(IDOK)またはキャンセル(IDCANCEL / 閉じるボタン)が押されたら閉じる
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
		}
		return (INT_PTR)FALSE;
	}
	////////////////////////////////////////////////////////////////////////
	// Called when plug-in setup is needed.
	int WINAPI BKC_OnPlugInSetup(HWND hWnd)
	{
		INT_PTR ret = DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, MyHelpDlgProc);

		if (ret == -1) {
			// ダイアログ作成失敗時のエラーコードを表示
			char buf[32];
			wsprintf(buf, "Error: %ld", GetLastError());
			MessageBox(hWnd, buf, "Dialog Error", MB_OK);
		}
		return 1;
	}



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
