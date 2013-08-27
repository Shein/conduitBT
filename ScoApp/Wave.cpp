#pragma managed(push, off)

#include "def.h"
#include "Wave.h"
#include "ScoApp.h"
#include "HfpSm.h"


/****************************************************************************************\
									Class Wave 
\****************************************************************************************/

Wave::Wave (cchar * task, ScoApp *parent) :
	DebLog(task), Thread(task), Parent(parent), hWave(0), State(STATE_IDLE), ErrorRaised(false), EventStart(), EventDataReady()
{
	memset (&Format, 0, sizeof(WAVEFORMATEX));

	for (int i=0; i<DataBlocks.Size; i++) {
		memset (&DataBlocks.Addr[i], 0, sizeof(WAVEBLOCK));
		DataBlocks.Addr[i].Hdr.lpData = (char*) DataBlocks.Addr[i].Data;
		DataBlocks.Addr[i].Hdr.dwBufferLength = ChunkSize;	// it's actual for WaveIn only, for WaveOut will be overridden 
	}

	int block = VoiceBitPerSample/8 * VoiceNchan;

	Format.wFormatTag		= VoiceFormat;
	Format.nChannels		= VoiceNchan;
	Format.nSamplesPerSec	= VoiceSampleRate;
	Format.nAvgBytesPerSec	= block * VoiceSampleRate;
	Format.nBlockAlign		= block;
	Format.wBitsPerSample	= VoiceBitPerSample;

	if (!EventStart.GetWaitHandle() || !EventDataReady.GetWaitHandle())
		throw IntException (DialAppError_InsufficientResources, "CreateEvent() failed");

	memset(&ScoOverlapped, 0, sizeof(OVERLAPPED)); 

	Thread::Construct();
	Thread::Execute();
	LogMsg("Thread ID = %d", Thread::GetThreadId());
}


Wave::~Wave ()
{
	if (State != STATE_IDLE)
		Close();
	State = STATE_DESTROYING;
	EventStart.Signal();
	Thread::WaitEnding();
	LogMsg("Destructed");
}


void Wave::Close()
{
	// Stop playback if necessary 
	if (State == STATE_PLAYING)
		Stop();
	CheckState (STATE_READY);
	CheckMmres (waveClose(hWave));
	State = STATE_IDLE;
}


void Wave::Play()
{
	CheckState (STATE_READY);
	ErrorRaised = false;
	State = STATE_PLAYING;
	EventStart.Signal();	// Wake up the thread's run() and start playback
}


void Wave::Stop()
{
	CheckState (STATE_PLAYING);
	LogMsg("%s: Stopping playback", Name);
	State = STATE_READY;
	EventDataReady.Signal();
	CheckMmres (waveReset(hWave));
	ReleaseCompletedBlocks(true);
}


void Wave::UnprepareHeader (WAVEHDR * whdr)
{
	try {
		LogMsg ("UnprepareHeader %X", whdr);
		CheckMmres (waveUnprepare(hWave, whdr, sizeof(WAVEHDR)));
	}
	catch (...) {
		// do nothing
	}
}


// virtual from Thread
void Wave::Run()
{
	LogMsg("Task started...");

	while (State != STATE_DESTROYING)
	{
		EventStart.Wait();
		LogMsg("Task running...");

		while (State == STATE_PLAYING)
		{
			WAVEBLOCK * wblock = DataBlocks.FetchNext ();
			if (!wblock) {
				LogMsg("CRITICAL ERROR: No free buffers for %s", this->Name);
				ReportVoiceStreamFailure();
				break;
			}
			if (ErrorRaised) {
				Sleep (0);
				continue;
			}
			RunBody(wblock);	// WaveIn or WaveOut running part
		}
	}
}


void Wave::ReleaseCompletedBlocks (bool clean_all)
{
	// This Release function may be called from 2 tasks: Run() & Wave::Stop()
	// So, to lock this code
	MUTEXLOCK(ReleaseMutex);

	// Release all blocks with the WHDR_DONE flag set
	while (WAVEBLOCK* wblock = DataBlocks.GetFirst()) 
	{
		if (!clean_all && ((wblock->Hdr.dwFlags & WHDR_DONE) == 0))
			return;
		UnprepareHeader (&wblock->Hdr);
		wblock->Hdr.dwFlags = 0;
		DataBlocks.ReleaseFirst();
	}
}


Wave::WAVEBLOCK*  Wave::GetCompletedBlock()
{
	if (WAVEBLOCK* wblock = DataBlocks.GetFirst()) 
	{
		if (wblock->Hdr.dwFlags & WHDR_DONE) {
			UnprepareHeader (&wblock->Hdr);
			wblock->Hdr.dwFlags = 0;
			DataBlocks.ReleaseFirst();
			return wblock;
		}
	}
	return 0;
}



/****************************************************************************************\
									Class WaveOut
\****************************************************************************************/

WaveOut::WaveOut (ScoApp *parent) :
	Wave ("WaveOut", parent)
{
	// WaveOut uses ScoOverlapped.hEvent when doing ReadFile
	ScoOverlapped.hEvent = (HANDLE) EventDataReady.GetWaitHandle();

	waveOpen	  = WaveOpen(waveOutOpen);
	waveClose	  = WaveClose(waveOutClose);
	waveReset	  = WaveReset(waveOutReset);
	waveUnprepare = WaveUnprepare(waveOutUnprepareHeader);
}


void WaveOut::Open ()
{
	CheckState (STATE_IDLE);
	CheckMmres (waveOpen (&hWave, WAVE_MAPPER, &Format, 0, (DWORD_PTR)this, CALLBACK_NULL));
	State = STATE_READY;
}


void Wave::ReportVoiceStreamFailure()
{
	ErrorRaised = true;
	HfpSm::PutEvent_Failure (DialAppError_ReadWriteScoError);
}


// virtual from Wave
void WaveOut::RunBody (WAVEBLOCK * wblock)
{
	DWORD	nbytes;
	BOOL	res;

	EventDataReady.Reset();	// this event is also assigned to ScoOverlapped
	res = ReadFile (Parent->hDevice, wblock->Data, ChunkSize, &nbytes, &ScoOverlapped);
	if (!res) {
		if (GetLastError() == ERROR_IO_PENDING) {
			LogMsg("Read from SCO pended...");
			res = GetOverlappedResult (Parent->hDevice, &ScoOverlapped, &nbytes, TRUE);
		}
	}

	if (res) {
		LogMsg("Read from SCO %d bytes", nbytes);
	}
	else {
		LogMsg("Read from SCO failed: GetLastError %d", GetLastError());
		ReportVoiceStreamFailure();
		goto endfunc;	// try to continue
	}

	if (State != STATE_PLAYING)
		goto endfunc;

	// Send wblock to the speaker device
	wblock->Hdr.dwBufferLength = nbytes;
	try {
		CheckMmres (waveOutPrepareHeader(HWAVEOUT(hWave), &wblock->Hdr, sizeof(WAVEHDR)));
		CheckMmres (waveOutWrite(HWAVEOUT(hWave), &wblock->Hdr, sizeof(WAVEHDR)));
	}
	catch (...) {
		// try to continue
		if (wblock->Hdr.dwFlags & WHDR_PREPARED)
			UnprepareHeader (&wblock->Hdr);
		wblock->Hdr.dwFlags = 0;
	}

	endfunc:
	ReleaseCompletedBlocks();
}



/****************************************************************************************\
									Class WaveIn
\****************************************************************************************/

WaveIn::WaveIn (ScoApp *parent) :
	Wave ("WaveIn ", parent)
{
	waveOpen	  = WaveOpen(waveInOpen);
	waveClose	  = WaveClose(waveInClose);
	waveReset	  = WaveReset(waveInReset);
	waveUnprepare = WaveUnprepare(waveInUnprepareHeader);
}


void WaveIn::Open ()
{
	CheckState (STATE_IDLE);
	CheckMmres (waveOpen (&hWave, WAVE_MAPPER, &Format, (DWORD_PTR)EventDataReady.GetWaitHandle(), (DWORD_PTR)this, CALLBACK_EVENT));
	State = STATE_READY;
}

// virtual from Wave
void WaveIn::RunBody (WAVEBLOCK * wblock)
{
	DWORD	n;
	BOOL	res;

	// Send new wblock to the microphone device
	try {
		CheckMmres (waveInPrepareHeader(HWAVEIN(hWave), &wblock->Hdr, sizeof(WAVEHDR)));
		CheckMmres (waveInAddBuffer(HWAVEIN(hWave), &wblock->Hdr, sizeof(WAVEHDR)));
		CheckMmres (waveInStart(HWAVEIN(hWave)));
	}
	catch (...) {
		// try to continue
		if (wblock->Hdr.dwFlags & WHDR_PREPARED)
			UnprepareHeader (&wblock->Hdr);
		wblock->Hdr.dwFlags = 0;
	}

	// Try to get another block filled by microphone
	LogMsg("Waiting data from Microphone...");
	n = 0;
	while ((wblock = GetCompletedBlock()) == 0) {
		if (State != STATE_PLAYING)
			return;
		if (++n == 4) {
			LogMsg("CRITICAL ERROR: Microphone got stuck");
			ReportVoiceStreamFailure();
			return;
		}
		EventDataReady.Wait(ChunkTime4Wait);
	}

	if (!wblock->Hdr.dwBytesRecorded || State!=STATE_PLAYING)
		return;

	res = WriteFile (Parent->hDevice, wblock->Data, wblock->Hdr.dwBytesRecorded, &n, &ScoOverlapped);
	if (!res) {
		if (GetLastError() == ERROR_IO_PENDING) {
			LogMsg("Write to SCO pended: %d bytes", wblock->Hdr.dwBytesRecorded);
		}
		else {
			LogMsg("Write to SCO failed: GetLastError %d", GetLastError());
		}
	}
}


#pragma managed(pop)
