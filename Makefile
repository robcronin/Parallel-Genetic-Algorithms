LIB = -lm


.PHONY: clean psparallel pdserial string rps

pdparallel: prisoners_parallel.c genetic.h
	mpicc $^ $(LIB) -o $@.out


clean :
	rm -f pdparallel.out pdserial.out string.out rps.out genetic.h.gch rpsgenetic.h.gch result.dat rock.dat scissor.dat paper.dat

pdserial: prisoners_serial.c genetic.h
	gcc $^ $(LIB) -o $@.out

string: string.c genetic.h
	gcc $^ $(LIB) -o $@.out

rps: serialrps.c rpsgenetic.h
	gcc $^ $(LIB) -o $@.out

test_master: pdparallel
	mpirun -n 4 ./pdparallel.out -p 120

test_island: pdparallel
	mpirun -n 4 ./pdparallel.out -i -p 120

test_string: string
	./string.out

test_serial: pdserial
	./pdserial.out

test_rps: rps
	./rps.out
