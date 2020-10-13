# CobaltStrike-BOF
Collection of beacon BOF written to learn windows and cobaltstrike. The DCOM lateral movement took sometime to figure out, and I did not find it done in other projects/repos. However, the WMI lateral movement (both Win32_Process.Create() and Event subscription are mainly done by others. What I did was minor modifications and porting it to BOF).


# 1 ) DCOM Lateral Movement
A quick PoC that uses DCOM (ShellWindows) via beacon object files for lateral movement.You can either specify credentials or use the current user. To use the current user, just leave the domain, username, and password empty. A short article can be about using COM objects in C [can be found here](https://yaxser.github.io/CobaltStrike-BOF/).
      
      
 
# 2 ) WMI Lateral Movement. 
Similar concepts to the previous one, but an interesting learning experince. Code adopted from [CIA Vault 8](https://wikileaks.org/ciav7p1/cms/page_11628905.html)
