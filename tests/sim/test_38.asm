; set interrupt handler
	li r0, @timer_isr.lo
	mtctl r0, q16
; set countdown -- 6 cycles (3 instructions)
	li r1, #6
	mtcp r1, c0, x1
; enable timer interrupt
	mfctl r3, q24
	ori r3, #1
	mtctl r3, q24
; enable interrupts
	mfctl r2, q10
	orih r2, #0x40 ; set GIE
	mtctl r2, q10
; start timer
	mtcp r3, c0, x0
; wait for interrupt
	sleep

; mask all external interrupts
	li r0, #0
	mtctl r3, q24
	sleep ; halt

timer_isr:
; acknowledge interrupt
	cpop r3, c0
	li r7, #0xFF
	shlli r7, #0xFF
	rfi
