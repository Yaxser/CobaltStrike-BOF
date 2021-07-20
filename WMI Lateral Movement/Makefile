BOFNAME := ProcCreate
BOFNAME2 := EventSub
CC_x64 := x86_64-w64-mingw32-g++
CC_x86 := i686-w64-mingw32-g++

all:
	$(CC_x64) -o $(BOFNAME).x64.o -c WMI-ProcessCreate.cpp -w
	$(CC_x86) -o $(BOFNAME).x86.o -c WMI-ProcessCreate.cpp -w
	$(CC_x64) -o $(BOFNAME2).x64.o -c WMI-EventSub.cpp -w
	$(CC_x86) -o $(BOFNAME2).x86.o -c WMI-EventSub.cpp -w
