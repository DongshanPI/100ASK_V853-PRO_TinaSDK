#!/bin/sh
echo "hotplug tf $(date)"#>>/run/udev.log
echo "[$#] $0, $1, $2"#>>/run/udev.log
if [ "$#" != "2" ];then
	echo " !! param err." #>> /run/udev.log
	exit 1
fi

if [ "$2" == "remove" ];then
	echo " <==umount"#>> /run/udev.log
	umount /mnt/extsd
fi

if [ "$2" == "add" ];then
	echo " <==mount"#>> /run/udev.log
	mount -t vfat /dev/mmcblk0p1 /mnt/extsd
fi
