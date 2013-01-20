;
; count the number of primes up to 100
;

; r7 <- 0 ; the sum
; r0 main counter -> 2 upto 100
;   r1 "divisor" counter -> 1 to r0
;      r2 <- r0 - r1
;      while sign_bit(r2) = '0'
;         if r2 = 0
;            r7 <- r7 + 1
;         r2 <- r2 - r1

ori r0, #2                             ; 0
ori r3, #100                           ; 1
lui r5, #2^7                           ; 2
ori r1, #1                             ; 3

loop1:
    loop2:

        sub r2, r0, r1                 ; 4

        loop3:

            and r4, r2, r5             ; 5
            bnz r4, %loop3_e           ; 6
            xor r4, r2, r6             ; 7
            bnz r4, %else              ; 8
                inci r7, #1            ; 9
                ba %loop2_e            ; 10 
            else:
            sub r2, r2, r1             ; 11
            ba %loop3                  ; 12

        loop3_e:
        inci r1, #1                    ; 13
        xor r4, r1, r0                 ; 14
        bnz r4, %loop2                 ; 15
 
    loop2_e:
    xor r1, r1, r1                     ; 16
    ori r1, #2                         ; 17
    xor r4, r3, r0                     ; 18
    inci r0, #1                        ; 19
    bnz r4, %loop1                     ; 20 

; signal finish in r6
ori r6, #ff                            ; 21
sub r7, r3, r7                         ; 22
; lock up the processor
;ba #0                                  ; 23

