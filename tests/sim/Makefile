ASM=../../asm
SIM=../../sim

test:
	./run-tests.sh all

%.asm.bin: %.asm
	${ASM} -i $< -o $@

%.asm.expect: %.asm.bin %.asm.script
	${SIM} -d -i $< -1 -i,../../nvram.dat < $*.asm.script > $@

remake: test_00.asm.expect test_01.asm.expect test_02.asm.expect \
        test_03.asm.expect test_04.asm.expect test_05.asm.expect \
        test_06.asm.expect test_07.asm.expect test_08.asm.expect \
        test_09.asm.expect test_10.asm.expect test_11.asm.expect \
        test_12.asm.expect test_13.asm.expect test_14.asm.expect \
        test_15.asm.expect test_16.asm.expect test_17.asm.expect \
        test_18.asm.expect                    test_20.asm.expect \
        test_21.asm.expect test_22.asm.expect test_23.asm.expect \
        test_24.asm.expect test_25.asm.expect test_26.asm.expect \
        test_27.asm.expect test_28.asm.expect test_29.asm.expect \
        test_30.asm.expect test_31.asm.expect test_32.asm.expect \
        test_33.asm.expect test_34.asm.expect test_35.asm.expect \
        test_36.asm.expect test_37.asm.expect test_38.asm.expect

clean:
	rm *.bin
