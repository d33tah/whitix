#!/bin/bash

#Check for at least an action and package name.
EXPECTED_ARGS=2
E_BADARGS=65

if [ $# -lt $EXPECTED_ARGS ]
then
	echo "Usage: `basename $0` -{i,u} [package names]"
	exit $E_BADARGS
fi

touch portlist

if [ $1 = "-i" ]
then
	if [ `grep '$2' portlist` = ""]
	then
		WGET_OUTPUT=$(2>&1 wget --progress=dot:mega "http://www.whitix.org/downloads/ports/$2.tar.gz")
		if [ $? -ne 0 ]
		then
			echo 1>&2 $0: "$WGET_OUTPUT" Exiting.
			exit 1
		fi
		tar -zxvf $2.tar.gz
		rm $2.tar.gz
		echo -e "$2\n" >> portlist
	fi
	exit 0
fi

if [ $1 = "-u" ]
then
	if [ `grep '$2' portlist` != ""]
	then
		rm -rf $2
		sed '/$2/ d' `echo portlist` > portlist
	fi
	exit 0
fi
