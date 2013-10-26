
class CResampler
{
public:
	CResampler();
	~CResampler();
	static bool CanResample(WAVEFORMATEX* pSourceFormat, WAVEFORMATEX* pDestFormat);

	//pDestFormat=NULL for no resampling
	IMediaBufferEx* CreateSample(IMediaSample* pSrcSample, WAVEFORMATEX* pSourceFormat, WAVEFORMATEX* pDestFormat);

private:
	HRESULT InitContext();
	void ReleaseContext();


    IUnknown *m_pTransform;
	IMediaObject *m_pDMO;

	WAVEFORMATEX* m_pCurrentSourceFormat;
	WAVEFORMATEX* m_pCurrentDestFormat;
};




//IMediaBuffer implementation with its own buffer that also holds a reference to a IMediaSample. When this class destroys itself (refcount=0) it will also release the IMediaSample.
//NB! The consumer of the IMediaBuffer only sees the converted data. The original IMediaSample data/size is hidden.
class CMediaBufferSampleWrapperWithConvertedData : public IMediaBufferEx
{
private:
    DWORD        m_cbLength;
    const DWORD  m_cbMaxLength;
    LONG         m_nRefCount;  // Reference count
    BYTE         *m_pbData;

	REFERENCE_TIME m_rtStart, m_rtEnd;
    IMediaSample *m_pSample;

    CMediaBufferSampleWrapperWithConvertedData(IMediaSample *pSourceSample, DWORD cbMaxLength, HRESULT& hr) :
        m_nRefCount(1),
        m_cbMaxLength(cbMaxLength),
        m_cbLength(0),
        m_pbData(NULL),
		m_pSample(pSourceSample)
    {
		m_pSample->AddRef();
		m_pSample->GetTime(&m_rtStart,&m_rtEnd);
        m_pbData = new BYTE[cbMaxLength];
        if (!m_pbData) 
        {
            hr = E_OUTOFMEMORY;
        }
    }

    ~CMediaBufferSampleWrapperWithConvertedData()
    {
        if (m_pbData) 
        {
            delete [] m_pbData;
        }
		if(m_pSample)
		{
			m_pSample->Release();
		}
    }

public:

    // Function to create a new IMediaBuffer object and return 
    // an AddRef'd interface pointer.
    static HRESULT Create(IMediaSample *pSourceSample, long cbMaxLen, IMediaBufferEx **ppBuffer)
    {
        HRESULT hr = S_OK;
        CMediaBufferSampleWrapperWithConvertedData *pBuffer = NULL;

        if (ppBuffer == NULL)
        {
            return E_POINTER;
        }

        pBuffer = new CMediaBufferSampleWrapperWithConvertedData(pSourceSample, cbMaxLen, hr);

        if (pBuffer == NULL)
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
           *ppBuffer = pBuffer;
           (*ppBuffer)->AddRef();
        }

        if (pBuffer)
        {
            pBuffer->Release();
        }
        return hr;
    }

    // IUnknown methods.
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
    {
        if (ppv == NULL) 
        {
            return E_POINTER;
        }
        else if (riid == IID_IMediaBuffer || riid == IID_IUnknown) 
        {
            *ppv = static_cast<IMediaBuffer *>(this);
            AddRef();
            return S_OK;
        }
        else
        {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_nRefCount);
    }

    STDMETHODIMP_(ULONG) Release()
    {
        LONG lRef = InterlockedDecrement(&m_nRefCount);
        if (lRef == 0) 
        {
            delete this;
            // m_cRef is no longer valid! Return lRef.
        }
        return lRef;  
    }

    // IMediaBuffer methods.
    STDMETHODIMP SetLength(DWORD cbLength)
    {
        if (cbLength > m_cbMaxLength) 
        {
            return E_INVALIDARG;
        }
        m_cbLength = cbLength;
        return S_OK;
    }

    STDMETHODIMP GetMaxLength(DWORD *pcbMaxLength)
    {
        if (pcbMaxLength == NULL) 
        {
            return E_POINTER;
        }
        *pcbMaxLength = m_cbMaxLength;
        return S_OK;
    }

    STDMETHODIMP GetBufferAndLength(BYTE **ppbBuffer, DWORD *pcbLength)
    {
        // Either parameter can be NULL, but not both.
        if (ppbBuffer == NULL && pcbLength == NULL) 
        {
            return E_POINTER;
        }
        if (ppbBuffer) 
        {
            *ppbBuffer = m_pbData;
        }
        if (pcbLength) 
        {
            *pcbLength = m_cbLength;
        }
        return S_OK;
    }

	REFERENCE_TIME STDMETHODCALLTYPE GetStartTime()
	{
		return m_rtStart;
	}

		REFERENCE_TIME STDMETHODCALLTYPE GetEndTime()
	{
		return m_rtEnd;
	}
};

//IMediaBuffer implementation that wraps a reference to a IMediaSample. When this class destroys itself (refcount=0) it will also release the IMediaSample.
//This class has no data except for IMediaSample.
class CMediaBufferSampleWrapper : public IMediaBufferEx
{
private:
    LONG         m_nRefCount;  // Reference count
	REFERENCE_TIME m_rtStart, m_rtEnd;
    IMediaSample *m_pSample;


public:
    CMediaBufferSampleWrapper(IMediaSample* pSample) :
        m_nRefCount(1),
        m_pSample(pSample)
    {
		m_pSample->GetTime(&m_rtStart,&m_rtEnd);
		m_pSample->AddRef();
    }

    ~CMediaBufferSampleWrapper()
    {
		if(m_pSample)
		{
			m_pSample->Release();
			m_pSample=NULL;
		}
    }



    // IUnknown methods.
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
    {
        if (ppv == NULL) 
        {
            return E_POINTER;
        }
        else if (riid == IID_IMediaBuffer || riid == IID_IUnknown) 
        {
            *ppv = static_cast<IMediaBuffer *>(this);
            AddRef();
            return S_OK;
        }
        else
        {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_nRefCount);
    }

    STDMETHODIMP_(ULONG) Release()
    {
        LONG lRef = InterlockedDecrement(&m_nRefCount);
        if (lRef == 0) 
        {
            delete this;
            // m_cRef is no longer valid! Return lRef.
        }
        return lRef;  
    }

    // IMediaBuffer methods.
    STDMETHODIMP SetLength(DWORD cbLength)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP GetMaxLength(DWORD *pcbMaxLength)
    {
        if (pcbMaxLength == NULL) 
        {
            return E_POINTER;
        }
        *pcbMaxLength = m_pSample->GetSize();
        return S_OK;
    }

    STDMETHODIMP GetBufferAndLength(BYTE **ppbBuffer, DWORD *pcbLength)
    {
        // Either parameter can be NULL, but not both.
        if (ppbBuffer == NULL && pcbLength == NULL) 
        {
            return E_POINTER;
        }
        if (ppbBuffer) 
        {
			m_pSample->GetPointer(ppbBuffer);
        }
        if (pcbLength) 
        {
            *pcbLength = m_pSample->GetActualDataLength();
        }
        return S_OK;
    }

	REFERENCE_TIME STDMETHODCALLTYPE GetStartTime()
	{
		return m_rtStart;
	}

		REFERENCE_TIME STDMETHODCALLTYPE GetEndTime()
	{
		return m_rtEnd;
	}
};
