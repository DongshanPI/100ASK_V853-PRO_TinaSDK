#!/bin/sh

gpio_dir="/sys/class/gpio"
io_num=`mjson_fetch /base/production/pintester/gpio_num`
input_io=`mjson_fetch /base/production/pintester/input_io`
output_io=`mjson_fetch /base/production/pintester/output_io`

tinatest_get_pin_num()
{
	local result
	local tmp=$(echo $1 | cut -c1-2)
	local num=$(echo $1 | cut -c3-4)
	case $tmp in
		PA)
			result=`expr 0 + $num`
			;;
		PB)
			result=`expr 32 + $num`
			;;
		PC)
			result=`expr 64 + $num`
			;;
		PD)
			result=`expr 96 + $num`
			;;
		PE)
			result=`expr 128 + $num`
			;;
		PF)
			result=`expr 160 + $num`
			;;
		PG)
			result=`expr 192 + $num`
			;;
		PH)
			result=`expr 192 + $num`
			;;
		PI)
			result=`expr 224 + $num`
			;;
		PJ)
			result=`expr 256 + $num`
			;;
		PP)
			result=`expr 2020 + $num`
			;;
		*)
			echo "PINSET ERROR!"
			echo "please set the right pin in menuconfig!"
			exit 1
	esac
	if [ "${2}" = "in" ]; then
		eval input_io$i=$result
	else
		eval output_io$i=$result
	fi
}

tinatest_get_pin_io()
{
for i in `seq 1 $io_num`
do
	eval input_io$i="$(echo "$input_io" | awk -v t="${i}" -F, '{print $t}')"
	eval tinatest_get_pin_num \$input_io$i "in"

	eval output_io$i="$(echo "$output_io" | awk -v t="${i}" -F, '{print $t}')"
	eval tinatest_get_pin_num \$output_io$i "out"
done
}

tinatest_check_input_io()
{
	echo "test output $*"
	for i in `seq 1 $io_num`
	do
		eval local in_io=\$input_io$i
		eval local out_io=\$output_io$i
		local value=`cat $gpio_dir/gpio$in_io/value`
		echo "result:gpio$in_io/value:$value"
		if ! [ ${value} -eq $* ]; then
			echo "Check gpio$in_io:"
			echo "Can't not set gpio$out_io output $* level!"
			exit 1
		fi
	done
}

tinatest_check_pin()
{
	for i in `seq 1 $io_num`
	do
		#set input_io in input function
		eval local in_io=\$input_io$i
		echo $in_io > $gpio_dir/export
		if ! [ -d "$gpio_dir/gpio$in_io" ]; then
			echo "Can't not apply for input gpio$in_io"
			exit 1
		fi
		echo "in" > $gpio_dir/gpio$in_io/direction

		#set output_io in output funciton and output high level
		eval local out_io=\$output_io$i
		echo $out_io > $gpio_dir/export
		if ! [ -d "$gpio_dir/gpio$out_io" ]; then
			echo "Can't not apply for input gpio$out_io"
			exit 1
		fi
		echo "out" > $gpio_dir/gpio$out_io/direction
		echo 1 > $gpio_dir/gpio$out_io/value
	done

	tinatest_check_input_io "1"

	#change output_io output low level
	for i in `seq 1 $io_num`
	do
		eval out_io=\$output_io$i
		echo 0 > $gpio_dir/gpio$out_io/value
	done

	tinatest_check_input_io "0"

	return 0
}
echo "Begin to check pintester"
echo "-------------------------"
echo "gpio num is $io_num"
tinatest_get_pin_io
tinatest_check_pin
[ $? -eq 0 ] && echo "pintester successful!"
