CC=gcc

CFLAGS=-g -Wall -Wextra -Wno-unused
LDFLAGS=-lpthread

default: instructions_test asm dasm sim index utils_test fifostdio

clean:
	rm *.o instructions_test asm dasm sim tags cscope.out

index: tags cscope.out

tags: *.c *.h
	rm -f tags
	ctags -d -t *.c *.h

cscope.out: *.c *.h
	rm -f cscope.out
	cscope -b

tests: instructions_test asm dasm tests/asm/asm_top.asm
	rm -f tests/asm/asm_top.bin tests/asm/asm_top.asm.cmp
	./asm -i tests/asm/asm_top.asm -o tests/asm/asm_top.bin
	./dasm -i tests/asm/asm_top.bin -o tests/asm/asm_top.asm.cmp
	diff -w tests/asm/asm_top.asm tests/asm/asm_top.asm.cmp
	./instructions_test

instr_table.o: instr_table.c instr_table.h
	${CC} ${CFLAGS} -c instr_table.c

instructions.o: instructions.c instructions.h
	${CC} ${CFLAGS} -c instructions.c

instructions_test.o: instructions_test.c
	${CC} ${CFLAGS} -c instructions_test.c

instructions_test: instructions.o instructions_test.o instr_table.o
	${CC} ${LDFLAGS} -o instructions_test instructions.o instructions_test.o instr_table.o

asm.o: asm.c instructions.h instr_table.h
	${CC} ${CFLAGS} -c asm.c

asm: asm.o instructions.o instr_table.o
	${CC} ${LDFLAGS} -o asm asm.o instructions.o instr_table.o

dasm.o: dasm.c instructions.h instr_table.h
	${CC} ${CFLAGS} -c dasm.c

dasm: dasm.o instructions.o instr_table.o
	${CC} ${LDFLAGS} -o dasm dasm.o instructions.o instr_table.o

sim_core.o: sim_core.c sim_core.h
	${CC} ${CFLAGS} -c sim_core.c

sim_memif.o: sim_memif.c sim_memif.h
	${CC} ${CFLAGS} -c sim_memif.c

sim_cp_if.o: sim_cp_if.c sim_cp_if.h
	${CC} ${CFLAGS} -c sim_cp_if.c

sim_cp_timer.o: sim_cp_timer.c sim_cp_timer.h
	${CC} ${CFLAGS} -c sim_cp_timer.c

sim_cp_nvram.o: sim_cp_nvram.c sim_cp_nvram.h
	${CC} ${CFLAGS} -c sim_cp_nvram.c

sim_cp_uart.o: sim_cp_uart.c sim_cp_uart.h
	${CC} ${CFLAGS} -c sim_cp_uart.c

sim.o: sim.c
	${CC} ${CFLAGS} -c sim.c

sim_utils.o: sim_utils.c sim_utils.h
	${CC} ${CFLAGS} -c sim_utils.c

sim_test: sim_test.o sim_core.o sim_utils.o
	${CC} ${LDFLAGS} -o sim_test sim_test.o sim_core.o sim_utils.o

sim: sim.o sim_core.o sim_utils.o instructions.o \
     instr_table.o utils.o circbuf.o sim_cp_if.o \
     sim_memif.o sim_cp_timer.o sim_cp_nvram.o \
     sim_cp_uart.o mbox.o
	${CC} ${LDFLAGS} -o sim \
	sim.o \
	sim_core.o \
	sim_utils.o \
	instructions.o \
	instr_table.o \
	utils.o \
	circbuf.o \
	sim_cp_if.o \
	sim_memif.o \
	sim_cp_timer.o \
	sim_cp_nvram.o \
	sim_cp_uart.o \
	mbox.o

sim_core_test.o: sim_core_test.c
	${CC} ${CFLAGS} -c sim_core_test.c

sim_core_test: sim_core_test.o sim_core.o instructions.o instr_table.o
	${CC} ${LDFLAGS} -o sim_core_test sim_core_test.o sim_core.o instructions.o instr_table.o

utils.o: utils.c utils.h
	${CC} ${CFLAGS} -c utils.c

circbuf.o: circbuf.c circbuf.h
	${CC} ${CFLAGS} -c circbuf.c

mbox.o: mbox.c mbox.h dlist.h
	${CC} ${CFLAGS} -c mbox.c

fifostdio: fifostdio.c
	${CC} -g -o fifostdio fifostdio.c
