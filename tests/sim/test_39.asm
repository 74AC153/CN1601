; set interrupt handler
	li r0, @handler.lo
	orih r0, @handler.hi
	mtctl r0, q17
; enable nvram interrupt
	li r0, #2
	mtctl r0, q24
; enable interrupts
	mfctl r2, q10
	orih r2, #0x40 ; set GIE
	mtctl r2, q10
; configure nram controller
	li r0, #3
	mtcp r0, c1, x0
; set write address
	li r1, #0xAA
	orih r1, #0xAA
	mtcp r1, c1, x2
	li r2, #0x1
	mtcp r2, c1, x3
; set write value
	li r3, #0xFE
	orih r3, #0xCA
	mtcp r3, c1, x4
; perform write
	li r4, #2
	cpop r4, c1
; wait for interrupt
	sleep
; perform read
	; change val register first
	li r3, #0xAA
	orih r3, #0xAA
	mtcp r3, c1, x4
	; perform read
	li r5, #1
	cpop r5, c1
; wait for interrupt
	sleep
; copy read value
	mfcp r6, c1, x4
; end here
; interrypt handler
handler:
	; acknowledge interrupt
	li r0, #3 ; NVRAM_IACK
	cpop r0, c1
	rfi
