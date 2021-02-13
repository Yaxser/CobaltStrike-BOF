# ReadMe

## Compile Instructions 

```
git clone https://github.com/Yaxser/CobaltStrike-BOF.git
cd CobaltStrike-BOF/WMI\ Lateral\ Movement/
make
```

## Cobalt Instructions

Once the binaries are made (or use the pre-existing ones), load the aggressor (.cna) files into the cobalt through the script manager.

Right click on a beacon and you will see the options to use these.

You will need to be admins of your target for these to work. If you are using it on localhost make sure you are elevated. 

## Known Bugs

* ProcCreate cannot use the users current context. You will need to supply creds for it to work. Currently only plaintext creds work. 
* EventSub does not work in x86 beacons.
* Looking at code it may have issues with kerberos auth, it is currently untested.

## Credits
Credit to Yaxser for the original cpp files. 

Credit to Phil Keeble and Steve Embling for tieing in the aggressor, making them dynamic and making them more user friendly. 
