#include <streams.h>
#include "StdAfx.h"
#include <assert.h>
#include <avrt.h>
#include "WASAPIRenderer.h"
#include "CResampler.h"
#include <wmcodecdsp.h>
#include <dmo.h>
#include "FormatUtils.h"
//#include "Mmreg.h"


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
	WORD formatTag=pSourceFormat->wFormatTag;
	if(pSourceFormat->wFormatTag==WAVE_FORMAT_EXTENSIBLE)
	{
		WAVEFORMATEXTENSIBLE *extensible=(WAVEFORMATEXTENSIBLE*)pSourceFormat;
		formatTag = CFormatUtils::SubFormatToFormatTag(extensible->SubFormat);
	}

	if( formatTag==WAVE_FORMAT_PCM || 
		formatTag==WAVE_FORMAT_ADPCM || 
		formatTag==WAVE_FORMAT_IEEE_FLOAT)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool SampleFormatEquals(WAVEFORMATEX* pformat1, WAVEFORMATEX* pformat2)
{
	if(pformat1 == pformat2)				//Both are NULL or both are pointer to same instance
		return true;
	if(pformat1==NULL || pformat2==NULL)	//One of them is null
		return false;

	return	
		pformat1->nChannels==pformat2->nChannels &&
		pformat1->nSamplesPerSec==pformat2->nSamplesPerSec &&
		pformat1->wFormatTag==pformat2->wFormatTag &&
		pformat1->wBitsPerSample==pformat2->wBitsPerSample;		   
}

bool IsMixingRequired(WAVEFORMATEX* pSourceFormat, WAVEFORMATEX* pDestFormat)
{
	if(pDestFormat==NULL)
		return false;

	return	
		pSourceFormat->nChannels!=pDestFormat->nChannels ||
		pSourceFormat->nSamplesPerSec!=pDestFormat->nSamplesPerSec ||
		pSourceFormat->wBitsPerSample!=pDestFormat->wBitsPerSample;		   
}

IMediaBufferEx* CResampler::CreateSample(IMediaSample* pSrcSample, WAVEFORMATEX* pSourceFormat, WAVEFORMATEX* pDestFormat)
{
	ASSERT(pSrcSample!=NULL);
	ASSERT(pSourceFormat!=NULL);
	CMediaBufferSampleWrapper* pSrcSampleWrapped=new CMediaBufferSampleWrapper(pSrcSample);

	if(!IsMixingRequired(pSourceFormat,pDestFormat))
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


DMO_MEDIA_TYPE* MediaTypeFromFormat(WAVEFORMATEX* pSrcFormat)
{
	DMO_MEDIA_TYPE* retValue = new DMO_MEDIA_TYPE();
	ZeroMemory(retValue, sizeof(DMO_MEDIA_TYPE));
	HRESULT hr = MoInitMediaType(retValue, sizeof(WAVEFORMATEX) + pSrcFormat->cbSize);				// Allocate memory for the format block.
    retValue->majortype  = MEDIATYPE_Audio;
    retValue->subtype    = MEDIASUBTYPE_PCM;
    retValue->formattype = FORMAT_WaveFormatEx;
	CopyMemory(retValue->pbFormat,pSrcFormat,sizeof(WAVEFORMATEX) + pSrcFormat->cbSize);
	return retValue;
}


//Initializes the resampler DMO based on m_pCurrentSourceFormat and m_pCurrentDestFormat
HRESULT CResampler::InitContext()
{
	ReleaseContext();  
	
	HRESULT hr = CoCreateInstance(CLSID_CResamplerMediaObject, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&m_pTransform);
	hr = m_pTransform->QueryInterface(IID_PPV_ARGS(&m_pDMO));

	IWMResamplerProps* pResamplerProps=NULL;
	hr= m_pTransform->QueryInterface(IID_PPV_ARGS(&pResamplerProps));

	if(hr==S_OK)
	{
		pResamplerProps->SetHalfFilterLength(60);
		pResamplerProps->Release();
	}

	//MFPKEY_WMRESAMP_CHANNELMTX		= SetUserChannelMtx
	//MFPKEY_WMRESAMP_LOWPASS_BANDWIDTH

	DMO_MEDIA_TYPE* mtSource = MediaTypeFromFormat(m_pCurrentSourceFormat);
    hr = m_pDMO->SetInputType(0, mtSource, 0);

	DMO_MEDIA_TYPE* mtDest = MediaTypeFromFormat(m_pCurrentDestFormat);
	hr = m_pDMO->SetOutputType(0, mtDest, 0);

	hr = MoFreeMediaType(mtSource);
	hr = MoFreeMediaType(mtDest);

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





