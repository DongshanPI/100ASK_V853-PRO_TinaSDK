#********************************************************************************
#-->Author:ranchao                                                              |
#-->Emil:flyranchao@allwinnertech.com                                           |
#-->Describe:Recognized The USB devices                                         |
#-->Time:2017/9/18                                                              |
#********************************************************************************
#!/bin/sh
usb_count=`mjson_fetch /base/production/hosttester/usb_count`
record=
count=0

#find usbs loop
while true
do
    list=`ls /sys/bus/usb/devices/ | awk '(NF && $1~/^([0-9]-[0-9])$/)'`
    if [ "x" == "x$list" ]; then
        ttips "no usb device found."
        exit 1
    fi
	for usb in ${list}
	do
		#no usb ,go back the begin of the loop
		[ -z "${usb}" ] && continue

		#no new usb ,go back the begin of the loop
		echo $record | grep "${usb}" &>/dev/null && continue

		#have new usb ,print the bus_port manufacturer product

        if [ 1 == 1 ]; then
        manufacturer=`cat /sys/bus/usb/devices/$usb/manufacturer`
        product=`cat /sys/bus/usb/devices/$usb/product`
        [ -z $manufacturer ] || [ -z $product ] || {
            ttips "usb devices found. bus_port is $usb" \
                -n  "manufacturer = $manufacturer, product = $product"
            exit 0;
        }
        else
		ttrue  "bus_port is $usb" \
			-n "manufacturer = `cat /sys/bus/usb/devices/$usb/manufacturer`" \
			-n "product = `cat /sys/bus/usb/devices/$usb/product`" \
			-n "Is the expected usb device info?"
		[ $? == 0 ] && exit 0;
        fi

		#Append record the information of new usb
		record="${record} ${usb}"

		#find a new usb then count++
		count=$(($count+1))

		#the detected USB is equal to the set?
		[ "$count" -ne "$usb_count" ] && continue
		exit 1
	done
	sleep 1
done
