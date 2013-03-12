#!/bin/bash
if [ $1 = "all" ]
then
	rm *.bin
	for i in test_*.asm;
	do
		echo $i
		../../asm -i $i -o $i.bin

		if [ -e $i.sh ]
		then
			./$i.sh
		else
			diff -c $i.expect <( ../../sim -i $i.bin -d -1 -i,../../nvram.dat < $i.script )
		fi

	done
else
	echo $1
	../../asm -i $1 -o $1.bin

	if [ -e $i.sh ]
	then
		./$i.sh
	else
		diff -c $1.expect <( ../../sim -i $1.bin -d -1 -i,../../nvram.dat < $1.script )
	fi
fi
