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

	if(pFormat->nChannels==4)
		return AV_CH_LAYOUT_4POINT0;

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
			return isFloat ? AV_SAMPLE_FMT_FLT : AV_SAMPLE_FMT_S32 ;
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

		m_pCurrentDestFormat->nSamplesPerSec=m_pCurrentDestFormat->nSamplesPerSec;

		InitContext();
	}

	byte *pSourceBuffer;
	pSrcSample->GetPointer(&pSourceBuffer);
	const byte **ppSourceBuffer=(const byte **)&pSourceBuffer;


	int bytesPerSampleDest=(m_pCurrentDestFormat->wBitsPerSample) / 8; 
	int bytesPerSampleSource=(m_pCurrentSourceFormat->wBitsPerSample) / 8;
	int sourceSize = pSrcSample->GetActualDataLength();
	int nSourceSamples = sourceSize / (m_pCurrentSourceFormat->nChannels * bytesPerSampleSource);
	int nDestSamples = av_rescale_rnd(swr_get_delay(m_avrContext, m_pCurrentSourceFormat->nSamplesPerSec) + nSourceSamples, m_pCurrentDestFormat->nSamplesPerSec, m_pCurrentSourceFormat->nSamplesPerSec, AV_ROUND_UP);
	//long ffAligned=FFALIGN(nDestSamples, 32);
	long destSize = nDestSamples * m_pCurrentDestFormat->nChannels * bytesPerSampleDest;

	//uint8_t *output;
	int linesize;
	int bufferSize = av_samples_get_buffer_size(&linesize ,m_pCurrentDestFormat->nChannels, nDestSamples, SampleFormatFromWaveFormat(m_pCurrentDestFormat), 0);
	
	auto retSample = SimpleSample::AllocateAndCreate(pSrcSample,bufferSize);
	byte *pRetBuffer = retSample->GetPointer();


	int nResampledSamples = swr_convert(m_avrContext, &pRetBuffer, nDestSamples, ppSourceBuffer, nSourceSamples);
	int nResampledBytes = nResampledSamples * m_pCurrentDestFormat->nChannels * bytesPerSampleDest;

	//retSample->SetActualDataLength(nResampledBytes);
	//int nRemainingSamplesInDelayBuffer=avresample_get_delay(m_avrContext);
	//int nRemainingSamplesInFifoBuffer=avresample_available(m_avrContext);
	//return SimpleSample::Create(pSrcSample,pRetBuffer, linesize);	
	return retSample;
}

HRESULT CResampler::InitContext()
{
	ReleaseContext();
	m_avrContext = swr_alloc();

	int sourceChannelLayout=ChannelLayoutFromWaveFormat(m_pCurrentSourceFormat);
	int destChannelLayout=ChannelLayoutFromWaveFormat(m_pCurrentDestFormat);

	AVSampleFormat sourceSampleFormat=SampleFormatFromWaveFormat(m_pCurrentSourceFormat);
	AVSampleFormat destSampleFormat=SampleFormatFromWaveFormat(m_pCurrentDestFormat);
	

	av_opt_set_int(m_avrContext, "in_channel_layout",  sourceChannelLayout, 0);
	av_opt_set(m_avrContext,"resampler","soxr",0);
	av_opt_set_int(m_avrContext, "in_channel_layout",  sourceChannelLayout, 0);
	av_opt_set_int(m_avrContext, "out_channel_layout", destChannelLayout, 0);
	av_opt_set_int(m_avrContext, "in_sample_rate", m_pCurrentSourceFormat->nSamplesPerSec, 0);
	av_opt_set_int(m_avrContext, "out_sample_rate",  m_pCurrentDestFormat->nSamplesPerSec, 0);
	av_opt_set_sample_fmt(m_avrContext, "in_sample_fmt", sourceSampleFormat, 0);
	av_opt_set_sample_fmt(m_avrContext, "out_sample_fmt", destSampleFormat, 0);
	int result = swr_init(m_avrContext);
	return S_OK;
}

void CResampler::ReleaseContext()
{
	if(m_avrContext)
	{
		swr_free(&m_avrContext);
	}
}