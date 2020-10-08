# CobaltStrike-BOF
Collection of beacon BOF written to learn windows and cobaltstrike


# 1 ) DCOM-Lateral-BOF.c
A quick PoC that uses DCOM (ShellWindows) for lateral movement. You will have to provide creds (username, password, and domain) on line 93-95 for it to work. If you wish to use the current user credentials, you can change line 110 to "authInfo->pAuthIdentityData = t2;", which is basically NULL. An aggressor script and a writeup (methodology) is on my to-do list... This is not meant to be used in production, it is just a PoC. A more useable version will land with the aggressor script.
      
      
      
# 2 ) after the write up...
