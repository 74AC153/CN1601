; test sw interrupt handling
	li r0, @isr.hi
	shlli r0, @isr.lo
	mtctl r0, q13
	int #0x123
	li r3, #0xca
	shlli r3, #0xfe
isr:
	xor r2, r2, r2
	li r1, #0xD0
	shlli r1, #0x0D
	rfi
