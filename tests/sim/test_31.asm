; test dropping into user mode and then back into kernel mode
	li r0, @kernelprog.hi
	li r0, @kernelprog.lo
	mtctl r0, q13 ; setup swi handler
	
	li r1, @userprog.hi
	li r1, @userprog.lo
	mtctl r1, q8 ; setup return address for rfi (epc)
	
	li r3, #0x20
	shlli r3, #0x00
	mfctl r2, q10
	or r2, r2, r3
	mtctl r2,q10 ; set return to user mode bit in status register

	rfi ; drop into user mode

kernelprog:
	and r5, r5, r5
	li r5, #0xD0
	shlli r5, #0x0D

userprog:
	xor r4, r4, r4
	li r4, #0xca
	shlli r4, #0xfe
	int #0x111
