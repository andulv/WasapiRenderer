#include <streams.h>
#include "StdAfx.h"
#include <assert.h>
#include <avrt.h>
#include "WASAPIRenderer.h"
#define AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED AUDCLNT_ERR(0x019)

CWASAPIRenderer::CWASAPIRenderer(LPCWSTR pDevID) : 
    _AudioClient(NULL),
    _RenderClient(NULL),
    _RenderBufferQueue(NULL),
    _RenderThread(NULL),
    _ShutdownEvent(NULL),
    _ProcessSamplesInQueueEvent(NULL),
    _AudioBufferReadyEvent(NULL),
	_ExitFeederLoopEvent(NULL),
	_pCurrentMediaType(NULL),
	_pCurrentSample(NULL),
	_CurrentSampleOffset(0),
	CurrentSampleStart(NULL),
	CurrentSampleEnd(NULL),
	_ShareMode(AUDCLNT_SHAREMODE_SHARED),
	InitializedMode(-1)
{
	IMMDeviceEnumerator *deviceEnumerator = NULL;
	UINT deviceCount;

	HRESULT hr=S_OK;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
	if (FAILED(hr))
	{			
		DebugPrintf(L"CWASAPIRenderer - Unable to instantiate device enumerator: %x\n", hr);
		goto exit;
	}

	if(pDevID)
		hr=deviceEnumerator->GetDevice(pDevID,&_Endpoint);
	else
		hr=deviceEnumerator->GetDefaultAudioEndpoint(eRender,eConsole,&_Endpoint);

	if (FAILED(hr))
	{
		DebugPrintf(L"CWASAPIRenderer - Unable to get device: %x\n", hr);
		goto exit;
	}

	_Endpoint->AddRef();

    //  Create our shutdown and samples ready events- we want auto reset events that start in the not-signaled state.
    _ShutdownEvent = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    if (_ShutdownEvent == NULL)
    {
        DebugPrintf(L"CWASAPIRenderer - Unable to create shutdown event: %d.\n", GetLastError());
        goto exit;
    }

    _AudioBufferReadyEvent = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    if (_AudioBufferReadyEvent == NULL)
    {
        DebugPrintf(L"CWASAPIRenderer - Unable to create samples ready event: %d.\n", GetLastError());
        goto exit;
    }

	_ProcessSamplesInQueueEvent = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE);
    if (_ProcessSamplesInQueueEvent == NULL)
    {
        DebugPrintf(L"CWASAPIRenderer - Unable to create samples ready event: %d.\n", GetLastError());
        goto exit;
    }

	//  Create our shutdown and samples ready events- we want auto reset events that start in the not-signaled state.
    _ExitFeederLoopEvent = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    if (_ExitFeederLoopEvent == NULL)
    {
        DebugPrintf(L"CWASAPIRenderer - Unable to create _ExitFeederLoopEvent event: %d.\n", GetLastError());
        goto exit;
    }

exit:
    SafeRelease(&deviceEnumerator);
}

CWASAPIRenderer::~CWASAPIRenderer(void) 
{
	Stop();
	ClearQueue();
    if (_ShutdownEvent)
    {
        CloseHandle(_ShutdownEvent);
        _ShutdownEvent = NULL;
    }
    if (_AudioBufferReadyEvent)
    {
        CloseHandle(_AudioBufferReadyEvent);
        _AudioBufferReadyEvent = NULL;
    }
	if (_ProcessSamplesInQueueEvent)
    {
        CloseHandle(_ProcessSamplesInQueueEvent);
        _ProcessSamplesInQueueEvent = NULL;
    }
	if (_ExitFeederLoopEvent)
    {
        CloseHandle(_ExitFeederLoopEvent);
        _ExitFeederLoopEvent = NULL;
    }

    SafeRelease(&_Endpoint);
}


WAVEFORMATEX* CWASAPIRenderer::GetWasapiMixFormat()
{
	WAVEFORMATEX* retValue=NULL;

	IAudioClient* audioClient;
    HRESULT hr = _Endpoint->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, NULL, reinterpret_cast<void **>(&audioClient));
    if (FAILED(hr))
    {
        DebugPrintf(L"CWASAPIRenderer - Unable to activate audio client: %x.\n", hr);
		goto exit;
    }
    hr = audioClient->GetMixFormat(&retValue);
exit:
	SafeRelease(&audioClient);
	return retValue;
}

//Returns true if format can be used in specified mode.
//Returns a suggested alternative mode if format can not be used.
bool CWASAPIRenderer::CheckFormat(WAVEFORMATEX* requestedFormat, WAVEFORMATEX** ppSuggestedFormat, AUDCLNT_SHAREMODE shareMode)
{
	WAVEFORMATEX* pSuggestedFormat=NULL;
	IAudioClient* audioClient;
	bool retValue=false;
    HRESULT hr = _Endpoint->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, NULL, reinterpret_cast<void **>(&audioClient));
    if (FAILED(hr))
    {
        DebugPrintf(L"CWASAPIRenderer - Unable to activate audio client: %x.\n", hr);
		return false;
    }

    hr = audioClient->IsFormatSupported(shareMode,requestedFormat, &pSuggestedFormat);
    if (hr == S_OK)
    {
		retValue=true;
		goto exit;
	}

	if(!pSuggestedFormat) {
		hr = audioClient->GetMixFormat(&pSuggestedFormat);
	}

exit:
	*ppSuggestedFormat=pSuggestedFormat;
	SafeRelease(&audioClient);
	return retValue;
}

void CWASAPIRenderer::SetIsProcessing(bool isOK)
{
	CAutoLock lock(&m_QueueLock);
	m_bIsProcessing=isOK;
	UpdateProcessSamplesInQueueEvent();
	if(isOK)
		ResetEvent(_ExitFeederLoopEvent);
	else
		SetEvent(_ExitFeederLoopEvent);
}


//  Start rendering - Create and start the render thread 
//  Returns true and does nothing if already running
bool CWASAPIRenderer::Start(UINT32 EngineLatency)
{
    HRESULT hr;
	if(_RenderThread)		//Already running
	{
		DebugPrintf(L"CWASAPIRenderer::Start - Already running.\n");
		return true;
	}

	DebugPrintf(L"CWASAPIRenderer::Start - Starting up.\n");
	//  Remember our configured latency in case we'll need it for a stream switch later.
    _EngineLatencyInMS = EngineLatency;

	//  Now create the thread which is going to drive the renderer.
    if (_ShutdownEvent)
    {
        ResetEvent(_ShutdownEvent);
    }

    _RenderThread = CreateThread(NULL, 0, WASAPIRenderThread, this, 0, NULL);
    if (_RenderThread == NULL)
    {
        DebugPrintf(L"Unable to create transport thread: %x.", GetLastError());
        return false;
    }
    return true;
}

//
//  Stop the renderer.
//
void CWASAPIRenderer::Stop()
{
    HRESULT hr;

    //  Tell the render thread to shut down, wait for the thread to complete then clean up all the stuff we 
    //  allocated in Start().

    if (_ShutdownEvent)
    {
        SetEvent(_ShutdownEvent);
    }

	if (_RenderThread)
    {
		DebugPrintf(L"CWASAPIRenderer::Stop() - Stopping.\n");
        WaitForSingleObject(_RenderThread, INFINITE);
        CloseHandle(_RenderThread);
        _RenderThread = NULL;
    }
	else
	{
		DebugPrintf(L"CWASAPIRenderer::Stop() - Already stopped.\n");
	}
}


void CWASAPIRenderer::ClearQueue()
{
	DebugPrintf(L"CWASAPIRenderer::ClearQueue()\n");
	CAutoLock lock(&m_QueueLock);
	while (_RenderBufferQueue != NULL)
    {
        RenderBuffer *node = _RenderBufferQueue;
		node->pSample->Release();
		node->pMediaType->Release();
        _RenderBufferQueue = node->_Next;
        delete node;
    }
	if(_pCurrentSample)
		_pCurrentSample->Release();
	if(_pCurrentMediaType)
		_pCurrentMediaType->Release();
	_pCurrentSample=NULL;
	_pCurrentMediaType=NULL;
	_CurrentSampleOffset=0;
	CurrentSampleStart=NULL;
	CurrentSampleEnd=NULL;
	UpdateProcessSamplesInQueueEvent();
}

void CWASAPIRenderer::AddSampleToQueue(IMediaBufferEx *pSample, RefCountingWaveFormatEx *pMediaType, bool isExclusive)
{
	RenderBuffer *newNode = new RenderBuffer();
	newNode->pSample=pSample;
	newNode->pMediaType=pMediaType;
	newNode->ExclusiveMode=isExclusive;
	{
		CAutoLock lock(&m_QueueLock);
		if(_RenderBufferQueue==NULL)
		{
			_RenderBufferQueue=newNode;
			UpdateProcessSamplesInQueueEvent();
		}
		else
		{
			RenderBuffer *tailNode = _RenderBufferQueue;
			while(tailNode->_Next!=NULL)
				tailNode = tailNode->_Next;
			tailNode->_Next=newNode;
		}
	}
}

REFERENCE_TIME CWASAPIRenderer::GetCurrentSampleTime()
{
	return CurrentSampleStart;
}

//Sets the event if we have samples (_RenderBufferQueue!=null && m_bIsProcessing)
//Caller should lock on QueueLock before calling
void CWASAPIRenderer::UpdateProcessSamplesInQueueEvent()
{
	if(_RenderBufferQueue&&m_bIsProcessing)
		SetEvent(_ProcessSamplesInQueueEvent);
	else
		ResetEvent(_ProcessSamplesInQueueEvent);

}

bool CWASAPIRenderer::ReleaseAudioEngine()
{
	HRESULT hr=S_OK;
	//if(_AudioClient)
	//{
	//	hr = _AudioClient->Stop();
	//	if (FAILED(hr))
	//	{
	//		DebugPrintf(L"Unable to stop audio client: %x\n", hr);
	//	}
	//}
	SafeRelease(&_AudioClient);
	SafeRelease(&_RenderClient);
	InitializedMode=-1;
	return true;
}


//
//  Initialize WASAPI in event driven mode, associate the audio client with our samples ready event handle, and retrieve 
//  a render client for the transport.
//
bool CWASAPIRenderer::InitializeAudioClient()
{
    REFERENCE_TIME bufferDuration = _EngineLatencyInMS*10000;
	REFERENCE_TIME period = bufferDuration;

	if(_ShareMode==AUDCLNT_SHAREMODE_SHARED)
		period=0;

    HRESULT hr = _AudioClient->Initialize(_ShareMode, 
                                          AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST, 
                                          bufferDuration, 
                                          period ,
										  _pCurrentMediaType->GetFormat(), 
                                          NULL);

    //  When rendering in exclusive mode event driven, the HDAudio specification requires that the buffers handed to the device must 
    //  be aligned on a 128 byte boundary.  When the buffer is initialized and the resulting buffer size would not be 128 byte aligned,
    //  we need to "swizzle" the periodicity of the engine to ensure that the buffers are properly aligned.
    if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) 
    {
        UINT32 bufferSize;
        DebugPrintf(L"Buffers not aligned. Aligning the buffers... \n");
        //
        //  Retrieve the buffer size for the audio client.  The buffer size returned is aligned to the nearest 128 byte
        //  boundary given the input buffer duration.
        //
        hr = _AudioClient->GetBufferSize(&bufferSize);
        if(FAILED(hr))
        {
            DebugPrintf(L"Unable to get audio client buffer: %x. \n", hr);
            return false;
        }

        //  Calculate the new aligned periodicity.  We do that by taking the buffer size returned (which is in frames),
        //  multiplying it by the frames/second in the render format (which gets us seconds per buffer), then converting the 
        //  seconds/buffer calculation into a REFERENCE_TIME.
        //
        bufferDuration = (REFERENCE_TIME)(10000.0 *                         // (REFERENCE_TIME / ms) *
                                          1000 *                            // (ms / s) *
                                          bufferSize /                      // frames /
                                          _pCurrentMediaType->GetFormat()->nSamplesPerSec +      // (frames / s)
                                          0.5);                             // rounding


        hr = _AudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, 
                                      AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST, 
                                      bufferDuration, 
                                      bufferDuration, 
                                      _pCurrentMediaType->GetFormat(), 
                                      NULL);
        if (FAILED(hr))
        {
            DebugPrintf(L"Unable to reinitialize audio client: %x \n", hr);
            return false;
        }

    }
    else if (FAILED(hr))
    {
        DebugPrintf(L"Unable to initialize audio client: %x.\n", hr);
        return false;
    }

    //
    //  Retrieve the buffer size for the audio client.
    //
    hr = _AudioClient->GetBufferSize(&_BufferSize);
    if(FAILED(hr))
    {
        DebugPrintf(L"Unable to get audio client buffer: %x. \n", hr);
        return false;
    }

    //  When rendering in event driven mode, we'll always have exactly a buffer's size worth of data
    //  available every time we wake up.
    _BufferSizePerPeriod = _BufferSize;

    hr = _AudioClient->GetService(IID_PPV_ARGS(&_RenderClient));
    if (FAILED(hr))
    {
        DebugPrintf(L"Unable to get new render client: %x.\n", hr);
        return false;
    }

	hr = _AudioClient->SetEventHandle(_AudioBufferReadyEvent);
    return true;
}


//  Initialize the renderer.
//	_pCurrentMediaType should be set to an valid (accepted) mediatype containing an WAVEFORMATEX(TENSIBLE) format block
bool CWASAPIRenderer::InitializeAudioEngine()
{
    //  Now activate an IAudioClient object on our preferred endpoint
    HRESULT hr = _Endpoint->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, NULL, reinterpret_cast<void **>(&_AudioClient));
    if (FAILED(hr))
    {
        DebugPrintf(L"Unable to activate audio client: %x.\n", hr);
    }

	_FrameSize = _pCurrentMediaType->GetFormat()->nBlockAlign;

    if (InitializeAudioClient())
	{
		InitializedMode=_ShareMode;
	    return true;
	}
	else
	{
		ReleaseAudioEngine();
        return false;
	}


}

//  Render thread - processes samples from the audio engine
DWORD CWASAPIRenderer::WASAPIRenderThread(LPVOID Context)
{
	CWASAPIRenderer *renderer = static_cast<CWASAPIRenderer *>(Context);
	HANDLE mmcssHandle = NULL;
    DWORD mmcssTaskIndex = 0;
	DWORD retValue=NULL;

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        DebugPrintf(L"Unable to initialize COM in render thread: %x\n", hr);
        return hr;
    }

    mmcssHandle = AvSetMmThreadCharacteristics(L"Audio", &mmcssTaskIndex);
    if (mmcssHandle == NULL)
    {
        DebugPrintf(L"Unable to enable MMCSS on render thread: %d\n", GetLastError());
    }

    retValue = renderer->DoRenderThread();

	AvRevertMmThreadCharacteristics(mmcssHandle);
    CoUninitialize();
	return retValue;

}


//Returns true if new mediatype is different from last mediatype
bool CWASAPIRenderer::PopulateCurrentFromQueue()
{
	bool reportChange=false;
	RenderBuffer* pulledNode=NULL;
	{
		CAutoLock lock(&m_QueueLock);
		pulledNode=_RenderBufferQueue;
		if(_RenderBufferQueue!=NULL)
		{
			_RenderBufferQueue=_RenderBufferQueue->_Next;
		}
		if(!_RenderBufferQueue)		//Last sample was pulled
			UpdateProcessSamplesInQueueEvent();
	}

	if(_pCurrentMediaType)		//Current set has value. Needs to release before setting.
	{
		_pCurrentSample->Release();
		_pCurrentSample=NULL;
		_CurrentSampleOffset=0;
		if(!pulledNode)
		{
			//Changed from non-null to null, reportchange=false (buffer underrun, not reinit) (Reinit will occur next time we receive a sample)
			_pCurrentMediaType->Release();
			_pCurrentMediaType=NULL;
		}
		else	//Pulled node has value. Compare with current set before releasing currentmediatype.
		{
			reportChange = *(pulledNode->pMediaType) != *_pCurrentMediaType;		//Sjekker om innholdet i mediatypene er ulike.
			_pCurrentMediaType->Release();
			_pCurrentSample=pulledNode->pSample;
			_pCurrentMediaType=pulledNode->pMediaType;

		}
	}
	else								//No need for release
	{
		if(pulledNode)
		{
			reportChange=true;		//Changed from null to non-null
			_CurrentSampleOffset=0;
			_pCurrentSample=pulledNode->pSample;
			_pCurrentMediaType=pulledNode->pMediaType;
		}
	}
	
	if(_pCurrentSample)
	{
		CurrentSampleStart=_pCurrentSample->GetStartTime();
		CurrentSampleEnd=_pCurrentSample->GetEndTime();
	}
	//else
	//{
	//	CurrentSampleStart=NULL;
	//	CurrentSampleEnd=NULL;
	//}


	//if(reportChange)
	//{
	//	_hasTriedExclusiveModeWithCurrentMediaType=false;
	//}

	if(pulledNode)
	{
		AUDCLNT_SHAREMODE newShareMode=pulledNode->ExclusiveMode ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED;
		if(newShareMode!=_ShareMode)		
		{
			//We can always switch from exclusive to shared. Trigger reinit by returning true.
			if(newShareMode==AUDCLNT_SHAREMODE_SHARED)
			{
				_ShareMode=newShareMode;
				reportChange=true;
				//_hasTriedExclusiveModeWithCurrentMediaType=false;
			}
			else //if(!_hasTriedExclusiveModeWithCurrentMediaType)
			//Check if we can switch to Exclusive Mode, if not we just want to continue in shared mode.
			{
				WAVEFORMATEX* formatNew=_pCurrentMediaType->GetFormat();
				WAVEFORMATEX* suggestedFormat=NULL;
				bool isOK=CheckFormat(formatNew,&suggestedFormat,newShareMode);
				if(isOK)
				{
					_ShareMode=newShareMode;
					reportChange=true;
				}
				if(suggestedFormat)
					CoTaskMemFree(suggestedFormat);
				//_hasTriedExclusiveModeWithCurrentMediaType=true;
			}
		}
		delete pulledNode;
	}
	return reportChange;
}


//  Returnerer ERROR_UNSUPPORTED_TYPE hvis den møter på en ny mediatype. (_pCurrentSample er ny(ubrukt) sample, _pCurrentMediaType er ny mediatype)
//  Returner E_ABORT ved shutdown. 
//  Returnerer S_FALSE ved buffer underrun.
int ctr=0;
HRESULT CWASAPIRenderer::FeedRenderer()
{
    HRESULT hrReturn=S_OK;
	int hr=_AudioClient->Start();
	BYTE *pData;
	while (hrReturn==S_OK)
    {
		HRESULT hr;
		HANDLE waitArray[2] = {_ExitFeederLoopEvent, _AudioBufferReadyEvent};
        DWORD waitResult = WaitForMultipleObjects(2, waitArray, FALSE, INFINITE);
        switch (waitResult)
        {
			case WAIT_OBJECT_0 + 0:     // _ExitFeederLoopEvent
				hrReturn=E_ABORT;
				break;
			case WAIT_OBJECT_0 + 1:     // _AudioBufferReadyEvent
				//  We need to provide the next buffer of samples to the audio renderer.
				ctr++;
				BYTE *pSampleData;
				UINT32 numFramesPadding=0;
			
				if(_ShareMode==AUDCLNT_SHAREMODE_SHARED)
				{
					 hr = _AudioClient->GetCurrentPadding(&numFramesPadding);
				}
				UINT32 renderBufferSizeInBytes = ((_BufferSizePerPeriod - numFramesPadding) * _FrameSize);
				hr = _RenderClient->GetBuffer((_BufferSizePerPeriod - numFramesPadding), &pData);

				int bytesCopied=0;
				int bytesToCopy=renderBufferSizeInBytes;
				bool mediaTypeChanged=false;
				while(_pCurrentSample!=NULL && bytesCopied<bytesToCopy && !mediaTypeChanged)
				{		
					DWORD sampleSize=0;
					_pCurrentSample->GetBufferAndLength(&pSampleData,&sampleSize);

					long offsetEnd=_CurrentSampleOffset + bytesToCopy-bytesCopied;
					if(offsetEnd>sampleSize)
						offsetEnd=sampleSize;

					//Copy fra sample+offset til sample+offsetend to pData;
					CopyMemory(pData+bytesCopied, pSampleData+_CurrentSampleOffset, offsetEnd-_CurrentSampleOffset);
					//DebugPrintf(L"ctr:%d. Bytes to copy: %d, Copied %d bytes from current sample offset %d to buffer location %d.\n",ctr,bytesToCopy,offsetEnd-_CurrentSampleOffset,_CurrentSampleOffset,bytesCopied);
					bytesCopied+=offsetEnd-_CurrentSampleOffset;
					if(offsetEnd==sampleSize)
					{
						mediaTypeChanged=PopulateCurrentFromQueue();

						//if(_pCurrentSample)
						//	DebugPrintf(L"Pulled sample from queue.\n");
						//else
						//	DebugPrintf(L"Failed to pull sample from queue.\n");
					}
					else
					{
						_CurrentSampleOffset+=(offsetEnd-_CurrentSampleOffset);
					}
				}
				//TODO:? Hvis bytescopied<bytestocopy, zeromemory resten. 
				hr = _RenderClient->ReleaseBuffer(_BufferSizePerPeriod-numFramesPadding, 0);

				if(mediaTypeChanged)
					hrReturn=ERROR_UNSUPPORTED_TYPE;
				else if(bytesCopied<bytesToCopy)		//No change, but still not managed to fill buffer=underrun
					hrReturn=S_FALSE;
				break;
        }//Switch
    }//While
	//hr = _RenderClient->GetBuffer(_BufferSize, &pData);
	//hr = _RenderClient->ReleaseBuffer(_BufferSize, AUDCLNT_BUFFERFLAGS_SILENT);
	hr=_AudioClient->Stop();
    return hrReturn;
}

DWORD CWASAPIRenderer::DoRenderThread()
{
    bool stillPlaying = true;
    HANDLE waitArray[2] = {_ShutdownEvent, _ProcessSamplesInQueueEvent};
	bool isInited=false;
	while(stillPlaying)
	{
        HRESULT hr;
			//Vent på samples (eller shutdown)
		DWORD waitResult = WaitForMultipleObjects(2, waitArray, FALSE, INFINITE);
		switch (waitResult)
		{
			case WAIT_OBJECT_0 + 0:					// _ShutdownEvent
				stillPlaying = false;				// We're done, exit the loop.
				break;
			case WAIT_OBJECT_0 + 1:					// _ProcessSamplesInQueueEvent
				break;			
		}

		if(stillPlaying)
		{
			if(_pCurrentSample && _pCurrentMediaType)
			{
				DebugPrintf(L"CWASAPIRenderer::DoRenderThread has samples. Entering feederloop. IsInited: %d\n",isInited);
				if(!isInited)
				{
					isInited=InitializeAudioEngine();	//Uses currentMediaType and _ShareMode (set in PopulateCurrentFromQueue) to init AudioClient
					DebugPrintf(L"CWASAPIRenderer::DoRenderThread -  Inited audioengine. Result:%d \n",isInited);
				}
				if(!isInited)
				{
					DebugPrintf(L"CWASAPIRenderer::DoRenderThread -  Init audioengine failed. Releasing current sample. \n",isInited);
					if(_pCurrentSample)
						_pCurrentSample->Release();
					if(_pCurrentMediaType)
						_pCurrentMediaType->Release();
					_pCurrentSample=NULL;
					_pCurrentMediaType=NULL;
					_CurrentSampleOffset=0;
					CurrentSampleStart=NULL;
					CurrentSampleEnd=NULL;	
				}
				else
				{	//IsInited
					hr=FeedRenderer();
					switch(hr)
					{
						case ERROR_UNSUPPORTED_TYPE:	//ny mediatype. (_pCurrentSample er ny(ubrukt) sample, _pCurrentMediaType er ny mediatype)
							DebugPrintf(L"CWASAPIRenderer::FeedRenderer returned unsupported type - Releasing AudioEngine. \n");
							isInited=!ReleaseAudioEngine();
							break;
						case S_FALSE:
							DebugPrintf(L"CWASAPIRenderer::FeedRenderer returned S_FALSE (buffer underrun)\n");
							CurrentSampleStart=NULL;
	//if(_pCurrentSample || _pCurrentMediaType || _RenderBufferQueue)
							//	DebugPrintf(L"INSANITY!\n");
							break;
						case E_ABORT:
							DebugPrintf(L"CWASAPIRenderer::FeedRenderer returned E_ABORT (PauseState)-  Stops feederloop. \n");
							//isInited=!ReleaseAudioEngine();
							//stillPlaying=false;
							break;
						default:
							DebugPrintf(L"CWASAPIRenderer::FeedRenderer returned %d - Releasing AudioEngine.  \n",hr);
							isInited=!ReleaseAudioEngine();
							break;
					}
				}
			}
			else	//_pCurrentSample==null || _pCurrentMediaType==null
			{
				bool mediaTypeChanged=PopulateCurrentFromQueue();
				if(mediaTypeChanged)
					isInited=!ReleaseAudioEngine();
			}
		}
	}
	isInited=!ReleaseAudioEngine();
    return 0;
}




