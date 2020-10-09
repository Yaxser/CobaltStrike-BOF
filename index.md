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


```C

```

### 3.  Finding the IID's
Finding IID’s is not so different from finding CLSID’s, lucky us. However, sometimes the tricky part is to find the right interface. What we want to do is porting the following line to C.
```powershell
$hb.Document.ActiveView.ExecuteShellCommand('cmd',$null,'/c echo Haboob > C:\hb.txt','7')
```
A good place to start is `OleView .NET.` Let’s see which interfaces are exposed in the MMC20.Application class. The supported interfaces tab will show you which interfaces are exposed by MMC20.Application class. 

![Finding-IIDs](https://github.com/Yaxser/CobaltStrike-BOF/blob/gh-pages/images/supported_ifc.png)

As you can see, the `Document` and `ActiveView` are not present so we cannot access their methods directly. So what and where are they? It turned out that the Document and ActiveView interfaces are stored as properties. Actually, ActiveView is not the name of the interface. The name of the interface is `View`, as you can see below. We already established that these interfaces are not accessible directly, so we need to retrieve them as properties. The `Document` property is stored inside the `Application` interface, and the `ActiveView` (i.e. `View` interface) property is stored inside `Document` the document interface. So, the interface we would like to retrieve first is the `Application` interface.

![IIDsAsProperties](https://github.com/Yaxser/CobaltStrike-BOF/blob/gh-pages/images/Application_ifc.png)



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
