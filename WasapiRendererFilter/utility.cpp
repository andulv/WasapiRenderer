/*--
 
Copyright (C) Microsoft Corporation
 
Module Name:
 
    Utility.cpp
 
Abstract:
 
    Various utility functions
 
Notes:
 
Revision History:
 
--*/
 
#include "stdafx.h"
#include "rpcdce.h"
#include <strsafe.h>
 
LPSTR
NewString(
    LPCSTR pszSource
    )
{
    LPSTR pszDest = NULL;
     
    if (pszSource) {
        size_t len = (strlen( pszSource ) + 1) * sizeof *pszSource;
     
        pszDest = static_cast<LPSTR>( ::CoTaskMemAlloc( len ) );
        if (pszDest) {
            ::CopyMemory( pszDest, pszSource, len );
        } else {
            throw (HRESULT) E_OUTOFMEMORY;
        }
    }
 
    return pszDest;
}
 
LPWSTR
NewStringW(
    LPWSTR pwszSource
    )
{
    LPWSTR pwszDest = NULL;
 
    if (pwszSource) {
        size_t len = (wcslen( pwszSource ) + 1) * sizeof *pwszSource;
     
        pwszDest = static_cast<LPWSTR>( ::CoTaskMemAlloc( len ) );
        if (pwszDest) {
            ::CopyMemory( pwszDest, pwszSource, len );
        } else {
            throw (HRESULT) E_OUTOFMEMORY;
        }
    }
 
    return pwszDest;
}
 
 
//
// UnicodeToAnsi converts the Unicode string pszW to an ANSI string
// and returns the ANSI string through ppszA. Space for the
// the converted string is allocated by UnicodeToAnsi.
//
 
HRESULT
UnicodeToAnsi(
    __in LPCWSTR pwszIn,
    __out LPSTR&  pszOut
    )
{
    size_t cbAnsi, cCharacters;
    DWORD dwError;
 
    // If input is null then just return the same.
    if (pwszIn == NULL) {
        pszOut = NULL;
        return NOERROR;
    }
 
    cCharacters = wcslen(pwszIn)+1;
 
    // Determine number of bytes to be allocated for ANSI string. An
    // ANSI string can have at most 2 bytes per character (for Double
    // Byte Character Strings.)
    cbAnsi = cCharacters*2;
 
    // Use of the OLE allocator is not required because the resultant
    // ANSI  string will never be passed to another COM component. You
    // can use your own allocator.
    pszOut = (LPSTR) CoTaskMemAlloc(cbAnsi);
    if (pszOut == NULL)
        return E_OUTOFMEMORY;
 
    // Convert to ANSI.
    if (WideCharToMultiByte(CP_ACP, 0, pwszIn, static_cast<int>(cCharacters), pszOut, static_cast<int>(cbAnsi), NULL, NULL) == 0) {
        dwError = GetLastError();
        CoTaskMemFree(pszOut);
        pszOut = NULL;
        return HRESULT_FROM_WIN32(dwError);
    }
 
    return NOERROR;
}
 
HRESULT
AnsiToUnicode(
    __in LPCSTR pszIn,
    __out LPWSTR& pwszOut
    )
{
    size_t cbAnsi, cbWide;
    DWORD dwError;
 
    // If input is null then just return the same.
    if (pszIn == NULL) {
        pwszOut = NULL;
        return NOERROR;
    }
 
    cbAnsi = strlen(pszIn) + 1;
    cbWide = cbAnsi * sizeof(*pwszOut);
 
    pwszOut = (LPWSTR) CoTaskMemAlloc(cbWide);
    if (pwszOut == NULL)
        return E_OUTOFMEMORY;
 
    // Convert to wide.
    if (MultiByteToWideChar(CP_ACP, 0, pszIn, static_cast<int>(cbAnsi), pwszOut, static_cast<int>(cbAnsi)) == 0) {
        dwError = GetLastError();
        CoTaskMemFree(pwszOut);
        pwszOut = NULL;
        return HRESULT_FROM_WIN32(dwError);
    }
 
    return NOERROR;
}
 
HRESULT
AnsiToGuid(
    LPCSTR szString,
    GUID& gId
    )
{
    LPWSTR wszString = NULL;
    HRESULT hr = S_OK;
    char tmp[39];
 
    if (szString == NULL) {
        hr = E_INVALIDARG;
    }
 
    if (SUCCEEDED( hr )) {
        if (*szString != '{') {
            hr = StringCchPrintfA(tmp, 39, "{%s}", szString);
            if (hr == S_OK)
                szString = tmp;
        }
    }
 
    if (SUCCEEDED( hr )) {
        hr = AnsiToUnicode(szString, wszString);
    }
 
    if (SUCCEEDED( hr )) {
        hr = CLSIDFromString(wszString, &gId);
        CoTaskMemFree(wszString);
    }
 
    return hr;
}
 
LPSTR
GuidToAnsi(
    GUID& gId
    )
{
    WCHAR tmp[39];
    char *szRet;
 
    StringFromGUID2(gId, tmp, sizeof tmp / sizeof *tmp);
    UnicodeToAnsi(tmp, szRet);
     
    return szRet;
}
 
//
// Logging functions
//
 
// Logs a string to the kernel debugger
void
TraceMsg(
    LPCWSTR msg,
    ...
    )
{
    WCHAR buf[4096];
    va_list args;
    va_start( args, msg );
    HRESULT hr = S_OK;//StringCchVPrintf( buf, NELEMENTS( buf ), msg, args );
    va_end( args );
 
    if (hr == S_OK)
        OutputDebugStringW( buf );
}
 
void
LogEvent(
    LPCWSTR pFormat,
    ...
    )
{
    UNREFERENCED_PARAMETER( pFormat );
 
    // Log an event in the Windows Event Log using manifest-based events.
    // See documentation for "EventWrite" function on MSDN for more information.
}
 
