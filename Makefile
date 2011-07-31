CC=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/arm-apple-darwin10-gcc-4.2.1
CFLAGS=-Wall -framework CoreFoundation -framework Security -dynamiclib -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS4.3.sdk/ -fPIE

isslfix.dylib: isslfix.c
	$(CC) $(CFLAGS) -o $@ $^
	ldid -S $@

