#to be used on Wheeler without docker!
SHELL := /bin/bash
CC     = mpic++
CFLAGS = -Wall -I /users/sahba/wheeler-scratch/git/sprng5/include
LDFLAGS = -L /users/sahba/wheeler-scratch/git/sprng5/lib
LDLIBS = -lgmp -lgsl -lgslcblas -lm -lsprng

bsp_prototype: bsp_prototype.c
