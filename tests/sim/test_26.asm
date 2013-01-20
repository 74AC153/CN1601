	li r1, @globalvar.hi
	shlli r1, @globalvar.lo
	ldw r0, r1, #1
	li r2, #0xab
	shlli r2, #0xcd
globalvar:
	.data #0xd00d
	.data #0xcafe
