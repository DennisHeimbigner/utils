TESTS=test0 test1 test2 test3 test4
#TESTS=test2


TESTFILES=${TESTS:%=%.rc}
RESULTS=${TESTS:%=%.out}

SRC=rcontest.c rcon.c
OBJ=${SRC:%.c=%.o}

all: ${OBJ}

tests: rcontest ${TESTSFILES}
	for t in ${TESTS} ; do \
	    echo "Test: $${t}" ; \
	    ./rcontest $${t}.rc > $${t}.out ; \
	    diff -wbB $${t}.rc $${t}.out ; \
	done

rcontest: ${OBJ}
	gcc -g -Wall -o rcontest ${OBJ}

${OBJ}: ${SRC}
	gcc -g -Wall -c ${SRC}

clean::
	rm -f rcontest *.o *.out

g:: rcontest
	gdb --args ./rcontest ${TESTS}
