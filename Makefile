SHELL := /bin/bash
CC     = mpic++
LDLIBS = -lsprng -lgmp -lgsl -lgslcblas

bsp_prototype: bsp_prototype.c
