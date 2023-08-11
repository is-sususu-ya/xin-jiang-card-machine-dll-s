namespace SPMTester
{
    partial class Form1
    {
        /// <summary>
        /// 必需的设计器变量。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清理所有正在使用的资源。
        /// </summary>
        /// <param name="disposing">如果应释放托管资源，为 true；否则为 false。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows 窗体设计器生成的代码

        /// <summary>
        /// 设计器支持所需的方法 - 不要修改
        /// 使用代码编辑器修改此方法的内容。
        /// </summary>
        private void InitializeComponent()
        {
            this.button1 = new System.Windows.Forms.Button();
            this.comboBox_dev = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.button_led_display = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.comboBox_color = new System.Windows.Forms.ComboBox();
            this.comboBox_LED_Alig = new System.Windows.Forms.ComboBox();
            this.textBox_LED_4 = new System.Windows.Forms.TextBox();
            this.textBox_LED_3 = new System.Windows.Forms.TextBox();
            this.textBox_LED_2 = new System.Windows.Forms.TextBox();
            this.textBox_LED_1 = new System.Windows.Forms.TextBox();
            this.button_led_clear = new System.Windows.Forms.Button();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.textBox_sound = new System.Windows.Forms.TextBox();
            this.button_sound_play = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.comboBox_vol = new System.Windows.Forms.ComboBox();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.textBox_lcd = new System.Windows.Forms.TextBox();
            this.button4 = new System.Windows.Forms.Button();
            this.label3 = new System.Windows.Forms.Label();
            this.comboBox_sence = new System.Windows.Forms.ComboBox();
            this.label5 = new System.Windows.Forms.Label();
            this.label_qrcode = new System.Windows.Forms.Label();
            this.textBox_log = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.tabPage3 = new System.Windows.Forms.TabPage();
            this.groupBox5 = new System.Windows.Forms.GroupBox();
            this.button_http_get = new System.Windows.Forms.Button();
            this.textBox_url_get = new System.Windows.Forms.TextBox();
            this.button_http_post = new System.Windows.Forms.Button();
            this.label13 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.comboBox_addr_index = new System.Windows.Forms.ComboBox();
            this.comboBox_type = new System.Windows.Forms.ComboBox();
            this.label7 = new System.Windows.Forms.Label();
            this.textBox_payload = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.textBox_protout = new System.Windows.Forms.TextBox();
            this.textBox_contout = new System.Windows.Forms.TextBox();
            this.button3 = new System.Windows.Forms.Button();
            this.buttobn_seturl = new System.Windows.Forms.Button();
            this.comboBox_index = new System.Windows.Forms.ComboBox();
            this.textBox_url = new System.Windows.Forms.TextBox();
            this.label12 = new System.Windows.Forms.Label();
            this.label11 = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.label_online = new System.Windows.Forms.Label();
            this.button_clr = new System.Windows.Forms.Button();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.tabControl1.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.tabPage2.SuspendLayout();
            this.tabPage3.SuspendLayout();
            this.groupBox5.SuspendLayout();
            this.groupBox4.SuspendLayout();
            this.SuspendLayout();
            // 
            // button1
            // 
            this.button1.Font = new System.Drawing.Font("宋体", 12F);
            this.button1.Location = new System.Drawing.Point(196, 10);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(75, 30);
            this.button1.TabIndex = 1;
            this.button1.Text = "打开";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // comboBox_dev
            // 
            this.comboBox_dev.Font = new System.Drawing.Font("宋体", 12F);
            this.comboBox_dev.FormattingEnabled = true;
            this.comboBox_dev.Items.AddRange(new object[] {
            "COM1",
            "COM2",
            "COM3",
            "COM4",
            "COM5"});
            this.comboBox_dev.Location = new System.Drawing.Point(64, 12);
            this.comboBox_dev.Name = "comboBox_dev";
            this.comboBox_dev.Size = new System.Drawing.Size(121, 24);
            this.comboBox_dev.TabIndex = 2;
            this.comboBox_dev.Text = "COM1";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("宋体", 12F);
            this.label1.Location = new System.Drawing.Point(16, 16);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(40, 16);
            this.label1.TabIndex = 3;
            this.label1.Text = "地址";
            // 
            // button_led_display
            // 
            this.button_led_display.Location = new System.Drawing.Point(310, 18);
            this.button_led_display.Name = "button_led_display";
            this.button_led_display.Size = new System.Drawing.Size(75, 23);
            this.button_led_display.TabIndex = 4;
            this.button_led_display.Text = "显示";
            this.button_led_display.UseVisualStyleBackColor = true;
            this.button_led_display.Click += new System.EventHandler(this.button_led_display_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.comboBox_color);
            this.groupBox1.Controls.Add(this.comboBox_LED_Alig);
            this.groupBox1.Controls.Add(this.textBox_LED_4);
            this.groupBox1.Controls.Add(this.textBox_LED_3);
            this.groupBox1.Controls.Add(this.textBox_LED_2);
            this.groupBox1.Controls.Add(this.textBox_LED_1);
            this.groupBox1.Controls.Add(this.button_led_clear);
            this.groupBox1.Controls.Add(this.button_led_display);
            this.groupBox1.Location = new System.Drawing.Point(8, 6);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(407, 121);
            this.groupBox1.TabIndex = 5;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "LED控制";
            // 
            // comboBox_color
            // 
            this.comboBox_color.FormattingEnabled = true;
            this.comboBox_color.Items.AddRange(new object[] {
            "RED",
            "GREEN",
            "BLUE",
            "CYAN"});
            this.comboBox_color.Location = new System.Drawing.Point(299, 68);
            this.comboBox_color.Name = "comboBox_color";
            this.comboBox_color.Size = new System.Drawing.Size(102, 20);
            this.comboBox_color.TabIndex = 7;
            this.comboBox_color.Text = "RED";
            // 
            // comboBox_LED_Alig
            // 
            this.comboBox_LED_Alig.FormattingEnabled = true;
            this.comboBox_LED_Alig.Items.AddRange(new object[] {
            "左对齐",
            "居中对齐",
            "右对齐",
            "从右向左滚动靠边停止",
            "从左向右滚动靠边停止",
            "循环从右向左滚动",
            "循环从左向右滚动"});
            this.comboBox_LED_Alig.Location = new System.Drawing.Point(296, 91);
            this.comboBox_LED_Alig.Name = "comboBox_LED_Alig";
            this.comboBox_LED_Alig.Size = new System.Drawing.Size(105, 20);
            this.comboBox_LED_Alig.TabIndex = 6;
            this.comboBox_LED_Alig.Text = "左对齐";
            // 
            // textBox_LED_4
            // 
            this.textBox_LED_4.Location = new System.Drawing.Point(11, 92);
            this.textBox_LED_4.Name = "textBox_LED_4";
            this.textBox_LED_4.Size = new System.Drawing.Size(279, 21);
            this.textBox_LED_4.TabIndex = 5;
            this.textBox_LED_4.Text = "支付通道";
            // 
            // textBox_LED_3
            // 
            this.textBox_LED_3.Location = new System.Drawing.Point(11, 68);
            this.textBox_LED_3.Name = "textBox_LED_3";
            this.textBox_LED_3.Size = new System.Drawing.Size(279, 21);
            this.textBox_LED_3.TabIndex = 5;
            this.textBox_LED_3.Text = "支持ETC、支付宝";
            // 
            // textBox_LED_2
            // 
            this.textBox_LED_2.Location = new System.Drawing.Point(11, 44);
            this.textBox_LED_2.Name = "textBox_LED_2";
            this.textBox_LED_2.Size = new System.Drawing.Size(279, 21);
            this.textBox_LED_2.TabIndex = 5;
            this.textBox_LED_2.Text = "交通技术";
            // 
            // textBox_LED_1
            // 
            this.textBox_LED_1.Location = new System.Drawing.Point(11, 20);
            this.textBox_LED_1.Name = "textBox_LED_1";
            this.textBox_LED_1.Size = new System.Drawing.Size(279, 21);
            this.textBox_LED_1.TabIndex = 5;
            this.textBox_LED_1.Text = "苏州德亚";
            // 
            // button_led_clear
            // 
            this.button_led_clear.Location = new System.Drawing.Point(310, 43);
            this.button_led_clear.Name = "button_led_clear";
            this.button_led_clear.Size = new System.Drawing.Size(75, 23);
            this.button_led_clear.TabIndex = 4;
            this.button_led_clear.Text = "清屏";
            this.button_led_clear.UseVisualStyleBackColor = true;
            this.button_led_clear.Click += new System.EventHandler(this.button_led_clear_Click);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.textBox_sound);
            this.groupBox2.Controls.Add(this.button_sound_play);
            this.groupBox2.Controls.Add(this.label2);
            this.groupBox2.Controls.Add(this.comboBox_vol);
            this.groupBox2.Location = new System.Drawing.Point(8, 146);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(407, 82);
            this.groupBox2.TabIndex = 5;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "语音控制";
            // 
            // textBox_sound
            // 
            this.textBox_sound.Location = new System.Drawing.Point(11, 20);
            this.textBox_sound.Name = "textBox_sound";
            this.textBox_sound.Size = new System.Drawing.Size(390, 21);
            this.textBox_sound.TabIndex = 5;
            this.textBox_sound.Text = "苏州德亚交通技术有限公司";
            // 
            // button_sound_play
            // 
            this.button_sound_play.Location = new System.Drawing.Point(326, 47);
            this.button_sound_play.Name = "button_sound_play";
            this.button_sound_play.Size = new System.Drawing.Size(75, 23);
            this.button_sound_play.TabIndex = 4;
            this.button_sound_play.Text = "播放";
            this.button_sound_play.UseVisualStyleBackColor = true;
            this.button_sound_play.Click += new System.EventHandler(this.button_sound_play_Click);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(160, 53);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(29, 12);
            this.label2.TabIndex = 3;
            this.label2.Text = "音量";
            // 
            // comboBox_vol
            // 
            this.comboBox_vol.FormattingEnabled = true;
            this.comboBox_vol.Items.AddRange(new object[] {
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8"});
            this.comboBox_vol.Location = new System.Drawing.Point(199, 50);
            this.comboBox_vol.Name = "comboBox_vol";
            this.comboBox_vol.Size = new System.Drawing.Size(121, 20);
            this.comboBox_vol.TabIndex = 2;
            this.comboBox_vol.Text = "4";
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.textBox_lcd);
            this.groupBox3.Controls.Add(this.button4);
            this.groupBox3.Controls.Add(this.label3);
            this.groupBox3.Controls.Add(this.comboBox_sence);
            this.groupBox3.Location = new System.Drawing.Point(6, 18);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(407, 82);
            this.groupBox3.TabIndex = 5;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "LCD屏控制";
            // 
            // textBox_lcd
            // 
            this.textBox_lcd.Location = new System.Drawing.Point(11, 20);
            this.textBox_lcd.Name = "textBox_lcd";
            this.textBox_lcd.Size = new System.Drawing.Size(390, 21);
            this.textBox_lcd.TabIndex = 5;
            this.textBox_lcd.Text = "苏E12345;123元;5小时三分钟";
            // 
            // button4
            // 
            this.button4.Location = new System.Drawing.Point(326, 47);
            this.button4.Name = "button4";
            this.button4.Size = new System.Drawing.Size(75, 23);
            this.button4.TabIndex = 4;
            this.button4.Text = "切换";
            this.button4.UseVisualStyleBackColor = true;
            this.button4.Click += new System.EventHandler(this.button4_Click);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(160, 53);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(29, 12);
            this.label3.TabIndex = 3;
            this.label3.Text = "场景";
            // 
            // comboBox_sence
            // 
            this.comboBox_sence.FormattingEnabled = true;
            this.comboBox_sence.Items.AddRange(new object[] {
            "1",
            "2",
            "3",
            "4"});
            this.comboBox_sence.Location = new System.Drawing.Point(199, 50);
            this.comboBox_sence.Name = "comboBox_sence";
            this.comboBox_sence.Size = new System.Drawing.Size(121, 20);
            this.comboBox_sence.TabIndex = 2;
            this.comboBox_sence.Text = "1";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("宋体", 12F);
            this.label5.Location = new System.Drawing.Point(16, 50);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(88, 16);
            this.label5.TabIndex = 6;
            this.label5.Text = "扫码结果：";
            // 
            // label_qrcode
            // 
            this.label_qrcode.AutoSize = true;
            this.label_qrcode.Font = new System.Drawing.Font("宋体", 16F);
            this.label_qrcode.Location = new System.Drawing.Point(104, 46);
            this.label_qrcode.Name = "label_qrcode";
            this.label_qrcode.Size = new System.Drawing.Size(274, 22);
            this.label_qrcode.TabIndex = 6;
            this.label_qrcode.Text = "111111111111111111111111";
            // 
            // textBox_log
            // 
            this.textBox_log.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox_log.Location = new System.Drawing.Point(455, 94);
            this.textBox_log.Multiline = true;
            this.textBox_log.Name = "textBox_log";
            this.textBox_log.Size = new System.Drawing.Size(433, 342);
            this.textBox_log.TabIndex = 7;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Font = new System.Drawing.Font("宋体", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.label6.Location = new System.Drawing.Point(456, 75);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(56, 16);
            this.label6.TabIndex = 8;
            this.label6.Text = "日志：";
            // 
            // tabControl1
            // 
            this.tabControl1.Controls.Add(this.tabPage1);
            this.tabControl1.Controls.Add(this.tabPage2);
            this.tabControl1.Controls.Add(this.tabPage3);
            this.tabControl1.Location = new System.Drawing.Point(12, 75);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(438, 317);
            this.tabControl1.TabIndex = 9;
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.groupBox1);
            this.tabPage1.Controls.Add(this.groupBox2);
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(430, 291);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "LED以及语音控制";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // tabPage2
            // 
            this.tabPage2.Controls.Add(this.groupBox3);
            this.tabPage2.Location = new System.Drawing.Point(4, 22);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(430, 291);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "LCD屏控制";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // tabPage3
            // 
            this.tabPage3.Controls.Add(this.groupBox5);
            this.tabPage3.Controls.Add(this.groupBox4);
            this.tabPage3.Location = new System.Drawing.Point(4, 22);
            this.tabPage3.Name = "tabPage3";
            this.tabPage3.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage3.Size = new System.Drawing.Size(430, 291);
            this.tabPage3.TabIndex = 2;
            this.tabPage3.Text = "Http转发控制";
            this.tabPage3.UseVisualStyleBackColor = true;
            // 
            // groupBox5
            // 
            this.groupBox5.Controls.Add(this.button_http_get);
            this.groupBox5.Controls.Add(this.textBox_url_get);
            this.groupBox5.Controls.Add(this.button_http_post);
            this.groupBox5.Controls.Add(this.label13);
            this.groupBox5.Controls.Add(this.label8);
            this.groupBox5.Controls.Add(this.comboBox_addr_index);
            this.groupBox5.Controls.Add(this.comboBox_type);
            this.groupBox5.Controls.Add(this.label7);
            this.groupBox5.Controls.Add(this.textBox_payload);
            this.groupBox5.Controls.Add(this.label4);
            this.groupBox5.Location = new System.Drawing.Point(6, 112);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Size = new System.Drawing.Size(412, 173);
            this.groupBox5.TabIndex = 11;
            this.groupBox5.TabStop = false;
            this.groupBox5.Text = "发起请求";
            // 
            // button_http_get
            // 
            this.button_http_get.Location = new System.Drawing.Point(331, 144);
            this.button_http_get.Name = "button_http_get";
            this.button_http_get.Size = new System.Drawing.Size(75, 23);
            this.button_http_get.TabIndex = 8;
            this.button_http_get.Text = "发起请求";
            this.button_http_get.UseVisualStyleBackColor = true;
            this.button_http_get.Click += new System.EventHandler(this.button_http_get_Click);
            // 
            // textBox_url_get
            // 
            this.textBox_url_get.Location = new System.Drawing.Point(9, 146);
            this.textBox_url_get.Name = "textBox_url_get";
            this.textBox_url_get.Size = new System.Drawing.Size(305, 21);
            this.textBox_url_get.TabIndex = 7;
            this.textBox_url_get.Text = "http://www.baidu.com";
            // 
            // button_http_post
            // 
            this.button_http_post.Location = new System.Drawing.Point(331, 18);
            this.button_http_post.Name = "button_http_post";
            this.button_http_post.Size = new System.Drawing.Size(75, 23);
            this.button_http_post.TabIndex = 6;
            this.button_http_post.Text = "发起请求";
            this.button_http_post.UseVisualStyleBackColor = true;
            this.button_http_post.Click += new System.EventHandler(this.button_http_post_Click);
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.Location = new System.Drawing.Point(9, 127);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(47, 12);
            this.label13.TabIndex = 5;
            this.label13.Text = "GET请求";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(9, 54);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(77, 12);
            this.label8.TabIndex = 5;
            this.label8.Text = "POST请求参数";
            // 
            // comboBox_addr_index
            // 
            this.comboBox_addr_index.FormattingEnabled = true;
            this.comboBox_addr_index.Items.AddRange(new object[] {
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9"});
            this.comboBox_addr_index.Location = new System.Drawing.Point(72, 21);
            this.comboBox_addr_index.Name = "comboBox_addr_index";
            this.comboBox_addr_index.Size = new System.Drawing.Size(58, 20);
            this.comboBox_addr_index.TabIndex = 0;
            this.comboBox_addr_index.Text = "0";
            // 
            // comboBox_type
            // 
            this.comboBox_type.FormattingEnabled = true;
            this.comboBox_type.Items.AddRange(new object[] {
            "json",
            "urlencode"});
            this.comboBox_type.Location = new System.Drawing.Point(208, 21);
            this.comboBox_type.Name = "comboBox_type";
            this.comboBox_type.Size = new System.Drawing.Size(58, 20);
            this.comboBox_type.TabIndex = 3;
            this.comboBox_type.Text = "json";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(149, 25);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(53, 12);
            this.label7.TabIndex = 5;
            this.label7.Text = "请求类型";
            // 
            // textBox_payload
            // 
            this.textBox_payload.Location = new System.Drawing.Point(92, 48);
            this.textBox_payload.Multiline = true;
            this.textBox_payload.Name = "textBox_payload";
            this.textBox_payload.Size = new System.Drawing.Size(308, 77);
            this.textBox_payload.TabIndex = 4;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(9, 25);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(53, 12);
            this.label4.TabIndex = 5;
            this.label4.Text = "选择地址";
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.textBox_protout);
            this.groupBox4.Controls.Add(this.textBox_contout);
            this.groupBox4.Controls.Add(this.button3);
            this.groupBox4.Controls.Add(this.buttobn_seturl);
            this.groupBox4.Controls.Add(this.comboBox_index);
            this.groupBox4.Controls.Add(this.textBox_url);
            this.groupBox4.Controls.Add(this.label12);
            this.groupBox4.Controls.Add(this.label11);
            this.groupBox4.Controls.Add(this.label10);
            this.groupBox4.Controls.Add(this.label9);
            this.groupBox4.Location = new System.Drawing.Point(6, 9);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(418, 97);
            this.groupBox4.TabIndex = 7;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "设定地址";
            // 
            // textBox_protout
            // 
            this.textBox_protout.Location = new System.Drawing.Point(210, 71);
            this.textBox_protout.Name = "textBox_protout";
            this.textBox_protout.Size = new System.Drawing.Size(56, 21);
            this.textBox_protout.TabIndex = 12;
            this.textBox_protout.Text = "5";
            // 
            // textBox_contout
            // 
            this.textBox_contout.Location = new System.Drawing.Point(76, 71);
            this.textBox_contout.Name = "textBox_contout";
            this.textBox_contout.Size = new System.Drawing.Size(58, 21);
            this.textBox_contout.TabIndex = 12;
            this.textBox_contout.Text = "5";
            // 
            // button3
            // 
            this.button3.Location = new System.Drawing.Point(311, 70);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(95, 23);
            this.button3.TabIndex = 2;
            this.button3.Text = "设置超时参数";
            this.button3.UseVisualStyleBackColor = true;
            this.button3.Click += new System.EventHandler(this.buttobn_settout_Click);
            // 
            // buttobn_seturl
            // 
            this.buttobn_seturl.Location = new System.Drawing.Point(331, 43);
            this.buttobn_seturl.Name = "buttobn_seturl";
            this.buttobn_seturl.Size = new System.Drawing.Size(75, 23);
            this.buttobn_seturl.TabIndex = 2;
            this.buttobn_seturl.Text = "设置地址";
            this.buttobn_seturl.UseVisualStyleBackColor = true;
            this.buttobn_seturl.Click += new System.EventHandler(this.buttobn_seturl_Click);
            // 
            // comboBox_index
            // 
            this.comboBox_index.FormattingEnabled = true;
            this.comboBox_index.Items.AddRange(new object[] {
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9"});
            this.comboBox_index.Location = new System.Drawing.Point(76, 43);
            this.comboBox_index.Name = "comboBox_index";
            this.comboBox_index.Size = new System.Drawing.Size(58, 20);
            this.comboBox_index.TabIndex = 0;
            this.comboBox_index.Text = "0";
            // 
            // textBox_url
            // 
            this.textBox_url.Location = new System.Drawing.Point(76, 16);
            this.textBox_url.Name = "textBox_url";
            this.textBox_url.Size = new System.Drawing.Size(336, 21);
            this.textBox_url.TabIndex = 1;
            this.textBox_url.Text = "http://baidu.com";
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Location = new System.Drawing.Point(140, 75);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(65, 12);
            this.label12.TabIndex = 5;
            this.label12.Text = "处理超时：";
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Location = new System.Drawing.Point(9, 75);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(53, 12);
            this.label11.TabIndex = 5;
            this.label11.Text = "连接超时";
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(9, 21);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(41, 12);
            this.label10.TabIndex = 5;
            this.label10.Text = "地址值";
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(9, 47);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(29, 12);
            this.label9.TabIndex = 5;
            this.label9.Text = "编号";
            // 
            // label_online
            // 
            this.label_online.AutoSize = true;
            this.label_online.Location = new System.Drawing.Point(311, 22);
            this.label_online.Name = "label_online";
            this.label_online.Size = new System.Drawing.Size(29, 12);
            this.label_online.TabIndex = 10;
            this.label_online.Text = "在线";
            // 
            // button_clr
            // 
            this.button_clr.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.button_clr.Location = new System.Drawing.Point(813, 68);
            this.button_clr.Name = "button_clr";
            this.button_clr.Size = new System.Drawing.Size(75, 23);
            this.button_clr.TabIndex = 11;
            this.button_clr.Text = "清空日志";
            this.button_clr.UseVisualStyleBackColor = true;
            this.button_clr.Click += new System.EventHandler(this.button_clr_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(893, 440);
            this.Controls.Add(this.button_clr);
            this.Controls.Add(this.label_online);
            this.Controls.Add(this.tabControl1);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.textBox_log);
            this.Controls.Add(this.label_qrcode);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.comboBox_dev);
            this.Controls.Add(this.button1);
            this.Name = "Form1";
            this.Text = "自助缴费机测试控件";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            this.tabControl1.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.tabPage2.ResumeLayout(false);
            this.tabPage3.ResumeLayout(false);
            this.groupBox5.ResumeLayout(false);
            this.groupBox5.PerformLayout();
            this.groupBox4.ResumeLayout(false);
            this.groupBox4.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.ComboBox comboBox_dev;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button button_led_display;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.TextBox textBox_LED_4;
        private System.Windows.Forms.TextBox textBox_LED_3;
        private System.Windows.Forms.TextBox textBox_LED_2;
        private System.Windows.Forms.TextBox textBox_LED_1;
        private System.Windows.Forms.Button button_led_clear;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.TextBox textBox_sound;
        private System.Windows.Forms.Button button_sound_play;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ComboBox comboBox_vol;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.TextBox textBox_lcd;
        private System.Windows.Forms.Button button4;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.ComboBox comboBox_sence;
        private System.Windows.Forms.ComboBox comboBox_LED_Alig;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label_qrcode;
        private System.Windows.Forms.TextBox textBox_log;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.TabPage tabPage2;
        private System.Windows.Forms.Label label_online;
        private System.Windows.Forms.ComboBox comboBox_color;
        private System.Windows.Forms.TabPage tabPage3;
        private System.Windows.Forms.Button buttobn_seturl;
        private System.Windows.Forms.TextBox textBox_url;
        private System.Windows.Forms.ComboBox comboBox_index;
        private System.Windows.Forms.ComboBox comboBox_type;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox textBox_payload;
        private System.Windows.Forms.ComboBox comboBox_addr_index;
        private System.Windows.Forms.Button button_http_post;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.GroupBox groupBox4;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.GroupBox groupBox5;
        private System.Windows.Forms.TextBox textBox_contout;
        private System.Windows.Forms.TextBox textBox_protout;
        private System.Windows.Forms.Button button3;
        private System.Windows.Forms.Label label12;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.TextBox textBox_url_get;
        private System.Windows.Forms.Label label13;
        private System.Windows.Forms.Button button_http_get;
        private System.Windows.Forms.Button button_clr;
    }
}

