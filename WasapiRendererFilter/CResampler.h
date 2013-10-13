#pragma once
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN


class ISampleProcessor
{
public:
    virtual int ProcessSample() = 0;
};


// Support for Version 6.0 styles
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma warning(push)
#pragma warning(disable:4244)
extern "C" {
#define __STDC_CONSTANT_MACROS

#include "libavresample/avresample.h"
#include "libavutil\opt.h"
}
#pragma warning(pop)


class CResampler
{
public:
	CResampler();
	~CResampler();
	static bool CanResample(WAVEFORMATEX* pSourceFormat, WAVEFORMATEX* pDestFormat);

	//pDestFormat=NULL for no resampling
	SimpleSample* CreateSample(IMediaSample* pSrcSample, WAVEFORMATEX* pSourceFormat, WAVEFORMATEX* pDestFormat);

private:
	HRESULT InitContext();
	void ReleaseContext();


	AVAudioResampleContext *m_avrContext;

	WAVEFORMATEX* m_pCurrentSourceFormat;
	WAVEFORMATEX* m_pCurrentDestFormat;
};