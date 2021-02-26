#子目录的Makefile直接读取其子目录就行
SUBDIRS=$(shell ls -l | grep ^d | awk '{print $$9}')
#以下同根目录下的makefile的相同代码的解释
#CUR_SOURCE=${wildcard *.c}
CUR_OBJS += img_png.o


all:$(clean) $(CUR_OBJS)

$(CUR_OBJS):%.o:%.c
	$(CC) -c $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@ $(CFLAGS) -march=i486 -m32 -D DMP -I /opt/openlib/pnglib/install/include/
ECHO:
	@echo $(SUBDIRS)

