SHELL := /bin/bash
CC     = mpic++
CFLAGS = -Wall
LDLIBS = -lsprng -lgsl -lgslcblas -lm

bsp_prototype: bsp_prototype.c
