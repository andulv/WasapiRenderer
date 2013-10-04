class CWASAPIRenderer;
class CWasapiInputPin;
class CWasapiFilter;

    enum ReceivedSampleActions
    {
        ReceivedSampleActions_Accept,
        ReceivedSampleActions_RejectSilent,
		ReceivedSampleActions_RejectLoud
    };


class CWasapiFilterManager : public IRendererFilterWasapi, public CBaseReferenceClock
{
    friend class CWasapiFilter;
    friend class CWasapiInputPin;
	friend class CBasePin;

    CWasapiFilter			*m_pFilter;       // Methods for filter interfaces
    CWasapiInputPin			*m_pPin;          // A simple rendered input pin
	CWASAPIRenderer			*m_pRenderer;

	CPosPassThru			*m_pPosition;      // Renderer position controls

    CCritSec				m_Lock;                // Main renderer critical section
    CCritSec				m_ReceiveLock;         // Sublock for received samples
    CCritSec				m_MediaTypeLock;         // Sublock for received samples

	RefCountingMediaType	*m_pCurrentMediaType;
	ReceivedSampleActions	m_currentMediaTypeSampleReceivedAction;
	bool					m_IsExclusive;

public:
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
	bool StopRendering(bool clearQueue);
	bool ClearQueue();
	bool CheckFormat(WAVEFORMATEX* requestedFormat, WAVEFORMATEX* suggestedFormat);
	STDMETHOD(GetWasapiMixFormat)(WAVEFORMATEX** ppFormat);
	STDMETHOD(GetExclusiveMode)(bool* pIsExclusive);
	STDMETHOD(SetExclusiveMode)(bool pIsExclusive);
	STDMETHOD(GetActiveMode)(int* pMode);
	STDMETHOD(SetDevice)(LPCWSTR pDevID);
protected:
	HRESULT SetCurrentMediaType(CMediaType* pmt);
	REFERENCE_TIME GetPrivateTime();
private:
    // Overriden to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
};

