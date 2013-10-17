#pragma once
#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <AudioPolicy.h>

class CFormatUtils
{
public:
	static int GetChannelMask (const int numChannels);
	static GUID FormatTagToSubFormat(DWORD formatTag);
	static DWORD SubFormatToFormatTag(GUID subFormat);
	static void SetCalculatedProperties(WAVEFORMATEX* format);
	static void ToEx(WAVEFORMATEXTENSIBLE* src, WAVEFORMATEX* dest);
	static void ToExtensible(WAVEFORMATEX* src, WAVEFORMATEXTENSIBLE* dest);
};