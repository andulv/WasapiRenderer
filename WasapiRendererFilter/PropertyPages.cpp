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
#pragma comment( lib, "comctl32.lib" )


CGrayProp::CGrayProp(IUnknown *pUnk) : 
  CBasePropertyPage(NAME("GrayProp"), pUnk, IDD_PROPERTYPAGE1, IDS_PROPERTYPAGE1_TITLE),
  m_pWasapiFilter(0)
{ 
}

HRESULT CGrayProp::OnConnect(IUnknown *pUnk)
{
	if (pUnk == NULL)
    {
        return E_POINTER;
    }
    ASSERT(m_pWasapiFilter == NULL);
    return pUnk->QueryInterface(__uuidof(IRendererFilterWasapi), reinterpret_cast<void**>(&m_pWasapiFilter));
}

HRESULT CGrayProp::OnDisconnect(void)
{
    if (m_pWasapiFilter)
    {
        m_pWasapiFilter->Release();
        m_pWasapiFilter = NULL;
    }
    return S_OK;
}

HRESULT CGrayProp::OnActivate(void)
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

	UpdateFormatText();

	return hr;
}

BOOL CGrayProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			break;

	}
	return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}

void CGrayProp::UpdateFormatText()
{
	HRESULT hr=S_OK;
	RefCountingWaveFormatEx* pFormat=NULL;

	hr = m_pWasapiFilter->GetCurrentInputFormat(&pFormat);
	if(pFormat)
	{
		WCHAR buffer[100];
		WAVEFORMATEX* pFormatEx=pFormat->GetFormat();
		_snwprintf_s(buffer, _TRUNCATE, L"%d / 0x%x", pFormatEx->nChannels, 0);
		SendDlgItemMessage(m_Dlg, IDC_STATIC_INPUT_CHANNELS, WM_SETTEXT, 0, (LPARAM)buffer);
		_snwprintf_s(buffer, _TRUNCATE, L"%d", pFormatEx->nSamplesPerSec);
		SendDlgItemMessage(m_Dlg, IDC_STATIC_INPUT_SAMPLERATE, WM_SETTEXT, 0, (LPARAM)buffer);

		_snwprintf_s(buffer, _TRUNCATE, L"%d (%d bits)", pFormatEx->wFormatTag, pFormatEx->wBitsPerSample);
		SendDlgItemMessage(m_Dlg, IDC_STATIC_INPUT_FORMAT, WM_SETTEXT, 0, (LPARAM)buffer);
		pFormat->Release();
	}

	hr = m_pWasapiFilter->GetCurrentResampledFormat(&pFormat);
	if(pFormat)
	{
		WCHAR buffer[100];
		WAVEFORMATEX* pFormatEx=pFormat->GetFormat();
		_snwprintf_s(buffer, _TRUNCATE, L"%d / 0x%x", pFormatEx->nChannels, 0);
		SendDlgItemMessage(m_Dlg, IDC_STATIC_RESAMPLER_CHANNELS, WM_SETTEXT, 0, (LPARAM)buffer);
		_snwprintf_s(buffer, _TRUNCATE, L"%d", pFormatEx->nSamplesPerSec);
		SendDlgItemMessage(m_Dlg, IDC_STATIC_RESAMPLER_SAMPLERATE, WM_SETTEXT, 0, (LPARAM)buffer);

		_snwprintf_s(buffer, _TRUNCATE, L"%d (%d bits)", pFormatEx->wFormatTag, pFormatEx->wBitsPerSample);
		SendDlgItemMessage(m_Dlg, IDC_STATIC_RESAMPLER_FORMAT, WM_SETTEXT, 0, (LPARAM)buffer);
		pFormat->Release();
	}


}

