#pragma once
#pragma managed(push, off)

#include "def.h"
#include "deblog.h"
#include "Wave.h"


typedef void (*ScoAppCb) ();


/*
 ************************************************************************************************
 C++ class for working with HFP Driver.
 All methods can throw char* exception with the error description (except the empty ScoApp() 
 constructor, which does nothing, and the destructor with Destruct() method).
 Construct() function is intenionaly separated from the constructor itself in order to simplify 
 exceptions handling.

 Usage: 
 1. To call once Init() to get driver's interface ID
 2. Create ScoApp object using its simple constructor
 3. Call the real constructor: Construct() method
 4. Call OpenSco() after the high level applications has already established a control connection
 ************************************************************************************************
 */
class ScoApp : public DebLog, public Thread
{
  public:
    HANDLE  hDevice;	// May be tested for detecting the object constructing state

  public:
	static void Init ();
	static void End  ();

  public:
	ScoApp(ScoAppCb connect_cb, ScoAppCb disconnect_cb) : DebLog("ScoApp "), Thread("ScoApp"), hDevice(0)
	{
		Construct(connect_cb, disconnect_cb);
	}

	~ScoApp()
	{
		Destruct();
	}

	void Construct (ScoAppCb connect_cb, ScoAppCb disconnect_cb);
	void Destruct  () throw();

	void StartServer (uint64 destaddr, bool readiness);
	void StopServer  ();

	void OpenSco   (bool waveonly = false);
	void CloseSco  (bool waveonly = false);
	void CloseScoLowLevel ();

	void VoiceStart ();

	void SetIncomingReadiness (bool readiness);

	bool IsConstructed()	{ return (hDevice!=0);	}
	bool IsStarted ()		{ return (DestAddr!=0); }
	bool IsOpen ()			{ return Open; }

  protected:
	void  OpenDriver ();
	void  ReopenDriver ();

  protected:
	// Separate Thread for waiting on EventScoConnect & EventScoDisconnect
    virtual void Run();

  protected:
    static PSP_DEVICE_INTERFACE_DETAIL_DATA  DeviceInterfaceDetailData;

  protected:
	UINT64		DestAddr;	// Address of a Destination Bluetooth device, it's also started server indication
	bool		Open;
	WaveOut	   *WaveOutDev;
	WaveIn	   *WaveInDev;
	Event		EventScoConnect;
	Event		EventScoDisconnect;
    bool		Destructing;
	ScoAppCb	ConnectCb;
	ScoAppCb	DisconnectCb;
};


#pragma managed(pop)

