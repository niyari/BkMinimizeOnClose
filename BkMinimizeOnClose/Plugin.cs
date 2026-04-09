using System.Runtime.InteropServices;
using System.Text;

namespace MinimizeOnClose;

// ==========================================
// Win32 API Definitions
// ==========================================
internal static partial class NativeMethods
{
    private const string User32 = "user32.dll";
    private const string Kernel32 = "kernel32.dll";

    internal const int GWL_WNDPROC = -4;
    internal const uint WM_SYSCOMMAND = 0x0112;
    internal const uint SC_CLOSE = 0xF060;
    internal const uint SC_MINIMIZE = 0xF020;
    internal const int VK_MENU = 0x12;

    [LibraryImport(Kernel32)]
    internal static partial IntPtr GetModuleHandleW(IntPtr lpModuleName);

    [LibraryImport(Kernel32, EntryPoint = "GetProcAddress")]
    internal static partial IntPtr GetProcAddress(IntPtr hModule, [MarshalAs(UnmanagedType.LPStr)] string lpProcName);

    [LibraryImport(User32)]
    internal static partial short GetKeyState(int nVirtKey);

    [LibraryImport(User32, EntryPoint = "PostMessageW")]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static partial bool PostMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

    [LibraryImport(User32, EntryPoint = "SetWindowLongW")]
    internal static partial int SetWindowLong(IntPtr hWnd, int nIndex, int dwNewLong);

    [LibraryImport(User32, EntryPoint = "CallWindowProcW")]
    internal static partial IntPtr CallWindowProc(IntPtr lpPrevWndFunc, IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate bool BKAPI_GetWindowHandles(out IntPtr lphMain, out IntPtr lphTree, out IntPtr lphList, out IntPtr lphView);
}

// ==========================================
// Becky! SDK Structure Definition
// ==========================================
[StructLayout(LayoutKind.Sequential, Pack = 1)]
public unsafe struct BKPLUGININFO
{
    public fixed byte szPlugInName[80];
    public fixed byte szVendor[80];
    public fixed byte szVersion[80];
    public fixed byte szDescription[256];
}

// ==========================================
// Hook Logic (Subclassing)
// ==========================================
public static class WindowHook
{
    private delegate IntPtr WndProcDelegate(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);
    private static WndProcDelegate? _hookDelegate;
    private static IntPtr _oldWndProc = IntPtr.Zero;

    public static void Install(IntPtr hWnd)
    {
        if (hWnd == IntPtr.Zero || _oldWndProc != IntPtr.Zero) return;

        _hookDelegate = HookProc;
        IntPtr pFunc = Marshal.GetFunctionPointerForDelegate(_hookDelegate);
        _oldWndProc = (IntPtr)NativeMethods.SetWindowLong(hWnd, NativeMethods.GWL_WNDPROC, (int)pFunc);
    }

    private static IntPtr HookProc(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam)
    {
        // 終了命令 (SC_CLOSE) を検知
        if (msg == NativeMethods.WM_SYSCOMMAND && ((uint)wParam & 0xFFF0) == NativeMethods.SC_CLOSE)
        {
            // Altキーが押されている（Alt+F4 または Alt+クリック）場合は、明示的操作としてスルー
            if (NativeMethods.GetKeyState(NativeMethods.VK_MENU) < 0)
            {
                return NativeMethods.CallWindowProc(_oldWndProc, hWnd, msg, wParam, lParam);
            }

            // それ以外のマウス操作（通常の×ボタン等）は、最小化へ振り替える
            NativeMethods.PostMessage(hWnd, NativeMethods.WM_SYSCOMMAND, (IntPtr)NativeMethods.SC_MINIMIZE, lParam);
            return IntPtr.Zero;
        }

        return NativeMethods.CallWindowProc(_oldWndProc, hWnd, msg, wParam, lParam);
    }
}

// ==========================================
// Becky! Plugin Entry Points
// ==========================================
public static unsafe class PluginEntry
{
    [UnmanagedCallersOnly(EntryPoint = "BKC_OnStart")]
    public static int BKC_OnStart()
    {
        ApplyHook();
        return 0;
    }

    [UnmanagedCallersOnly(EntryPoint = "BKC_OnCommand")]
    public static int BKC_OnCommand(IntPtr hWnd, int nID)
    {
        ApplyHook();
        return 0;
    }

    private static void ApplyHook()
    {
        IntPtr hBecky = NativeMethods.GetModuleHandleW(IntPtr.Zero);
        IntPtr addr = NativeMethods.GetProcAddress(hBecky, "BKA_GetWindowHandles");

        if (addr != IntPtr.Zero)
        {
            var getWindowHandles = Marshal.GetDelegateForFunctionPointer<NativeMethods.BKAPI_GetWindowHandles>(addr);
            if (getWindowHandles(out IntPtr hMain, out _, out _, out _))
            {
                WindowHook.Install(hMain);
            }
        }
    }

    [UnmanagedCallersOnly(EntryPoint = "BKC_OnPlugInInfo")]
    public static int BKC_OnPlugInInfo(IntPtr lpPlugInInfo)
    {
        if (lpPlugInInfo == IntPtr.Zero) return 0;

        // SDK構造体として扱う
        BKPLUGININFO* info = (BKPLUGININFO*)lpPlugInInfo;

        SetFixedString(info->szPlugInName, "BkMinimizeOnClose", 80);
        SetFixedString(info->szVendor, "Sou Niyari", 80);
        SetFixedString(info->szVersion, "1.6.0.0", 80);
        SetFixedString(info->szDescription, "メインウインドウの×ボタンを最小化に振り替えます。", 256);

        return 0;
    }

    private static void SetFixedString(byte* pTarget, string source, int maxSize)
    {
        if (pTarget == null) return;

        Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);
        var sjis = Encoding.GetEncoding(932); // Shift-JIS (CP932)
        byte[] bytes = sjis.GetBytes(source);

        int copySize = Math.Min(bytes.Length, maxSize - 1);

        for (int i = 0; i < copySize; i++)
        {
            pTarget[i] = bytes[i];
        }
        pTarget[copySize] = 0; // Null terminator
    }

    [UnmanagedCallersOnly(EntryPoint = "BKC_OnExit")]
    public static int BKC_OnExit() => 0;
}