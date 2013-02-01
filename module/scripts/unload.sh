#!/bin/bash

if [ $1 = "modzpu" ] ; then
	rm -f /dev/zpu
fi

rmmod $1

echo "Modul $1 entladen."
