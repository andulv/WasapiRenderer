#include <streams.h>
#include <initguid.h>
#include "stdafx.h"
#include <stdio.h>
#include "WasapiRendererFilterUids.h"

////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

//
// DllRegisterSever
//
// Handle the registration of this filter
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer


//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}


void DebugPrintf(const wchar_t *str, ...)
{
  wchar_t buf[2048];

  va_list ptr;
  va_start(ptr,str);
  vswprintf(buf,str,ptr);

  OutputDebugString(buf);
}