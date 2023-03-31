#!/bin/sh

./etc/bluetooth/bt_init.sh start

if [ $? -eq 0 ]; then
	exit 0;
else
	exit 1;
fi

