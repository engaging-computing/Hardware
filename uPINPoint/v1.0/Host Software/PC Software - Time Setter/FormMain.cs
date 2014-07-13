
// NOTE: In order for this program to "find" a USB device with a given VID and PID, 
// both the VID and PID in the USB device descriptor (in the USB firmware on the 
// microcontroller), as well as in this PC application source code, must match.

// This C# program makes use of several functions in setupapi.dll and
// other Win32 DLLs.  However, one cannot call the functions directly in a 
// 32-bit DLL if the project is built in "Any CPU" mode, when run on a 64-bit OS.
// When configured to build an "Any CPU" executable, the executable will "become"
// a 64-bit executable when run on a 64-bit OS.  On a 32-bit OS, it will run as 
// a 32-bit executable, and the pointer sizes and other aspects of this 
// application will be compatible with calling 32-bit DLLs.

// Therefore, on a 64-bit OS, this application will not work unless it is built in
// "x86" mode.  When built in this mode, the exectuable always runs in 32-bit mode
// even on a 64-bit OS.  This allows this application to make 32-bit DLL function 
// calls, when run on either a 32-bit or 64-bit OS.

// By default, on a new project, C# normally wants to build in "Any CPU" mode.
// To switch to "x86" mode, open the "Configuration Manager" window.  In the 
// "Active solution platform:" drop down box, select "x86".  If this option does
// not appear, select: "<New...>" and then select the x86 option in the 
// "Type or select the new platform:" drop down box.

using System;
using System.Globalization;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Threading;

namespace PINPoint_v5_0
{
    public partial class FormMain : Form
    {
        #region Global Variables

        USBHIDManager MyHIDHandler;

        DateTime NewDeviceTime;
        bool NewDeviceTimePending = false;  // Updated by read thread, used by timer event to update the GUI (needs to be atomic)

        Int32 NewSensorDataBatteryVoltage;
        Int32 NewSensorDataTemperature;
        Int32 NewSensorDataPressure;
        Int32 NewSensorDataAltitude;
        Int32 NewSensorDataAccelX;
        Int32 NewSensorDataAccelY;
        Int32 NewSensorDataAccelZ;
        Int32 NewSensorDataAccelMag;
        Int32 NewSensorDataLight;
        bool NewSensorDataPending = false;  // Updated by read thread, used by timer event to update the GUI (needs to be atomic)

        String NewMessage = "";
        bool NewMessagePending = false;     // Updated by read thread, used by timer event to update the GUI (needs to be atomic)

        bool SetDateTimePending = false;	// Updated by button click event handler, used by ReadWriteThread (needs to be atomic)

        bool ReadSystemInfoPending = false;

        // Application command codes
        internal const Byte CMD_READ_ALL = 0x80;
        internal const Byte CMD_SET_DATE_TIME = 0x81;

        internal const Byte CMD_READ_SYSTEM_INFO = 0x70;

        #endregion

        // Need to check "Allow unsafe code" checkbox in build properties to use unsafe keyword.  Unsafe is needed to
        // properly interact with the unmanged C++ style APIs used to find and connect with the USB device.
        public unsafe FormMain()
        {
            InitializeComponent();

            MyHIDHandler = new USBHIDManager("Vid_04D8&Pid_0054");

            MyHIDHandler.USBDeviceAttached += USBDeviceAttached;
            MyHIDHandler.USBDeviceRemoved += USBDeviceRemoved;

            MyHIDHandler.RegisterForDeviceNotification(this);

            // Now make an initial attempt to find the USB device, in case it was already connected to the PC prior to launching the application
            MyHIDHandler.AttemptAttach();

            // Perform USB read/write operations in a separate thread.  Otherwise,
            // the Read/Write operations are effectively blocking functions and can lock up the
            // user interface if the I/O operations take a long time to complete.
            WriteThread.RunWorkerAsync();
            ReadThread.RunWorkerAsync();
        }

        // This is a callback function that gets called when a Windows message is received by the form.
        // We will receive various different types of messages, but the ones we really want to use are the WM_DEVICECHANGE messages.
        protected override void WndProc(ref Message m)
        {
            if (MyHIDHandler != null)
                MyHIDHandler.ProcessWindowsMessage(ref m);

            base.WndProc(ref m);
        }

        // Event handlers for USB devices being attached and removed
        public void USBDeviceAttached(Object sender)
        {
            toolStripStatusLabelConnected.Text = "Connected";
            toolStripStatusLabelConnected.BackColor = System.Drawing.Color.FromArgb(((int)((byte)0)), ((int)((byte)192)), ((int)((byte)0)));

            setAllButtonsEnabled(true);

            NewDeviceTimePending = false;
            NewSensorDataPending = false;

            NewMessage = "";
            NewMessagePending = false;

            SetDateTimePending = false;

            //textBoxMessages.Text = "";
        }

        public void USBDeviceRemoved(Object sender)
        {
            toolStripStatusLabelConnected.Text = "Disconnected";
            toolStripStatusLabelConnected.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(192)))), ((int)(((byte)(0)))), ((int)(((byte)(0)))));
        }

        #region Device Command Methods

        // Send the command to the device to set the clock
        private void DeviceCommand_SetDateTime()
        {
            DateTime dateTime;
            Byte[] outBuffer = new Byte[65]; // allocate a memory buffer equal to the OUT endpoint size + 1
            uint bytesWritten = 0;

            outBuffer[0] = 0x00; // The first byte is the "Report ID" and does not get sent over the USB.  Always set = 0.
            outBuffer[1] = CMD_SET_DATE_TIME;

            dateTime = DateTime.Now;

            // we need to convert to BCD for the device
            outBuffer[2] = (byte) ((((dateTime.Date.Year - 2000) / 10) << 4) | ((dateTime.Date.Year - 2000) % 10));
            outBuffer[3] = (byte) (((dateTime.Date.Month / 10) << 4) | (dateTime.Date.Month % 10));
            outBuffer[4] = (byte) (((dateTime.Date.Day / 10) << 4) | (dateTime.Date.Day % 10));
            outBuffer[5] = (byte) dateTime.Date.DayOfWeek;

            outBuffer[6] = (byte) (((dateTime.TimeOfDay.Hours / 10) << 4) | (dateTime.TimeOfDay.Hours % 10));
            outBuffer[7] = (byte) (((dateTime.TimeOfDay.Minutes / 10) << 4) | (dateTime.TimeOfDay.Minutes % 10));

            outBuffer[8] = (byte) (((dateTime.TimeOfDay.Seconds / 10) << 4) | (dateTime.TimeOfDay.Seconds % 10));

            for (UInt16 i = 9; i < 65; i++)	// This loop is not strictly necessary.  Simply initializes unused bytes to
                outBuffer[i] = 0xFF;		// 0xFF for lower EMI and power consumption when driving the USB cable.

            // Now send the packet to the USB firmware on the microcontroller
            MyHIDHandler.Write(outBuffer, 65, ref bytesWritten);
        }

        // Send a generic single-byte command
        private void DeviceCommand_GenericCommand(Byte cmd)
        {
            Byte[] outBuffer = new Byte[65]; // allocate a memory buffer equal to the OUT endpoint size + 1
            uint bytesWritten = 0;

            outBuffer[0] = 0x00; // The first byte is the "Report ID" and does not get sent over the USB.  Always set = 0.
            outBuffer[1] = cmd;

            for (UInt16 i = 9; i < 65; i++)	// This loop is not strictly necessary.  Simply initializes unused bytes to
                outBuffer[i] = 0xFF;		// 0xFF for lower EMI and power consumption when driving the USB cable.

            // Now send the packet to the USB firmware on the microcontroller
            MyHIDHandler.Write(outBuffer, 65, ref bytesWritten);
        }

        #endregion

        #region Read/Write Thread Methods

        /*
         * These threads do the actual USB read/write operations (but only when AttachedState == true) to the USB device.
         * It is generally preferrable to write applications so that read and write operations are handled in a separate
         * thread from the main form.  This makes it so that the main form can remain responsive, even if the I/O operations
         * take a very long time to complete.
         *
         * Since this is a separate thread, this code below executes independently from the rest of the
         * code in this application.  All this thread does is read and write to the USB device.  It does not update
         * the form directly with the new information it obtains (such as the ANxx/POT Voltage or pushbutton state).
         * The information that this thread obtains is stored in atomic global variables.
         * Form updates are handled by the FormUpdateTimer Tick event handler function.
         *
         * This application sends packets to the endpoint buffer on the USB device by using the "WriteFile()" function.
         * This application receives packets from the endpoint buffer on the USB device by using the "ReadFile()" function.
         * Both of these functions are documented in the MSDN library.  Calling ReadFile() is a not perfectly straight
         * foward in C# environment, since one of the input parameters is a pointer to a buffer that gets filled by ReadFile().
         * The ReadFile() function is therefore called through a wrapper function ReadFileManagedBuffer().
         *
         * All ReadFile() and WriteFile() operations in this example project are synchronous.  They are blocking function
         * calls and only return when they are complete, or if they fail because of some event, such as the user unplugging
         * the device.  It is possible to call these functions with "overlapped" structures, and use them as non-blocking
         * asynchronous I/O function calls.  
         *
         * Note:  This code may perform differently on some machines when the USB device is plugged into the host through a
         * USB 2.0 hub, as opposed to a direct connection to a root port on the PC.  In some cases the data rate may be slower
         * when the device is connected through a USB 2.0 hub.  This performance difference is believed to be caused by
         * the issue described in Microsoft knowledge base article 940021:
         * http://support.microsoft.com/kb/940021/en-us 
         *
         * Higher effective bandwidth (up to the maximum offered by interrupt endpoints), both when connected
         * directly and through a USB 2.0 hub, can generally be achieved by queuing up multiple pending read and/or
         * write requests simultaneously.  This can be done when using	asynchronous I/O operations (calling ReadFile() and
         * WriteFile()	with overlapped structures).  The Microchip	HID USB Bootloader application uses asynchronous I/O
         * for some USB operations and the source code can be used	as an example.
         */

        // Process sending out new packets when necessary
        private void WriteThread_DoWork(object sender, DoWorkEventArgs e)
        {
		    while (true)
		    {
                try
                {
                    if (MyHIDHandler.State != USBHIDManager.AttachedState.Attached) // Do not try to use the read/write handles unless the USB device is attached and ready
                    {
                        Thread.Sleep(5); // Add a small delay.  Otherwise, this while(true) loop can execute very fast and cause 
                                         // high CPU utilization, with no particular benefit to the application.
                        continue;
                    }

                    // GUI generated commands
                    if (SetDateTimePending == true)
                    {
                        DeviceCommand_SetDateTime();
                        SetDateTimePending = false;
                    }

                    if (ReadSystemInfoPending == true)
                    {
                        DeviceCommand_GenericCommand(CMD_READ_SYSTEM_INFO);
                        ReadSystemInfoPending = false;
                    }
                }
                catch
                {
                    // Exceptions can occur during the read or write operations.  For example,
                    // exceptions may occur if for instance the USB device is physically unplugged
                    // from the host while the above read/write functions are executing.

                    // Don't need to do anything special in this case.  The application will automatically
                    // re-establish communications based on the global AttachedState boolean variable used
                    // in conjunction with the WM_DEVICECHANGE messages to dyanmically respond to Plug and Play
                    // USB connection events.
                }
		    }
        }

        private void BufferPackInt32(Int32 data, Byte[] buf)
        {
            buf[0] = (byte)(data >> 24);
            buf[1] = (byte)(data >> 16);
            buf[2] = (byte)(data >> 8);
            buf[3] = (byte)data;
        }

        private Int32 BufferUnpackInt32(Byte[] buf)
        {
            UInt32 i;

            i = ((((UInt32)buf[0]) << 24) & 0xFF000000) + ((((UInt32)buf[1]) << 16) & 0x00FF0000)
                + ((((UInt32)buf[2]) << 8) & 0x0000FF00) + (((UInt32)buf[3]) & 0x000000FF);

            return (Int32)i;
        }

        // Process incoming packets from the device
        private void ReadThread_DoWork(object sender, DoWorkEventArgs e)
        {
            Byte[] inBuffer = new Byte[65];  // Allocate a memory buffer equal to the IN endpoint size + 1
            uint bytesRead  = 0;
            Int32 temp;
            UInt32 utemp;

            while (true)
            {
                try
                {
                    if (MyHIDHandler.State != USBHIDManager.AttachedState.Attached) // Do not try to use the read/write handles unless the USB device is attached and ready
                    {
                        Thread.Sleep(4); // Add a small delay.  Otherwise, this while(true) loop can execute very fast and cause 
                        // high CPU utilization, with no particular benefit to the application.
                        continue;
                    }

                    // read incoming packets and update the user interface
                    if (!MyHIDHandler.Read(inBuffer, 65, ref bytesRead))
                    {
                        //I2CNewMessage += "Read failed.\r\n";
                        //I2CNewMessagePending = true;
                        continue;
                    }

                    // INBuffer[0] is the report ID, which we don't care about
                    // INBuffer[1] is an echo back of the command (or a response code)
                    // INBuffer[2-64] vary by type

                    switch (inBuffer[1])
                    {
                        case CMD_READ_ALL:
                            try
                            {
                                // hack to convert BCD to binary
                                NewDeviceTime = new DateTime(Int32.Parse(inBuffer[2].ToString("X2")) + 2000, Int32.Parse(inBuffer[3].ToString("X2")),
                                    Int32.Parse(inBuffer[4].ToString("X2")), Int32.Parse(inBuffer[6].ToString("X2")), Int32.Parse(inBuffer[7].ToString("X2")),
                                    Int32.Parse(inBuffer[8].ToString("X2")));
                            }
                            catch
                            {

                            }

                            NewDeviceTimePending = true;

                            NewSensorDataBatteryVoltage = ((Int32)inBuffer[9] << 24) + ((Int32)inBuffer[10] << 16) + ((Int32)inBuffer[11] << 8) + (Int32)inBuffer[12];

                            NewSensorDataTemperature = ((Int32)inBuffer[13] << 24) + ((Int32)inBuffer[14] << 16) + ((Int32)inBuffer[15] << 8) + (Int32)inBuffer[16];
                            NewSensorDataPressure = ((Int32)inBuffer[17] << 24) + ((Int32)inBuffer[18] << 16) + ((Int32)inBuffer[19] << 8) + (Int32)inBuffer[20];
                            NewSensorDataAltitude = ((Int32)inBuffer[21] << 24) + ((Int32)inBuffer[22] << 16) + ((Int32)inBuffer[23] << 8) + (Int32)inBuffer[24];

                            NewSensorDataAccelX = ((Int32)inBuffer[25] << 24) + ((Int32)inBuffer[26] << 16) + ((Int32)inBuffer[27] << 8) + (Int32)inBuffer[28];
                            NewSensorDataAccelY = ((Int32)inBuffer[29] << 24) + ((Int32)inBuffer[30] << 16) + ((Int32)inBuffer[31] << 8) + (Int32)inBuffer[32];
                            NewSensorDataAccelZ = ((Int32)inBuffer[33] << 24) + ((Int32)inBuffer[34] << 16) + ((Int32)inBuffer[35] << 8) + (Int32)inBuffer[36];
                            NewSensorDataAccelMag = ((Int32)inBuffer[37] << 24) + ((Int32)inBuffer[38] << 16) + ((Int32)inBuffer[39] << 8) + (Int32)inBuffer[40];

                            NewSensorDataLight = ((Int32)inBuffer[41] << 24) + ((Int32)inBuffer[42] << 16) + ((Int32)inBuffer[43] << 8) + (Int32)inBuffer[44];

                            NewSensorDataPending = true;
                            break;

                        case CMD_READ_SYSTEM_INFO:
                            NewMessage = "Read System Info: ";
                            NewMessage += "Firmware version 0x" + inBuffer[2].ToString("X2") + inBuffer[3].ToString("X2") + "\r\n";
                            utemp = (((UInt32)inBuffer[4] << 24) + ((UInt32)inBuffer[5] << 16) + ((UInt32)inBuffer[6] << 8) + (UInt32)inBuffer[7]);
                            NewMessage += "Serial # " + utemp + "\r\n";
                            temp = (((Int32)inBuffer[8] << 24) + ((Int32)inBuffer[9] << 16) + ((Int32)inBuffer[10] << 8) + (Int32)inBuffer[11]);
                            NewMessage += "Temperature Offset: " + temp + "\r\n";
                            temp = (((Int32)inBuffer[12] << 24) + ((Int32)inBuffer[13] << 16) + ((Int32)inBuffer[14] << 8) + (Int32)inBuffer[15]);
                            NewMessage += "Pressure Offset: " + temp + "\r\n";
                            temp = (((Int32)inBuffer[16] << 24) + ((Int32)inBuffer[17] << 16) + ((Int32)inBuffer[18] << 8) + (Int32)inBuffer[19]);
                            NewMessage += "Accel X Offset: " + temp + "\r\n";
                            temp = (((Int32)inBuffer[20] << 24) + ((Int32)inBuffer[21] << 16) + ((Int32)inBuffer[22] << 8) + (Int32)inBuffer[23]);
                            NewMessage += "Accel Y Offset: " + temp + "\r\n";
                            temp = (((Int32)inBuffer[24] << 24) + ((Int32)inBuffer[25] << 16) + ((Int32)inBuffer[26] << 8) + (Int32)inBuffer[27]);
                            NewMessage += "Accel Z Offset: " + temp + "\r\n";
                            temp = (((Int32)inBuffer[28] << 24) + ((Int32)inBuffer[29] << 16) + ((Int32)inBuffer[30] << 8) + (Int32)inBuffer[31]);
                            NewMessage += "Light Offset: " + temp + "\r\n";
                            NewMessagePending = true;
                            break;

                        default:
                            break;
                    }
                }
                catch
                {
                    // Exceptions can occur during the read or write operations.  For example,
                    // exceptions may occur if for instance the USB device is physically unplugged
                    // from the host while the above read/write functions are executing.

                    // Don't need to do anything special in this case.  The application will automatically
                    // re-establish communications based on the global AttachedState boolean variable used
                    // in conjunction with the WM_DEVICECHANGE messages to dyanmically respond to Plug and Play
                    // USB connection events.
                }

            }
        }

        #endregion

        #region GUI Event Callbacks

        private void setAllButtonsEnabled(bool en)
        {

        }

        private void buttonSetDateTime_Click(object sender, EventArgs e)
        {
            SetDateTimePending = true;	// will get used asynchronously by the ReadWriteThread
        }

        #region GUI Control Change Callbacks

        #endregion

        private void FormMain_FormClosed(object sender, FormClosedEventArgs e)
        {

        }

        private void FormUpdateTimer_Tick(object sender, EventArgs e)
        {
            // This timer tick event handler function is used to update the user interface on the form, based on data
            // obtained asynchronously by the ReadWriteThread and the WM_DEVICECHANGE event handler functions.

            // Update the various status indicators on the form with the latest info obtained from the ReadWriteThread()
            if (MyHIDHandler.State != USBHIDManager.AttachedState.Attached)
                return;

            if (NewDeviceTimePending == true)
            {
                NewDeviceTimePending = false;

                textBoxYear.Text = NewDeviceTime.Year.ToString();
                textBoxMonth.Text = NewDeviceTime.Month.ToString();
                textBoxDay.Text = NewDeviceTime.Day.ToString();

                textBoxHour.Text = NewDeviceTime.Hour.ToString();
                textBoxMinute.Text = NewDeviceTime.Minute.ToString();
                textBoxSecond.Text = NewDeviceTime.Second.ToString();
            }

            if (NewSensorDataPending == true)
            {
                NewSensorDataPending = false;

                textBoxBatteryVoltage.Text = ((double)NewSensorDataBatteryVoltage / 100.0).ToString("F2");

                textBoxTemperature.Text = ((double)NewSensorDataTemperature / 10.0).ToString("F1");
                textBoxPressure.Text = ((double)NewSensorDataPressure / 1000.0).ToString("F3");
                textBoxAltitude.Text = NewSensorDataAltitude.ToString();

                textBoxAccelX.Text = ((double)NewSensorDataAccelX / 100.0).ToString("F2");
                textBoxAccelY.Text = ((double)NewSensorDataAccelY / 100.0).ToString("F2");
                textBoxAccelZ.Text = ((double)NewSensorDataAccelZ / 100.0).ToString("F2");
                textBoxAccelMag.Text = ((double)NewSensorDataAccelMag / 100.0).ToString("F2");

                textBoxLight.Text = ((double)NewSensorDataLight / 10.0).ToString("F1");
            }

            if (NewMessagePending == true)
            {
                NewMessagePending = false;

//                textBoxMessages.AppendText(NewMessage);
                NewMessage = "";
            }
        }

        #endregion

        private void buttonReadSystemInfo_Click(object sender, EventArgs e)
        {
            ReadSystemInfoPending = true; // will get used asynchronously by the ReadWriteThread
        }
    }
}

