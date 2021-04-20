#Simple makefile for libusb application

#PKG_CONFIG=/media/slash/500G/i.mx8/src/imx8mp/sdk/toolchain/sysroots/x86_64-pokysdk-linux/usr/bin/pkg-config
PKG_CONFIG=/media/slash/project/home/slash/project/DFI/IMX8MP/sdk/5.4-zeus/sysroots/x86_64-pokysdk-linux/usr/bin/pkg-config
OBJ=USB31_TX_COMPLIANCE.o

USB31_TX_COMPLIANCE: $(OBJ)
	$(CC) $(OBJ) -lusb-1.0 -o USB31_TX_COMPLIANCE

USB31_TX_COMPLIANCE.o: USB31_TX_COMPLIANCE.c
	$(CC) -O2 -c USB31_TX_COMPLIANCE.c `${PKG_CONFIG} --libs --cflags libusb-1.0`

clean :
	-rm *.o $(objects) *.exe USB31_TX_COMPLIANCE
