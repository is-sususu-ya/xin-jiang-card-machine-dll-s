using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace ALBTester
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private delegate void onNewLog(string text);

        private void real_print_log(string text)
        {
            text = String.Format("{0} {1}", DateTime.Now.Ticks.ToString(""), text );
            if( text.EndsWith("\n" ))
                this.textBoxlog.Text += text;
            else
                this.textBoxlog.Text += text + "\r\n";
        }

        private void onALBEvetTrigger(IntPtr h, int a, int b )
        {
            string txt = String.Format("ALB EVENT：{0}\r\n", a);
            this.BeginInvoke(new onNewLog(real_print_log), txt );
        }

        void ltrace(string text)
        {
            this.BeginInvoke(new onNewLog(real_print_log), text );
        }

        IntPtr alb = IntPtr.Zero;

        private void button1_Click(object sender, EventArgs e)
        {
            byte[] input = System.Text.Encoding.Default.GetBytes(this.textBoxip.Text);
            if( alb != IntPtr.Zero )
            {
                ALBSDK.DEV_Close(alb);
            }
            ltrace("打开栏杆机！");
            alb = ALBSDK.DEV_Open(input);
            if( alb != IntPtr.Zero )
            {
                ALBSDK.DEV_SetEventHandle(alb, onALBEvetTrigger  );
                ltrace("设置消息！");
            }
            else
            {
                ltrace("连接失败！");
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (alb != IntPtr.Zero)
            {
                ltrace("关闭栏杆机！");
                ALBSDK.DEV_Close(alb);
                alb = IntPtr.Zero;
            }
            else
            {
                ltrace("请先连接！");
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            int val = 0;
            if(alb != IntPtr.Zero)
            {
                ltrace("获取状态！");
                ALBSDK.DEV_GetStatus(alb, out val);
                ltrace(String.Format("Status: {0}", val));
            }
            else
            {
                ltrace("请先连接！");
            }
        }

        private void button4_Click(object sender, EventArgs e)
        {
            if (alb != IntPtr.Zero)
            {
                ALBSDK.DEV_ALB_Ctrl(alb, 1);
                ltrace("台杆操作！");
            }
            else
            {
                ltrace("请先连接！");
            }
        }

        private void button5_Click(object sender, EventArgs e)
        {
            if (alb != IntPtr.Zero)
            {
                ALBSDK.DEV_ALB_Ctrl(alb, 0);
                ltrace("落杆操作！");
            }else
            {
                ltrace("请先连接！");
            }
        }
    }
}
