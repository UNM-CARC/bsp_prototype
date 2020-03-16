SHELL := /bin/bash
CC     = mpic++
CFLAGS = -Wall
LDLIBS = -lsprng -lgsl -lgslcblas -lm -lrabbitmq 

bsp_prototype: bsp_prototype.c amqp_producer.c amqp_producer.h
