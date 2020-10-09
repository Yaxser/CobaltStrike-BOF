# Exploiting (D)COM in C. CobaltStrike BOF as PoC.
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
HRESULT CoInitialize(NULL);
```

### 2.  Find the CLSID for MMC20.Application class
There are many ways to find the CLSID for a given class. The easiest way is to use Google. You can also find a CLSID programmatically using CLSIDFromProgID function.
```C
CLSID clsid;
HRESULT CLSIDFromProgID( L”MMC20.Application”, &clsid);
```
Another way to find the CLSID is using OleView .NET from James Forshaw. It is an excellent tool to inspect COM objects. You can explore ProgIDs and filter for MMC20 and copy the GUID. This tool has by no doubt much more to offer than just copying GUIDs.

![Finding-CLSID](https://github.com/Yaxser/CobaltStrike-BOF/blob/gh-pages/images/MMC20CLSID.png)

Now, if you obtain the GUID as a string, you have to convert it to a CLSID using CLSIDFromString. Note the curly brackets, they are necessary.

```C
wchar_t *MMC20_CLSID = L”{49B2791A-B1AE-4C90-9B8E-E860BA07F889}”
CLSID clsid;
hr = CLSIDFromString(MMC20_CLSID, &clsid);
```




### 3.  Finding the IID's
Finding IID’s is not so different from finding CLSID’s, lucky us. However, sometimes the tricky part is to find the right interface. What we want to do is porting the following line to C.
```powershell
$obj.Document.ActiveView.ExecuteShellCommand('cmd',$null,'/c calc.exe','7')
```
A good place to start is `OleView .NET.` Let’s see which interfaces are exposed in the MMC20.Application class. The supported interfaces tab will show you which interfaces are exposed by MMC20.Application class. 

![Finding-IIDs](https://github.com/Yaxser/CobaltStrike-BOF/blob/gh-pages/images/supported_ifc.png)

As you can see, the `Document` and `ActiveView` are not present so we cannot access their methods directly. So what and where are they? It turned out that the Document and ActiveView interfaces are stored as properties. Actually, ActiveView is not the name of the interface. The name of the interface is `View`, as you can see below. We already established that these interfaces are not accessible directly, so we need to retrieve them as properties. The `Document` property is stored inside the `Application` interface, and the `ActiveView` (i.e. `View` interface) property is stored inside `Document` the document interface. So, the interface we would like to retrieve first is the `Application` interface. From there, we will get access to the Document and View interfaces without needing their IID. 

![IIDsAsProperties](https://github.com/Yaxser/CobaltStrike-BOF/blob/gh-pages/images/Application_ifc.png)


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

![CLSCTX](https://github.com/Yaxser/CobaltStrike-BOF/blob/gh-pages/images/CLSCTX.png)

```C
IDispatch *ApplicationIfc
hr = CoCreateInstance(&clsid, NULL, CLSCTX_LOCAL_SERVER, &ApplicationIID, (void**)&ApplicationIfc);
```
If this call succeeded, we will retrieve an MMC20.Application object that allows us to access the Application interface via `ApplicationIfc`.


### 5. Getting the pointers to Document and View interfaces 
The call to `CoCreateInstance` stored a pointer to the Application interface in the `ApplicationIfc` variable. Now, we will use this pointer to retrieve the `Document` property. The document property is actually an interface, but since it is not exposed to us, we cannot use the typical (`QueryInterface`) method to retrieve it. However, we can retrieve it using the method named `Document` from the application interface, which will store a pointer to the Document interface in the pointer we pass to it. The picture from the OleView (by Microsoft) shows the prototype for that function.

![DocumentIfc](https://github.com/Yaxser/CobaltStrike-BOF/blob/gh-pages/images/Document.png)

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
ApplicationIfc->lpVtbl->Release(ApplicationIfc->lpVtbl); //We must release whatever we acquire to keep the system clean
}

```

The `VARIANT` type has two members. The first member is `vt`, which identifies the *v*ariant *t*ype, and the second member is the variant itself. So, in our case the first member (vt) will be `VT_DISPATCH`, and the second member will be `pdispVal` (pointer to IDispatch interface, in our case the Document interface). In other words, our document interface will be now located at `vDocIfc->pdispVal`.

Now that we have obtained a pointer to the document interface, let’s obtain a pointer to the View interface. The call is identical to the previous one, we just change the variables. But… what if we did not want to hardcode the method dispatch ID? What if we knew that we wanted to call a function named ActiveView but did not want to open OleView to find its number? Easy. If we know the name of the method, we can use the function `GetIDsOfNames()` to retrieve its dispatch ID.


```C

```


---

You can use the [editor on GitHub](https://github.com/Yaxser/CobaltStrike-BOF/edit/gh-pages/index.md) to maintain and preview the content for your website in Markdown files.

Whenever you commit to this repository, GitHub Pages will run [Jekyll](https://jekyllrb.com/) to rebuild the pages in your site, from the content in your Markdown files.

### Markdown

Markdown is a lightweight and easy-to-use syntax for styling your writing. It includes conventions for

```markdown
Syntax highlighted code block

# Header 1
## Header 2
### Header 3

- Bulleted
- List

1. Numbered
2. List

**Bold** and _Italic_ and `Code` text

[Link](url) and ![Image](src)
```

For more details see [GitHub Flavored Markdown](https://guides.github.com/features/mastering-markdown/).

### Jekyll Themes

Your Pages site will use the layout and styles from the Jekyll theme you have selected in your [repository settings](https://github.com/Yaxser/CobaltStrike-BOF/settings). The name of this theme is saved in the Jekyll `_config.yml` configuration file.

### Support or Contact

Having trouble with Pages? Check out our [documentation](https://docs.github.com/categories/github-pages-basics/) or [contact support](https://github.com/contact) and we’ll help you sort it out.



```C
HRESULT CoInitialize( LPVOID pvReserved );
```
