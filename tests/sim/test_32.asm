; timer testing -- ensure enable, disable decrement, and ext interrupt
; set interrupt handler
	li r0, @timer_isr.lo
	mtctl r0, q16
; set countdown -- 6 cycles (3 instructions)
	li r1, #6
	mtcp r1, c0, x1
; enable timer interrupt
	li r3, #1
	mtctl r3, q24
; enable interrupts
	mfctl r2, q10
	orih r2, #0x40 ; set GIE
	mtctl r2, q10
; reset counter & enable timer
	xor r6, r6, r6
	mtcp r3, c0, x0
; wait for interrupt
wait:
	inci r6, #1
	ba %wait

timer_isr:
; acknowledge interrupt
	cpop r3, c0
	li r7, #0xFF
	shlli r7, #0xFF
