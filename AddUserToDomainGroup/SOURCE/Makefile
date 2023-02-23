BOF := AddUserToDomainGroup
CC_x64 := x86_64-w64-mingw32-g++
STRIP_x64 := x86_64-w64-mingw32-strip

all:
	$(CC_x64) -o ../$(BOF).o -c $(BOF).cpp -Wno-write-strings 
	$(STRIP_x64) --strip-unneeded ../$(BOF).o

clean:
	rm ../$(BOF).o
