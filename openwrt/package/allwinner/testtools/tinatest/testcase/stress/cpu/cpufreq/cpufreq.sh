#!/bin/ash

###########################################################
#function:cpufreq stress test.
###########################################################

testcase_path="/stress/cpu/cpufreq"
###########################################################

function Debug()
{
	#echo [DEBUG] $* > /dev/ttyS0
	return 0
}

###########################################################

Debug "get target..."
TARGET=$(get_target)

# TEST_NUMBERS - counts of test round
TEST_NUMBERS=`mjson_fetch ${testcase_path}/test_numbers`
echo "TEST_NUMBERS(from tinatest): ${TEST_NUMBERS}"

# TEST_INTERVAL - counts of test round
TEST_INTERVAL=`mjson_fetch ${testcase_path}/test_interval`
echo "TEST_INTERVAL(from tinatest): ${TEST_INTERVAL}"

###########################################################


save_cpufreq_governor=""
save_cpufreq_cur_frequency=""

# globe var
PATH_CPUFREQ_CUR_GOVERNOR=""
PATH_CPUFREQ_CUR_FREQUENCY=""

cpufreq_frequency_table=""
cpufreq_cur_governor=""
cpufreq_cur_frequency=""
cpufreq_max_frequency=""
cpufreq_min_frequency=""


help_info()
{
	echo ""
	echo "*****************************************************************"
	echo "./xx.sh -h"
	echo "  print the massage."
	echo "./xx.sh 1000 500000"
	echo "  \$1: times of test, it is TEST_NUMBERS"
	echo "  \$2: interval of test,(unit us), it is TEST_INTERVAL"
	echo "*****************************************************************"
}

PATH_CPU_MAX="/sys/devices/system/cpu/kernel_max"
# enable all non-boot cpus
enable_none_boot_cpu()
{
	if [ -f "${PATH_CPU_MAX}" ]; then
		local cpuonline=0
		local all_cpu=$(cat ${PATH_CPU_MAX})

		if [ "${all_cpu}" -gt 0 ];then
			for i in $(seq 1 ${all_cpu})
			do
				cpuonline="/sys/devices/system/cpu/cpu$i/online"

				[ ! -w "${cpuonline}" ] && continue;
				echo 1 > ${cpuonline}
			done
		fi
	fi
}

# disable all non-boot cpus, we shouldn't call it.
disable_none_boot_cpu()
{
	if [ -f "${PATH_CPU_MAX}" ]; then
		local cpuonline=""
		local all_cpu=$(cat ${PATH_CPU_MAX})

		if [ "${all_cpu}" -gt 0 ];then
			for i in $(seq 1 ${all_cpu})
			do
				cpuonline="/sys/devices/system/cpu/cpu$i/online"

				[ ! -w "${cpuonline}" ] && continue;
				echo 0 > ${cpuonline}
			done
		fi
	fi
}



# get cpufreq frequency table.
get_cpufreq_frequency_table()
{
	local ret=""
	local PATH_CPUFREQ_TABLE=""

	set -- "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies" \
	       "/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"

	case $TARGET in
	r16-*)
		PATH_CPUFREQ_TABLE="/sys/devices/system/cpu/cpu0/cpufreq/stats/trans_table"
		cpufreq_frequency_table=$(cat ${PATH_CPUFREQ_TABLE} | awk 'NR==2 {print}' | awk '$1=" " {print}')
		;;
	*)
		for f in $@ ; do
			if [ -r "$f" ] ; then
				cpufreq_frequency_table=$(cat $f); ret=$?;
				[ $ret -eq 0 ] && break;
			fi
		done
		;;
	esac

	Debug "cpufreq_frequency_table: ${cpufreq_frequency_table}"

	echo ${cpufreq_frequency_table}
}


# get cpufreq cur frequency.
get_cpufreq_cur_frequency()
{
	local ret=""

	set -- "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"  \
		"/sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq"

	for f in $@ ; do
		if [ -r "$f" ] ; then
			cpufreq_cur_frequency=$(cat $f); ret=$?;
			PATH_CPUFREQ_CUR_FREQUENCY=$f
			[ "$ret" -eq 0 ] && break;
		fi
	done

	Debug "cpufreq_cur_frequency: ${cpufreq_cur_frequency}"

	echo ${cpufreq_cur_frequency}
}

# get cpufreq max frequency.
get_cpufreq_max_frequency()
{
	local ret=""

	set -- "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"  \
		"/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq"

	for f in $@ ; do
		if [ -r "$f" ] ; then
			cpufreq_max_frequency=$(cat $f); ret=$?;
			[ "$ret" -eq 0 ] && break;
		fi
	done

	Debug "cpufreq_max_frequency: ${cpufreq_max_frequency}"

	echo ${cpufreq_max_frequency}
}

# get cpufreq min frequency.
get_cpufreq_min_frequency()
{
	local ret=""

	set -- "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"  \
		"/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq"

	for f in $@ ; do
		if [ -r "$f" ] ; then
			cpufreq_min_frequency=$(cat $f); ret=$?;
			[ "$ret" -eq 0 ] && break;
		fi
	done

	Debug "cpufreq_min_frequency: ${cpufreq_min_frequency}"

	echo ${cpufreq_min_frequency}
}

# get cpufreq cur governor.
get_cpufreq_cur_governor()
{
	local ret=""

	set -- "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"  \
		"/sys/devices/system/cpu/cpufreq/policy0/scaling_governor"

	for f in $@ ; do
		if [ -r "$f" ] ; then
			cpufreq_cur_governor=$(cat $f); ret=$?;
			PATH_CPUFREQ_CUR_GOVERNOR=$f
			[ "$ret" -eq 0 ] && break;
		fi
	done

	Debug "cpufreq_cur_governor: ${cpufreq_cur_governor}"

	echo ${cpufreq_cur_governor}
}

# set freq
# $1 is TEST_NUMBERS
# $2 is TEST_TYPE, random, maxmin, seq
# $2 is TEST_INTERVAL, us, default 500000(500ms)
cpufreq_set()
{

	local test_number=$1
	local test_type=$2
	local test_interval=$3

	local cur_count=1
	local freq_target=1

	echo "===>>cpufreq test($test_type) start"

	# save cur frequency
	get_cpufreq_cur_frequency > /dev/null
	save_cpufreq_cur_frequency=${cpufreq_cur_frequency}

	# change to userspace.
	[ -w "${PATH_CPUFREQ_CUR_GOVERNOR}" ] && \
		echo userspace > ${PATH_CPUFREQ_CUR_GOVERNOR}
	[ $? -ne 0 ] && echo "Failed to change userspace." && return
	local PATH_CPUFREQ_SET_SPEED=$(dirname ${PATH_CPUFREQ_CUR_GOVERNOR})"/scaling_setspeed"

	get_cpufreq_frequency_table > /dev/null
	size=$(echo ${cpufreq_frequency_table} | awk '{print NF}')

	while [ "$cur_count" -le "$test_number" ]
	do
		usleep $test_interval
		echo No.$cur_count
		case "$test_type" in
			random)
				# ash does not support $RANDOM, so ...
				#number=$RANDOM
				# srand()'s seed, by default, keys off of current date+time.
				# if awk is called multiple times within the same second,
				# you almost certainly will get the same value.
				#number=`awk 'BEGIN { srand(); print int(rand() * 32767); }'`
				number=$(</dev/urandom tr -dc 0-9 2>/dev/null | head -c 5 | sed 's/^0\{,4\}//g' | sed 's/^0/1/g')
				let "number %= $size"
				let "number += 1"
				freq_target=`echo $cpufreq_frequency_table | awk '{print $num}' num="$number"`
				;;
			seq)
				number=$cur_count
				let "number %= $size"
				let "number += 1"
				freq_target=`echo $cpufreq_frequency_table | awk '{print $num}' num="$number"`
				;;
			maxmin)
				number=$cur_count
				let "number %= 2"
				if [ "$number" -eq 1 ];then
					get_cpufreq_min_frequency > /dev/null
					freq_target=${cpufreq_min_frequency}
				else
					get_cpufreq_max_frequency > /dev/null
					freq_target=${cpufreq_max_frequency}
				fi
				;;
			*)
				return
				;;
		esac

		echo "freq_target=${freq_target}"
		echo ${freq_target} > ${PATH_CPUFREQ_SET_SPEED}

		get_cpufreq_cur_frequency > /dev/null
		[ ${cpufreq_cur_frequency} -eq ${freq_target} ] && \
			echo "set cpufreq[${freq_target}] OK"

		let cur_count++

	done

	echo "===<<cpufreq test($test_type) done."

	[ -n "${save_cpufreq_cur_frequency}" ] && \
		Debug "restore ${save_cpufreq_cur_frequency} to ${PATH_CPUFREQ_SET_SPEED}" && \
		echo ${save_cpufreq_cur_frequency} > ${PATH_CPUFREQ_SET_SPEED}

}

save_env()
{
	get_cpufreq_cur_governor > /dev/null
	save_cpufreq_governor=${cpufreq_cur_governor}

}

restore_env()
{
	[ -n "${save_cpufreq_governor}" ] && \
		Debug "restore ${save_cpufreq_governor} to ${PATH_CPUFREQ_CUR_GOVERNOR}" && \
		echo ${save_cpufreq_governor} > ${PATH_CPUFREQ_CUR_GOVERNOR}
}

#==================================================================================
#main function
#===================================================================================

if [ -n "$1" ] ; then
	# help info
	[ x"$1" = x"-h" ] && help_info && return

	# set default TEST_NUMBERS, when exec by console.
	# there will return 1, when $1 is number, or return 0.
	echo "$1" | grep [^0-9] > /dev/null
	[ $? -eq 1 ] && TEST_NUMBERS=$1 && \
	echo "TEST_NUMBERS(from shell): ${TEST_NUMBERS}"
fi

if [ -n "$2" ] ; then
	# set default TEST_INTERVAL, when exec by console.
	# there will return 1, when $2 is number, or return 0.
	echo "$2" | grep [^0-9] > /dev/null
	[ $? -eq 1 ] && TEST_INTERVAL=$2 && \
	echo "TEST_INTERVAL(from shell): ${TEST_INTERVAL}"

fi

# check parameter
[ -z "${TEST_NUMBERS}" ] && echo "ERROR: TEST_NUMBERS isn't defined." && \
	TEST_NUMBERS=10000 && echo "use default value: ${TEST_NUMBERS}"

[ -z "${TEST_INTERVAL}" ] && echo "ERROR: TEST_INTERVAL isn't defined." && \
	TEST_INTERVAL=500000 && echo "use default value: ${TEST_INTERVAL}"

save_env

echo ""
cpufreq_set ${TEST_NUMBERS} seq ${TEST_INTERVAL}
echo ""
cpufreq_set ${TEST_NUMBERS} maxmin ${TEST_INTERVAL}
echo ""
cpufreq_set ${TEST_NUMBERS} random ${TEST_INTERVAL}
echo ""
echo "=================all cpufreq test done!!!==================="

restore_env
