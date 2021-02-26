CC=$(CROSS_COMPILE)gcc
TAR = bootup

all += main.o
all += user/cpy.o
all += user/def_list.o
all += conf.o
all += net.o
all += mtime.o
all += file_permis.o
all += appOnline.o
all += dev/gpio.o
all += dev/wdt.o
all += dev/Dev_serial.o


#$(TAR): $(all)
#	$(CC) -o $(TAR) $(all) -m32 -march=i486 $(LDFLAGS)
#	cp $(TAR) /nfs/linuxVersion/
#clean:
#	rm -rf $(TAR) $(all) 


LIBS += -pthread 

CFLAGS = -fno-stack-protector -I ./include 
CFLAGS += -fgnu89-inline

export CC
$(TAR): $(all)
	$(CC) $(CFLAGS) -o $(TAR) $(all) -m32 -march=i486 $(LIBS)
	#cp $(TAR) /nfs/linuxVersion/
	$(RM) -f *.gch *.bak $(all) 
	
%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $< -m32 -march=i486

.PHONY: clean
clean:
	rm -rf $(TAR) $(all) 
	

