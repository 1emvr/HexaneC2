# HexaneC2
## Overview:
This framework is a compilation of every publicly available resource I could think of with some very minimal UI/UX features. A good portion of the code is derived from https://github.com/HavocFramework/Havoc.

The idea is to lay the groundwork for custom implementations and experiment with new features later on. This is not a production-ready C2 and I do not recommend using it in any real environments. 

There are plenty of IOCs that are intrinsic to the methods applied (if you know them, you can change it) and network communication is completely naked at the moment.
Not many EDRs (that I know of) have the capabilities of finding these indicators but an experienced blue team lead may find it. :^)

## TODO:
### Priorities:
(implant)
- testing implant P2P communication/ fixing protocol
- testing COFF loader
- re-implement generic thread stack spoofing/ sleepobf
- initial callback: get ETW-TI/kernel event options
- redirector rotation 
- hot-swap configs (sleep/redirectors)
- write documentation

### C2 infrastructure:
#### Redirectors:
- re-implement http listener
- automation of infraC2
- protocol support (do not break)
- forking non-C2 traffic to webpage (filtering)
- access control/ filtering
- forwarding through reverse proxy (nginx)
- (optional) "LetsEncrypt"
- (optional) multiple egress profiles

#### C2 Server:
- SSH connection
- save states db
- staging

### Wish-List:
- implement dll manual mapping: https://github.com/bats3c/DarkLoadLibrary
- implement indirect syscalls/proxying through kernbase/kern32
- implement COFF data cache
- server logging
