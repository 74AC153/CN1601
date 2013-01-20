	li r0, @read.hi
	shlli r0, @read.lo
	li r1, @write.hi
	shlli r1, @write.lo
	ll r2, r0, #0
	sc r2, r1, #0
	inci r3, #1
read:
	.data #0xcafe
write:
	.data #0xf00d
