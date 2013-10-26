class CWasapiFilterProperties : public CBasePropertyPage
{
private:
    IRendererFilterWasapi *m_pWasapiFilter;    // Pointer to the filter's custom interface.
	WasapiDeviceInfo *m_pDeviceInfos;
	void UpdateFormatText();
	void PopulateDeviceList();

public:
    CWasapiFilterProperties(IUnknown *pUnk);
	virtual HRESULT 		OnConnect(IUnknown *pUnk);
	virtual HRESULT 		OnDisconnect();
	virtual HRESULT 		OnActivate();

	BOOL OnReceiveMessage(HWND hwnd,UINT uMsg, WPARAM wParam, LPARAM lParam);

	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) 
	{
		CWasapiFilterProperties *pNewObject = new CWasapiFilterProperties(pUnk);
		if (pNewObject == NULL) 
		{
			*pHr = E_OUTOFMEMORY;
		}
		return pNewObject;
	} 

};