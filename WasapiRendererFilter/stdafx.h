// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


//#include "targetver.h"
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1
#include <new>
#include <windows.h>
#include <strsafe.h>
#include <objbase.h>
#pragma warning(push)
#pragma warning(disable : 4201)
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#pragma warning(pop)
#include <mfapi.h>
#include <mfidl.h>
#include <assert.h>

class IMediaBufferEx : public IMediaBuffer
{
public:
	//virtual HRESULT STDMETHODCALLTYPE SetSampleTimes(REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd) = 0;

	virtual REFERENCE_TIME STDMETHODCALLTYPE GetStartTime() = 0;
	virtual REFERENCE_TIME STDMETHODCALLTYPE GetEndTime() = 0;
        
};

//extern bool DisableMMCSS;

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

void DebugPrintf(const wchar_t *str, ...);

