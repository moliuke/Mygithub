CC=$(CROSS_COMPILE)gcc

CFLAGS = -fno-stack-protector -I ./include -D DMP

TAR = modconfig
all += modconfig.o
all += ../../../config/conf.o

export CC
$(TAR): $(all)
	$(CC) $(CFLAGS) -o $(TAR) $(all) -m32 -march=i486 $(LDFLAGS) $(LIBS)
	cp $(TAR) /nfs/linuxVersion/
	$(RM) -f *.gch *.bak $(all) 
	
%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $< -m32 -march=i486

.PHONY: clean
clean:
	rm -rf $(TAR) $(all) 


