        li    r0,  @target.hi
        shlli r0, @target.lo
        jr    r0, #1
        li    r0, #2
target: li    r0, #3
        li    r0, #0xFF

