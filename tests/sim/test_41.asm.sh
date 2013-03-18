#!/bin/sh
mkfifo fifo_miso fifo_mosi
../../sim -i test_41.asm.bin -2 -r,fifo_miso,-t,fifo_mosi &
rm test_41.asm.out
cat -u fifo_mosi > test_41.asm.out &
sleep 2
/bin/echo -n asdfq > fifo_miso
sleep 2
diff test_41.asm.out.expect test_41.asm.out
