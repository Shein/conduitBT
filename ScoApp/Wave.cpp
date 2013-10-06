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
#include <wmcodecdsp.h>
#include <uuids.h>
#include <dmort.h>
#include <propsys.h>



/****************************************************************************************\
									Class Wave 
\****************************************************************************************/

Wave::Wave (cchar * task, ScoApp *parent) :
	DebLog(task), Thread(task), Parent(parent), hWave(0), State(STATE_IDLE), ErrorRaised(false), IoErrorsCnt(0), EventStart(), EventDataReady()
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


void Wave::Destruct ()
{
	if (State == STATE_PLAYING)
		Stop();
	State = STATE_DESTROYING;
	EventStart.Signal();
	Thread::WaitEnding();
	LogMsg("Destructed");
	delete this; // the destructor is empty!
}


void Wave::Play()
{
	CHECK_STATE (STATE_READY);
	ErrorRaised = IoErrorsCnt = 0;
	State = STATE_PLAYING;
	LogMsg("Start playing");
	EventStart.Signal();	// Wake up the thread's run() and start playback
}


void Wave::Stop()
{
	CHECK_STATE (STATE_PLAYING);
	LogMsg("Wave::Stop: starting to stop playback");
	State = STATE_READY;

	RunMutex.Lock();
	// Here Run task should complete all current buffers and jump to sleep on EventStart.Wait()
	RunMutex.Unlock();
	LogMsg("Wave::Stop: playback stoped");
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

	try {
		State = STATE_IDLE;
		RunInit();	// WaveIn or WaveOut Init running 
		State = STATE_READY;
		HfpSm::PutEvent_Ok ();	// Inform HFP SM that WaveIn/WaveOut object completed its init phase

		while (State != STATE_DESTROYING)
		{
			EventStart.Wait();
			// EventStart may be signaled by Play() or Destruct()
			// They set STATE_PLAYING or STATE_DESTROYING correspondingly.
			LogMsg("Task running...");

			if (State == STATE_DESTROYING)
				break;

			RunMutex.Lock();

			RunStart();	// WaveIn or WaveOut Start running 
			while (State == STATE_PLAYING)
			{
				WAVEBLOCK * wblock = DataBlocks.FetchNext ();
				LogMsg ("WAVEBLOCK Fetched:  %X", wblock);
				if (!wblock) {
					LogMsg("ERROR: No free buffers for %s", this->Name);
					ReportVoiceStreamFailure (DialAppError_WaveBuffersError);
					break; // ErrorRaised
				}

				RunBody(wblock);	// WaveIn or WaveOut running part

				if (ErrorRaised) {
					// Stop this playing, waiting close...
					// The State is still STATE_PLAYING!
					break;
				}
			}

			// We are stopping: complete possible currently paying buffers 
			LogMsg("Wave::Run: State(%d)!=STATE_PLAYING, going to sleep...", State);

			RunStop();	// WaveIn or WaveOut Stop running 

			RunMutex.Unlock();
		}

		RunEnd();	// WaveIn or WaveOut End running
	}
	catch (int err)
	{
		LogMsg("EXCEPTION %d", err);
		HfpSm::PutEvent_Failure(err);	// if we have an error when starting voice, we should to notice
	}
}


void Wave::ReleaseCompletedBlocks (bool clean_all)
{
	// If clean_all = false - to release 1st blocks with WHDR_DONE=1
	// If clean_all = true  - to release all blocks: with WHDR_DONE=1 and WHDR_DONE=0 after waiting 

	int i = 0;

	while (WAVEBLOCK* wblock = DataBlocks.GetFirst()) 
	{
		if (!clean_all && ((wblock->Hdr.dwFlags & WHDR_DONE) == 0))
			return;

		while ((wblock->Hdr.dwFlags & WHDR_DONE) == 0) {
			if (i++ == 0)
				LogMsg ("ReleaseCompletedBlocks: Poling %X ...", wblock);
			Sleep(0);
		}
		if (i) {
			LogMsg ("ReleaseCompletedBlocks: Poling finished (%d iterations)", i);
			i = 0;
		}

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


void Wave::ReportVoiceStreamFailure (int error)
{
	ErrorRaised = error;
	HfpSm::PutEvent_Failure (error);
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



void WaveOut::RunInit ()
{
	CHECK_MMRES (waveOpen (&hWave, WAVE_MAPPER, &Format, 0, (DWORD_PTR)this, CALLBACK_NULL));
	LogMsg("Open hWave = %X", hWave);
}


void WaveOut::RunEnd ()
{
	CHECK_MMRES (waveClose(hWave));
}


void WaveOut::RunStart ()
{
}


void WaveOut::RunStop ()
{
	/*
	KS: Do not use waveReset() at all! It is very problematic...
		We will poll WHDR_DONE in ReleaseCompletedBlocks(true)
	try {
		CHECK_MMRES (waveReset(hWave));
	}
	catch (...) {
		ReportVoiceStreamFailure (DialAppError_WaveApiError);
	}
	*/
	ReleaseCompletedBlocks(true);
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
			//LogMsg("Read from SCO pended...");
			res = GetOverlappedResult (Parent->hDevice, &ScoOverlapped, &nbytes, TRUE);
		}
	}

	if (res) {
		LogMsg("Read from SCO %d bytes", nbytes);
	}
	else {
		LogMsg("Read from SCO failed: GetLastError %d", GetLastError());
		if (++IoErrorsCnt > NumVoiceIoErrors2Report) {
			ReportVoiceStreamFailure (DialAppError_ReadScoError);
			IoErrorsCnt = 0;
		}
		wblock->Hdr.dwFlags = WHDR_DONE;	// try to continue, mark the buffer as done
		goto endfunc;
	}

	if (State != STATE_PLAYING) {
		wblock->Hdr.dwFlags = WHDR_DONE;	// mark the buffer as done
		goto endfunc;
	}

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
	Wave ("WaveIn ", parent), MediaBuffer(ChunkSize)
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


void WaveIn::RunInit ()
{
	DMO_MEDIA_TYPE  mediaType;
	WAVEFORMATEX*	pwav;

	CHECK_HRESULT (CoInitializeEx (NULL, COINIT_APARTMENTTHREADED));

	MediaObject = NULL;
	memset(&mediaType, 0, sizeof(mediaType));

	CHECK_HRESULT (CoCreateInstance(CLSID_CWMAudioAEC, NULL, CLSCTX_INPROC_SERVER, IID_IMediaObject, (void**) &MediaObject));
	CHECK_HRESULT (MoInitMediaType(&mediaType, sizeof(WAVEFORMATEX)));

	mediaType.majortype = MEDIATYPE_Audio;
	mediaType.subtype = MEDIASUBTYPE_PCM;
	mediaType.lSampleSize = 0;
	mediaType.bFixedSizeSamples = TRUE;
	mediaType.bTemporalCompression = FALSE;
	mediaType.formattype = FORMAT_WaveFormatEx;

	pwav = (WAVEFORMATEX*)mediaType.pbFormat;
	memcpy(pwav, &Format, sizeof(Format));

	CHECK_HRESULT (MediaObject->SetOutputType(0, &mediaType, 0));

	IPropertyStore *props;
	PROPVARIANT propvar = {0};

	CHECK_HRESULT (MediaObject->QueryInterface(IID_IPropertyStore, (void**)&props));

	propvar.vt = VT_I4;
	propvar.lVal = 0; // System Mode - SINGLE_CHANNEL_AEC
	CHECK_HRESULT (props->SetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, propvar));

	DataBuffer.dwStatus = 0;
	DataBuffer.pBuffer = &MediaBuffer;

	MoFreeMediaType(&mediaType);

	LogMsg("MediaObject = %X", MediaObject);
}


void WaveIn::RunEnd ()
{
	MediaObject->Release();
	CoUninitialize();
}


void WaveIn::RunStart ()
{
	MediaObject->AllocateStreamingResources();
	FirstIter = true;
}


void WaveIn::RunStop ()
{
	MediaObject->FreeStreamingResources();
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
	DWORD	dwStatus;
	HRESULT hr;

	MediaBuffer.m_data   = wblock->Data;
	MediaBuffer.m_length = 0;

	hr = MediaObject->ProcessOutput(0, 1, &DataBuffer, &dwStatus);
	LogMsg("IMediaObject::ProcessOutput completed, HRESULT %X", hr);

	if (hr == S_FALSE) // means nothing to process
		goto finalize;

	if (hr != S_OK) {
		ReportVoiceStreamFailure (DialAppError_MediaObjectInError);
		return;
	}

	/*
	  IMediaObject::ProcessOutput possible values (according to MSDN):
		E_FAIL			Failure
		E_INVALIDARG	Invalid argument
		E_POINTER		NULL pointer argument
		S_FALSE			No output was generated
		S_OK			Success
	*/
 
	if (FirstIter)
		EventDataReady.Wait (ChunkTime4Wait/2);

	res = WriteFile (Parent->hDevice, MediaBuffer.m_data, MediaBuffer.m_length, &n, &ScoOverlapped);

	if (DataBuffer.dwStatus == DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE)
		LogMsg("DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE, size %d", MediaBuffer.m_length);

	if (!res) {
		if (GetLastError() == ERROR_IO_PENDING) 
			LogMsg("Write to SCO pended: %d bytes", MediaBuffer.m_length);
		else 
			LogMsg("Write to SCO failed: GetLastError %d", GetLastError());
	}

	finalize:
	DataBlocks.ReleaseFirst();
	MediaBuffer.m_data = 0;

	if (FirstIter)
		FirstIter = false;
	else 
		EventDataReady.Wait (ChunkTime4Wait);

#endif // USE_DMO
}


#pragma managed(pop)
