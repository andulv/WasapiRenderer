#include "WasapiHelper.h"
#include "stdafx.h"
#include <functiondiscoverykeys.h>

//
//  Retrieves the device friendly name for a particular device in a device collection.
//
LPWSTR WasapiHelper::GetDeviceID(IMMDeviceCollection *DeviceCollection, UINT DeviceIndex)
{
    IMMDevice *device;
    LPWSTR deviceId;
    HRESULT hr;

    hr = DeviceCollection->Item(DeviceIndex, &device);
    if (FAILED(hr))
    {
        //DebugPrintf(L"Unable to get device %d: %x\n", DeviceIndex, hr);
        return NULL;
    }
    hr = device->GetId(&deviceId);
    if (FAILED(hr))
    {
        //DebugPrintf(L"Unable to get device %d id: %x\n", DeviceIndex, hr);
        return NULL;
    }

    wchar_t *returnValue = _wcsdup(deviceId);
    if (returnValue == NULL)
    {
        //DebugPrintf(L"Unable to allocate buffer for return\n");
        return NULL;
    }
    return returnValue;
}