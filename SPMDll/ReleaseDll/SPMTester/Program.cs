using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace SPMTester
{
    static class Program
    {
        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        [STAThread]
        static void Main()
        {
            AppDomain.CurrentDomain.UnhandledException += new UnhandledExceptionEventHandler(HandlehandledExceptionEvent);
            Application.ThreadException += new System.Threading.ThreadExceptionEventHandler(TraceException);
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }

        static void HandlehandledExceptionEvent(object sender, UnhandledExceptionEventArgs e)
        {
            if (!File.Exists("Dump"))
                Directory.CreateDirectory("Dump");
            string logName = "./Dump/Exception.log";
            string time = DateTime.Now.ToString("[yyyy-MM-dd HH:mm:ss.fff]");//获取当前系统时间 
            StreamWriter mySw = File.AppendText(logName);
            string text = "========  系统未捕获的异常数据堆栈 ========\r\n";
            text += sender.ToString() + "\r\n";
            text += e.ToString() + "\r\n";
            text += e.ExceptionObject.ToString() + "\r\n";
            string write_content = time + " " + text;
            mySw.WriteLine(write_content);
            mySw.Close();
        }
         
        static void TraceException(object sender, System.Threading.ThreadExceptionEventArgs e)
        {
            if (!File.Exists("Dump"))
                Directory.CreateDirectory("Dump");
            string logName = "./Dump/Exception.log";
            string time = DateTime.Now.ToString("[yyyy-MM-dd HH:mm:ss.fff]");//获取当前系统时间 
            StreamWriter mySw = File.AppendText(logName);
            string text = "========  线程异常关闭时数据堆栈 ========\r\n";
            text += sender.ToString() + "\r\n";
            text += e.ToString() + "\r\n";
            text += e.Exception.StackTrace + "\r\n";
            string write_content = time + " " + text;
            mySw.WriteLine(write_content);
            mySw.Close();
        }
    }
}
