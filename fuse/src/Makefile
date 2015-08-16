RANLIB = ranlib
AR = ar
CC = gcc
# OPTIMIZE_FLAGS = -O69
DEBUG_FLAGS = -g -DDEBUG
CFLAGS = -Wall -Wstrict-prototypes $(OPTIMIZE_FLAGS) $(DEBUG_FLAGS)
CPPFLAGS = $(DEBUG_FLAGS) -D_FILE_OFFSET_BITS=64 -DTEST_FUSE
LDFLAGS = $(OPTIMIZE_FLAGS) $(DEBUG_FLAGS)


TARGETS = dfs
SRCS = async_pclose.cpp berlekamp.cpp command.cpp debug.cpp dev_cmd.cpp \
	dfs_file.cpp dfs_fuse.cpp distribute.cpp download.cpp download_buffer.cpp \
	galois.cpp galois_table.cpp merge_split.cpp parse_conf.cpp rs.cpp \
	rs_config.cpp updown_command.cpp upload.cpp
LIBS = -lfuse -lpthread

all: $(TARGETS)

$(TARGET_LIB):
	make -C $(TARGET_LIB_DIR) $(TARGET_LIB)
	cp $(TARGET_LIB_DIR)/$(TARGET_LIB) .

OBJS = async_pclose.o berlekamp.o command.o debug.o dev_cmd.o \
	dfs_file.o dfs_fuse.o distribute.o download.o download_buffer.o \
	galois.o galois_table.o merge_split.o parse_conf.o rs.o \
	rs_config.o updown_command.o upload.o


dfs: $(OBJS)
	g++ -g -o dfs $(OBJS) $(LIBS) 

clean:
	rm -f *.o $(TARGETS)
	rm -f cache/*

tag:
	ctags *cpp *h 

dep:
	makedepend $(SRCS) #I/usr/include/linux
