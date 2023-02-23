#pragma once

#include <windows.h>
#include <iads.h>
#include <adshlp.h>
#include <dsrole.h>
#include <initguid.h>
#define MAX_LDAP_DN 512 // note: actual max is 4096

extern "C"
{

    // OLE32
    WINBASEAPI HRESULT WINAPI OLE32$CoUninitialize(void);
    WINBASEAPI HRESULT WINAPI OLE32$CoInitialize(void);
    WINBASEAPI HRESULT WINAPI OLE32$CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);

    // OLEAUT32
    WINBASEAPI void WINAPI OLEAUT32$SysFreeString(BSTR);

    // NETAPI32
    WINBASEAPI DWORD WINAPI NETAPI32$DsRoleGetPrimaryDomainInformation(LPCWSTR lpServer, DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel, PBYTE *Buffer);

    // MSVCRT
    WINBASEAPI int __cdecl MSVCRT$swprintf_s(wchar_t *buffer, size_t sizeOfBuffer, const wchar_t *format, ...);
    WINBASEAPI void __cdecl MSVCRT$memset(void *dest, int c, size_t count);
    
    // KERNEL32
    WINBASEAPI DWORD WINAPI KERNEL32$GetLastError(VOID);

    // ACTIVEDS
    WINBASEAPI HRESULT ACTIVEDS$ADsGetObject(LPCWSTR lpszPathName, REFIID riid, VOID **ppObject);

    VOID go(IN PCHAR Args, IN ULONG Length);
}