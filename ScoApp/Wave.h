#pragma once
#pragma managed(push, off)


#include "def.h"
#include "deblog.h"
#include "thread.h"
#include "fifo_cse.h"
#include "DialAppType.h"


class ScoApp;

class Wave : public DebLog, public Thread
{
  public:
	enum {
		//
		// Note: the following voice configuration is also duplicated in the SCO driver!
		//
		VoiceFormat		  = WAVE_FORMAT_ALAW,		// Voice PCM format
		VoiceSampleRate	  = 8000,					// Voice Rate samples/sec
		VoiceNchan		  = 1,						// Number of channels (1-mono,2-stereo)
		VoiceBitPerSample = 8,						// Bits per sample

		ChunkSize = 2048,							// Size of one chunk for read and write operation 
		ChunkTime = ChunkSize * 1000 / (VoiceSampleRate * VoiceBitPerSample/8),	// Send time of one chunk in milliseconds
		ChunkTime4Wait = ChunkTime + ChunkTime/10	// When waiting on event/semaphore to use this value
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
	void CheckMmres (MMRESULT res) {
		if (res != MMSYSERR_NOERROR)
			throw IntException (DialAppError_WaveApiError, "ERROR: MMRESULT = %d", res);
	}

	void CheckState (STATE expected) {
		if (State != expected)
			throw IntException (DialAppError_InternalError, "Wave object unexpected state (%d != %d)", State, expected);
	}

  protected:
	void UnprepareHeader (WAVEHDR * whdr);
    void ReleaseCompletedBlocks (bool clean_all = false);
	WAVEBLOCK* GetCompletedBlock();
	void ReportVoiceStreamFailure();

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
	Mutex			ReleaseMutex;

	FIFO_ALLOC<WAVEBLOCK,8>	DataBlocks;
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
	void Open();

  protected:
    virtual void RunBody (WAVEBLOCK * wblock);
};



#pragma managed(pop)
