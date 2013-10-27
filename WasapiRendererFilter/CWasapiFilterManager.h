class CWASAPIRenderer;
class CWasapiInputPin;
class CWasapiFilter;
class CResampler;

    enum ReceivedSampleActions
    {
        ReceivedSampleActions_Accept,
        ReceivedSampleActions_RejectSilent,
		ReceivedSampleActions_RejectLoud
    };


class CWasapiFilterManager : public IRendererFilterWasapi, 
							 public ISpecifyPropertyPages,
							 public CBaseReferenceClock
{
    friend class CWasapiFilter;
    friend class CWasapiInputPin;
	friend class CBasePin;

    CWasapiFilter			*m_pFilter;       // Methods for filter interfaces
    CWasapiInputPin			*m_pPin;          // A simple rendered input pin
	CWASAPIRenderer			*m_pRenderer;
	CResampler				*m_pResampler;


	CPosPassThru			*m_pPosition;      // Renderer position controls

    CCritSec				m_Lock;                // Main renderer critical section
    CCritSec				m_ReceiveLock;         // Sublock for received samples
    CCritSec				m_MediaTypeLock;         // Sublock for received samples

	RefCountingWaveFormatEx	*m_pCurrentMediaTypeReceive;
	RefCountingWaveFormatEx	*m_pCurrentMediaTypeResample;
	//RefCountingWaveFormatEx	*m_pCurrentMediaTypeResample;
	ReceivedSampleActions	m_currentMediaTypeSampleReceivedAction;
	bool					m_IsExclusive;

public:
	STDMETHODIMP GetPages(CAUUID *pPages)
    {
        if (pPages == NULL) return E_POINTER;
        pPages->cElems = 1;
        pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
        if (pPages->pElems == NULL) 
        {
            return E_OUTOFMEMORY;
        }
        pPages->pElems[0] = CLSID_WasapiProp;
        return S_OK;
    }

    STDMETHODIMP QueryInterface(REFIID riid, __deref_out void **ppv) { 
        return GetOwner()->QueryInterface(riid,ppv);            
    };                                                          
    STDMETHODIMP_(ULONG) AddRef() {                             
        return GetOwner()->AddRef();                            
    };                                                          
    STDMETHODIMP_(ULONG) Release() {                            
        return GetOwner()->Release();                           
    };
    CWasapiFilterManager(LPUNKNOWN pUnk, HRESULT *phr);
    ~CWasapiFilterManager();

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    HRESULT SampleReceived(IMediaSample *pSample);
	bool StartRendering();
	bool PauseRendering();
	bool StopRendering(bool clearQueue, bool clearFormats);
	bool ClearQueue();
	bool CheckFormat(WAVEFORMATEX* requestedFormat);
	STDMETHOD(GetWasapiMixFormat)(WAVEFORMATEX** ppFormat);
	STDMETHOD(GetExclusiveMode)(bool* pIsExclusive);
	STDMETHOD(SetExclusiveMode)(bool pIsExclusive);
	STDMETHOD(GetActiveMode)(int* pMode);
	STDMETHOD(SetDevice)(LPCWSTR pDevID);
	STDMETHOD(GetDevice)(LPWSTR* ppDevID);

	STDMETHOD(GetCurrentInputFormat)(RefCountingWaveFormatEx** ppFormat);
	STDMETHOD(GetCurrentResampledFormat)(RefCountingWaveFormatEx** ppFormat);
	STDMETHOD(GetDeviceInfos)(bool includeDisconnected, WasapiDeviceInfo** ppDestInfos, int* pInfoCount, int* pIndexDefault);

private:
	HRESULT SetFormatReceived(CMediaType* pmt);
	HRESULT SetFormatProcessed();
	REFERENCE_TIME GetPrivateTime();
    // Overriden to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
};

