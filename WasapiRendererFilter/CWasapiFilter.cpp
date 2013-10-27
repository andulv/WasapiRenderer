#include <streams.h>

#include "WasapiRendererFilteruids.h"
#include "WasapiRendererFilter.h"
#include "CWasapiFilter.h"
#include "CWasapiInputPin.h"
#include "WASAPIRenderer.h"
#include "WasapiHelper.h"
#include "stdafx.h"
#include "CWasapiFilterManager.h"
#include "PropertyPages.h"

// Setup data

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_NULL,            // Major type
    &MEDIASUBTYPE_NULL          // Minor type
};

const AMOVIESETUP_PIN sudPins =
{
    L"Input",                   // Pin string name
    FALSE,                      // Is it rendered
    FALSE,                      // Is it an output
    FALSE,                      // Allowed none
    FALSE,                      // Likewise many
    &CLSID_NULL,                // Connects to filter
    L"Output",                  // Connects to pin
    1,                          // Number of types
    &sudPinTypes                // Pin information
};

const AMOVIESETUP_FILTER sudDump =
{
    &CLSID_WasapiRendererFilter,                // Filter CLSID
    L"WasapiRendererFilter",                    // String name
    MERIT_DO_NOT_USE,           // Filter merit
    1,                          // Number pins
    &sudPins                    // Pin details
};


//
//  Object creation stuff
//
CFactoryTemplate g_Templates[]= 
{
	{ L"WasapiRendererFilter", &CLSID_WasapiRendererFilter, CWasapiFilterManager::CreateInstance, NULL, &sudDump },
	{ L"Saturation Props", &CLSID_WasapiProp, CWasapiFilterProperties::CreateInstance, NULL, NULL }
};
int g_cTemplates =sizeof(g_Templates)/sizeof(g_Templates[0]);


// Constructor

CWasapiFilter::CWasapiFilter(CWasapiFilterManager *pDump,
                         LPUNKNOWN pUnk,
                         CCritSec *pLock,
                         HRESULT *phr) :
    CBaseFilter(NAME("CWasapiFilter"), pUnk, pLock, CLSID_WasapiRendererFilter),
    m_pManager(pDump)
{
	HRESULT hr=S_OK;
}







//
// GetPin
//
CBasePin * CWasapiFilter::GetPin(int n)
{
    if (n == 0) {
        return m_pManager->m_pPin;
    } else {
        return NULL;
    }
}


//
// GetPinCount
//
int CWasapiFilter::GetPinCount()
{
    return 1;
}


//
// Stop
//
// Overriden to close the dump file
//
	bool bStarted=false;
STDMETHODIMP CWasapiFilter::Stop()
{
	bStarted=false;
 	DebugPrintf(L"CWasapiFilter::Stop \n");
    CAutoLock cObjectLock(m_pLock);
	m_pManager->StopRendering(true,false);  
	HRESULT hr=CBaseFilter::Stop();
    return hr;
}

FILTER_STATE CWasapiFilter::GetState()
{
	return m_State;
}

REFERENCE_TIME CWasapiFilter::GetStartTime()
{
	LONGLONG units=m_tStart.GetUnits();
	return m_tStart.m_time;
}

STDMETHODIMP CWasapiFilter::Pause()
{
 	DebugPrintf(L"CWasapiFilter::Pause \n");
    CAutoLock cObjectLock(m_pLock);
	HRESULT hr=CBaseFilter::Pause();
	m_pManager->PauseRendering();
 	DebugPrintf(L"CWasapiFilter::Pause returning\n");
	bStarted=false;
    return hr;
}


// Run
//
    // the start parameter is the difference to be added to the
    // sample's stream time to get the reference time for
    // its presentation
STDMETHODIMP CWasapiFilter::Run(REFERENCE_TIME tStart)
{
	bStarted=true;
 	DebugPrintf(L"CWasapiFilter::Run \n");
    CAutoLock cObjectLock(m_pLock);
	HRESULT hr=CBaseFilter::Run(tStart);
	m_pManager->StartRendering();
    return hr;
}
