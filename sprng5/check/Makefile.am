SUBDIRS = lfg lcg lcg64 cmrg mlfg pmlcg .

bin_SCRIPTS = checksprng timesprng 
CLEANFILES = $(bin_SCRIPTS)
dist_data_DATA = insertlib gencheck gentime .insertlib
EXTRA_DIST = insertlib gencheck gentime .insertlib
CHKDIR = $(top_srcdir)/check

.NOTPARALLEL:

checksprng: gencheck
	cp $(CHKDIR)/insertlib ./insertlibcp
	cp ./insertlibcp ./insertlib
	rm ./insertlibcp
	rm -f $(CHKDIR)/checksprng
	chmod u+x $(CHKDIR)/gencheck
	@$(CHKDIR)/gencheck
	chmod u+x ./checksprng

timesprng: gentime
	rm -f $(CHKDIR)/timesprng
	chmod u+x $(CHKDIR)/gentime
	@$(CHKDIR)/gentime
	chmod u+x ./timesprng
