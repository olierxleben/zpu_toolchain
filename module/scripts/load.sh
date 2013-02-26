#!/bin/bash

if ! lsmod | grep -cq "${1}" ; then
	if ! insmod $1.ko ; then
		echo "Module ${1} could not be loaded!" >&2
		exit 1
	else
		echo "Loaded module ${1}."
	fi
else
	echo "Module ${1} is already loaded."
fi

if [ $1 = "modzpu" ] ; then
	if [ ! -f /dev/zpu ] ; then
		MAJOR=$(cat /proc/devices | grep zpu | cut -f1 -d' ' | head -n1)

		if [ -n "${MAJOR}" ] ; then
			FILENAME="/dev/zpu"
			rm -f $FILENAME
			mknod $FILENAME c $MAJOR 0
			chmod go+rw $FILENAME
			echo "$FILENAME erstellt."
		else
			echo "Device \"zpu\" not found. Is PCI device present?" >&2
		fi
	else
		echo "Devile file /dev/zpu is already present."
	fi
fi

echo "Loaded module $1."
