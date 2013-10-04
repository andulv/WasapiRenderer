#pragma once
#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <AudioPolicy.h>

class WasapiHelper
{
public:
	static LPWSTR GetDeviceID(IMMDeviceCollection *DeviceCollection, UINT DeviceIndex);
};