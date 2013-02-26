#!/bin/bash

if [ $1 = "modzpu" ] ; then
	rm -f /dev/zpu
	echo "Removed device file /dev/zpu"
fi

if ! rmmod $1 ; then
	echo "Could not unload module ${1}." >&2
	exit 1
fi

echo "Unloaded module $1."
