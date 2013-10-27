#pragma once

class RefCountingWaveFormatEx;
class WasapiDeviceInfo;

// {202F2BEE-B160-40ac-8BC1-30B6456DED61}
DEFINE_GUID(CLSID_WasapiRendererFilter, 0x202f2bee, 0xb160, 0x40ac, 0x8b, 0xc1, 0x30, 0xb6, 0x45, 0x6d, 0xed, 0x61);


// {17DC41A7-EBEA-4CDA-BDD4-A9C0AC33CF9B}
DEFINE_GUID(CLSID_WasapiProp, 0x17dc41a7, 0xebea, 0x4cda, 0xbd, 0xd4, 0xa9, 0xc0, 0xac, 0x33, 0xcf, 0x9b);

MIDL_INTERFACE("0B752BCC-EFEA-4004-A0C3-1B1A0571E75A")
IRendererFilterWasapi : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetExclusiveMode(bool* pIsExclusive) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetExclusiveMode(bool pIsExclusive) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentInputFormat(RefCountingWaveFormatEx** ppFormat) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentResampledFormat(RefCountingWaveFormatEx** ppFormat) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDeviceInfos(bool includeDisconnected, WasapiDeviceInfo** ppDestInfos, int* pInfoCount, int* pIndexDefault) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetWasapiMixFormat(WAVEFORMATEX** ppFormat) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetActiveMode(int* pMode) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetDevice(LPCWSTR pDevID)=0;
	virtual HRESULT STDMETHODCALLTYPE GetDevice(LPWSTR* ppDevID)=0;
};




