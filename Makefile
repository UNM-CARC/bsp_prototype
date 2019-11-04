SHELL := /bin/bash
CC     = mpic++
CFLAGS = -Wall
LDLIBS = -lgmp -lgsl -lgslcblas -lm -L /users/sahba/wheeler-scratch/git/sprng5/lib -lsprng

bsp_prototype: bsp_prototype.c
