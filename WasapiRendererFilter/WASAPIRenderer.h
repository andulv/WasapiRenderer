// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
#pragma once
#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <AudioPolicy.h>

class RefCountingWaveFormatEx
{
public:
	RefCountingWaveFormatEx():
		m_refCount(0),
		m_pFormat(NULL)
	{
		AddRef();
	}

	LONG AddRef()
	{
		return InterlockedIncrement(&m_refCount);
		m_refCount++;
	}

	LONG Release()
	{
		int newValue=-1;
		newValue= InterlockedDecrementAcquire(&m_refCount);
		if(newValue==0)
		{
			delete m_pFormat;
			delete this;
		}
		return newValue;
	}


	BOOL operator == (const RefCountingWaveFormatEx& rt) const
	{
		// I don't believe we need to check sample size or
		// temporal compression flags, since I think these must
		// be represented in the type, subtype and format somehow. They
		// are pulled out as separate flags so that people who don't understand
		// the particular format representation can still see them, but
		// they should duplicate information in the format block.

		if(m_pFormat==NULL && rt.m_pFormat==NULL)
			return true;

		if(m_pFormat==NULL || rt.m_pFormat==NULL)
			return false;

		return 
			m_pFormat->cbSize==rt.m_pFormat->cbSize &&
			m_pFormat->nAvgBytesPerSec==rt.m_pFormat->nAvgBytesPerSec &&
			m_pFormat->nBlockAlign==rt.m_pFormat->nBlockAlign &&
			m_pFormat->nChannels==rt.m_pFormat->nChannels &&
			m_pFormat->nSamplesPerSec==rt.m_pFormat->nSamplesPerSec &&
			m_pFormat->wBitsPerSample==rt.m_pFormat->wBitsPerSample &&
			m_pFormat->wFormatTag==rt.m_pFormat->wFormatTag;
	}

	BOOL operator != (const RefCountingWaveFormatEx& rt) const
	{
		/* Check to see if they are equal */

		if (*this == rt) {
			return FALSE;
		}
		return TRUE;
	}

	WAVEFORMATEX* GetFormat()
	{
		return m_pFormat;
	}

	static RefCountingWaveFormatEx* CopyAndCreate(WAVEFORMATEX* pSource)
	{
		int memSize=sizeof(WAVEFORMATEX) + pSource->cbSize;
		byte* pDest = new byte[memSize];
		CopyMemory(pDest,pSource,memSize);

		RefCountingWaveFormatEx* retValue=new RefCountingWaveFormatEx();
		retValue->m_pFormat=(WAVEFORMATEX*)pDest;
		return retValue;
	}

protected:
	volatile LONG m_refCount;
	WAVEFORMATEX* m_pFormat;
};

class SimpleSample
{
public:
	SimpleSample():
		m_refCount(0),
		m_pSampleData(NULL),
		m_dataLength(0)
	{
		AddRef();
	}

	LONG AddRef()
	{
		return InterlockedIncrement(&m_refCount);
		m_refCount++;
	}

	LONG Release()
	{
		int newValue=-1;
		newValue= InterlockedDecrementAcquire(&m_refCount);
		if(newValue==0)
		{
			m_pSample->Release();
			delete m_pSampleData;
			delete this;
		}
		return newValue;
	}


	static SimpleSample* CopyAndCreate(IMediaSample* pSource)
	{
		SimpleSample* retValue=new SimpleSample();
		retValue->m_dataLength = pSource->GetActualDataLength();
		pSource->GetTime(&(retValue->TimeStart),&(retValue->TimeEnd));

		byte* pSourceBuffer;
		pSource->GetPointer(&pSourceBuffer);

		byte* pDestBuffer = new byte[retValue->m_dataLength];
		CopyMemory(pDestBuffer, pSourceBuffer, retValue->m_dataLength);
		retValue->m_pSampleData=pDestBuffer;
		retValue->m_pSample=pSource;
		pSource->AddRef();
		return retValue;
	}

		static SimpleSample* Create(IMediaSample* pSource, byte* buffer,  long destSize)
	{
		SimpleSample* retValue=new SimpleSample();
		pSource->GetTime(&(retValue->TimeStart),&(retValue->TimeEnd));

		retValue->m_dataLength = destSize;
		retValue->m_pSampleData=buffer;
		retValue->m_pSample=pSource;
		pSource->AddRef();
		return retValue;
	}

	static SimpleSample* AllocateAndCreate(IMediaSample* pSource,  long destSize)
	{
		SimpleSample* retValue=new SimpleSample();
		pSource->GetTime(&(retValue->TimeStart),&(retValue->TimeEnd));

		byte* pDestBuffer = new byte[destSize];
		ZeroMemory(pDestBuffer,destSize);

		retValue->m_dataLength = destSize;
		retValue->m_pSampleData=pDestBuffer;
		retValue->m_pSample=pSource;
		pSource->AddRef();
		return retValue;
	}


	void SetActualDataLength(long value)
	{
		m_dataLength=value;
	}
	long GetActualDataLength()
	{
		return m_dataLength;
	}

	byte* GetPointer()
	{
		return m_pSampleData;
	}

public:
		REFERENCE_TIME TimeStart, TimeEnd;

protected:
	volatile LONG m_refCount;
	byte* m_pSampleData;
	long m_dataLength;

	IMediaSample* m_pSample;

};

struct RenderBuffer
{
    RenderBuffer *  _Next;
	SimpleSample *pSample;
	RefCountingWaveFormatEx *pMediaType;
	bool ExclusiveMode;

    RenderBuffer() :
        _Next(NULL),
		pSample(NULL),
		pMediaType(NULL),
		ExclusiveMode(false)
    {
    }

    ~RenderBuffer()
    {
    }
};

class CWASAPIRenderer
{
public:
	CWASAPIRenderer(LPCWSTR pDevID);
    ~CWASAPIRenderer(void);

	WAVEFORMATEX* GetWasapiMixFormat();
	bool CheckFormat(WAVEFORMATEX* requestedFormat, WAVEFORMATEX** suggestedFormat, AUDCLNT_SHAREMODE shareMode);
    bool Start(UINT32 EngineLatency);
    void Stop();
	void SetIsProcessing(bool isOK);
	void AddSampleToQueue(SimpleSample *pSample, RefCountingWaveFormatEx *pMediaType, bool isExclusive);
	void ClearQueue();
	REFERENCE_TIME		GetCurrentSampleTime();
	int					InitializedMode;

private:
    //
    //  Core Audio Rendering member variables.
    //
    IMMDevice			*_Endpoint;
    IAudioClient		*_AudioClient;
    IAudioRenderClient	*_RenderClient;

	//Sets the event if we have samples (_RenderBufferQueue!=null && m_bIsProcessing)
	void				UpdateProcessSamplesInQueueEvent();

    HANDLE				_RenderThread;
    HANDLE				_ShutdownEvent;
	HANDLE				_ExitFeederLoopEvent;
    HANDLE				_AudioBufferReadyEvent;
	HANDLE				_ProcessSamplesInQueueEvent;

    UINT32				_FrameSize;
    UINT32				_BufferSize;
    UINT32				_BufferSizePerPeriod;

    LONG				_EngineLatencyInMS;

    //  Render buffer management. - Accessed from multiple threads
    RenderBuffer		*_RenderBufferQueue;
    CCritSec			m_QueueLock;
	bool				m_bIsProcessing;

    //  Render buffer management. - Accessed only from renderer thread
	SimpleSample			*_pCurrentSample;
	RefCountingWaveFormatEx	*_pCurrentMediaType;
	AUDCLNT_SHAREMODE		_ShareMode;
	bool					_hasTriedExclusiveModeWithCurrentMediaType;
	LONG					_CurrentSampleOffset;
	bool					PopulateCurrentFromQueue();

	REFERENCE_TIME CurrentSampleStart;
	REFERENCE_TIME CurrentSampleEnd;

	//  Audio client management - Accessed only from renderer thread
	bool InitializeAudioEngine();
	bool InitializeAudioClient();
	bool ReleaseAudioEngine();
	HRESULT FeedRenderer();

    static DWORD __stdcall WASAPIRenderThread(LPVOID Context);
    DWORD CWASAPIRenderer::DoRenderThread();
};
