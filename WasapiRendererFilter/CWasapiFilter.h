class CWasapiFilterManager;

class CWasapiFilter : public CBaseFilter//, public CBaseReferenceClock
{
    CWasapiFilterManager * const m_pManager;
public:

    // Constructor
    CWasapiFilter(CWasapiFilterManager *pDump,
                LPUNKNOWN pUnk,
                CCritSec *pLock,
                HRESULT *phr);

    // Pin enumeration
    CBasePin * GetPin(int n);
    int GetPinCount();

    // Open and close the file as necessary
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP Pause();
    STDMETHODIMP Stop();
	FILTER_STATE GetState();
	REFERENCE_TIME GetStartTime();
};


//  Pin object
