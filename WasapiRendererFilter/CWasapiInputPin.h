class CWasapiFilterManager;

class CWasapiInputPin : public CRenderedInputPin
{
    CWasapiFilterManager    * const m_pManager;			// Main renderer object
    CCritSec * const m_pReceiveLock;					// Sample critical section
    REFERENCE_TIME m_tLast;								// Last sample receive time
	REFERENCE_TIME m_tSegmentStart;

public:

    CWasapiInputPin(CWasapiFilterManager *pDump,
                  LPUNKNOWN pUnk,
                  CBaseFilter *pFilter,
                  CCritSec *pLock,
                  CCritSec *pReceiveLock,
                  HRESULT *phr);

    // Do something with this media sample
    STDMETHODIMP Receive(IMediaSample *pSample);
    STDMETHODIMP EndOfStream(void);
    STDMETHODIMP ReceiveCanBlock();

	STDMETHODIMP BeginFlush(void);

    // Write detailed information about this sample to a file
    HRESULT WriteStringInfo(IMediaSample *pSample);

    // Check if the pin can support this specific proposed type and format
    HRESULT CheckMediaType(const CMediaType* mt);
	HRESULT SetMediaType(const CMediaType *);

	HRESULT CompleteConnect(IPin *pReceivePin);
    // Break connection
    HRESULT BreakConnect();

	HRESULT Active();
	HRESULT Inactive();

    // Track NewSegment
    STDMETHODIMP NewSegment(REFERENCE_TIME tStart,
                            REFERENCE_TIME tStop,
                            double dRate);

	    STDMETHODIMP NotifyAllocator(
                    IMemAllocator * pAllocator,
                    BOOL bReadOnly);
};
