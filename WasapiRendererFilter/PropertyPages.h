class CGrayProp : public CBasePropertyPage
{
private:
    IRendererFilterWasapi *m_pWasapiFilter;    // Pointer to the filter's custom interface.

	void UpdateFormatText();

public:
    // Constructor
    CGrayProp(IUnknown *pUnk);
	virtual HRESULT 		OnConnect(IUnknown *pUnk);
	virtual HRESULT 		OnDisconnect();
	virtual HRESULT 		OnActivate();

	BOOL OnReceiveMessage(HWND hwnd,UINT uMsg, WPARAM wParam, LPARAM lParam);

	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) 
	{
		CGrayProp *pNewObject = new CGrayProp(pUnk);
		if (pNewObject == NULL) 
		{
			*pHr = E_OUTOFMEMORY;
		}
		return pNewObject;
	} 

};