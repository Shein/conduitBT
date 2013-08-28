/*******************************************************************\
 Filename    :  Wave.cpp
 Purpose     :  Input and Output Wave API 
\*******************************************************************/

#pragma managed(push, off)

#include "def.h"
#include "Wave.h"
#include "ScoApp.h"
#include "HfpSm.h"

// DMO
#include <uuids.h>
#include <dmort.h>
#include <propsys.h>



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
	firstIter = true; // for jitter buffer
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
	CHECK_STATE (STATE_READY);
	if(hWave)
		CHECK_MMRES (waveClose(hWave));
	State = STATE_IDLE;
}


void Wave::Play()
{
	CHECK_STATE (STATE_READY);
	ErrorRaised = false;
	State = STATE_PLAYING;
	EventStart.Signal();	// Wake up the thread's run() and start playback
}


void Wave::Stop()
{
	CHECK_STATE (STATE_PLAYING);
	LogMsg("Stopping playback");
	State = STATE_READY;
	EventDataReady.Signal();

	RunMutex.Lock();
	if (hWave) {
		CHECK_MMRES (waveReset(hWave));
		LogMsg("Stopping playback", Name);
		ReleaseCompletedBlocks(true);
	}
	RunMutex.Unlock();
}


void Wave::UnprepareHeader (WAVEHDR * whdr)
{
	try {
		CHECK_MMRES (waveUnprepare(hWave, whdr, sizeof(WAVEHDR)));
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

		RunMutex.Lock();
		while (State == STATE_PLAYING)
		{
			WAVEBLOCK * wblock = DataBlocks.FetchNext ();
			LogMsg ("WAVEBLOCK Fetched:  %X", wblock);
			if (!wblock) {
				LogMsg("ERROR: No free buffers for %s", this->Name);
				ReportVoiceStreamFailure (DialAppError_WaveBuffersError);
				break;
			}
			if (ErrorRaised) {
				Sleep (0);
				continue;
			}
			RunBody(wblock);	// WaveIn or WaveOut running part
		}
		RunMutex.Unlock();
	}
}


void Wave::ReleaseCompletedBlocks (bool clean_all)
{
	// This Release function may be called from 2 tasks: Run() & Wave::Stop()
	// So, to lock this code
	// For now it is locked in Stop/Run

	// Release all blocks with the WHDR_DONE flag set
	while (WAVEBLOCK* wblock = DataBlocks.GetFirst()) 
	{
		if (!clean_all && ((wblock->Hdr.dwFlags & WHDR_DONE) == 0))
			return;
		UnprepareHeader (&wblock->Hdr);
		wblock->Hdr.dwFlags = 0;
		DataBlocks.ReleaseFirst();
		LogMsg ("WAVEBLOCK Released: %X", wblock);
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
			LogMsg ("WAVEBLOCK Released: %X", wblock);
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
	CHECK_STATE (STATE_IDLE);
	CHECK_MMRES (waveOpen (&hWave, WAVE_MAPPER, &Format, 0, (DWORD_PTR)this, CALLBACK_NULL));
	LogMsg("Open hWave = %X", hWave);
	State = STATE_READY;
}


void Wave::ReportVoiceStreamFailure (int error)
{
	ErrorRaised = true;
	HfpSm::PutEvent_Failure (error);
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
			res = GetOverlappedResult (Parent->hDevice, &ScoOverlapped, &nbytes, TRUE);
		}
	}

	if (res) {
		LogMsg("Read from SCO %d bytes", nbytes);
	}
	else {
		LogMsg("Read from SCO failed: GetLastError %d", GetLastError());
		ReportVoiceStreamFailure (DialAppError_ReadScoError);
		wblock->Hdr.dwFlags = WHDR_DONE;	// try to continue, mark the buffer as done
		goto endfunc;
	}

	if (State != STATE_PLAYING)
		goto endfunc;

	// Send wblock to the speaker device
	wblock->Hdr.dwBufferLength = nbytes;
	try {
		CHECK_MMRES (waveOutPrepareHeader(HWAVEOUT(hWave), &wblock->Hdr, sizeof(WAVEHDR)));
		CHECK_MMRES (waveOutWrite(HWAVEOUT(hWave), &wblock->Hdr, sizeof(WAVEHDR)));
	}
	catch (...) {
		wblock->Hdr.dwFlags = WHDR_DONE;	// try to continue, mark the buffer as done
	}

	endfunc:
	ReleaseCompletedBlocks();
}



/****************************************************************************************\
									Class WaveIn
\****************************************************************************************/

WaveIn::WaveIn (ScoApp *parent) :
	Wave ("WaveIn ", parent), mediaBuffer(ChunkSize)
{
#ifndef DMO_ENABLED
	waveOpen	  = WaveOpen(waveInOpen);
	waveClose	  = WaveClose(waveInClose);
	waveReset	  = WaveReset(waveInReset);
	waveUnprepare = WaveUnprepare(waveInUnprepareHeader);
#else
	waveOpen	  = 0;
	waveClose	  = 0;
	waveReset	  = 0;
	waveUnprepare = 0;
#endif
}


void WaveIn::Open ()
{
	CHECK_STATE (STATE_IDLE);

	#ifndef DMO_ENABLED
	CHECK_MMRES (waveOpen (&hWave, WAVE_MAPPER, &Format, (DWORD_PTR)EventDataReady.GetWaitHandle(), (DWORD_PTR)this, CALLBACK_EVENT));
	#else
	DmoInit();
	#endif
	State = STATE_READY;
}


// virtual from Wave
void WaveIn::RunBody (WAVEBLOCK * wblock)
{
	DWORD	n;
	BOOL	res;

#ifndef DMO_ENABLED
 	// Send new wblock to the microphone device
	try {
		CHECK_MMRES (waveInPrepareHeader(HWAVEIN(hWave), &wblock->Hdr, sizeof(WAVEHDR)));
		CHECK_MMRES (waveInAddBuffer(HWAVEIN(hWave), &wblock->Hdr, sizeof(WAVEHDR)));
		CHECK_MMRES (waveInStart(HWAVEIN(hWave)));
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
			LogMsg("ERROR: Microphone got stuck");
			ReportVoiceStreamFailure(DialAppError_WaveInError);
			return;
		}
		EventDataReady.Wait(ChunkTime4Wait);
	}

	if (!wblock->Hdr.dwBytesRecorded || State!=STATE_PLAYING)
		return;

	res = WriteFile (Parent->hDevice, wblock->Data, wblock->Hdr.dwBytesRecorded, &n, &ScoOverlapped);
	if (!res) {
		if (GetLastError() == ERROR_IO_PENDING)
			LogMsg("Write to SCO pended: %d bytes", wblock->Hdr.dwBytesRecorded);
		else
			LogMsg("Write to SCO failed: GetLastError %d", GetLastError());
	}

#else
	DWORD dwStatus;
	HRESULT hr;

	mediaBuffer.m_data = wblock->Data;
	mediaBuffer.m_length = 0;

	hr = mediaObject->ProcessOutput(0, 1, &dataBuffer, &dwStatus);
	if (hr == S_OK)
	{
		if (firstIter)
			EventDataReady.Wait (ChunkTime4Wait/2);

		res = WriteFile (Parent->hDevice, mediaBuffer.m_data, mediaBuffer.m_length, &n, &ScoOverlapped);

		if (dataBuffer.dwStatus == DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE)
			LogMsg("ProcessOutput DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE, size %d: ", mediaBuffer.m_length);

		if (!res) {
			if (GetLastError() == ERROR_IO_PENDING) 
				LogMsg("Write to SCO pended: %d bytes", mediaBuffer.m_length);
			else 
				LogMsg("Write to SCO failed: GetLastError %d", GetLastError());
		}
	}
	else if (hr != S_FALSE) // FALSE means nothing to process
	{
		LogMsg("ProcessOutput result: %x", hr);
	}

	DataBlocks.ReleaseFirst();
	mediaBuffer.m_data = NULL;

	if (firstIter && hr == S_OK) {
		firstIter = false;
		return;
	}

	EventDataReady.Wait (ChunkTime4Wait);
#endif // USE_DMO
}


void WaveIn::DmoInit()
{
	DMO_MEDIA_TYPE  mediaType;
	WAVEFORMATEX*	pwav;

	mediaObject = NULL;
	memset(&mediaType, 0, sizeof(mediaType));

	CHECK_HRESULT (CoInitializeEx (NULL, COINIT_APARTMENTTHREADED));
	CHECK_HRESULT (CoCreateInstance(CLSID_CWMAudioAEC, NULL, CLSCTX_INPROC_SERVER, IID_IMediaObject, (void**) &mediaObject));
	CHECK_HRESULT (MoInitMediaType(&mediaType, sizeof(WAVEFORMATEX)));

	mediaType.majortype = MEDIATYPE_Audio;
	mediaType.subtype = MEDIASUBTYPE_PCM;
	mediaType.lSampleSize = 0;
	mediaType.bFixedSizeSamples = TRUE;
	mediaType.bTemporalCompression = FALSE;
	mediaType.formattype = FORMAT_WaveFormatEx;

	pwav = (WAVEFORMATEX*)mediaType.pbFormat;
	memcpy(pwav, &Format, sizeof(Format));

	CHECK_HRESULT (mediaObject->SetOutputType(0, &mediaType, 0));

	IPropertyStore *props;
	PROPVARIANT propvar = {0};

	CHECK_HRESULT (mediaObject->QueryInterface(IID_IPropertyStore, (void**)&props));

	propvar.vt = VT_I4;
	propvar.lVal = 0; // System Mode - SINGLE_CHANNEL_AEC
	CHECK_HRESULT (props->SetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, propvar));

	dataBuffer.dwStatus = 0;
	dataBuffer.pBuffer = &mediaBuffer;

	MoFreeMediaType(&mediaType);
}


#pragma managed(pop)
