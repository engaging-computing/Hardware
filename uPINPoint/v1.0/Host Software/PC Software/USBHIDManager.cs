
using System;
using System.Globalization;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Threading;
using System.Text.RegularExpressions;
using System.IO;
using Microsoft.Win32.SafeHandles;

namespace PINPoint_v5_0
{
    class USBHIDManager
    {
        /// Win32 specific stuff    
        private class Win32Wrapper
        {
            // Constant definitions for certain WM_DEVICECHANGE messages
            public const uint WM_DEVICECHANGE = 0x0219;
            public const uint DBT_DEVICEARRIVAL = 0x8000;
            public const uint DBT_DEVICEREMOVEPENDING = 0x8003;
            public const uint DBT_DEVICEREMOVECOMPLETE = 0x8004;
            public const uint DBT_CONFIGCHANGED = 0x0018;

            // Constant definitions from setupapi.h, which we aren't allowed to include directly since this is C#
            public const uint DIGCF_PRESENT = 0x02;
            public const uint DIGCF_DEVICEINTERFACE = 0x10;

            // Constants for CreateFile() and other file I/O functions
            public const short FILE_ATTRIBUTE_NORMAL = 0x80;
            public const short INVALID_HANDLE_VALUE = -1;
            public const uint GENERIC_READ = 0x80000000;
            public const uint GENERIC_WRITE = 0x40000000;
            public const uint CREATE_NEW = 1;
            public const uint CREATE_ALWAYS = 2;
            public const uint OPEN_EXISTING = 3;
            public const uint FILE_SHARE_READ = 0x00000001;
            public const uint FILE_SHARE_WRITE = 0x00000002;

            // Other constant definitions
            public const uint DBT_DEVTYP_DEVICEINTERFACE = 0x05;
            public const uint DEVICE_NOTIFY_WINDOW_HANDLE = 0x00;
            public const uint ERROR_SUCCESS = 0x00;
            public const uint ERROR_NO_MORE_ITEMS = 0x00000103;
            public const uint SPDRP_HARDWAREID = 0x00000001;

            // Various structure definitions for structures that this code will be using
            public struct SP_DEVICE_INTERFACE_DATA
            {
                public uint cbSize;               // DWORD
                public Guid InterfaceClassGuid;   // GUID
                public uint Flags;                // DWORD
                public uint Reserved;             // ULONG_PTR MSDN says ULONG_PTR is "typedef unsigned __int3264 ULONG_PTR;"  
            }

            public struct SP_DEVICE_INTERFACE_DETAIL_DATA
            {
                public uint cbSize;               // DWORD
                public char[] DevicePath;         // TCHAR array of any size
            }

            public struct SP_DEVINFO_DATA
            {
                public uint cbSize;       // DWORD
                public Guid ClassGuid;    // GUID
                public uint DevInst;      // DWORD
                public uint Reserved;     // ULONG_PTR  MSDN says ULONG_PTR is "typedef unsigned __int3264 ULONG_PTR;"  
            }

            public struct DEV_BROADCAST_DEVICEINTERFACE
            {
                public uint dbcc_size;            // DWORD
                public uint dbcc_devicetype;      // DWORD
                public uint dbcc_reserved;        // DWORD
                public Guid dbcc_classguid;       // GUID
                public char[] dbcc_name;          // TCHAR array
            }

            // DLL Imports.  Need these to access various C style unmanaged functions contained in their respective DLL files.
            // --------------------------------------------------------------------------------------------------------------
            // Returns a HDEVINFO type for a device information set.  We will need the 
            // HDEVINFO as in input parameter for calling many of the other SetupDixxx() functions.
            [DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            public static extern IntPtr SetupDiGetClassDevs(
                ref Guid ClassGuid,     // LPGUID    Input: Need to supply the class GUID. 
                IntPtr Enumerator,      // PCTSTR    Input: Use NULL here, not important for our purposes
                IntPtr hwndParent,      // HWND      Input: Use NULL here, not important for our purposes
                uint Flags);            // DWORD     Input: Flags describing what kind of filtering to use.

            // Gives us "PSP_DEVICE_INTERFACE_DATA" which contains the Interface specific GUID (different
            // from class GUID).  We need the interface GUID to get the device path.
            [DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            public static extern bool SetupDiEnumDeviceInterfaces(
                IntPtr DeviceInfoSet,           // Input: Give it the HDEVINFO we got from SetupDiGetClassDevs()
                IntPtr DeviceInfoData,          // Input (optional)
                ref Guid InterfaceClassGuid,    // Input 
                uint MemberIndex,               // Input: "Index" of the device you are interested in getting the path for.
                ref SP_DEVICE_INTERFACE_DATA DeviceInterfaceData);    // Output: This function fills in an "SP_DEVICE_INTERFACE_DATA" structure.

            // SetupDiDestroyDeviceInfoList() frees up memory by destroying a DeviceInfoList
            [DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            public static extern bool SetupDiDestroyDeviceInfoList(
                IntPtr DeviceInfoSet);          // Input: Give it a handle to a device info list to deallocate from RAM.

            // SetupDiEnumDeviceInfo() fills in an "SP_DEVINFO_DATA" structure, which we need for SetupDiGetDeviceRegistryProperty()
            [DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            public static extern bool SetupDiEnumDeviceInfo(
                IntPtr DeviceInfoSet,
                uint MemberIndex,
                ref SP_DEVINFO_DATA DeviceInterfaceData);

            // SetupDiGetDeviceRegistryProperty() gives us the hardware ID, which we use to check to see if it has matching VID/PID
            [DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            public static extern bool SetupDiGetDeviceRegistryProperty(
                IntPtr DeviceInfoSet,
                ref SP_DEVINFO_DATA DeviceInfoData,
                uint Property,
                ref uint PropertyRegDataType,
                IntPtr PropertyBuffer,
                uint PropertyBufferSize,
                ref uint RequiredSize);

            // SetupDiGetDeviceInterfaceDetail() gives us a device path, which is needed before CreateFile() can be used.
            [DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            public static extern bool SetupDiGetDeviceInterfaceDetail(
                IntPtr DeviceInfoSet,                   // Input: Wants HDEVINFO which can be obtained from SetupDiGetClassDevs()
                ref SP_DEVICE_INTERFACE_DATA DeviceInterfaceData,                    // Input: Pointer to an structure which defines the device interface.  
                IntPtr DeviceInterfaceDetailData,      // Output: Pointer to a SP_DEVICE_INTERFACE_DETAIL_DATA structure, which will receive the device path.
                uint DeviceInterfaceDetailDataSize,     // Input: Number of bytes to retrieve.
                ref uint RequiredSize,                  // Output (optional): The number of bytes needed to hold the entire struct 
                IntPtr DeviceInfoData);                 // Output (optional): Pointer to a SP_DEVINFO_DATA structure

            // Overload for SetupDiGetDeviceInterfaceDetail().  Need this one since we can't pass NULL pointers directly in C#.
            [DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            public static extern bool SetupDiGetDeviceInterfaceDetail(
                IntPtr DeviceInfoSet,                   // Input: Wants HDEVINFO which can be obtained from SetupDiGetClassDevs()
                ref SP_DEVICE_INTERFACE_DATA DeviceInterfaceData,               // Input: Pointer to an structure which defines the device interface.  
                IntPtr DeviceInterfaceDetailData,       // Output: Pointer to a SP_DEVICE_INTERFACE_DETAIL_DATA structure, which will contain the device path.
                uint DeviceInterfaceDetailDataSize,     // Input: Number of bytes to retrieve.
                IntPtr RequiredSize,                    // Output (optional): Pointer to a DWORD to tell you the number of bytes needed to hold the entire struct 
                IntPtr DeviceInfoData);                 // Output (optional): Pointer to a SP_DEVINFO_DATA structure

            // Need this function for receiving all of the WM_DEVICECHANGE messages.  See MSDN documentation for
            // description of what this function does/how to use it. Note: name is remapped "RegisterDeviceNotificationUM" to
            // avoid possible build error conflicts.
            [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            public static extern IntPtr RegisterDeviceNotification(IntPtr hRecipient, IntPtr NotificationFilter, uint Flags);

            [DllImport("user32.dll", SetLastError = true)]
            public static extern bool UnregisterDeviceNotification(IntPtr hHandle);

            // Takes in a device path and opens a handle to the device.
            [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            public static extern SafeFileHandle CreateFile(string lpFileName, uint dwDesiredAccess, uint dwShareMode,
                IntPtr lpSecurityAttributes, uint dwCreationDisposition, uint dwFlagsAndAttributes, IntPtr hTemplateFile);

            // Uses a handle (created with CreateFile()), and lets us write USB data to the device.
            [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            public static extern bool WriteFile(SafeFileHandle hFile, byte[] lpBuffer, uint nNumberOfBytesToWrite,
                ref uint lpNumberOfBytesWritten, IntPtr lpOverlapped);

            // Uses a handle (created with CreateFile()), and lets us read USB data from the device.
            [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            public static extern bool ReadFile(SafeFileHandle hFile, IntPtr lpBuffer, uint nNumberOfBytesToRead,
                ref uint lpNumberOfBytesRead, IntPtr lpOverlapped);

            // Globally Unique Identifier (GUID) for HID class devices.  Windows uses GUIDs to identify things.
            public static Guid InterfaceClassGuid = new Guid(0x4d1e55b2, 0xf16f, 0x11cf, 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30);
        }

        //Delegate for event handler to handle the device events 
        public delegate void USBDeviceEventHandler(Object sender);

        /// Add handlers for these events in your form to be notified of removable device events.
        public event USBDeviceEventHandler USBDeviceAttached;
        public event USBDeviceEventHandler USBDeviceRemoved;

        // Need to keep track of the USB device attachment status for proper plug and play operation
        public enum AttachedState
        {
            NotAttached,
            Attached,
            AttachedButBroken
        }

        private AttachedState _state = AttachedState.NotAttached;
        public AttachedState State
        {
            get { return _state; }
        }

        SafeFileHandle WriteHandleToUSBDevice = null;
        SafeFileHandle ReadHandleToUSBDevice = null;

        // Use the formatting: "Vid_xxxx&Pid_xxxx" where xxxx is a 16-bit hexadecimal number.
        // Make sure the value appearing in the parathesis matches the USB device descriptor
        // of the device that this aplication is intending to find.
        String DeviceIDToFind;

        IntPtr DeviceEventHandle = IntPtr.Zero;
        String DevicePath = null;

        public unsafe USBHIDManager(String deviceIDToFind)
        {
            DeviceIDToFind = deviceIDToFind;
        }

        ~USBHIDManager()
        {
            UnregisterForDeviceNotification();
        }

        public void RegisterForDeviceNotification(Form form)
        {
            UnregisterForDeviceNotification();

            // Register for WM_DEVICECHANGE notifications.  This code uses these messages to detect plug and play connection/disconnection events for USB devices
            Win32Wrapper.DEV_BROADCAST_DEVICEINTERFACE DeviceBroadcastHeader = new Win32Wrapper.DEV_BROADCAST_DEVICEINTERFACE();
            DeviceBroadcastHeader.dbcc_devicetype = Win32Wrapper.DBT_DEVTYP_DEVICEINTERFACE;
            DeviceBroadcastHeader.dbcc_size = (uint)Marshal.SizeOf(DeviceBroadcastHeader);
            DeviceBroadcastHeader.dbcc_reserved = 0;	//Reserved says not to use...
            DeviceBroadcastHeader.dbcc_classguid = Win32Wrapper.InterfaceClassGuid;

            // Need to get the address of the DeviceBroadcastHeader to call RegisterDeviceNotification(), but
            // can't use "&DeviceBroadcastHeader".  Instead, using a roundabout means to get the address by 
            // making a duplicate copy using Marshal.StructureToPtr().
            IntPtr pDeviceBroadcastHeader = IntPtr.Zero;  //Make a pointer.
            pDeviceBroadcastHeader = Marshal.AllocHGlobal(Marshal.SizeOf(DeviceBroadcastHeader)); //allocate memory for a new DEV_BROADCAST_DEVICEINTERFACE structure, and return the address 
            Marshal.StructureToPtr(DeviceBroadcastHeader, pDeviceBroadcastHeader, false);  //Copies the DeviceBroadcastHeader structure into the memory already allocated at DeviceBroadcastHeaderWithPointer
            DeviceEventHandle = Win32Wrapper.RegisterDeviceNotification(form.Handle, pDeviceBroadcastHeader, Win32Wrapper.DEVICE_NOTIFY_WINDOW_HANDLE);
        }

        public void UnregisterForDeviceNotification()
        {
            if (DeviceEventHandle != IntPtr.Zero)
                Win32Wrapper.UnregisterDeviceNotification(DeviceEventHandle);

            DeviceEventHandle = IntPtr.Zero;
        }

        public void ProcessWindowsMessage(ref Message m)
        {
            if (m.Msg != Win32Wrapper.WM_DEVICECHANGE)
                return;

            if ((int)m.WParam == Win32Wrapper.DBT_DEVICEARRIVAL
                || (int)m.WParam == Win32Wrapper.DBT_DEVICEREMOVEPENDING
                || (int)m.WParam == Win32Wrapper.DBT_DEVICEREMOVECOMPLETE
                || (int)m.WParam == Win32Wrapper.DBT_CONFIGCHANGED)
            {
                // WM_DEVICECHANGE messages by themselves are quite generic, and can be caused by a number of different
                // sources, not just a USB hardware device.  Therefore, we must check to find out if any changes relavant
                // to your device (with known VID/PID) took place before doing any kind of opening or closing of handles/endpoints.
                // (the message could have been totally unrelated to the application/USB device)

                AttemptAttach();
            }
        }

        // FUNCTION:	GetUSBDevicePath()
        // PURPOSE:	    Check if a USB device is currently plugged in with a matching VendorID and ProductID
        // INPUT:	    Uses globally declared String DevicePath, globally declared GUID, and the MY_DEVICE_ID constant.
        // OUTPUT:	    Returns BOOL.  TRUE when device with matching VID/PID found.  FALSE if device with VID/PID could not be found.
        //			    When returns TRUE, the globally accessable "DetailedInterfaceDataStructure" will contain the device path
        //			    to the USB device with the matching VID/PID.
        bool GetUSBDevicePath()
        {
		    /* 
		    Before we can "connect" our application to our USB embedded device, we must first find the device.
		    USB can have many devices simultaneously connected, so somehow we have to find our device only.
		    This is done with the Vendor ID (VID) and Product ID (PID).  Each USB product line should have
		    a unique combination of VID and PID.  

		    Microsoft has created a number of functions which are useful for finding plug and play devices.  Documentation
		    for each function used can be found in the MSDN library.  We will be using the following functions (unmanaged C functions):

		    SetupDiGetClassDevs()					//provided by setupapi.dll, which comes with Windows
		    SetupDiEnumDeviceInterfaces()			//provided by setupapi.dll, which comes with Windows
		    GetLastError()							//provided by kernel32.dll, which comes with Windows
		    SetupDiDestroyDeviceInfoList()			//provided by setupapi.dll, which comes with Windows
		    SetupDiGetDeviceInterfaceDetail()		//provided by setupapi.dll, which comes with Windows
		    SetupDiGetDeviceRegistryProperty()		//provided by setupapi.dll, which comes with Windows
		    CreateFile()							//provided by kernel32.dll, which comes with Windows

            In order to call these unmanaged functions, the Marshal class is very useful.
             
		    We will also be using the following unusual data types and structures.  Documentation can also be found in
		    the MSDN library:

		    PSP_DEVICE_INTERFACE_DATA
		    PSP_DEVICE_INTERFACE_DETAIL_DATA
		    SP_DEVINFO_DATA
		    HDEVINFO
		    HANDLE
		    GUID

		    The ultimate objective of the following code is to get the device path, which will be used elsewhere for getting
		    read and write handles to the USB device.  Once the read/write handles are opened, only then can this
		    PC application begin reading/writing to the USB device using the WriteFile() and ReadFile() functions.

		    Getting the device path is a multi-step round about process, which requires calling several of the
		    SetupDixxx() functions provided by setupapi.dll.
		    */

            try
            {
		        IntPtr deviceInfoTable = IntPtr.Zero;
                Win32Wrapper.SP_DEVICE_INTERFACE_DATA interfaceDataStructure = new Win32Wrapper.SP_DEVICE_INTERFACE_DATA();
                Win32Wrapper.SP_DEVICE_INTERFACE_DETAIL_DATA detailedInterfaceDataStructure = new Win32Wrapper.SP_DEVICE_INTERFACE_DETAIL_DATA();
                Win32Wrapper.SP_DEVINFO_DATA devInfoData = new Win32Wrapper.SP_DEVINFO_DATA();

		        uint interfaceIndex = 0;
		        uint dwRegType = 0;
		        uint dwRegSize = 0;
                uint dwRegSize2 = 0;
		        uint structureSize = 0;
		        IntPtr propertyValueBuffer = IntPtr.Zero;
		        uint errorStatus;
		        uint loopCounter = 0;

		        // First populate a list of plugged in devices (by specifying "DIGCF_PRESENT"), which are of the specified class GUID. 
                deviceInfoTable = Win32Wrapper.SetupDiGetClassDevs(ref Win32Wrapper.InterfaceClassGuid, IntPtr.Zero, IntPtr.Zero, Win32Wrapper.DIGCF_PRESENT | Win32Wrapper.DIGCF_DEVICEINTERFACE);

                if (deviceInfoTable != IntPtr.Zero)
                {
		            // Now look through the list we just populated.  We are trying to see if any of them match our device. 
		            while (true)
		            {
                        interfaceDataStructure.cbSize = (uint)Marshal.SizeOf(interfaceDataStructure);

                        if (Win32Wrapper.SetupDiEnumDeviceInterfaces(deviceInfoTable, IntPtr.Zero, ref Win32Wrapper.InterfaceClassGuid, interfaceIndex, ref interfaceDataStructure))
			            {
                            errorStatus = (uint)Marshal.GetLastWin32Error();

                            if (errorStatus == Win32Wrapper.ERROR_NO_MORE_ITEMS)	// Did we reach the end of the list of matching devices in the DeviceInfoTable?
				            {	// Cound not find the device.  Must not have been attached.
                                Win32Wrapper.SetupDiDestroyDeviceInfoList(deviceInfoTable);	// Clean up the old structure we no longer need.
					            return false;		
				            }
			            }
			            else // Else some other kind of unknown error ocurred...
			            {
                            errorStatus = (uint)Marshal.GetLastWin32Error();
                            Win32Wrapper.SetupDiDestroyDeviceInfoList(deviceInfoTable);	// Clean up the old structure we no longer need.
				            return false;	
			            }

			            // Now retrieve the hardware ID from the registry.  The hardware ID contains the VID and PID, which we will then 
			            // check to see if it is the correct device or not.

			            // Initialize an appropriate SP_DEVINFO_DATA structure.  We need this structure for SetupDiGetDeviceRegistryProperty().
                        devInfoData.cbSize = (uint)Marshal.SizeOf(devInfoData);
                        Win32Wrapper.SetupDiEnumDeviceInfo(deviceInfoTable, interfaceIndex, ref devInfoData);

			            // First query for the size of the hardware ID, so we can know how big a buffer to allocate for the data.
                        Win32Wrapper.SetupDiGetDeviceRegistryProperty(deviceInfoTable, ref devInfoData, Win32Wrapper.SPDRP_HARDWAREID, ref dwRegType, IntPtr.Zero, 0, ref dwRegSize);

			            // Allocate a buffer for the hardware ID.
                        // Should normally work, but could throw exception "OutOfMemoryException" if not enough resources available.
                        propertyValueBuffer = Marshal.AllocHGlobal((int)dwRegSize);

			            // Retrieve the hardware IDs for the current device we are looking at.  PropertyValueBuffer gets filled with a 
			            // REG_MULTI_SZ (array of null terminated strings).  To find a device, we only care about the very first string in the
			            // buffer, which will be the "device ID".  The device ID is a string which contains the VID and PID, in the example 
			            // format "Vid_04d8&Pid_003f".
                        Win32Wrapper.SetupDiGetDeviceRegistryProperty(deviceInfoTable, ref devInfoData, Win32Wrapper.SPDRP_HARDWAREID, ref dwRegType, propertyValueBuffer, dwRegSize, ref dwRegSize2);

			            // Now check if the first string in the hardware ID matches the device ID of the USB device we are trying to find.
			            String DeviceIDFromRegistry = Marshal.PtrToStringUni(propertyValueBuffer); // Make a new string, fill it with the contents from the PropertyValueBuffer

			            Marshal.FreeHGlobal(propertyValueBuffer);		// No longer need the PropertyValueBuffer, free the memory to prevent potential memory leaks

			            // Convert both strings to lower case.  This makes the code more robust/portable accross OS Versions
			            DeviceIDFromRegistry = DeviceIDFromRegistry.ToLowerInvariant();	
			            DeviceIDToFind = DeviceIDToFind.ToLowerInvariant();				

                        // Now check if the hardware ID we are looking at contains the correct VID/PID
			            if (DeviceIDFromRegistry.Contains(DeviceIDToFind))
			            {
                            // Device must have been found.  In order to open I/O file handle(s), we will need the actual device path first.
				            // We can get the path by calling SetupDiGetDeviceInterfaceDetail(), however, we have to call this function twice:  The first
				            // time to get the size of the required structure/buffer to hold the detailed interface data, then a second time to actually 
				            // get the structure (after we have allocated enough memory for the structure.)
                            detailedInterfaceDataStructure.cbSize = (uint)Marshal.SizeOf(detailedInterfaceDataStructure);

				            // First call populates "StructureSize" with the correct value
                            Win32Wrapper.SetupDiGetDeviceInterfaceDetail(deviceInfoTable, ref interfaceDataStructure, IntPtr.Zero, 0, ref structureSize, IntPtr.Zero);

                            // Need to call SetupDiGetDeviceInterfaceDetail() again, this time specifying a pointer to a SP_DEVICE_INTERFACE_DETAIL_DATA buffer with the correct size of RAM allocated.
                            // First need to allocate the unmanaged buffer and get a pointer to it.
                            IntPtr pUnmanagedDetailedInterfaceDataStructure = IntPtr.Zero;  // Declare a pointer.
                            pUnmanagedDetailedInterfaceDataStructure = Marshal.AllocHGlobal((int)structureSize); // Reserve some unmanaged memory for the structure.
                            detailedInterfaceDataStructure.cbSize = 6; // Initialize the cbSize parameter (4 bytes for DWORD + 2 bytes for unicode null terminator)
                            Marshal.StructureToPtr(detailedInterfaceDataStructure, pUnmanagedDetailedInterfaceDataStructure, false); //Copy managed structure contents into the unmanaged memory buffer.

                            // Now call SetupDiGetDeviceInterfaceDetail() a second time to receive the device path in the structure at pUnmanagedDetailedInterfaceDataStructure.
                            if (Win32Wrapper.SetupDiGetDeviceInterfaceDetail(deviceInfoTable, ref interfaceDataStructure, pUnmanagedDetailedInterfaceDataStructure, structureSize, IntPtr.Zero, IntPtr.Zero))
                            {
                                // Need to extract the path information from the unmanaged "structure".  The path starts at (pUnmanagedDetailedInterfaceDataStructure + sizeof(DWORD)).
                                IntPtr pToDevicePath = new IntPtr((uint)pUnmanagedDetailedInterfaceDataStructure.ToInt32() + 4);  //Add 4 to the pointer (to get the pointer to point to the path, instead of the DWORD cbSize parameter)
                                DevicePath = Marshal.PtrToStringUni(pToDevicePath); // Now copy the path information into the globally defined DevicePath String.
                                
                                // We now have the proper device path, and we can finally use the path to open I/O handle(s) to the device.
                                Win32Wrapper.SetupDiDestroyDeviceInfoList(deviceInfoTable);	// Clean up the old structure we no longer need.
                                Marshal.FreeHGlobal(pUnmanagedDetailedInterfaceDataStructure);  // No longer need this unmanaged SP_DEVICE_INTERFACE_DETAIL_DATA buffer.  We already extracted the path information.
                                return true; // Returning the device path in the global DevicePath String
                            }
                            else // Some unknown failure occurred
                            {
                                uint errorCode = (uint)Marshal.GetLastWin32Error();
                                Win32Wrapper.SetupDiDestroyDeviceInfoList(deviceInfoTable);	// Clean up the old structure.
                                Marshal.FreeHGlobal(pUnmanagedDetailedInterfaceDataStructure);  // No longer need this unmanaged SP_DEVICE_INTERFACE_DETAIL_DATA buffer.  We already extracted the path information.
                                return false;    
                            }
                        }

			            interfaceIndex++;

			            // Keep looping until we either find a device with matching VID and PID, or until we run out of devices to check.
			            // However, just in case some unexpected error occurs, keep track of the number of loops executed.
			            // If the number of loops exceeds a very large number, exit anyway, to prevent inadvertent infinite looping.
			            if (++loopCounter == 10000000) // Surely there aren't more than 10 million devices attached to any forseeable PC...
				            return false;
		            }
                }

                return false;
            }
            catch
            {
                // something went wrong if we get here -- maybe a Marshal.AllocHGlobal() failed due to insufficient resources or something.
			    return false;	
            }
        }

        // Attempts to find the device of interest and attach to it (open file handles).
        // Returns ture if something has changed (attached/detached), false if there was no change.
        public void AttemptAttach()
        {
            // First check and make sure at least one device with matching VID/PID is attached
            if (!GetUSBDevicePath())
            {
                // Device must not be connected (or not programmed with correct firmware)...

                if (_state == AttachedState.NotAttached)
                    return;

                // If AttachedState is currently set to true, it means the device was just now disconnected
                WriteHandleToUSBDevice.Close();
                ReadHandleToUSBDevice.Close();
                _state = AttachedState.NotAttached;

                if (USBDeviceRemoved != null)
                    USBDeviceRemoved(this);

                return;
            }

            // If we get here, it means that the device is currently connected and was found.
            // We need to decide what to do, based on whether or not the device was previously known to be
            // connected or not.

            // If we are already attached to the device just do nothing
            if (_state == AttachedState.Attached)
                return;

            uint ErrorStatusWrite;
            uint ErrorStatusRead;

            // We obtained the proper device path (from CheckIfPresentAndGetUSBDevicePath() function call), and it
            // is now possible to open read and write handles to the device.
            WriteHandleToUSBDevice = Win32Wrapper.CreateFile(DevicePath, Win32Wrapper.GENERIC_WRITE, Win32Wrapper.FILE_SHARE_READ | Win32Wrapper.FILE_SHARE_WRITE, IntPtr.Zero, Win32Wrapper.OPEN_EXISTING, 0, IntPtr.Zero);
            ErrorStatusWrite = (uint)Marshal.GetLastWin32Error();
            ReadHandleToUSBDevice = Win32Wrapper.CreateFile(DevicePath, Win32Wrapper.GENERIC_READ, Win32Wrapper.FILE_SHARE_READ | Win32Wrapper.FILE_SHARE_WRITE, IntPtr.Zero, Win32Wrapper.OPEN_EXISTING, 0, IntPtr.Zero);
            ErrorStatusRead = (uint)Marshal.GetLastWin32Error();

            if (ErrorStatusWrite == Win32Wrapper.ERROR_SUCCESS && ErrorStatusRead == Win32Wrapper.ERROR_SUCCESS)
                _state = AttachedState.Attached;
            else // for some reason the device is physically plugged in, but one or both of the read/write handles didn't open successfully...
            {
                // Flag so that next time a WM_DEVICECHANGE message occurs, can retry to re-open read/write pipes
                _state = AttachedState.AttachedButBroken;

                if (ErrorStatusWrite == Win32Wrapper.ERROR_SUCCESS)
                    WriteHandleToUSBDevice.Close();

                if (ErrorStatusRead == Win32Wrapper.ERROR_SUCCESS)
                    ReadHandleToUSBDevice.Close();
            }

            if (USBDeviceAttached != null)
                USBDeviceAttached(this);

            return;
        }

        public unsafe bool Read(byte[] InBuffer, uint nNumberOfBytesToRead, ref uint lpNumberOfBytesRead)
        {
            IntPtr pInBuffer = IntPtr.Zero;

            try
            {
                pInBuffer = Marshal.AllocHGlobal((int)nNumberOfBytesToRead);    // Allocate some unmanged RAM for the receive data buffer.

                if (Win32Wrapper.ReadFile(ReadHandleToUSBDevice, pInBuffer, nNumberOfBytesToRead, ref lpNumberOfBytesRead, IntPtr.Zero))
                {
                    Marshal.Copy(pInBuffer, InBuffer, 0, (int)lpNumberOfBytesRead); // Copy over the data from unmanged memory into the managed byte[] INBuffer
                    Marshal.FreeHGlobal(pInBuffer);
                    return true;
                }
                else
                {
                    Marshal.FreeHGlobal(pInBuffer);
                    return false;
                }
            }
            catch
            {
                if (pInBuffer != IntPtr.Zero)
                    Marshal.FreeHGlobal(pInBuffer);

                return false;
            }
        }

        public bool Write(byte[] lpBuffer, uint nNumberOfBytesToWrite, ref uint lpNumberOfBytesWritten)
        {
            return Win32Wrapper.WriteFile(WriteHandleToUSBDevice, lpBuffer, nNumberOfBytesToWrite, ref lpNumberOfBytesWritten, IntPtr.Zero);
        }
    }
}
