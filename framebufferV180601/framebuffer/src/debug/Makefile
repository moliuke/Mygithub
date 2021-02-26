OBJS=*.o
ODIR=obj
$(ROOT_DIR)/$(BIN_DIR)/$(BIN):$(ODIR)/$(OBJS) 
	$(CC) -o $@ $^ -march=i486 -m32 $(LDFLAGS) $(LIBS)
	#cp $(ROOT_DIR)/$(BIN_DIR)/$(BIN) /nfs/linuxVersion