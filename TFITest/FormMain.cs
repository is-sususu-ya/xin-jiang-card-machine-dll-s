using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Threading;

namespace DisplayTest
{
    public partial class FormMain : Form
    {

        class TFIProgram
        {

            //创建句柄
            [DllImport("TFI.dll", EntryPoint = "TFI_Create", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
            public static extern IntPtr TFI_Create();
            //释放句柄
            [DllImport("TFI.dll", EntryPoint = "TFI_Destroy", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
            public static extern bool TFI_Destroy(IntPtr handle);
            //打开串口
            [DllImport("TFI.dll", EntryPoint = "TFI_OpenCom", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
            public static extern bool TFI_OpenCom(IntPtr handle, int iComID);
            //关闭串口
            [DllImport("TFI.dll", EntryPoint = "TFI_Close", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
            public static extern bool TFI_Close(IntPtr handle);

            [DllImport("TFI.dll", EntryPoint = "TFI_OpenNet", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
            public static extern bool TFI_OpenNet(IntPtr handle, byte[] IP);

            //发送文字
            [DllImport("TFI.dll", EntryPoint = "TFI_SendLine", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
            public static extern bool TFI_SendLine(IntPtr handle, int nLineNo, uint nColor, int nRollMode, int nRollSpeed, byte[] szText);
            //清除屏幕
            [DllImport("TFI.dll", EntryPoint = "TFI_ClearAll", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
            public static extern bool TFI_ClearAll(IntPtr handle);
            //语音播放
            [DllImport("TFI.dll", EntryPoint = "TFI_SendVoice", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
            public static extern bool TFI_SendVoice(IntPtr handle,int vol,byte[] szText);
            //报警
            [DllImport("TFI.dll", EntryPoint = "TFI_AlarmSet", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
            public static extern bool TFI_AlarmSet(IntPtr handle, byte bAlarm, byte nTime);
            //通行灯           
            [DllImport("TFI.dll", EntryPoint = "TFI_SetPassLight", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
            public static extern bool TFI_SetPassLight(IntPtr handle, int x, int y, byte bPass);

            [DllImport("TFI.dll", EntryPoint = "TFI_SetCallBack", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
            public static extern bool TFI_SetCallBack(IntPtr handle, MsgHandler ps);

            [DllImport("TFI.dll", EntryPoint = "TFI_SetPicture", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
            public static extern bool TFI_SetPicture(IntPtr handle, uint nColor, int x, int bOpen);

            [DllImport("TFI.dll", EntryPoint = "TFI_GetQrCode", SetLastError = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
            public static extern bool TFI_GetQrCode(IntPtr handle, byte[] szText);

        }
        [UnmanagedFunctionPointer(CallingConvention.StdCall)] 
        public  delegate void MsgHandler(IntPtr h,int buffer);
        public MsgHandler Msg = null; 
        private IntPtr handle = IntPtr.Zero;
        static SynchronizationContext cont = null;
        long code = 0;  // 记录收到的次数
        public static FormMain form1;
        public  FormMain()
        {
            bool i;
            InitializeComponent();
            Msg = new MsgHandler(ProcTFIEvent);
            System.Windows.Forms.Control.CheckForIllegalCrossThreadCalls = false;
            form1 = this; 
        }

        private void real_log(string t)
        {
            this.textBox2.Text += t;
            form1.textBox2.Focus();//获取焦点
            form1.textBox2.Select(form1.textBox2.TextLength, 0);//光标定位到文本最后
            form1.textBox2.ScrollToCaret();//滚动到光标处
        }

        private void trace_log(string te)
        {
            this.textBox2.BeginInvoke(new log_txt(real_log), te);
        }

        private delegate void log_txt(string t);

    

        //回调   获取二维码数据  按键
        public void ProcTFIEvent(IntPtr h, int evt)
        { 
            if( evt == 1 )
            {
                // online
            }else if( evt == 2 )
            {
                //offline
            }else if(evt == 3 )
            {
                // qrcode  comming 
                byte[] buf = new byte[256];
                TFIProgram.TFI_GetQrCode(h, buf);
                string text = System.Text.Encoding.Default.GetString(buf); 
                trace_log("扫码结果" + text );
            }
        } 
         
        private void button4_Click(object sender, EventArgs e)
        {
            string com;
            int icom;
            com = this.comboBox2.Text; 
            if (com == "COM1")
                icom = 1;
            else if (com == "COM2")
                icom = 2;
            else if (com == "COM3")
                icom = 3;
            else if (com == "COM4")
                icom = 4;
            else if (com == "COM5")
                icom = 5;
            else if (com == "COM6")
                icom = 6;
            else if (com == "COM7")
                icom = 7;
            else if (com == "COM8")
                icom = 8;
            else if (com == "COM9")
                icom = 9;
            else if (com == "COM10")
                icom = 10;
            else
                icom = 11;

            if (handle != IntPtr.Zero)
            {
                tfi_close();
            }
            handle = TFIProgram.TFI_Create();
            TFIProgram.TFI_OpenCom(handle, icom);
            TFIProgram.TFI_SetCallBack(handle,Msg);
            button4.Enabled = false;
        }

        void tfi_close()
        {
            if (handle != IntPtr.Zero)
            {
                
                TFIProgram.TFI_Close(handle);
                TFIProgram.TFI_Destroy(handle);
                handle = IntPtr.Zero;
            }
        }

        private void button5_Click(object sender, EventArgs e)
        {
            tfi_close();
            button4.Enabled = true;
        }
// 屏幕显示
        private void button6_Click(object sender, EventArgs e)
        {
            string color;
            string Line;
            string mode;
            string speed;
            int ispeed = 1;
            int imode = 0;
            int iLine = 1; ;
            uint icolor = 0xff;
            color = this.comboBox3.Text;
            Line = this.comboBox4.Text;
            mode = this.comboBox5.Text;
            speed = this.comboBox6.Text;
            if (color == "RED")
            {
                icolor = 0xff;
            }
            else if (color == "GREEN")
            {
                icolor = 0xff00;
            }
            else if (color == "BLUE")
            {
                icolor = 0xff0000;
            }
            else if (color == "CYAN") // 青色
            {
                icolor = 0xffff00;
            }
            else
            {
                icolor = 0xff;
            }

            if (Line == "1")
            {
                iLine = 1;
            }
            else if (Line == "2")
            {
                iLine = 2;
            }
            else if (Line == "3")
            {
                iLine = 3;
            }
            else if (Line == "4")
            {
                iLine = 4;
            }

            if (mode == "左对齐")
                imode = 0;
            else if (mode == "居中")
                imode = 1;
            else if (mode == "滚动")
                imode = 5;

            if (speed == "0")
                ispeed = 0;
            else if (speed == "1")
                ispeed = 1;
            else if (speed == "2")
                ispeed = 2;
            else if (speed == "3")
                ispeed = 3;
            byte[] buf = System.Text.Encoding.Default.GetBytes(textBox3.Text);
            TFIProgram.TFI_SendLine(handle, iLine, icolor, imode, ispeed, buf);
        }
// 屏幕清除
        private void button7_Click(object sender, EventArgs e)
        {
            TFIProgram.TFI_ClearAll(handle);
        }
// 语音播报
        private void button12_Click(object sender, EventArgs e)
        {
            byte[] buf = System.Text.Encoding.Default.GetBytes(textBox5.Text);
            TFIProgram.TFI_SendVoice(handle,3, buf);
        }

        private void button13_Click(object sender, EventArgs e)
        {
            /*
            TFIProgram.TFI_AlarmSet(handle, 1, 0);
            button13.Enabled = false;

            button14.Enabled = true;  */

        }
         
        private void button15_Click(object sender, EventArgs e)
        {
            textBox2.Text = "";
        }

        // 网口打开
        private void button10_Click(object sender, EventArgs e)
        {
            if (handle != IntPtr.Zero)
            {
                tfi_close();
            }
            handle = TFIProgram.TFI_Create();
            byte[] buf = System.Text.Encoding.Default.GetBytes(textBox4.Text);
            TFIProgram.TFI_OpenNet(handle, buf); 
            TFIProgram.TFI_SetCallBack(handle, Msg); 
            button10.Enabled = false;
        }

        private void button11_Click(object sender, EventArgs e)
        {
            if (handle != IntPtr.Zero)
            {             
                TFIProgram.TFI_Close(handle);
                TFIProgram.TFI_Destroy(handle);
                handle = IntPtr.Zero;
            }
            button10.Enabled = true;
        }
  
        private void button17_Click(object sender, EventArgs e)
        {
            TFIProgram.TFI_SetPicture(handle, 0xff0000, 0, 0);
        }

        private void FormMain_Load(object sender, EventArgs e)
        {

        }
    }
}
