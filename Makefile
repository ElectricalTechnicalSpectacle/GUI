PRG      = qdppa
ARCH     = intel
CC       = gcc
CC_ARM   = arm-linux-gnueabi-gcc
CFLAGS   = `pkg-config --cflags gtk+-3.0` -g -std=c99 -Wall
LDFLAGS  = `pkg-config --libs gtk+-3.0` -g 
IP_BGLBN = 10.0.0.6

all: intel

intel: intel.o
	$(CC) $(LDFLAGS) -o intel-$(PRG) $(PRG).o intel-libcsv/libcsv.a
	
arm: arm.o
	$(CC_ARM) $(LDFLAGS) -o arm-$(PRG) $(PRG).o arm-libcsv/libcsv.a

intel.o: $(PRG).c csv.h
	$(CC) $(CFLAGS) -c $(PRG).c

arm.o: $(PRG).c csv.h
	$(CC_ARM) $(CFLAGS) -c $(PRG).c

send:
	sshpass -p 'temppwd' scp arm-$(PRG) debian@$(IP_BGLBN):/home/debian/bin

clean:
	rm -f *.o *~ $(PRG)

