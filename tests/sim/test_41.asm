; setup interrupt handlers
	li r0, @handler.lo
	orih r0, @handler.hi
	mtctl r0, q18  ; q18 holds interrupt handler addr for CP2
	li r0, #4      ; bit 2 -- enable interrupts for CP2
	mtctl r0, q24  ; q24 is interrupt enable mask register
; setup uart
	li r0, #2 ; CP_UART_REG_CTL_RX_AVAIL_INT
	mtcp r0, c2, x0 ; x0 is CP_UART_REG_CTL
	li r0, #8 ; CP_UART_INSTR_FIFOS_ON
	cpop r0, c2
; setup sentinel value and enable interrupts
	xor r6, r6, r6
	mfctl r0, q10  ; q10 is status
	orih r0, #0x40 ; set GIE
	mtctl r0, q10
; wait for interrupt, exit when r6 is nonzero
wait:
	sleep
	bz r6, %wait
; exit -- unset GIE and sleep
	mfctl r2, q10
	andih r2, #0xBF ; unset GIE
	mtctl r2, q10
	sleep

handler:
; acknowledge interrupt
	li r0, #10 ; CP_UART_INT_ACK
	cpop r0, c2
; read character from uart
	li r0, #2 ; CP_UART_INSTR_RECV
	cpop r0, c2
	mfcp r0, c2, x2 ; x2 is CP_UART_REG_VAL
; check to see if we should terminate
	li r1, #0x71 ; ascii 'q'
	sub r2, r1, r0   ; if r0 == 0x71 ('q') : r6 = 1
	bnz r2, %notdone
	li r6, #1
	ba %reset
notdone:
; increment character value: 
	inci r0, #1
; send it back
	mtcp r0, c2, x2
	li r0, #1 ; CP_UART_INSTR_SEND
	cpop r0, c2
reset:
; reset rx interrupt
	li r0, #6 ; CP_UART_INSTR_RESET_RX
	cpop r0, c2
; return
	rfi
