#!/bin/bash
if [ $1 = "all" ]
then
	rm *.bin
	for i in test_*.asm;
	do
		echo $i
		../../asm -i $i -o $i.bin
		diff -c $i.expect <( ../../sim -i $i.bin -d < $i.script )
	done
else
	echo $1
	../../asm -i $1 -o $1.bin
	diff -c $1.expect <( ../../sim -i $1.bin -d < $1.script )
fi
