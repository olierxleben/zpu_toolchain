#!/bin/bash

insmod $1.ko
if [ $? -ne 0 ] ; then
	echo "Modul $1 konnte nicht geladen werden!"
	exit 1
fi

if [ $1 = "modzpu" ] ; then
	MAJOR=$(cat /proc/devices | grep zpu | cut -f1 -d' ' | head -n1)

	FILENAME="/dev/zpu"
	rm -f $FILENAME
	mknod $FILENAME c $MAJOR 0
	chmod go+rw $FILENAME
	echo "$FILENAME erstellt."
fi

echo "Modul $1 geladen."
