CC=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/arm-apple-darwin10-gcc-4.2.1
CFLAGS=-O3 -Wall -framework CoreFoundation -framework Security -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS4.3.sdk/ 

all: isslfix.dylib extrainst_ prerm

isslfix.dylib: isslfix.c blacklist.c
	$(CC) $(CFLAGS) -dynamiclib -o $@ $^
	ldid -S $@

extrainst_: extrainst_.m
	$(CC) $(CFLAGS) -framework Foundation -o $@ $^
	ldid -S $@

prerm: prerm.m
	$(CC) $(CFLAGS) -framework Foundation -o $@ $^
	ldid -S $@

clean:
	rm isslfix.dylib extrainst_ prerm

