//------------------------------------------------------------------------------
// File: DumpUIDs.h
//
// Desc: DirectShow sample code - CLSIDs used by the dump renderer.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

// {202F2BEE-B160-40ac-8BC1-30B6456DED61}
DEFINE_GUID(CLSID_WasapiRendererFilter, 
0x202f2bee, 0xb160, 0x40ac, 0x8b, 0xc1, 0x30, 0xb6, 0x45, 0x6d, 0xed, 0x61);


MIDL_INTERFACE("0B752BCC-EFEA-4004-A0C3-1B1A0571E75A")
IRendererFilterWasapi : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetWasapiMixFormat(WAVEFORMATEX** ppFormat) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetExclusiveMode(bool* pIsExclusive) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetExclusiveMode(bool pIsExclusive) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetActiveMode(int* pMode) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetDevice(LPCWSTR pDevID)=0;
};

//MIDL_INTERFACE("28851006-602D-49bc-81B4-51F91A861275")
//IRendererFilterRegisterWasapi : public IUnknown
//{
//public:
//    virtual HRESULT STDMETHODCALLTYPE RegisterWasapi(IRendererFilterWasapi *pWasapi) = 0;
//};

