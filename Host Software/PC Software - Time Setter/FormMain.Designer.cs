namespace PINPoint_v5_0
{
    partial class FormMain
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FormMain));
            this.WriteThread = new System.ComponentModel.BackgroundWorker();
            this.ReadThread = new System.ComponentModel.BackgroundWorker();
            this.FormUpdateTimer = new System.Windows.Forms.Timer(this.components);
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
            this.toolStripStatusLabelConnected = new System.Windows.Forms.ToolStripStatusLabel();
            this.toolStripProgressBar = new System.Windows.Forms.ToolStripProgressBar();
            this.buttonSetDateTime = new System.Windows.Forms.Button();
            this.textBoxYear = new System.Windows.Forms.TextBox();
            this.textBoxMonth = new System.Windows.Forms.TextBox();
            this.textBoxDay = new System.Windows.Forms.TextBox();
            this.textBoxHour = new System.Windows.Forms.TextBox();
            this.textBoxMinute = new System.Windows.Forms.TextBox();
            this.textBoxSecond = new System.Windows.Forms.TextBox();
            this.textBoxAccelX = new System.Windows.Forms.TextBox();
            this.textBoxAccelY = new System.Windows.Forms.TextBox();
            this.textBoxAccelZ = new System.Windows.Forms.TextBox();
            this.textBoxTemperature = new System.Windows.Forms.TextBox();
            this.textBoxPressure = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.textBoxLight = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.textBoxAccelMag = new System.Windows.Forms.TextBox();
            this.label7 = new System.Windows.Forms.Label();
            this.textBoxAltitude = new System.Windows.Forms.TextBox();
            this.label8 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.label11 = new System.Windows.Forms.Label();
            this.label12 = new System.Windows.Forms.Label();
            this.label13 = new System.Windows.Forms.Label();
            this.label14 = new System.Windows.Forms.Label();
            this.label15 = new System.Windows.Forms.Label();
            this.label16 = new System.Windows.Forms.Label();
            this.label17 = new System.Windows.Forms.Label();
            this.label18 = new System.Windows.Forms.Label();
            this.groupBoxAcceleration = new System.Windows.Forms.GroupBox();
            this.label26 = new System.Windows.Forms.Label();
            this.label25 = new System.Windows.Forms.Label();
            this.label24 = new System.Windows.Forms.Label();
            this.label23 = new System.Windows.Forms.Label();
            this.label22 = new System.Windows.Forms.Label();
            this.label21 = new System.Windows.Forms.Label();
            this.label20 = new System.Windows.Forms.Label();
            this.label19 = new System.Windows.Forms.Label();
            this.label28 = new System.Windows.Forms.Label();
            this.textBoxBatteryVoltage = new System.Windows.Forms.TextBox();
            this.label29 = new System.Windows.Forms.Label();
            this.statusStrip1.SuspendLayout();
            this.groupBoxAcceleration.SuspendLayout();
            this.SuspendLayout();
            // 
            // WriteThread
            // 
            this.WriteThread.WorkerReportsProgress = true;
            this.WriteThread.DoWork += new System.ComponentModel.DoWorkEventHandler(this.WriteThread_DoWork);
            // 
            // ReadThread
            // 
            this.ReadThread.DoWork += new System.ComponentModel.DoWorkEventHandler(this.ReadThread_DoWork);
            // 
            // FormUpdateTimer
            // 
            this.FormUpdateTimer.Enabled = true;
            this.FormUpdateTimer.Interval = 6;
            this.FormUpdateTimer.Tick += new System.EventHandler(this.FormUpdateTimer_Tick);
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel1,
            this.toolStripStatusLabelConnected,
            this.toolStripProgressBar});
            this.statusStrip1.Location = new System.Drawing.Point(0, 108);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(994, 22);
            this.statusStrip1.TabIndex = 28;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // toolStripStatusLabel1
            // 
            this.toolStripStatusLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
            this.toolStripStatusLabel1.Padding = new System.Windows.Forms.Padding(5, 0, 5, 0);
            this.toolStripStatusLabel1.Size = new System.Drawing.Size(54, 17);
            this.toolStripStatusLabel1.Text = "Status";
            // 
            // toolStripStatusLabelConnected
            // 
            this.toolStripStatusLabelConnected.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(192)))), ((int)(((byte)(0)))), ((int)(((byte)(0)))));
            this.toolStripStatusLabelConnected.BorderSides = ((System.Windows.Forms.ToolStripStatusLabelBorderSides)((((System.Windows.Forms.ToolStripStatusLabelBorderSides.Left | System.Windows.Forms.ToolStripStatusLabelBorderSides.Top) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Right) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Bottom)));
            this.toolStripStatusLabelConnected.BorderStyle = System.Windows.Forms.Border3DStyle.SunkenOuter;
            this.toolStripStatusLabelConnected.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.toolStripStatusLabelConnected.ForeColor = System.Drawing.Color.Black;
            this.toolStripStatusLabelConnected.Name = "toolStripStatusLabelConnected";
            this.toolStripStatusLabelConnected.Padding = new System.Windows.Forms.Padding(5, 0, 5, 0);
            this.toolStripStatusLabelConnected.Size = new System.Drawing.Size(97, 17);
            this.toolStripStatusLabelConnected.Text = "Disconnected";
            // 
            // toolStripProgressBar
            // 
            this.toolStripProgressBar.Name = "toolStripProgressBar";
            this.toolStripProgressBar.Padding = new System.Windows.Forms.Padding(20, 0, 0, 0);
            this.toolStripProgressBar.Size = new System.Drawing.Size(820, 16);
            // 
            // buttonSetDateTime
            // 
            this.buttonSetDateTime.Location = new System.Drawing.Point(203, 10);
            this.buttonSetDateTime.Name = "buttonSetDateTime";
            this.buttonSetDateTime.Size = new System.Drawing.Size(130, 23);
            this.buttonSetDateTime.TabIndex = 29;
            this.buttonSetDateTime.Text = "Set Device Date/Time";
            this.buttonSetDateTime.UseVisualStyleBackColor = true;
            this.buttonSetDateTime.Click += new System.EventHandler(this.buttonSetDateTime_Click);
            // 
            // textBoxYear
            // 
            this.textBoxYear.Location = new System.Drawing.Point(47, 12);
            this.textBoxYear.Name = "textBoxYear";
            this.textBoxYear.Size = new System.Drawing.Size(38, 20);
            this.textBoxYear.TabIndex = 30;
            // 
            // textBoxMonth
            // 
            this.textBoxMonth.Location = new System.Drawing.Point(98, 12);
            this.textBoxMonth.Name = "textBoxMonth";
            this.textBoxMonth.Size = new System.Drawing.Size(38, 20);
            this.textBoxMonth.TabIndex = 31;
            // 
            // textBoxDay
            // 
            this.textBoxDay.Location = new System.Drawing.Point(149, 12);
            this.textBoxDay.Name = "textBoxDay";
            this.textBoxDay.Size = new System.Drawing.Size(38, 20);
            this.textBoxDay.TabIndex = 32;
            // 
            // textBoxHour
            // 
            this.textBoxHour.Location = new System.Drawing.Point(47, 38);
            this.textBoxHour.Name = "textBoxHour";
            this.textBoxHour.Size = new System.Drawing.Size(38, 20);
            this.textBoxHour.TabIndex = 33;
            // 
            // textBoxMinute
            // 
            this.textBoxMinute.Location = new System.Drawing.Point(98, 38);
            this.textBoxMinute.Name = "textBoxMinute";
            this.textBoxMinute.Size = new System.Drawing.Size(38, 20);
            this.textBoxMinute.TabIndex = 34;
            // 
            // textBoxSecond
            // 
            this.textBoxSecond.Location = new System.Drawing.Point(149, 38);
            this.textBoxSecond.Name = "textBoxSecond";
            this.textBoxSecond.Size = new System.Drawing.Size(38, 20);
            this.textBoxSecond.TabIndex = 35;
            // 
            // textBoxAccelX
            // 
            this.textBoxAccelX.Location = new System.Drawing.Point(28, 19);
            this.textBoxAccelX.Name = "textBoxAccelX";
            this.textBoxAccelX.Size = new System.Drawing.Size(54, 20);
            this.textBoxAccelX.TabIndex = 37;
            // 
            // textBoxAccelY
            // 
            this.textBoxAccelY.Location = new System.Drawing.Point(28, 45);
            this.textBoxAccelY.Name = "textBoxAccelY";
            this.textBoxAccelY.Size = new System.Drawing.Size(54, 20);
            this.textBoxAccelY.TabIndex = 38;
            // 
            // textBoxAccelZ
            // 
            this.textBoxAccelZ.Location = new System.Drawing.Point(191, 19);
            this.textBoxAccelZ.Name = "textBoxAccelZ";
            this.textBoxAccelZ.Size = new System.Drawing.Size(54, 20);
            this.textBoxAccelZ.TabIndex = 39;
            // 
            // textBoxTemperature
            // 
            this.textBoxTemperature.Location = new System.Drawing.Point(415, 12);
            this.textBoxTemperature.Name = "textBoxTemperature";
            this.textBoxTemperature.Size = new System.Drawing.Size(57, 20);
            this.textBoxTemperature.TabIndex = 40;
            // 
            // textBoxPressure
            // 
            this.textBoxPressure.Location = new System.Drawing.Point(415, 38);
            this.textBoxPressure.Name = "textBoxPressure";
            this.textBoxPressure.Size = new System.Drawing.Size(57, 20);
            this.textBoxPressure.TabIndex = 41;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(8, 22);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(14, 13);
            this.label1.TabIndex = 42;
            this.label1.Text = "X";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(171, 22);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(14, 13);
            this.label2.TabIndex = 43;
            this.label2.Text = "Z";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(8, 48);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(14, 13);
            this.label3.TabIndex = 44;
            this.label3.Text = "Y";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(342, 15);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(67, 13);
            this.label4.TabIndex = 45;
            this.label4.Text = "Temperature";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(361, 41);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(48, 13);
            this.label5.TabIndex = 46;
            this.label5.Text = "Pressure";
            // 
            // textBoxLight
            // 
            this.textBoxLight.Location = new System.Drawing.Point(603, 38);
            this.textBoxLight.Name = "textBoxLight";
            this.textBoxLight.Size = new System.Drawing.Size(57, 20);
            this.textBoxLight.TabIndex = 47;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(567, 41);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(30, 13);
            this.label6.TabIndex = 48;
            this.label6.Text = "Light";
            // 
            // textBoxAccelMag
            // 
            this.textBoxAccelMag.Location = new System.Drawing.Point(191, 45);
            this.textBoxAccelMag.Name = "textBoxAccelMag";
            this.textBoxAccelMag.Size = new System.Drawing.Size(54, 20);
            this.textBoxAccelMag.TabIndex = 49;
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(128, 48);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(57, 13);
            this.label7.TabIndex = 50;
            this.label7.Text = "Magnitude";
            // 
            // textBoxAltitude
            // 
            this.textBoxAltitude.Location = new System.Drawing.Point(603, 12);
            this.textBoxAltitude.Name = "textBoxAltitude";
            this.textBoxAltitude.Size = new System.Drawing.Size(57, 20);
            this.textBoxAltitude.TabIndex = 51;
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(511, 15);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(86, 13);
            this.label8.TabIndex = 52;
            this.label8.Text = "Pressure Altitude";
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(9, 15);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(30, 13);
            this.label9.TabIndex = 55;
            this.label9.Text = "Date";
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(9, 41);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(30, 13);
            this.label10.TabIndex = 56;
            this.label10.Text = "Time";
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label11.Location = new System.Drawing.Point(85, 10);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(14, 20);
            this.label11.TabIndex = 57;
            this.label11.Text = ":";
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label12.Location = new System.Drawing.Point(85, 36);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(14, 20);
            this.label12.TabIndex = 58;
            this.label12.Text = ":";
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label13.Location = new System.Drawing.Point(136, 36);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(14, 20);
            this.label13.TabIndex = 59;
            this.label13.Text = ":";
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label14.Location = new System.Drawing.Point(136, 10);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(14, 20);
            this.label14.TabIndex = 60;
            this.label14.Text = ":";
            // 
            // label15
            // 
            this.label15.AutoSize = true;
            this.label15.Location = new System.Drawing.Point(472, 15);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(18, 13);
            this.label15.TabIndex = 61;
            this.label15.Text = "°C";
            // 
            // label16
            // 
            this.label16.AutoSize = true;
            this.label16.Location = new System.Drawing.Point(474, 41);
            this.label16.Name = "label16";
            this.label16.Size = new System.Drawing.Size(26, 13);
            this.label16.TabIndex = 62;
            this.label16.Text = "kPa";
            // 
            // label17
            // 
            this.label17.AutoSize = true;
            this.label17.Location = new System.Drawing.Point(662, 15);
            this.label17.Name = "label17";
            this.label17.Size = new System.Drawing.Size(15, 13);
            this.label17.TabIndex = 63;
            this.label17.Text = "m";
            // 
            // label18
            // 
            this.label18.AutoSize = true;
            this.label18.Location = new System.Drawing.Point(662, 41);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(20, 13);
            this.label18.TabIndex = 64;
            this.label18.Text = "lux";
            // 
            // groupBoxAcceleration
            // 
            this.groupBoxAcceleration.Controls.Add(this.label26);
            this.groupBoxAcceleration.Controls.Add(this.label25);
            this.groupBoxAcceleration.Controls.Add(this.label24);
            this.groupBoxAcceleration.Controls.Add(this.label23);
            this.groupBoxAcceleration.Controls.Add(this.label22);
            this.groupBoxAcceleration.Controls.Add(this.label21);
            this.groupBoxAcceleration.Controls.Add(this.label20);
            this.groupBoxAcceleration.Controls.Add(this.label19);
            this.groupBoxAcceleration.Controls.Add(this.textBoxAccelX);
            this.groupBoxAcceleration.Controls.Add(this.label1);
            this.groupBoxAcceleration.Controls.Add(this.textBoxAccelY);
            this.groupBoxAcceleration.Controls.Add(this.label3);
            this.groupBoxAcceleration.Controls.Add(this.textBoxAccelZ);
            this.groupBoxAcceleration.Controls.Add(this.label2);
            this.groupBoxAcceleration.Controls.Add(this.textBoxAccelMag);
            this.groupBoxAcceleration.Controls.Add(this.label7);
            this.groupBoxAcceleration.Location = new System.Drawing.Point(694, 10);
            this.groupBoxAcceleration.Name = "groupBoxAcceleration";
            this.groupBoxAcceleration.Size = new System.Drawing.Size(288, 75);
            this.groupBoxAcceleration.TabIndex = 65;
            this.groupBoxAcceleration.TabStop = false;
            this.groupBoxAcceleration.Text = "Acceleration";
            // 
            // label26
            // 
            this.label26.AutoSize = true;
            this.label26.Font = new System.Drawing.Font("Microsoft Sans Serif", 6F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label26.Location = new System.Drawing.Point(268, 48);
            this.label26.Name = "label26";
            this.label26.Size = new System.Drawing.Size(9, 9);
            this.label26.TabIndex = 72;
            this.label26.Text = "2";
            // 
            // label25
            // 
            this.label25.AutoSize = true;
            this.label25.Font = new System.Drawing.Font("Microsoft Sans Serif", 6F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label25.Location = new System.Drawing.Point(268, 22);
            this.label25.Name = "label25";
            this.label25.Size = new System.Drawing.Size(9, 9);
            this.label25.TabIndex = 71;
            this.label25.Text = "2";
            // 
            // label24
            // 
            this.label24.AutoSize = true;
            this.label24.Font = new System.Drawing.Font("Microsoft Sans Serif", 6F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label24.Location = new System.Drawing.Point(105, 48);
            this.label24.Name = "label24";
            this.label24.Size = new System.Drawing.Size(9, 9);
            this.label24.TabIndex = 70;
            this.label24.Text = "2";
            // 
            // label23
            // 
            this.label23.AutoSize = true;
            this.label23.Font = new System.Drawing.Font("Microsoft Sans Serif", 6F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label23.Location = new System.Drawing.Point(105, 22);
            this.label23.Name = "label23";
            this.label23.Size = new System.Drawing.Size(9, 9);
            this.label23.TabIndex = 69;
            this.label23.Text = "2";
            // 
            // label22
            // 
            this.label22.AutoSize = true;
            this.label22.Location = new System.Drawing.Point(248, 48);
            this.label22.Name = "label22";
            this.label22.Size = new System.Drawing.Size(25, 13);
            this.label22.TabIndex = 68;
            this.label22.Text = "m/s";
            // 
            // label21
            // 
            this.label21.AutoSize = true;
            this.label21.Location = new System.Drawing.Point(248, 22);
            this.label21.Name = "label21";
            this.label21.Size = new System.Drawing.Size(25, 13);
            this.label21.TabIndex = 67;
            this.label21.Text = "m/s";
            // 
            // label20
            // 
            this.label20.AutoSize = true;
            this.label20.Location = new System.Drawing.Point(85, 48);
            this.label20.Name = "label20";
            this.label20.Size = new System.Drawing.Size(25, 13);
            this.label20.TabIndex = 66;
            this.label20.Text = "m/s";
            // 
            // label19
            // 
            this.label19.AutoSize = true;
            this.label19.Location = new System.Drawing.Point(85, 22);
            this.label19.Name = "label19";
            this.label19.Size = new System.Drawing.Size(25, 13);
            this.label19.TabIndex = 65;
            this.label19.Text = "m/s";
            // 
            // label28
            // 
            this.label28.AutoSize = true;
            this.label28.Location = new System.Drawing.Point(200, 41);
            this.label28.Name = "label28";
            this.label28.Size = new System.Drawing.Size(40, 13);
            this.label28.TabIndex = 66;
            this.label28.Text = "Battery";
            // 
            // textBoxBatteryVoltage
            // 
            this.textBoxBatteryVoltage.Location = new System.Drawing.Point(246, 38);
            this.textBoxBatteryVoltage.Name = "textBoxBatteryVoltage";
            this.textBoxBatteryVoltage.Size = new System.Drawing.Size(71, 20);
            this.textBoxBatteryVoltage.TabIndex = 67;
            // 
            // label29
            // 
            this.label29.AutoSize = true;
            this.label29.Location = new System.Drawing.Point(318, 41);
            this.label29.Name = "label29";
            this.label29.Size = new System.Drawing.Size(14, 13);
            this.label29.TabIndex = 68;
            this.label29.Text = "V";
            // 
            // FormMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(994, 130);
            this.Controls.Add(this.label29);
            this.Controls.Add(this.textBoxBatteryVoltage);
            this.Controls.Add(this.label28);
            this.Controls.Add(this.groupBoxAcceleration);
            this.Controls.Add(this.label18);
            this.Controls.Add(this.label17);
            this.Controls.Add(this.label16);
            this.Controls.Add(this.label15);
            this.Controls.Add(this.label14);
            this.Controls.Add(this.label13);
            this.Controls.Add(this.label12);
            this.Controls.Add(this.label11);
            this.Controls.Add(this.label10);
            this.Controls.Add(this.label9);
            this.Controls.Add(this.label8);
            this.Controls.Add(this.textBoxAltitude);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.textBoxLight);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.textBoxPressure);
            this.Controls.Add(this.textBoxTemperature);
            this.Controls.Add(this.textBoxSecond);
            this.Controls.Add(this.textBoxMinute);
            this.Controls.Add(this.textBoxHour);
            this.Controls.Add(this.textBoxDay);
            this.Controls.Add(this.textBoxMonth);
            this.Controls.Add(this.textBoxYear);
            this.Controls.Add(this.buttonSetDateTime);
            this.Controls.Add(this.statusStrip1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "FormMain";
            this.Text = "uPINPoint Time Setter";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.FormMain_FormClosed);
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this.groupBoxAcceleration.ResumeLayout(false);
            this.groupBoxAcceleration.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.ComponentModel.BackgroundWorker WriteThread;
        private System.Windows.Forms.Timer FormUpdateTimer;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabelConnected;
        private System.Windows.Forms.ToolStripProgressBar toolStripProgressBar;
        private System.ComponentModel.BackgroundWorker ReadThread;
        private System.Windows.Forms.Button buttonSetDateTime;
        private System.Windows.Forms.TextBox textBoxYear;
        private System.Windows.Forms.TextBox textBoxMonth;
        private System.Windows.Forms.TextBox textBoxDay;
        private System.Windows.Forms.TextBox textBoxHour;
        private System.Windows.Forms.TextBox textBoxMinute;
        private System.Windows.Forms.TextBox textBoxSecond;
        private System.Windows.Forms.TextBox textBoxAccelX;
        private System.Windows.Forms.TextBox textBoxAccelY;
        private System.Windows.Forms.TextBox textBoxAccelZ;
        private System.Windows.Forms.TextBox textBoxTemperature;
        private System.Windows.Forms.TextBox textBoxPressure;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox textBoxLight;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.TextBox textBoxAccelMag;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.TextBox textBoxAltitude;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.Label label12;
        private System.Windows.Forms.Label label13;
        private System.Windows.Forms.Label label14;
        private System.Windows.Forms.Label label15;
        private System.Windows.Forms.Label label16;
        private System.Windows.Forms.Label label17;
        private System.Windows.Forms.Label label18;
        private System.Windows.Forms.GroupBox groupBoxAcceleration;
        private System.Windows.Forms.Label label22;
        private System.Windows.Forms.Label label21;
        private System.Windows.Forms.Label label20;
        private System.Windows.Forms.Label label19;
        private System.Windows.Forms.Label label26;
        private System.Windows.Forms.Label label25;
        private System.Windows.Forms.Label label24;
        private System.Windows.Forms.Label label23;
        private System.Windows.Forms.Label label28;
        private System.Windows.Forms.TextBox textBoxBatteryVoltage;
        private System.Windows.Forms.Label label29;
    }
}

