SHELL := /bin/bash
CC     = mpic++
CFLAGS = -Wall
LDLIBS = -lsprng -lgmp -lgsl -lgslcblas

bsp_prototype: bsp_prototype.c
