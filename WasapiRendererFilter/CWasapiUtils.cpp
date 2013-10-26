#include "StdAfx.h"
//#include <assert.h>
//#include <avrt.h>
//#include "WASAPIRenderer.h"
#include "CWasapiUtils.h"
#include "FormatUtils.h"
#include <Functiondiscoverykeys_devpkey.h>

LPWSTR GetStringProperty(IPropertyStore* pProps, REFPROPERTYKEY key)
{
	HRESULT hr=S_OK;
	LPWSTR pwszDest = NULL;
	PROPVARIANT varName;

    PropVariantInit(&varName);			// Initialize container for property value.
    hr = pProps->GetValue(key, &varName);
     
   if (hr==S_OK) {
	    LPWSTR pwszSource=varName.pwszVal;
		if(pwszSource){
			size_t len = (wcslen( pwszSource ) + 1) * sizeof *pwszSource;
     
			pwszDest = static_cast<LPWSTR>( ::CoTaskMemAlloc( len ) );
			if (pwszDest) {
				::CopyMemory( pwszDest, pwszSource, len );
			} else {
				throw (HRESULT) E_OUTOFMEMORY;
			}
		}
		PropVariantClear(&varName);
    }
 
    return pwszDest;
}

UINT GetUIntProperty(IPropertyStore* pProps, REFPROPERTYKEY key)
{
	HRESULT hr=S_OK;
	UINT retValue=0;
	PROPVARIANT varName;

    PropVariantInit(&varName);			// Initialize container for property value.
    hr = pProps->GetValue(key, &varName);
     
   if (hr==S_OK) {
	  retValue=varName.uintVal;
	  PropVariantClear(&varName);
    }
    return retValue;
}


HRESULT CWasapiUtils::GetDeviceInfos(bool includeDisconnected, WasapiDeviceInfo** ppDestInfos, int* pInfoCount, int* pIndexDefault)
{
	HRESULT hr=S_OK;

	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDeviceCollection *pCollection = NULL;
	UINT  endpointCount;

	*ppDestInfos=NULL;

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEnumerator));
	if (FAILED(hr))
	{			
		DebugPrintf(L"CWasapiUtils::GetDeviceInfos - Unable to instantiate device enumerator: %x\n", hr);
		goto exit;
	}

	UINT state = includeDisconnected ? DEVICE_STATE_ACTIVE | DEVICE_STATE_NOTPRESENT : DEVICE_STATE_ACTIVE;

    hr = pEnumerator->EnumAudioEndpoints(eRender, state, &pCollection);
	if (FAILED(hr))
	{			
		DebugPrintf(L"CWasapiUtils::GetDeviceInfos - Unable to enumerate endpoints. %x\n", hr);
		goto exit;
	}

    hr = pCollection->GetCount(&endpointCount);
	if (FAILED(hr))
	{			
		DebugPrintf(L"CWasapiUtils::GetDeviceInfos - Unable to get endpoint count. %x\n", hr);
		goto exit;
	}

	*pInfoCount=endpointCount;

	if (endpointCount == 0)
    {
        DebugPrintf(L"CWasapiUtils::GetDeviceInfos - No endpoints found.\n");
		return S_FALSE;
    }
	IMMDevice *pDefaultDevice=NULL;
	LPWSTR defaultEndpointId=NULL;
	hr = pEnumerator->GetDefaultAudioEndpoint(eRender,eConsole,&pDefaultDevice);	
	hr = pDefaultDevice->GetId(&defaultEndpointId);
	pDefaultDevice->Release();

	*ppDestInfos=new WasapiDeviceInfo[endpointCount];
	WasapiDeviceInfo* pDestInfos=*ppDestInfos;
	for (ULONG i = 0; i < endpointCount; i++)
    {
		IMMDevice *pEndpoint = NULL;
		IPropertyStore *pProps = NULL;
		LPWSTR pwszID = NULL;

        hr = pCollection->Item(i, &pEndpoint);					// Get pointer to endpoint number i.
        //EXIT_ON_ERROR(hr)

        // Get the endpoint ID string.
        hr = pEndpoint->GetId(&pwszID);
		if(wcscmp(defaultEndpointId,pwszID)==0) {
			*pIndexDefault=i;
		}
        //EXIT_ON_ERROR(hr)
		pDestInfos[i].DeviceId=pwszID;
        
        hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);
        //EXIT_ON_ERROR(hr)

		pDestInfos[i].DeviceFriendlyName=GetStringProperty(pProps,PKEY_Device_FriendlyName);
		pDestInfos[i].ControlPanelPageProviderId=GetStringProperty(pProps,PKEY_AudioEndpoint_ControlPanelPageProvider);
		pDestInfos[i].FormFactor=GetUIntProperty(pProps,PKEY_AudioEndpoint_FormFactor);
        SafeRelease(&pProps);
        SafeRelease(&pEndpoint);
    }

	*ppDestInfos=pDestInfos;
exit:
	SafeRelease(&pEnumerator);
    SafeRelease(&pCollection);
	return hr;
}