TESTS=test0 test1 test2 test3 test4
#TESTS=test2


TESTFILES=${TESTS:%=%.json}
RESULTS=${TESTS:%=%.out}

SRC=jsontest.c json.c ConvertUTF.c

all:: tests

tests: jsontest ${TESTSFILES}
	for t in ${TESTS} ; do \
	    echo "Test: $${t}" ; \
	    ./jsontest $${t}.json > $${t}.out ; \
	    diff -wbB $${t}.json $${t}.out ; \
	done

jsontest: ${SRC}
	gcc -g -Wall -o jsontest ${SRC}

clean::
	rm -f jsontest *.o *.out

g:: jsontest
	gdb --args ./jsontest ${TESTS}
