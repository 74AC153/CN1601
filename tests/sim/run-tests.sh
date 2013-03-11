#!/bin/bash
if [ $1 = "all" ]
then
	rm *.bin
	for i in test_*.asm;
	do
		echo $i
		../../asm -i $i -o $i.bin

		mkfifo ./fifo_miso
		../../fifostdio ./fifo_miso &
		mkfifo ./fifo_mosi
		cat -u ./fifo_mosi &

		if [ -e $i.sh ]
		then
			./$i.sh
		else
			diff -c $i.expect <( ../../sim -i $i.bin -d -1 -i,../../nvram.dat -2 -r,fifo_miso,-t,fifo_mosi < $i.script )
		fi

		ps -ef | grep 'cat -u ./fifo_mosi' | grep -v grep | awk '{ print $2 }' | xargs kill -2
		if [ -e ./fifo_mosi ]
		then
			rm fifo_mosi
		fi
		ps -ef | grep 'fifostdio ./fifo_miso' | grep -v grep | awk '{ print $2 }' | xargs kill -2
		if [ -e ./fifo_miso ]
		then
			rm fifo_miso
		fi
	done
else
	echo $1
	../../asm -i $1 -o $1.bin

	mkfifo ./fifo_miso
	../../fifostdio ./fifo_miso &
	mkfifo ./fifo_mosi
	cat -u ./fifo_mosi &

	if [ -e $i.sh ]
	then
		./$i.sh
	else
		diff -c $1.expect <( ../../sim -i $1.bin -d -1 -i,../../nvram.dat -2 -r,fifo_miso,-t,fifo_mosi < $1.script )
	fi

	ps -ef | grep 'cat -u ./fifo_mosi' | grep -v grep | awk '{ print $2 }' | xargs kill -2
	if [ -e ./fifo_mosi ]
	then
		rm fifo_mosi
	fi
	ps -ef | grep 'fifostdio ./fifo_miso' | grep -v grep | awk '{ print $2 }' | xargs kill -2
	if [ -e ./fifo_miso ]
	then
		rm fifo_miso
	fi
fi
