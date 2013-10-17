#include "FormatUtils.h"

int CFormatUtils::GetChannelMask (const int numChannels)
{
    switch (numChannels)
    {
        case 1:   return 0;
        case 2:   return 1 + 2; // SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT
        case 5:   return 1 + 2 + 4 + 16 + 32; // SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT
        case 6:   return 1 + 2 + 4 + 8 + 16 + 32; // SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT
        case 7:   return 1 + 2 + 4  + 16 + 32 + 512 + 1024; // SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER  | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT
        case 8:   return 1 + 2 + 4 + 8 + 16 + 32 + 512 + 1024; // SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT
        default:  break;
    }

    return 0;
}


GUID CFormatUtils::FormatTagToSubFormat(DWORD formatTag)
{
	switch(formatTag)
	{
		case WAVE_FORMAT_PCM:			return KSDATAFORMAT_SUBTYPE_PCM;
		case WAVE_FORMAT_IEEE_FLOAT:	return KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
		case WAVE_FORMAT_DRM:			return KSDATAFORMAT_SUBTYPE_DRM;
		case WAVE_FORMAT_ALAW:			return KSDATAFORMAT_SUBTYPE_ALAW;
		case WAVE_FORMAT_MULAW:			return KSDATAFORMAT_SUBTYPE_MULAW;
		case WAVE_FORMAT_ADPCM:			return KSDATAFORMAT_SUBTYPE_ADPCM;
		default:						return KSDATAFORMAT_SUBTYPE_NONE;
	}
}

DWORD CFormatUtils::SubFormatToFormatTag(GUID subFormat)
{

	if(subFormat==KSDATAFORMAT_SUBTYPE_PCM)			return WAVE_FORMAT_PCM;
	if(subFormat==KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)	return WAVE_FORMAT_IEEE_FLOAT;
	else if(subFormat==KSDATAFORMAT_SUBTYPE_DRM)	return WAVE_FORMAT_DRM;
	else if(subFormat==KSDATAFORMAT_SUBTYPE_ALAW)	return WAVE_FORMAT_ALAW;
	else if(subFormat==KSDATAFORMAT_SUBTYPE_MULAW)	return WAVE_FORMAT_MULAW;
	else if(subFormat==KSDATAFORMAT_SUBTYPE_ADPCM)	return WAVE_FORMAT_ADPCM;

	return 0;
}

void CFormatUtils::SetCalculatedProperties(WAVEFORMATEX* format)
{
	DWORD bytesPerSample=format->wBitsPerSample / 8;
	format->nAvgBytesPerSec=(DWORD) (format->nSamplesPerSec * format->nChannels * bytesPerSample);
	format->nBlockAlign=format->nChannels * bytesPerSample;
}

void CFormatUtils::ToEx(WAVEFORMATEXTENSIBLE* src, WAVEFORMATEX* dest)
{
	WAVEFORMATEX* srcEx= (WAVEFORMATEX*)src;

	dest->cbSize=0;
	dest->wFormatTag=SubFormatToFormatTag(src->SubFormat);
	dest->nChannels=srcEx->nChannels;
	dest->nSamplesPerSec=srcEx->nSamplesPerSec;
	dest->wBitsPerSample=srcEx->wBitsPerSample;
	SetCalculatedProperties(dest);
}



void CFormatUtils::ToExtensible(WAVEFORMATEX* src, WAVEFORMATEXTENSIBLE* dest)
{
	WAVEFORMATEX* destEx= &dest->Format;

	destEx->cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
	destEx->wFormatTag=WAVE_FORMAT_EXTENSIBLE;
	destEx->nChannels=src->nChannels;
	destEx->nSamplesPerSec=src->nSamplesPerSec;
	destEx->wBitsPerSample=src->wBitsPerSample;
	SetCalculatedProperties(destEx);

	dest->Samples.wValidBitsPerSample=destEx->wBitsPerSample;
	dest->SubFormat=FormatTagToSubFormat(src->wFormatTag);
	dest->dwChannelMask=GetChannelMask(destEx->nChannels);
}





