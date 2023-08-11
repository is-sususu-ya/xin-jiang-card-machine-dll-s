using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace SPMTester
{
    class SPMSDK
    {
        private const string DLL_NAME = "SPMAPI.dll";
        public const int EVT_ONLINE = 1;
        public const int EVT_OFFLINE = 2;
        public const int EVT_QRCODE = 3;
        public const int SPM_EVT_IOCHANGE = 4;
        public const int SPM_EVT_HTTP_RESPONSE = 5;

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void SPMCallBack(IntPtr ptr, int code);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static  IntPtr SPM_Create();

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_Destory(IntPtr h);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_IsOnline(IntPtr h);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_Open(IntPtr h, string name);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_Close(IntPtr h);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_SetCallBack(IntPtr h, SPMCallBack cb);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_Led_ClearAll(IntPtr h);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_Led_SendLineText(IntPtr h, int nLineNumber, int color, int alignType, byte[] buf, int len );

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_Voice_SendText(IntPtr h, int vol, byte[] buf, int len);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_Lcd_ChangeContext(IntPtr h, int index, byte[] buf, int len);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_SyncTime(IntPtr h);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_Reboot(IntPtr h);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_EnableQrCode(IntPtr h, bool en);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_GetQrCode(IntPtr h, byte[] en);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_GpioOutPut(IntPtr h, int pin, int val);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        // GPIO 脉冲输出
        public extern static bool SPM_GpioPulse(IntPtr h, int pin, int tout);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        // GPIO 反向脉冲输出
        public extern static bool SPM_GpioPulseNegative(IntPtr h, int pin, int tout);

        // 读取DI值
        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_ReadDi(IntPtr h, ref int parma);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_SetHttpProxyAddress(IntPtr h, int index, byte[] url, int size);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_SetHttpProxyConfig(IntPtr h, int con_tout, int pro_tou);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_SetHttpPostProxyRequest(IntPtr h, int index, int id, int type, byte[] url, int size);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_SetHttpGetProxyRequest(IntPtr h, int id, byte[] url, int size);

        [DllImport(DLL_NAME, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static bool SPM_GetHttpProxyResponse(IntPtr h, ref int id, ref int ret, byte[] content, int max_size);
    }
}
