using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace ALBTester
{
    class ALBSDK
    {

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void ALBEvent(IntPtr ptr, int code, int param );

        [DllImport(@"ALB.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static IntPtr DEV_Open(byte[] ip);

        [DllImport(@"ALB.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static int DEV_Close(IntPtr h);

        [DllImport(@"ALB.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static int DEV_ALB_Ctrl(IntPtr h, int op);
         
        [DllImport(@"ALB.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static int DEV_GetStatus(IntPtr h, out int op);
         
        [DllImport(@"ALB.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public extern static int DEV_SetEventHandle(IntPtr h, ALBEvent ob);


    }
}
