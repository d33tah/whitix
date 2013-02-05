#!/bin/bash

#Check for at least an action and package name.
EXPECTED_ARGS=1
E_BADARGS=65

if [ $# -lt $EXPECTED_ARGS ]
then
	echo "Usage: `basename $0` [package name]"
	exit $E_BADARGS
fi

stty -echo
read -p "Password for whitix.org ports account:" -e PASSWORD
stty echo

echo "Sucessful."

# Tar and gzip the directory.
tar -cf $1.tar $1
gzip -f $1.tar
echo "FTPing."
ftp -inv ftp.whitix.org<<ENDFTP
user ports $PASSWORD
bin
put $1.tar.gz
bye
ENDFTP

echo -e "\n\nUploaded $1 successfully."
