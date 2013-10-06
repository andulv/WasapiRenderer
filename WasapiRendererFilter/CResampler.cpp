#include "CResampler.h"


//#include <streams.h>
//#include "WasapiRendererFilteruids.h"

//#include "CWasapiInputPin.h"
//#include "WasapiRendererFilter.h"
//#include "WASAPIRenderer.h"
//#include "WasapiHelper.h"
//#include "stdafx.h"
//#include "CWasapiFilterManager.h"

int CResampler::GetDeviceID()
{
	m_avrContext = avresample_alloc_context();
	return 1;
}