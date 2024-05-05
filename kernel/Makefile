NAME := kernel
SRCS := kernel.c
export MODULE  := L0
export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
export ARCH := x86_64-qemu
NAME           := kernel
#SRCS           := framework/main.c $(shell find -L ./src/ -name "*.c")
INC_PATH       := include/ framework/
endif

include $(AM_HOME)/Makefile
include ../oslabs.mk
image: git
