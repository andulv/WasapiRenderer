#include <streams.h>
#include "StdAfx.h"
#include <assert.h>
#include <avrt.h>
#include "WASAPIRenderer.h"
#include "CResampler.h"

//#include <streams.h>
//#include "WasapiRendererFilteruids.h"
//#include "CWasapiInputPin.h"
//#include "WasapiRendererFilter.h"
//#include "WASAPIRenderer.h"
//#include "WasapiHelper.h"
//#include "stdafx.h"
//#include "CWasapiFilterManager.h"

CResampler::CResampler()
	: m_avrContext(NULL),
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

int ChannelLayoutFromWaveFormat(WAVEFORMATEX* pFormat)
{
	if(pFormat->nChannels==1)
		return AV_CH_LAYOUT_MONO;

	return AV_CH_LAYOUT_STEREO;
}

AVSampleFormat SampleFormatFromWaveFormat(WAVEFORMATEX* pFormat)
{
	bool isFloat=false;
	bool isUnknown=true;
	if(pFormat->wFormatTag==WAVE_FORMAT_IEEE_FLOAT) {
		isFloat=true;
		isUnknown=false;
	}
	else if(pFormat->wFormatTag==WAVE_FORMAT_PCM) {
		isFloat=false;
		isUnknown=false;
	}
	else if(pFormat->wFormatTag==WAVE_FORMAT_EXTENSIBLE) {
		WAVEFORMATEXTENSIBLE* pExtens=(WAVEFORMATEXTENSIBLE*)pFormat;
		if(pExtens->SubFormat==KSDATAFORMAT_SUBTYPE_PCM){
			isFloat=false;
			isUnknown=false;
		}
		else if(pExtens->SubFormat==KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
			isFloat=true;
			isUnknown=false;
		}
	}

	if(isUnknown)
		return AV_SAMPLE_FMT_NONE;


	switch (pFormat->wBitsPerSample)
	{
		case 8:
			return isFloat ? AV_SAMPLE_FMT_NONE : AV_SAMPLE_FMT_U8;;			//Dont support 8 bit float...
		case 16:
			return isFloat ? AV_SAMPLE_FMT_FLT : AV_SAMPLE_FMT_S16 ;
		case 24:
			return isFloat ? AV_SAMPLE_FMT_NONE : AV_SAMPLE_FMT_S32 ;			//libswresample does not support 24 bit. We must convert to 32... (and 24 bit can only be integers)
		case 32:
			return isFloat ? AV_SAMPLE_FMT_DBL : AV_SAMPLE_FMT_S32 ;
		default:
			return AV_SAMPLE_FMT_NONE;
			break;
	}
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

SimpleSample* CResampler::CreateSample(IMediaSample* pSrcSample, WAVEFORMATEX* pSourceFormat, WAVEFORMATEX* pDestFormat)
{
	//if(SampleFormatEquals(pSourceFormat,pDestFormat))
	//{
	//	return SimpleSample::CopyAndCreate(pSrcSample);			//No conversion needed. Received sample can be played back directly.
	//}

	if(m_avrContext == NULL )//|| !SampleFormatEquals(pSourceFormat, m_pCurrentSourceFormat) || ! SampleFormatEquals(pDestFormat, m_pCurrentDestFormat))
	{
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

		m_pCurrentDestFormat->nSamplesPerSec=m_pCurrentDestFormat->nSamplesPerSec / 2;

		InitContext();
	}

	byte *pSourceBuffer[1];
	pSrcSample->GetPointer(&pSourceBuffer[0]);

	int bytesPerSampleDest=(m_pCurrentDestFormat->wBitsPerSample) / 8; 
	int bytesPerSampleSource=(m_pCurrentSourceFormat->wBitsPerSample) / 8;
	int sourceSize = pSrcSample->GetActualDataLength();
	int nSourceSamples = sourceSize / (m_pCurrentSourceFormat->nChannels * bytesPerSampleSource);
	double dDestSamples = (double)nSourceSamples * ((double)m_pCurrentDestFormat->nSamplesPerSec / (double)m_pCurrentSourceFormat->nSamplesPerSec);
	int nDestSamples = (int)dDestSamples + 1;

	long ffAligned=FFALIGN(nDestSamples, 32);
	long destSize = nDestSamples * m_pCurrentDestFormat->nChannels * bytesPerSampleDest;
	auto retSample = SimpleSample::AllocateAndCreate(pSrcSample,destSize);		

	byte *retBuffer[1];
	retBuffer[0]=retSample->GetPointer();

	int nResampledSamples = avresample_convert(m_avrContext, retBuffer, destSize, nDestSamples, pSourceBuffer, sourceSize, nSourceSamples);
	int nResampledBytes = nResampledSamples * m_pCurrentDestFormat->nChannels * bytesPerSampleDest;

	retSample->SetActualDataLength(nResampledBytes);
	int nRemainingSamplesInDelayBuffer=avresample_get_delay(m_avrContext);
	int nRemainingSamplesInFifoBuffer=avresample_available(m_avrContext);

	return retSample;
}

HRESULT CResampler::InitContext()
{
	ReleaseContext();
	m_avrContext = avresample_alloc_context();

	int sourceChannelLayout=ChannelLayoutFromWaveFormat(m_pCurrentSourceFormat);
	int destChannelLayout=ChannelLayoutFromWaveFormat(m_pCurrentDestFormat);

	AVSampleFormat sourceSampleFormat=SampleFormatFromWaveFormat(m_pCurrentSourceFormat);
	AVSampleFormat destSampleFormat=SampleFormatFromWaveFormat(m_pCurrentDestFormat);
	
	av_opt_set_int(m_avrContext, "in_channel_layout",  sourceChannelLayout, 0);
	av_opt_set_int(m_avrContext, "out_channel_layout", destChannelLayout, 0);
	av_opt_set_int(m_avrContext, "in_sample_rate", m_pCurrentSourceFormat->nSamplesPerSec, 0);
	av_opt_set_int(m_avrContext, "out_sample_rate",  m_pCurrentDestFormat->nSamplesPerSec, 0);
	av_opt_set_int(m_avrContext, "in_sample_fmt", sourceSampleFormat, 0);
	av_opt_set_int(m_avrContext, "out_sample_fmt", destSampleFormat, 0);
	int result = avresample_open(m_avrContext);
	return S_OK;
}

void CResampler::ReleaseContext()
{
	if(m_avrContext)
	{
		avresample_free(&m_avrContext);
		m_avrContext=NULL;
	}
}