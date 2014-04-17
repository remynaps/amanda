exename = ama

objfiles = amcheck.o amcon.o amerror.o ameval.o amio.o amlex.o amlib.o ammem.o ammodify.o amparse.o ampatter.o amprint.o amstack.o amsyslib.o amtable.o

compiler = gcc

.c.o:
	$(compiler) -c -DAMA_READLINE -O6 $*.c

exe: $(objfiles)
	$(compiler) -O6 $(objfiles) -lm -lreadline -o $(exename)

clean:
	rm *.o
