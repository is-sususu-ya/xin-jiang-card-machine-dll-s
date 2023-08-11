using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace SPMTester
{
    public partial class Form1 : Form
    {
        private IntPtr hSPM = IntPtr.Zero;
        public delegate void onQrcode(String v);
        public delegate void onLog(String v);
        public delegate void onDevMsg(int msg); 


        private void QrCodeUpdate(string t)
        {
            this.label_qrcode.Text = t;
        }

        private void trace_log(string text)
        {
            this.textBox_log.Text += text;
            this.textBox_log.Text += "\r\n";
        }

        public Form1()
        {
            InitializeComponent();
        }
        private bool SystemCheck()
        {
            if (hSPM == null)
            {
                MessageBox.Show("系统还未初始化！\r\n");
                return false;
            }
            else
                return true;
        } 

        private void devMsgHandle(int evet)
        {
            if (evet == 1)
                this.label_online.Text = "在线";
            else
                this.label_online.Text = "离线";
        }

        private byte[] cbbuffer = new byte[4096];
        private string content = "";
        private string qrcode = "";

        private void HandleSPMEvent(IntPtr h, int code)
        {
            int ret = 0;
            int id = 0;
            int index = 0;
            if (code == SPMSDK.EVT_OFFLINE)
            {
                this.BeginInvoke(new onLog(trace_log), "设备离线");
                this.BeginInvoke(new onDevMsg(devMsgHandle), 0 );
            }
            else if (code == SPMSDK.EVT_ONLINE)
            {
                this.BeginInvoke(new onLog(trace_log), "设备上线");
                this.BeginInvoke(new onDevMsg(devMsgHandle), 1);
            }
            else if (code == SPMSDK.EVT_QRCODE)
            { 
                SPMSDK.SPM_GetQrCodeEx(hSPM, ref index,  cbbuffer);
                qrcode = Encoding.Default.GetString(cbbuffer);
                content = String.Format( "QRCODE: [{0}][{1}]", index, qrcode );
                this.BeginInvoke(new onLog(trace_log), content);
                this.BeginInvoke(new onQrcode(QrCodeUpdate), qrcode ); 
            }else if( code == SPMSDK.SPM_EVT_HTTP_RESPONSE )
            { 
                SPMSDK.SPM_GetHttpProxyResponse(hSPM,  ref id, ref ret, cbbuffer, cbbuffer.Length  );
                string text = Encoding.UTF8.GetString(cbbuffer);
                content = string.Format("HTTP ID：{0} \nCODE: {1} \nHTTP响应内容 {2}", id, ret, text);
                this.BeginInvoke(new onLog(trace_log), content );
            }else if( code == SPMSDK.SPM_EVT_IOCHANGE )
            {
                int di = 0;
                SPMSDK.SPM_ReadDi(h, ref di);
                content = String.Format("IO Change=> {0}", di );
                this.BeginInvoke(new onLog(trace_log), content);
            }
        }

        SPMSDK.SPMCallBack onSPMEvent = null;

        private void button1_Click(object sender, EventArgs e)
        {
            try
            { 
                if (hSPM != IntPtr.Zero)
                {
                    SPMSDK.SPM_Close(hSPM);
                    SPMSDK.SPM_Destory(hSPM);
                }
                hSPM = SPMSDK.SPM_Create();
                if (hSPM == IntPtr.Zero)
                {
                    MessageBox.Show("返回对象为空！");
                    return;
                }
                if (onSPMEvent == null)
                    onSPMEvent = new SPMSDK.SPMCallBack(HandleSPMEvent);
                SPMSDK.SPM_SetCallBack(hSPM, onSPMEvent );
                // byte[] buf = Encoding.Default.GetBytes( this.comboBox_dev.Text );
                string text = this.comboBox_dev.Text;
                if (!SPMSDK.SPM_Open(hSPM, text))
                {
                    MessageBox.Show("打开设备错误,请查询动态库日志！\r\n");
                    return;
                } 
            }
            catch (Exception ex)
            {
                MessageBox.Show("出现错误：" + ex.Message);
            }
        }

        static int getColrValue(String text)
        {
            if ("RED".Equals(text))
                return 0x0000ff;
            else if ("GREEN".Equals(text))
                return 0xff00;
            else if ("BLUE".Equals(text))
                return 0xff0000;
            else if ("CYAN".Equals(text))
                return 0xffff00;
            else
                return 0xffff00;
        }

        private void button_led_display_Click(object sender, EventArgs e)
        {
            if (!SystemCheck())
                return;
            string color = comboBox_color.Text;
            int ali = comboBox_LED_Alig.SelectedIndex;
            byte[] buf = null;
            if (this.textBox_LED_1.Text.Length > 0)
            {
                buf = Encoding.Default.GetBytes(this.textBox_LED_1.Text);
                SPMSDK.SPM_Led_SendLineText(hSPM, 1, getColrValue(color), ali, buf, buf.Length);
            }
            if (this.textBox_LED_2.Text.Length > 0)
            {

                buf = Encoding.Default.GetBytes(this.textBox_LED_2.Text);
                SPMSDK.SPM_Led_SendLineText(hSPM, 2, getColrValue(color), ali, buf, buf.Length);
            }
            if (this.textBox_LED_3.Text.Length > 0)
            { 
                buf = Encoding.Default.GetBytes(this.textBox_LED_3.Text);
                SPMSDK.SPM_Led_SendLineText(hSPM, 3, getColrValue(color), ali, buf, buf.Length);
            }
            if (this.textBox_LED_4.Text.Length > 0)
            {
                buf = Encoding.Default.GetBytes(this.textBox_LED_4.Text);
                SPMSDK.SPM_Led_SendLineText(hSPM, 4, getColrValue(color), ali, buf, buf.Length);
            }
        }

        private void button_led_clear_Click(object sender, EventArgs e)
        {
            if (!SystemCheck())
                return;
            SPMSDK.SPM_Led_ClearAll(hSPM);
        }

        private void button_sound_play_Click(object sender, EventArgs e)
        {
            if (!SystemCheck())
                return;
            byte[] buf = Encoding.Default.GetBytes(this.textBox_sound.Text);
            if (buf.Length == 0)
            {
                MessageBox.Show("请先输入播报信息！");
                return;
            }
            SPMSDK.SPM_Voice_SendText(hSPM, comboBox_vol.SelectedIndex, buf, buf.Length);
        }

        private void button4_Click(object sender, EventArgs e)
        {
            if (!SystemCheck())
                return;
            byte[] buf = Encoding.Default.GetBytes(this.textBox_lcd.Text);
            SPMSDK.SPM_Lcd_ChangeContext(hSPM, comboBox_sence.SelectedIndex + 1, buf, buf.Length);
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            comboBox_LED_Alig.SelectedIndex = 1;
        }

        private void buttobn_seturl_Click(object sender, EventArgs e)
        {
            byte[] url = Encoding.Default.GetBytes(this.textBox_url.Text);
            int index = this.comboBox_index.SelectedIndex;
            SPMSDK.SPM_SetHttpProxyAddress(hSPM, index, url, url.Length); 
        }

        private void buttobn_settout_Click(object sender, EventArgs e)
        {
            trace_log("SPM_SetHttpProxyConfig");
            int v1 = Int32.Parse(this.textBox_contout.Text);
            int v2 = Int32.Parse(this.textBox_protout.Text);
            if( !SPMSDK.SPM_SetHttpProxyConfig(hSPM, v1, v2))
            {
                MessageBox.Show("SPM_SetHttpProxyConfig 错误！");
            }
        }

        private int index = 1;
  
        private void button_http_post_Click(object sender, EventArgs e)
        { 
            index = (index++) % 255 + 1;
            int v1 = this.comboBox_addr_index.SelectedIndex;
            int v2 = this.comboBox_type.SelectedIndex;
            trace_log(String.Format("{0} - {1}", v1, v2 ));
            byte[] data = Encoding.UTF8.GetBytes(this.textBox_payload.Text);
            trace_log("发起请求：" + this.textBox_payload.Text);
            SPMSDK.SPM_SetHttpPostProxyRequest(hSPM, v1, index, v2, data, data.Length); 
        }

        private void button_http_get_Click(object sender, EventArgs e)
        {
            index = (index++) % 255 + 1;
            byte[] data = Encoding.UTF8.GetBytes(this.textBox_url_get.Text);
            SPMSDK.SPM_SetHttpGetProxyRequest(hSPM, index, data, data.Length );
        }

        private void button_clr_Click(object sender, EventArgs e)
        {
            textBox_log.Text = "";
        }

        private void button2_Click(object sender, EventArgs e)
        {
            button_led_display_Click(null, null);
            button_sound_play_Click(null, null);
        }

        private void button5_Click(object sender, EventArgs e)
        {
            button_sound_play_Click(null, null);
            button4_Click(null, null);
        }
    }
}
