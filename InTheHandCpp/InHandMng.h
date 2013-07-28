#pragma once

#include "def.h"
#include "deblog.h"
#include "InHandType.h"
#include "HfpSm.h"


using namespace System;
using namespace System::Text;
using namespace System::Threading;
using namespace System::Runtime::InteropServices;
using namespace System::Net::Sockets;
using namespace System::IO;
using namespace System::Windows::Forms;

using namespace InTheHand::Net;
using namespace InTheHand::Net::Sockets;
using namespace InTheHand::Net::Bluetooth;
using namespace InTheHand::Net::Bluetooth::AttributeIds;
using namespace InTheHand::Windows::Forms;


/*
 ************************************************************************************************
 C++/CLI wrapper class for InTheHand C# library.
 Its purpose to expose InTheHand's bluetooth devices SDP & RFCOMM connectivity
 For now this class is fully static
 ************************************************************************************************
 */
public ref class InHandMng
{
  public:
	static BluetoothClient^				BthCli;		// InTheHand lib local BluetoothClient object
	static array<BluetoothDeviceInfo^>^	Devices;	// Paired devices list 

  public:
	static void Init();
	static void End();
	static void ScanDevices();

	static int	GetDevices (InHandDev* &devices);
	static void	FreeDevices(InHandDev* &devices, int n);

	static void BeginConnect (BluetoothAddress^ bthaddr);
	static void BeginHfpConnect (bool establish_hfp_connection);
	static void Disconnect ();
	static void StartCall (String ^number);
	static void Answer ();
	static void EndCall ();

  protected:
	static void ProcessIoException (IOException ^ex);
	static void SendAtCommand (String ^at);
	static void RecvAtCommand (array<Char> ^buf, int len);
	static void ConnectCallback (IAsyncResult ^ar);
	static void ReceiveThreadFn (Object ^state);

  protected: 
	// String^ utilities
	static cchar* String2Pchar (String ^str) { return (cchar*) Marshal::StringToHGlobalAnsi(str).ToPointer(); }
	static void   FreePchar	(cchar *str)	 { Marshal::FreeHGlobal((IntPtr)((void*)str)); }
	static wchar* String2Wchar (String ^str) { return (wchar*) Marshal::StringToHGlobalUni(str).ToPointer(); }
	static void   FreeWchar	(wchar *str)	 { Marshal::FreeHGlobal((IntPtr)((void*)str)); }

	static void LogMsg (String ^str)
	{
		cchar * chstr = String2Pchar(str);
		InHandLog.LogMsg (chstr);
		FreePchar(chstr);
	}

  protected:
	static NetworkStream^	StreamNet;
	static StreamWriter^	StreamWtr;
};


void InHandMng::Init ()
{
    try {
		BthCli = gcnew BluetoothClient();
		ScanDevices();
    }
	catch (Exception^ ex) {
		LogMsg ("EXCEPTION in InHandMng::Init: " + ex->Message);
		throw int(DialAppError_InitBluetoothRadioError);
    }
}


void InHandMng::End()
{
	BthCli  = nullptr;
	Devices = nullptr;
}
	

void InHandMng::ScanDevices ()
{
	Devices = BthCli->DiscoverDevices (255, false, true/*remembered*/, false, false);
}


int InHandMng::GetDevices (InHandDev* &devices)
{
	int	len = Devices->Length;
	if (len <= 0)
		return 0;

	devices = new InHandDev[len];

	int	i = 0;
	for each (BluetoothDeviceInfo^ item in Devices) {
		devices[i].Address = item->DeviceAddress->ToInt64();
		devices[i].Name	   = (wchar*) Marshal::StringToHGlobalUni(item->DeviceName).ToPointer();
		i++;
	}
	return len;
}


void InHandMng::ProcessIoException (IOException ^ex)
{
	SocketException ^sex = dynamic_cast<SocketException^>(ex->InnerException);
	if (sex) {
		SocketError Err = sex->SocketErrorCode;
		LogMsg("!! Send SocketException: " + Err.ToString() + " (" + Err.ToString("D") + ") " + ex->Message);
	}
	else {
		LogMsg("!! Send IOException: " + ex->Message);
	}
}


void InHandMng::FreeDevices (InHandDev* &devices, int n)
{
	if (devices) {
		for (int i=0; i<n; i++)
			Marshal::FreeHGlobal((IntPtr)((void*)devices[i].Name));

		delete[] devices;
		devices = 0;
	}
}


void InHandMng::SendAtCommand (String ^at)
{
	try
	{
		StreamWtr->Write(at + "\r");
		LogMsg("HF Sent: " + at);
		StreamWtr->Flush();
		return;
	}
	catch (IOException ^ex) {
		ProcessIoException (ex);
	}
	catch (Exception ^ex) {
		LogMsg(ex->Message);
	}

	HfpSm::PutEvent_Failure();
}


void InHandMng::RecvAtCommand (array<Char> ^buf, int len)
{
	String ^str = gcnew String (buf, 0, len);

	cchar* strunm = String2Pchar(str);
	InHandLog.LogMsg (strunm + 2);	// skip leading <cr><lf>
	FreePchar(strunm);

	if (str->Contains ("ERROR")) {
		HfpSm::PutEvent_Failure();
	} 
	if (str->Contains ("+CIEV: 3,0")) {
		HfpSm::PutEvent_CallSetup(SMEV_CallSetup_None);
	} 
	if (str->Contains ("+CIEV: 3,1")) {
		// Instead of 'RING'
		HfpSm::PutEvent_IncomingCall();
	} 
	else if (str->Contains ("+CIEV: 3,2")) {
		HfpSm::PutEvent_CallSetup(SMEV_CallSetup_Outgoing);
	}
	else if (str->Contains ("+CIEV: 2,0")) {
		HfpSm::PutEvent_EndCall();
	}
	else if (str->Contains ("+CIEV: 2,1")) {
		HfpSm::PutEvent_Answer ();
	}
	else if (str->Contains ("+COLP")) {
		int i1 = str->IndexOf('"');
		int i2 = str->LastIndexOf('"');
		if (i1 > 0  &&  i2 > i1) {
			String ^s = str->Substring (i1+1, i2-i1-1);
			wchar* strunm = String2Wchar(s);
			HfpSm::PutEvent_AbonentInfo (strunm);
			FreeWchar(strunm);
		}
	}
}


void InHandMng::ReceiveThreadFn (Object ^state)
{
	ASSERT_ (StreamNet->CanRead);

	StreamReader ^rdr = gcnew StreamReader(StreamNet, Encoding::ASCII);
	array<Char> ^buf = gcnew array<Char>(250);

	try
	{
		while (true)
		{
			// We don't use ReadLine because we then don't get to see the CR/LF characters. And we often get the series \r\r\n
			// which should appear as one new line, but would appear as two if we did textBox.Append("\n") each ReadLine.
			int nread = rdr->Read(buf, 0, buf->Length);
			if (nread == 0) {
				InHandLog.LogMsg ("ReceiveThreadFn read 0 bytes. Finalizing...");
				//newBluetoothClient();
				return;
			}
			RecvAtCommand (buf, nread);
		}
	}
	catch (System::IO::IOException ^ioex)
	{
		SocketException ^sex = dynamic_cast<SocketException^>(ioex->InnerException);
		if (sex) {
			SocketError Err = sex->SocketErrorCode;
			LogMsg ("!! SocketException: " + Err.ToString() + " (" + Err.ToString("D") + ") " + ioex->Message);
		}
		else {
			LogMsg ("!! IOException: " + ioex->Message);
		}
	}
	catch (ObjectDisposedException ^odex)
	{
		LogMsg ("!! ObjectDisposedException: " + odex->Message);
	}
	//TODO
	// 	finally
	// 	{
	// 		newBluetoothClient();
	// 	}
}


#if 0
// Because of the problem in both 32feet's SelectBluetoothDeviceDialog and native BluetoothSelectDevices func,
// (when fAddNewDeviceWizard = FALSE), it was implemented in DialApp.cpp - dialappBluetoothSelectDevice()
// In the future the dialappBluetoothSelectDevice func will be used
uint64 InHandMng::UiSelectDevice ()
{
    SelectBluetoothDeviceDialog^ dlg = gcnew SelectBluetoothDeviceDialog();
    DialogResult rslt = dlg->ShowDialog();
	if (rslt != DialogResult->OK) {
        return 0;
    }
    return dlg->SelectedDevice->DeviceAddress->ToInt64();
}
#endif


void InHandMng::ConnectCallback (IAsyncResult ^ar)
{
	LogMsg ("InHandMng::ConnectCallback");
	try
	{
		BthCli->EndConnect(ar);
		StreamNet = BthCli->GetStream();
		StreamWtr = gcnew StreamWriter (StreamNet, Encoding::ASCII);
		HfpSm::PutEvent_Connected();
		ThreadPool::QueueUserWorkItem(gcnew WaitCallback(ReceiveThreadFn));
		return;
	}
	catch (NullReferenceException^)	 {
		LogMsg ("ConnectFailed - NullReferenceException");
	}
	catch (ObjectDisposedException^) {
		LogMsg ("ConnectFailed - ObjectDisposedException");
	}
	catch (System::Net::Sockets::SocketException ^sex) {
		LogMsg ("Connect failed: " + sex->SocketErrorCode.ToString() + " (" + sex->SocketErrorCode.ToString("D") + ") " + sex->Message);
	}

	HfpSm::PutEvent_Failure();
}


void InHandMng::BeginConnect (BluetoothAddress^ bthaddr)
{
	try
	{
		AsyncCallback^ cbk = gcnew AsyncCallback (&ConnectCallback);
		BthCli->BeginConnect(bthaddr, BluetoothService::Handsfree, cbk, nullptr);
	}
	catch (SocketException^ sex)
	{
		LogMsg ("Connect failed: " + sex->SocketErrorCode.ToString() + " (" + sex->SocketErrorCode.ToString("D") + "); " + sex->Message);
		HfpSm::PutEvent_Failure();
	}
}


void InHandMng::BeginHfpConnect (bool establish_hfp_connection)
{
	if (establish_hfp_connection) {
		SendAtCommand("AT+BRSF=16");
		SendAtCommand("AT+CIND=?");
		SendAtCommand("AT+CMER=3,0,0,1");
		SendAtCommand("AT+CMEE=1");
	}
	HfpSm::PutEvent_HfpConnected();
}


void InHandMng::Disconnect ()
{
	if (StreamWtr)
	{
		StreamWtr->Close();
		ASSERT__(!BthCli->Connected);
	}
	//BthCli->Close();
	//BthCli = gcnew BluetoothClient();
}


void InHandMng::StartCall(String^ number)
{
	//SendAtCommand("AT+BLDN"); - redial last
	SendAtCommand("ATD" + number + ";");
}


void InHandMng::Answer()
{
	SendAtCommand("ATA");
}


void InHandMng::EndCall()
{
	SendAtCommand("AT+CHUP");
	SendAtCommand("ATH");
}

