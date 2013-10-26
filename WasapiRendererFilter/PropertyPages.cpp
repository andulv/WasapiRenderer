#include <streams.h>
#include "WasapiRendererFilteruids.h"

#include "CWasapiFilter.h"
#include "CWasapiInputPin.h"
#include "WasapiRendererFilter.h"
#include "WASAPIRenderer.h"
#include "WasapiHelper.h"
#include "stdafx.h"
#include "PropertyPages.h"

#include "resource.h"
#include "resource1.h"
#include <Commctrl.h>
#include <wchar.h>
#include "utility.h"
#include "CWasapiUtils.h"

#pragma comment( lib, "comctl32.lib" )


CWasapiFilterProperties::CWasapiFilterProperties(IUnknown *pUnk) : 
  CBasePropertyPage(NAME("GrayProp"), pUnk, IDD_PROPERTYPAGE1, IDS_PROPERTYPAGE1_TITLE),
  m_pWasapiFilter(NULL),
  m_pDeviceInfos(NULL)
{ 
}

HRESULT CWasapiFilterProperties::OnConnect(IUnknown *pUnk)
{
	if (pUnk == NULL)
    {
        return E_POINTER;
    }
    ASSERT(m_pWasapiFilter == NULL);
    return pUnk->QueryInterface(__uuidof(IRendererFilterWasapi), reinterpret_cast<void**>(&m_pWasapiFilter));
}

HRESULT CWasapiFilterProperties::OnDisconnect(void)
{
    if (m_pWasapiFilter)
    {
        m_pWasapiFilter->Release();
        m_pWasapiFilter = NULL;
    }
	if(m_pDeviceInfos)
	{
		delete[] m_pDeviceInfos;
		m_pDeviceInfos=NULL;
	}
    return S_OK;
}

HRESULT CWasapiFilterProperties::OnActivate(void)
{
	INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_BAR_CLASSES;
    if (InitCommonControlsEx(&icc) == FALSE)
    {
        return E_FAIL;
    }

	HRESULT hr=S_OK;
	bool isExlusive=false;
	hr = m_pWasapiFilter->GetExclusiveMode(&isExlusive);
	SendDlgItemMessage(m_Dlg, IDC_CHK_EXCLUSIVE, BM_SETCHECK, isExlusive, 0);

	PopulateDeviceList();
	UpdateFormatText();
	return hr;
}

BOOL CWasapiFilterProperties::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch ( uMsg )
	{
		// WM_COMMAND is sent when an event occurs
		// the HIWORD of WPARAM holds the notification event code
		case WM_COMMAND:
			if ( HIWORD( wParam ) == BN_CLICKED )
			{
				// some button/checkbox has been clicked
				// the lower word of wParam holds the controls ID

				if ( LOWORD( wParam ) == IDC_CHK_EXCLUSIVE) 
				{
					bool isCheckedControl = SendDlgItemMessage(m_Dlg, IDC_CHK_EXCLUSIVE, BM_GETCHECK, 0, 0);
					m_pWasapiFilter->SetExclusiveMode(isCheckedControl);
					return (LRESULT) 1;
				}
				else if ( LOWORD( wParam ) == IDC_BUTTON_REFRESH) 
				{
					UpdateFormatText();
					return (LRESULT) 1;
				}

			}
			else if(HIWORD( wParam ) == CBN_SELCHANGE)
			{
				if ( LOWORD( wParam ) == IDC_COMBO1) 
				{
					int selectedIndex=	SendDlgItemMessage(m_Dlg,IDC_COMBO1, CB_GETCURSEL  ,(WPARAM) 0,(LPARAM) 0); 
					m_pWasapiFilter->SetDevice(m_pDeviceInfos[selectedIndex].DeviceId);
					return (LRESULT) 1;
				}

			}
			break;

	}
	return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}

void CWasapiFilterProperties::PopulateDeviceList()
{
	HRESULT hr=S_OK;
	int iFormats=0;
	int iSelected=-1;
	int iDefault=-1;

	if(m_pDeviceInfos)
	{
		delete m_pDeviceInfos;
		m_pDeviceInfos=NULL;
	}


	hr = m_pWasapiFilter->GetDeviceInfos(false,&m_pDeviceInfos,&iFormats, &iDefault);
	SendDlgItemMessage(m_Dlg,IDC_COMBO1, CB_RESETCONTENT ,(WPARAM) 0,(LPARAM) 0); 
	for (int k = 0; k < iFormats; k += 1)
    {
        auto newString=NewStringW(m_pDeviceInfos[k].DeviceFriendlyName);
        // Add string to combobox.
		SendDlgItemMessage(m_Dlg,IDC_COMBO1, CB_ADDSTRING,(WPARAM) 0,(LPARAM) newString); 
    }
	LPWSTR selectedDevice=NULL;
	hr=m_pWasapiFilter->GetDevice(&selectedDevice);
	if(hr==S_OK) 
	{
		for (int k = 0; k < iFormats; k += 1)
		{
			if(wcscmp(m_pDeviceInfos[k].DeviceId,selectedDevice)==0)
			{
				iSelected=k;
			}
		}
	}

	if(iSelected>-1)
		SendDlgItemMessage(m_Dlg,IDC_COMBO1, CB_SETCURSEL  ,(WPARAM) iSelected,(LPARAM) 0); 
	else
		SendDlgItemMessage(m_Dlg,IDC_COMBO1, CB_SETCURSEL  ,(WPARAM) iDefault,(LPARAM) 0); 
}

void CWasapiFilterProperties::UpdateFormatText()
{
	HRESULT hr=S_OK;
	RefCountingWaveFormatEx* pFormat=NULL;

	hr = m_pWasapiFilter->GetCurrentInputFormat(&pFormat);
	WCHAR buffer[100];
	if(pFormat)
	{
		WAVEFORMATEX* pFormatEx=pFormat->GetFormat();
		_snwprintf_s(buffer, _TRUNCATE, L"%d / 0x%x", pFormatEx->nChannels, 0);
		SendDlgItemMessage(m_Dlg, IDC_STATIC_INPUT_CHANNELS, WM_SETTEXT, 0, (LPARAM)buffer);
		_snwprintf_s(buffer, _TRUNCATE, L"%d", pFormatEx->nSamplesPerSec);
		SendDlgItemMessage(m_Dlg, IDC_STATIC_INPUT_SAMPLERATE, WM_SETTEXT, 0, (LPARAM)buffer);

		_snwprintf_s(buffer, _TRUNCATE, L"%d (%d bits)", pFormatEx->wFormatTag, pFormatEx->wBitsPerSample);
		SendDlgItemMessage(m_Dlg, IDC_STATIC_INPUT_FORMAT, WM_SETTEXT, 0, (LPARAM)buffer);
		pFormat->Release();
	}
	else
	{
		_snwprintf_s(buffer, _TRUNCATE, L"n/a");
		SendDlgItemMessage(m_Dlg, IDC_STATIC_INPUT_CHANNELS, WM_SETTEXT, 0, (LPARAM)buffer);
		_snwprintf_s(buffer, _TRUNCATE,  L"n/a");
		SendDlgItemMessage(m_Dlg, IDC_STATIC_INPUT_SAMPLERATE, WM_SETTEXT, 0, (LPARAM)buffer);
		_snwprintf_s(buffer, _TRUNCATE,  L"n/a");
		SendDlgItemMessage(m_Dlg, IDC_STATIC_INPUT_FORMAT, WM_SETTEXT, 0, (LPARAM)buffer);
	}



	hr = m_pWasapiFilter->GetCurrentResampledFormat(&pFormat);
	if(pFormat)
	{
		WAVEFORMATEX* pFormatEx=pFormat->GetFormat();
		_snwprintf_s(buffer, _TRUNCATE, L"%d / 0x%x", pFormatEx->nChannels, 0);
		SendDlgItemMessage(m_Dlg, IDC_STATIC_RESAMPLER_CHANNELS, WM_SETTEXT, 0, (LPARAM)buffer);
		_snwprintf_s(buffer, _TRUNCATE, L"%d", pFormatEx->nSamplesPerSec);
		SendDlgItemMessage(m_Dlg, IDC_STATIC_RESAMPLER_SAMPLERATE, WM_SETTEXT, 0, (LPARAM)buffer);

		_snwprintf_s(buffer, _TRUNCATE, L"%d (%d bits)", pFormatEx->wFormatTag, pFormatEx->wBitsPerSample);
		SendDlgItemMessage(m_Dlg, IDC_STATIC_RESAMPLER_FORMAT, WM_SETTEXT, 0, (LPARAM)buffer);
		pFormat->Release();
	}
	else
	{
		_snwprintf_s(buffer, _TRUNCATE, L"n/a");
		SendDlgItemMessage(m_Dlg, IDC_STATIC_RESAMPLER_CHANNELS, WM_SETTEXT, 0, (LPARAM)buffer);
		_snwprintf_s(buffer, _TRUNCATE, L"n/a");
		SendDlgItemMessage(m_Dlg, IDC_STATIC_RESAMPLER_SAMPLERATE, WM_SETTEXT, 0, (LPARAM)buffer);

		_snwprintf_s(buffer, _TRUNCATE, L"n/a");
		SendDlgItemMessage(m_Dlg, IDC_STATIC_RESAMPLER_FORMAT, WM_SETTEXT, 0, (LPARAM)buffer);	}


}

