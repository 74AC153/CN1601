TESTS= \
	empty \
	or and nor xor \
	add sub mul div \
	uadd usub umul udiv \
	shl shr shra wshr \
	lui inci deci andi ori \
	shli shri shrai wshri \
	ba bal bz bnz \
	jr jlr \
	ldw stw ll sc \
	primes


default: $(patsubst %,%.bin,$(TESTS))

tests: $(patsubst %,%.test,$(TESTS))

#expect: $(patsubst %,%.expect,$(TESTS))

clean :
	rm *.bin simtest

%.bin : %.asm
	../asm -i $< -o $@

%.test : %.bin
	../sim -i $< -r > simtest
	diff simtest $*.expect

%.expect : %.bin
	../sim -i $< -r > $@

%.run : %.bin
	../sim -i $< -r
