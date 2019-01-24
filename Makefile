SHELL := /bin/bash
CC    = mpic++
PY    = python
SUB   = qsub
LOCAL = $(HOME)/.local
LIBS  = $(LOCAL)/lib
INC   = $(LOCAL)/include
DAT   = $(HOME)/cdse2018/data
APP   = $(DAT)/scripts/app_gen

TMP_FLAG = -lclog

all:
	$(CC) $(APP)/appGenNew.c -I$(INC) -L$(LIBS) -lsprng -lgmp -lgsl -lgslcblas -o $(APP)/app_gen_trace
	$(CC) $(APP)/appGenNew.c -I$(INC) -L$(LIBS) -lsprng -lgmp -lgsl -lgslcblas -o $(APP)/app_gen_metrics -DUSE_METRICS
	$(CC) $(APP)/appGenNew.c -I$(INC) -L$(LIBS) -lsprng -lgmp -lgsl -lgslcblas -o $(APP)/app_gen

dat:
	$(PY) $(DAT)/data_gen.py

run: all
	$(SUB) $(DAT)/batch/simple.pbs

clean:
	rm $(APP)/app_gen $(APP)/app_gen_trace $(APP)/app_gen_metrics
