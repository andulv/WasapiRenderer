using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Security;

using DirectShowLib;

namespace WasapiRendererFilter.Sample
{
    [ComVisible(true), ComImport, SuppressUnmanagedCodeSecurity,
     Guid("0B752BCC-EFEA-4004-A0C3-1B1A0571E75A"),
     InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IWasapiRendererFilter
    {
        [PreserveSig]
        int GetWasapiMixFormat(ref IntPtr pFormat);
        [PreserveSig]
        int GetExclusiveMode(ref bool pIsExclusive);
        [PreserveSig]
        int SetExlusiveMode(bool IsExclusive);
        [PreserveSig]
        int GetActiveMode(ref int pMode);
    }

    public static class WasapiRendererFilter
    {
        public static Guid ClsId = new Guid("{202F2BEE-B160-40ac-8BC1-30B6456DED61}");
    }
}


