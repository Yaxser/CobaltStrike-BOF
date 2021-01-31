# CobaltStrike BOF
Collection of beacon BOF.

# 1 ) DCOM Lateral Movement
A quick PoC that uses DCOM (ShellWindows) via beacon object files for lateral movement.You can either specify credentials or use the current user. To use the current user, just leave the domain, username, and password empty. A short article can be about using COM objects in C [can be found here](https://yaxser.github.io/CobaltStrike-BOF/).
      
# 2 ) WMI Lateral Movement - Win32_Process Create 
Similar concepts to the previous one, but an interesting learning experince. Code adopted from [CIA Vault 8](https://wikileaks.org/ciav7p1/cms/page_11628905.html). This method uses the class Win32_Process.

# 3 ) WMI Lateral Movement - Event Subscription
This one uses WMI events for lateral movement. Most of the heavy lifting was done by [wumb0in](https://wumb0.in/scheduling-callbacks-with-wmi-in-cpp.html)

# 4 ) On-demand C2
This is an implementation of an on-demand C2 using dotnet BOF. The beacon will enter a sleep state until an email with a given word (in subject or body) is provided. This way your beacon will only call home ONLY when you want it to call home. When the beacon calls home, it will call home with whatever sleep time configured in the malleable profile. When you are done you can run the BOF again and the beacon will sleep until you send another email. As an extra, the email with the given word will be deleted before the user get notified about it.

### Instructions
1 ) Download the BOF .NET project (cheers to [CCob](https://twitter.com/_EthicalChaos_) for the brilliant work!) and follow CCob's guide [here](https://github.com/CCob/BOF.NET) to load the the dll into the beacon

2 ) Execute it using the following: bofnet_execute On_Demand_C2_BOF.OnDemandC2Class subject COVlD-19*

3 ) Now, to have a callback from your beacon, you can send an email like
[SUSPECTED SPAM] The new COVlD-19 update

The email will directly get redirected to Deleted Items and beacon will be calling home again! 

*This is a COVLD-19 with a small L to ensure uniqueness

You can also specify the body option like the following
bofnet_execute On_Demand_C2_BOF.OnDemandC2Class body "it can also be a sentence!"

If you do so, the application will only trigger if the body of an email contains "it can also be a sentence!". However, this method will cause the prompt "a program is trying to access email address information" each time an email is received, so I recommend to use it ONLY IF you know that this feature is disabled (it is quite common to see it disabled in Enterprises, but an additional OPSEC never hurts).



## Why are you doing this?
I ported these techniques to BOF in order to learn more about Windows, CobaltStrike, and lateral movement. I have a curiosity that copy/pasting powershell commands is killing.

## Did you write this from the ground up?
The DCOM lateral movement took sometime to figure out, and I did not find it done in other projects/repos. However, the WMI lateral movement parts are mainly done by others. What I did was minor modifications and porting it to BOF.

## Note on quality
I am not a seasoned developer yet, so use with care. Before pushing these scripts to GIT, they were tested on an Enterprise environment where a network MDR service is provided, and no alerts were trigged. However, it goes without saying that you should modify and test the scripts before you run them in your engagements. If you need assistant, please do not hesistate to contact me. Also, if you are interested in having aggressor scripts for these BOF, please lemme know!

## Can I/We reach you?
Yes, on[Twitter](https://twitter.com/Yas_o_h) or by [email](mailto:Y.Alhazmi@student.fontys.nl)

## Can I/We help you?
Yes, with a star, a retweet, or by inviting me to your Red Team after I graduate from uni.

## Acknowledgement
Big thanks to [rsmudge](https://github.com/rsmudge) for his cintinous support and responsiveness to questions. The articles by [domchell](https://github.com/dmchell) served as a great introduction and helped in shaping my priorities.
