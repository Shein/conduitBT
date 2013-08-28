/*******************************************************************\
 Filename    :  Wave.h
 Purpose     :  Input and Output Wave API 
\*******************************************************************/

#pragma once
#pragma managed(push, off)


#include <wmcodecdsp.h> // IMediaBuffer

#include "def.h"
#include "deblog.h"
#include "thread.h"
#include "fifo_cse.h"
#include "DialAppType.h"


// Enable DirectX Media Objects for Voice input instead of WaveIn API
#define DMO_ENABLED


class ScoApp;

class MediaBuffer : public IMediaBuffer
{
  public:

	MediaBuffer (DWORD maxLength) :
		m_ref(0),
		m_maxLength(maxLength),
		m_length(0),
		m_data(NULL){}

	HRESULT SetLength (DWORD cbLength)
	{
		if (cbLength > m_maxLength) {
			return E_INVALIDARG;
		} else {
			m_length = cbLength;
			return S_OK;
		}
	}

	HRESULT GetMaxLength (DWORD *maxLength)
	{
		if (maxLength == NULL) {
			return E_POINTER;
		}
		*maxLength = m_maxLength;
		return S_OK;
	}

	HRESULT GetBufferAndLength (BYTE **buffer, DWORD *length)
	{
		if (buffer == NULL || length == NULL) {
			return E_POINTER;
		}
		*buffer = m_data;
		*length = m_length;
		return S_OK;
	}

	HRESULT QueryInterface (REFIID riid, void **iface)
	{
		if (iface == NULL) {
			return E_POINTER;
		}
		if (riid == IID_IMediaBuffer || riid == IID_IUnknown) {
			*iface = static_cast<IMediaBuffer *>(this);
			AddRef();
			return S_OK;
		}
		*iface = NULL;
		return E_NOINTERFACE;
	}

	ULONG AddRef()
	{
		return InterlockedIncrement(&m_ref);
	}

	ULONG Release()
	{
		LONG lRef = InterlockedDecrement(&m_ref);
		if (lRef == 0) {
			delete this;
		}
		return lRef;
	}

  public:
	DWORD        m_length;
	const DWORD  m_maxLength;
	LONG         m_ref;
	BYTE         *m_data;
};



class Wave : public DebLog, public Thread
{
  public:
	enum {
		//
		// Note: the following voice configuration is also duplicated in the SCO driver!
		//
		VoiceFormat		  = WAVE_FORMAT_PCM,		// Voice PCM format
		VoiceSampleRate	  = 8000,					// Voice Rate samples/sec
		VoiceNchan		  = 1,						// Number of channels (1-mono,2-stereo)
		VoiceBitPerSample = 16,						// Bits per sample

		ChunkSize = 4096,							// Size of one chunk for read and write operation 
		ChunkTime = ChunkSize * 1000 / (VoiceSampleRate * VoiceBitPerSample/8),	// Send time of one chunk in milliseconds
		#ifndef DMO_ENABLED
		ChunkTime4Wait = (ChunkTime + ChunkTime/10)	// When waiting on event/semaphore to use this value
		#else
		ChunkTime4Wait = 100 
		#endif
	};

	struct WAVEBLOCK {
		WAVEHDR	Hdr;
		UINT8	Data[ChunkSize];
	};

	enum STATE {
		STATE_IDLE,			// Not opened or closed
		STATE_READY,		// After opening, ready to be played
		STATE_PLAYING,		// Now playing
		STATE_DESTROYING	// Set in destructor to stop the task 
	};

  public:
	Wave (cchar * task, ScoApp *parent);
	~Wave ();

  public:
	// Open is different in WaveIn & WaveOut
	// void Open();
	void Close();
	void Play();
	void Stop();

  protected:
	typedef MMRESULT (WINAPI *WaveOpen)	 (HWAVE*, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD);
	typedef MMRESULT (WINAPI *WaveClose) (HWAVE);
	typedef MMRESULT (WINAPI *WaveReset) (HWAVE);
	typedef MMRESULT (WINAPI *WaveUnprepare) (HWAVE, WAVEHDR*, UINT);

	WaveOpen		waveOpen;
	WaveClose		waveClose;
	WaveReset		waveReset;
	WaveUnprepare	waveUnprepare;

  protected:
	void CheckMmres (MMRESULT res, cchar * file, int line) {
		if (res != MMSYSERR_NOERROR)
			throw IntException (DialAppError_WaveApiError, "ERROR [%s:%d]: MMRESULT = %d", file, line, res);
	}

	void CheckHresult (HRESULT res, cchar * file, int line) {
		if (FAILED(res))
			throw IntException (DialAppError_InitMediaDeviceError, "ERROR [%s:%d]: HRESULT = %d", file, line, res);
	}

	void CheckState (STATE expected, cchar * file, int line) {
		if (State != expected)
			throw IntException (DialAppError_InternalError, "ERROR [%s:%d]: Wave object unexpected state (%d != %d)", file, line, State, expected);
	}

  protected:
	void UnprepareHeader (WAVEHDR * whdr);
    void ReleaseCompletedBlocks (bool clean_all = false);
	WAVEBLOCK* GetCompletedBlock();
	void ReportVoiceStreamFailure(int error);

    virtual void Run();

	// RunBody() will be implemented in WaveOut & WaveIn, will be called from Run
    virtual void RunBody (WAVEBLOCK * wblock) = 0;

  protected:
	HWAVE			hWave;
	ScoApp		   *Parent;
	WAVEFORMATEX	Format;
	STATE			State;
	bool			ErrorRaised;
	Event			EventStart;
	Event			EventDataReady;
	OVERLAPPED		ScoOverlapped;
	Mutex			RunMutex;

	FIFO_ALLOC<WAVEBLOCK,8>	DataBlocks;
	bool firstIter; // for jitter buffer
};



class WaveOut : public Wave
{
  public:
	WaveOut (ScoApp *parent);
	void Open();

  protected:
    virtual void RunBody (WAVEBLOCK * wblock);
};



class WaveIn : public Wave
{
  public:
	WaveIn (ScoApp *parent);

	~WaveIn()
	{
		mediaObject->FreeStreamingResources();
		mediaObject->Release();
	}

	void Open();

	IMediaObject	*mediaObject;
	MediaBuffer		mediaBuffer;
	DMO_OUTPUT_DATA_BUFFER dataBuffer;

  protected:
    virtual void RunBody (WAVEBLOCK * wblock);
	void DmoInit();
};


/*******************************************************************\
					Error Codes Check Macros
\*******************************************************************/

#define CHECK_MMRES(res)		CheckMmres (res,__FUNCTION__,__LINE__)
#define CHECK_HRESULT(res)		CheckHresult(res,__FUNCTION__,__LINE__)
#define CHECK_STATE(state)		CheckState(state,__FUNCTION__,__LINE__)


#pragma managed(pop)

