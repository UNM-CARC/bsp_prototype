SHELL := /bin/bash
CC     = mpic++
HPCLINK = hpclink
CFLAGS = -lsprng -lgmp -lgsl -lgslcblas
PY     = python
SUB    = qsub
LOCAL  = $(CURDIR)/.local
DAT    = $(CURDIR)
LIBS   = $(LOCAL)/lib
INC    = $(LOCAL)/include
APP    = $(DAT)/scripts/app_gen
NODES  = 2

TMP_FLAG = -lclog

all:
	$(CC) $(APP)/appGenNew.c -g -I$(INC) -L$(LIBS) $(CFLAGS) -o $(APP)/app_gen_trace
	$(CC) $(APP)/appGenNew.c -g -I$(INC) -L$(LIBS) $(CFLAGS) -o $(APP)/app_gen_metrics -DUSE_METRICS
	$(CC) $(APP)/appGenNew.c -g -I$(INC) -L$(LIBS) $(CFLAGS) -o $(APP)/app_gen

dat:
	$(PY) $(DAT)/data_gen.py $(NODES)

run: all
	$(SUB) $(DAT)/batch/simple.pbs

clean:
	rm $(APP)/app_gen $(APP)/app_gen_trace $(APP)/app_gen_metrics
cleantmp:
	rm -rf /tmp/results/
