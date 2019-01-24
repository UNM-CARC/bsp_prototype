CC    =mpic++
LOCAL =$(HOME)/.local
LIBS  =$(LOCAL)/lib
INC   =$(LOCAL)/include
TMP_FLAG= -lclog

all:
	 $(CC) appGen.c -I$(INC) -L$(LIBS) -lsprng -lgmp -lgsl -lgslcblas -o app_gen_trace
	 $(CC) appGen.c -I$(INC) -L$(LIBS) -lsprng -lgmp -lgsl -lgslcblas -o app_gen_metrics -DUSE_METRICS
	 $(CC) appGen.c -I$(INC) -L$(LIBS) -lsprng -lgmp -lgsl -lgslcblas -o app_gen

clean:
	rm app_gen app_gen_trace app_gen_metrics
