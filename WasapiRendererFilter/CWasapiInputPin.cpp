#include <streams.h>

#include "WasapiRendererFilteruids.h"
#include "WasapiRendererFilter.h"
#include "CWasapiInputPin.h"
#include "CWasapiFilter.h"
#include "WASAPIRenderer.h"
#include "WasapiHelper.h"
#include "stdafx.h"
#include "CWasapiFilterManager.h"

//
//  Definition of CWasapiInputPin
//
CWasapiInputPin::CWasapiInputPin(CWasapiFilterManager *pDump,
                             LPUNKNOWN pUnk,
                             CBaseFilter *pFilter,
                             CCritSec *pLock,
                             CCritSec *pReceiveLock,
                             HRESULT *phr) :

    CRenderedInputPin(NAME("CWasapiInputPin"),
                  pFilter,                   // Filter
                  pLock,                     // Locking
                  phr,                       // Return code
                  L"Input"),                 // Pin name
    m_pReceiveLock(pReceiveLock),
    m_pManager(pDump),
    m_tLast(0),
	m_tSegmentStart(0)
{
}


//
// CheckMediaType
//
// Check if the pin can support this specific proposed type and format
//
HRESULT CWasapiInputPin::CheckMediaType(const CMediaType* mt)
{
	HRESULT hr=S_FALSE;
	if(mt->formattype==FORMAT_WaveFormatEx)
	{
		bool isOK=m_pManager->CheckFormat((WAVEFORMATEX*)mt->pbFormat);
		if(isOK)
			hr=S_OK;
		DebugPrintf(L"CWasapiInputPin::CheckMediaType - WaveFormatEx: ISOK: %d\n",isOK);
	}
	else if(mt->formattype==FORMAT_None)
	{
		hr=S_FALSE;
		DebugPrintf(L"CWasapiInputPin::CheckMediaType - FORMAT_None. OK.\n");
	}
	else
	{
		DebugPrintf(L"CWasapiInputPin::CheckMediaType - Unkown format. Not OK.\n");
	}
    return hr;
}

HRESULT CWasapiInputPin::SetMediaType(const CMediaType *pmt)
{
	HRESULT hr = CBasePin::SetMediaType(pmt);
	DebugPrintf(L"CWasapiInputPin::SetMediaType\n");
	m_pManager->SetFormatReceived((CMediaType*)pmt);
    return hr;
}

HRESULT CWasapiInputPin::NotifyAllocator(
                    IMemAllocator * pAllocator,
                    BOOL bReadOnly)
{
	return CBaseInputPin::NotifyAllocator(pAllocator,bReadOnly);
}

//
// BreakConnect
//
// Break a connection
//
HRESULT CWasapiInputPin::BreakConnect()
{
    //if (m_pManager->m_pPosition != NULL) {
    //    m_pManager->m_pPosition->ForceRefresh();
    //}

	DebugPrintf(L"CWasapiInputPin::BreakConnect \n");
	m_pManager->StopRendering(true,true);

    return CRenderedInputPin::BreakConnect();
}


HRESULT CWasapiInputPin::CompleteConnect(IPin *pReceivePin)
{
	return CRenderedInputPin::CompleteConnect(pReceivePin);
}

//
// ReceiveCanBlock
//
// We don't hold up source threads on Receive
//
STDMETHODIMP CWasapiInputPin::ReceiveCanBlock()
{
    return S_FALSE;
}

//
// Receive
//
// Do something with this media sample
//
STDMETHODIMP CWasapiInputPin::Receive(IMediaSample *pSample)
{
//	DebugPrintf(L"Receive\n");

	CAutoLock lock(m_pReceiveLock);
    CheckPointer(pSample,E_POINTER);
	bool mediaTypeFailed=false;

    HRESULT hr = CBaseInputPin::Receive(pSample);
	if(hr!=S_OK)
	{
		DebugPrintf(L"CWasapiInputPin::Receive CBaseInputPin returned error. %d \n",hr);
		goto exit;
	}

	CMediaType* mediaType=NULL;
	hr=pSample->GetMediaType((AM_MEDIA_TYPE**)&mediaType);
	if(hr==S_OK)
	{
		hr = CBasePin::SetMediaType(mediaType);
		DeleteMediaType(mediaType);
	}

	hr= m_pManager->SampleReceived(pSample);
	if(hr==S_FALSE)		//Media type rejected by Wasapi engine
	{
		DebugPrintf(L"CWasapiInputPin::Receive returning EC_ERRORABORT. \n");
		CBasePin::EndOfStream();
		m_pFilter->NotifyEvent(EC_ERRORABORT,NULL,NULL);
		goto exit;
	}
exit:
	return hr;
 }

HRESULT CWasapiInputPin::Active()
{
	DebugPrintf(L"CWasapiInputPin::Active \n");
	return CRenderedInputPin::Active();
}
HRESULT CWasapiInputPin::Inactive()
{
	DebugPrintf(L"CWasapiInputPin::Inactive \n");
	return CBaseInputPin::Inactive();
}

STDMETHODIMP CWasapiInputPin::BeginFlush(void)
{
	DebugPrintf(L"CWasapiInputPin::BeginFlush \n");
	HRESULT hr=CBaseInputPin::BeginFlush();
	m_pManager->ClearQueue();
	return hr;
}

// Todo: Also, if the filter processes Receive calls asynchronously, the pin should wait to send the EC_COMPLETE event until the filter has processed all pending samples.
// http://msdn.microsoft.com/en-us/library/dd375164%28VS.85%29.aspx, http://msdn.microsoft.com/en-us/library/dd388900%28VS.85%29.aspx
STDMETHODIMP CWasapiInputPin::EndOfStream(void)
{
    CAutoLock lock(m_pReceiveLock);
	DebugPrintf(L"CWasapiInputPin::EndOfStream \n");
    return CRenderedInputPin::EndOfStream();

} // EndOfStream


//
// NewSegment
//
// Called when we are seeked
//
STDMETHODIMP CWasapiInputPin::NewSegment(REFERENCE_TIME tStart,
                                       REFERENCE_TIME tStop,
                                       double dRate)
{
	DebugPrintf(L"CWasapiInputPin::NewSegment \n");
	m_tSegmentStart=tStart;
    m_tLast = 0;
    return S_OK;

} // NewSegment

