# AddUserToDomainGroup BOF
A BOF tool that can be used to add a user to a domain group using IADsGroup COM interface

## How to compile
1. Make sure that Mingw-w64 g++  has been installed (`apt install g++-mingw-w64-x86-64`).
2. Enter the SOURCE folder within the tool folder.
3. Type "make" to compile the object files.
4. Use Cobal Strike script manager to import the `AddUserToDomainGroup.cna` script.

## Usage
Running the tools is straightforward. Once you imported the CNA script using Cobalt Strike's Script Manager, they are available as Cobalt Strike commands that can be executed within a beacon. This tools supports the following command:
* `AddUserToDomainGroup <username (samAccountName)> <groupname>`

## Example
`AddUserToDomainGroup "TestUser" "Domain Admins"`

## Limitations
This BOF cannot be used to add users to local groups. If you want a BOF that can add users to local and domain groups [check the remote-ops BOF repo by TrustedSec](https://github.com/trustedsec/CS-Remote-OPs-BOF/tree/main/src/Remote/addusertogroup).

## Support
This BOF tool has been successfully compiled on Mac OSX systems and used on Windows 8.1+ (x64) systems. Compiling the BOF code should also work on other systems (Linux, Windows) that have the Mingw-w64 compiler installed.

## Credits
This very nice documentation and project structure is taken from [Outflank's C2 Tool Collection](https://github.com/outflanknl/C2-Tool-Collection) and I always use TrustedSec's [bofdefs.h](https://github.com/trustedsec/CS-Remote-OPs-BOF/blob/main/src/common/bofdefs.h) for the dynamic function resolution definitions.
