#子目录的Makefile直接读取其子目录就行
#SUBDIRS=$(shell ls -l | grep ^d | awk '{print $$9}')
#SUBDIRS:=$(patsubst cmd,,$(SUBDIRS))
#SUBDIRS:=$(patsubst list,,$(SUBDIRS))

SUBDIRS += ../../module/PNG

SUBDIRS += general
SUBDIRS += HebeiErQin
SUBDIRS += BoZhou
SUBDIRS += ZhuHaiProj
SUBDIRS += Malaysia
SUBDIRS += liandong
SUBDIRS += FuZhou
SUBDIRS += HeAo




#以下同根目录下的makefile的相同代码的解释
CUR_SOURCE=${wildcard *.c}

#CUR_SOURCE += $(filter-out SWR_init.c, $(CUR_SOURCE))
#CUR_SOURCE:=$(patsubst SWR_init.c,,$(CUR_SOURCE))


CUR_OBJS=${patsubst %.c, %.o, $(CUR_SOURCE)}
all:$(clean) $(SUBDIRS) $(CUR_OBJS)
$(SUBDIRS):ECHO 
	make -C $@
$(CUR_OBJS):%.o:%.c
	$(CC) -c $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@ $(CFLAGS) -march=i486 -m32 -D DMP
ECHO:
	@echo $(SUBDIRS)

