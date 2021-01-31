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
