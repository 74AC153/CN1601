	li r1, @globalvar.hi
	shlli r1, @globalvar.lo
	li r2, #0xbe
	shlli r2, #0xef
	stw r2, r1, #1
	ldw r0, r1, #1
	inci r3, #1
globalvar:
	.data #0xd00d
	.data #0xcafe
