
#############################Compiler Tools Setting
CC := /usr/bin/gcc
PRJ_HOME := $(shell pwd)

#############################source file
DIRSRC := $(wildcard *.c ./src/*.c)
SRCS := $(notdir $(DIRSRC))
#include file
INC := $(PRJ_HOME)/inc
#KERNEL_INCLUDE := /usr/src/linux-headers-2.6.32-5-common/include

#object file
OBJS := $(patsubst %.c,%.o,$(SRCS))
#OBJS := DPU_test.o linkLayer.o
#out file
OUTDIR := $(PRJ_HOME)/out
TEMPDIR:=$(PRJ_HOME)/tempdir
#############################complie flags
CFLAGS := -Wall -O -g
CFLAGS += -I ${INC}

LDFLAGS:=-L /usr/local/lib -ljpeg
#CFLAGS += -I ${KERNEL_INCLUDE}
#CFLAGS += -I /usr/src/linux-headers-2.6.32-5-common/arch/x86/include
##############################execute file
TARGET := start-dmp
all:$(TARGET)
$(TARGET):$(OBJS)	
	$(CC) -o $@ $(OBJS)	$(LDFLAGS)
	rm -f $(OBJS)
	mv -f $(TARGET) $(OUTDIR)/$(TARGET)
#$(OBJS):$(DIRSRC)
#	$(CC) -c $(CFLAGS) -o $@ $<
#load-urls.o:$(PRJ_HOME)/src/main.c
#	$(CC) -c $(CFLAGS) -o $@ $<
main.o:$(PRJ_HOME)/src/main.c
	$(CC) -c $(CFLAGS) -o $@ $<
	
.PHONY:clean
clean:
	rm $(OUTDIR)/*
