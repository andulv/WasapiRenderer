#include <streams.h>
#include "StdAfx.h"
#include <assert.h>
#include <avrt.h>
#include "WASAPIRenderer.h"
#include "CResampler.h"
#include <wmcodecdsp.h>
#include <dmo.h>

//#include <streams.h>
//#include "WasapiRendererFilteruids.h"
//#include "CWasapiInputPin.h"
//#include "WasapiRendererFilter.h"
//#include "WASAPIRenderer.h"
//#include "WasapiHelper.h"
//#include "stdafx.h"
//#include "CWasapiFilterManager.h"

CResampler::CResampler()
	: m_pTransform(NULL),
	m_pDMO(NULL),
	m_pCurrentDestFormat(NULL),
	m_pCurrentSourceFormat(NULL)
{
}


CResampler::~CResampler()
{
	ReleaseContext();
	delete m_pCurrentDestFormat;
	delete m_pCurrentSourceFormat;
}


bool CResampler::CanResample(WAVEFORMATEX* pSourceFormat, WAVEFORMATEX* pDestFormat)
{
	return 1;
}

bool SampleFormatEquals(WAVEFORMATEX* pformat1, WAVEFORMATEX* pformat2)
{
	if(pformat1==NULL && pformat2==NULL)
		return true;
	if(pformat1==NULL && pformat2!=NULL)
		return false;
	if(pformat1!=NULL && pformat2==NULL)
		return false;

	return	
		pformat1->nChannels==pformat2->nChannels &&
		pformat1->nSamplesPerSec==pformat2->nSamplesPerSec &&
		pformat1->wFormatTag==pformat2->wFormatTag &&
		pformat1->wBitsPerSample==pformat2->wBitsPerSample;		   
}

IMediaBufferEx* CResampler::CreateSample(IMediaSample* pSrcSample, WAVEFORMATEX* pSourceFormat, WAVEFORMATEX* pDestFormat)
{
	CMediaBufferSampleWrapper* pSrcSampleWrapped=new CMediaBufferSampleWrapper(pSrcSample);

	if(pDestFormat== NULL || SampleFormatEquals(pSourceFormat,pDestFormat))
	{
		return pSrcSampleWrapped;			//No conversion needed. Received sample can be played back directly.
	}

	if(m_pTransform == NULL || !SampleFormatEquals(pSourceFormat, m_pCurrentSourceFormat) || ! SampleFormatEquals(pDestFormat, m_pCurrentDestFormat))
	{
		//Set source and destination formats and init resampler context

		if(m_pCurrentSourceFormat) {
			delete m_pCurrentSourceFormat;
		}
		int memSize=sizeof(WAVEFORMATEX) + pSourceFormat->cbSize;
		byte* pBuffer = new byte[memSize];
		CopyMemory(pBuffer,pSourceFormat,memSize);
		m_pCurrentSourceFormat = (WAVEFORMATEX*)pBuffer; 

		if(m_pCurrentDestFormat) {
			delete m_pCurrentDestFormat;
		}
		memSize=sizeof(WAVEFORMATEX) + pDestFormat->cbSize;
		pBuffer = new byte[memSize];
		CopyMemory(pBuffer,pDestFormat,memSize);
		m_pCurrentDestFormat = (WAVEFORMATEX*)pBuffer; 

		InitContext();
	}

	//Calculate size needed for resampling destination buffer
	int bytesPerSampleDest=(m_pCurrentDestFormat->wBitsPerSample) / 8; 
	int bytesPerSampleSource=(m_pCurrentSourceFormat->wBitsPerSample) / 8;
	int sourceSize = pSrcSample->GetActualDataLength();
	int nSourceSamples = sourceSize / (m_pCurrentSourceFormat->nChannels * bytesPerSampleSource);
	double dDestSamples = (double)nSourceSamples * ((double)m_pCurrentDestFormat->nSamplesPerSec / (double)m_pCurrentSourceFormat->nSamplesPerSec);
	int nDestSamples = (int)dDestSamples + 1;			//Ensure that value is not rounded down
	long destSize= nDestSamples * bytesPerSampleDest * m_pCurrentDestFormat->nChannels;

	REFERENCE_TIME timeStart, timeEnd;
	pSrcSample->GetTime(&timeStart,&timeEnd);
	HRESULT hr = m_pDMO->ProcessInput(0,pSrcSampleWrapped,NULL,timeStart,timeEnd);

	DWORD ouProcessStatus;
	IMediaBufferEx *pDestMediaBuffer=NULL;
	hr = CMediaBufferSampleWrapperWithConvertedData::Create(pSrcSample, destSize, &pDestMediaBuffer);
	DMO_OUTPUT_DATA_BUFFER* outputBuffer=new DMO_OUTPUT_DATA_BUFFER();
	outputBuffer->pBuffer=pDestMediaBuffer;
	hr = m_pDMO->ProcessOutput(NULL,1,outputBuffer,&ouProcessStatus);
	pSrcSampleWrapped->Release();
	delete outputBuffer;
	return pDestMediaBuffer;
}


//Initializes the resampler DMO based on m_pCurrentSourceFormat and m_pCurrentDestFormat
HRESULT CResampler::InitContext()
{
	ReleaseContext();
    
	
	HRESULT hr = CoCreateInstance(CLSID_CResamplerMediaObject, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&m_pTransform);
	hr = m_pTransform->QueryInterface(IID_PPV_ARGS(&m_pDMO));

	DMO_MEDIA_TYPE mtSource;
	ZeroMemory(&mtSource, sizeof(DMO_MEDIA_TYPE));
	hr = MoInitMediaType(&mtSource, sizeof(WAVEFORMATEX) + m_pCurrentSourceFormat->cbSize);				// Allocate memory for the format block.
    mtSource.majortype  = MEDIATYPE_Audio;
    mtSource.subtype    = MEDIASUBTYPE_PCM;
    mtSource.formattype = FORMAT_WaveFormatEx;
	CopyMemory(mtSource.pbFormat,m_pCurrentSourceFormat,sizeof(WAVEFORMATEX) + m_pCurrentSourceFormat->cbSize);
    hr = m_pDMO->SetInputType(0, &mtSource, 0);
	hr = MoFreeMediaType(&mtSource);

	DMO_MEDIA_TYPE mtDest;
	ZeroMemory(&mtDest, sizeof(DMO_MEDIA_TYPE));
	hr = MoInitMediaType(&mtDest, sizeof(WAVEFORMATEX) + m_pCurrentDestFormat->cbSize);				// Allocate memory for the format block.
    mtDest.majortype  = MEDIATYPE_Audio;
    mtDest.subtype    = MEDIASUBTYPE_PCM;
    mtDest.formattype = FORMAT_WaveFormatEx;
	CopyMemory(mtDest.pbFormat,m_pCurrentDestFormat,sizeof(WAVEFORMATEX) + m_pCurrentDestFormat->cbSize);
	hr = m_pDMO->SetOutputType(0, &mtDest, 0);
	hr = MoFreeMediaType(&mtDest);

	return S_OK;
}

void CResampler::ReleaseContext()
{
	if(m_pTransform)
	{
		SafeRelease(&m_pTransform);
		m_pTransform=NULL;
	}

	if(m_pDMO)
	{
		SafeRelease(&m_pDMO);
		m_pDMO=NULL;
	}
}