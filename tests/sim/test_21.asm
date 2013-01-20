        ba %first
        li r0, #1
        li r0, #2
second: li r0, #0xFF
        li r0, #3
first:  bal %second
        li r0, #6
