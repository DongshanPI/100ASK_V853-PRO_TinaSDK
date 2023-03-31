#!/bin/ash
###########################################################
#function:standby stress test.
#author:  flyranchao@allwinnertech.com
#time:   create at    2017/10/10
#        refator at   2018/12/20
###########################################################

testcase_path="/stress/standby"
###########################################################
# Before do standby, reboot first
# If call standby on tinatest by adb, this script will do
# standby without any log or tinatest will go wrong.
# To fix it, I set this test as may_reboot, and set
# run_times to 2, which enable reboot before do standby test.
# By this, this will reboot in first time and do standby
# test in next time

function Debug()
{
#	echo "[DEBUG] $*" > /dev/ttyS0
	return 0
}


may_reboot=$(mjson_fetch ${testcase_path}/may_reboot)
if [ "$may_reboot" = "true" ] ; then
    REBOOT_STATUS="/mnt/UDISK/tinatest.reboot.status"
    [ -f "${REBOOT_STATUS}" ] && \
        [ "$(head -n 1 ${REBOOT_STATUS})" == "0" ] && reboot -f
fi
###########################################################

# TEST_NUMBERS - counts of test round
TEST_NUMBERS=`mjson_fetch ${testcase_path}/test_numbers`
echo "TEST_NUMBERS(from tinatest): ${TEST_NUMBERS}"
# STANDBY_PERIOD - standby time
STANDBY_PERIOD=`mjson_fetch ${testcase_path}/standby_period`
echo "STANDBY_PERIOD(from tinatest): ${STANDBY_PERIOD}"
# RUNNING_PERIOD - running time
RUNNING_PERIOD=`mjson_fetch ${testcase_path}/running_period`
echo "RUNNING_PERIOD(from tinatest): ${RUNNING_PERIOD}"

CONFIG_PRINTK_LEVEL=`mjson_fetch ${testcase_path}/printk_level`
echo "CONFIG_PRINTK_LEVEL(from tinatest): ${CONFIG_PRINTK_LEVEL}"
CONFIG_CONSOLE_SUSPEND=`mjson_fetch ${testcase_path}/console_suspend`
echo "CONFIG_CONSOLE_SUSPEND(from tinatest): ${CONFIG_CONSOLE_SUSPEND}"
CONFIG_PM_PRINT_TIMES=`mjson_fetch ${testcase_path}/pm_print_times`
echo "CONFIG_PM_PRINT_TIMES(from tinatest): ${CONFIG_PM_PRINT_TIMES}"
CONFIG_INITCALL_DEBUG=`mjson_fetch ${testcase_path}/initcall_debug`
echo "CONFIG_INITCALL_DEBUG(from tinatest): ${CONFIG_INITCALL_DEBUG}"


PATH_CONSOLE_SUSPEND='/sys/module/printk/parameters/console_suspend'
PATH_INITCALL_DEBUG='/sys/module/kernel/parameters/initcall_debug'
PATH_PM_PRINT_TIMES='/sys/power/pm_print_times'
PATH_PRINTK_LEVEL='/proc/sys/kernel/printk'


backup_console_suspend=
backup_initcall_debug=
backup_pm_print_times=
backup_printk_level=
backup_scene_lock=
backup_wake_lock=


function help_info()
{
	echo ""
	echo "*****************************************************************"
	echo "./xx.sh -h"
	echo "  print the massage"
	echo "./xx.sh 10000 5 5 8N1Y"
	echo "  \$1: counts of test numbers, it is TEST_NUMBERS"
	echo "  \$2: standby period,(unit s), it is STANDBY_PERIOD"
	echo "  \$3: running period,(unit s), it is RUNNING_PERIOD"
	echo "  \$4: split: PRINTK_LEVEL, CONSOLE_SUSPEND, PM_PRINT_TIMES, INITCALL_DEBUG"
	echo "*****************************************************************"
}

#get the target
function auto_get_target()
{
	Debug "get target..."
	TARGET=$(get_target)
	case ${TARGET} in
		r6-* | r11-* | r7-*| r328s2-* | r328s3-* | mr112-*)
			STANDBY_TYPE="normal_standby"
			;;
	        r16-*)
		        STANDBY_TYPE="earlysuspend"
			;;
	        r40-* | r18-* | r30-* | mr813-*)
		        STANDBY_TYPE="super_standby"
			;;
	        *)
		        STANDBY_TYPE="super_standby"
			;;
	esac
	Debug "platform = ${TARGET}" "STANDBY_TYPE=${STANDBY_TYPE}"
}

function save_env()
{
	Debug "save env..."
	local tmp_path=''

	tmp_path=${PATH_CONSOLE_SUSPEND}
	[ -r "${tmp_path}" ] && backup_console_suspend=$(cat ${tmp_path})
	Debug "backup_console_suspend=${backup_console_suspend}"

	tmp_path=${PATH_INITCALL_DEBUG}
	[ -r "${tmp_path}" ] && backup_initcall_debug=$(cat ${tmp_path})
	Debug "backup_initcall_debug=${backup_initcall_debug}"

	tmp_path=${PATH_PM_PRINT_TIMES}
	[ -r "${tmp_path}" ] && backup_pm_print_times=$(cat ${tmp_path})
	Debug "backup_pm_print_times=${backup_pm_print_times}"

	tmp_path=${PATH_PRINTK_LEVEL}
	[ -r "${tmp_path}" ] && backup_printk_level=$(cat ${tmp_path})
	Debug "backup_printk_level=${backup_printk_level}"

	tmp_path='/sys/power/scene_lock'
	if [ -r "${tmp_path}" ] ; then
		backup_scene_lock=`cat ${tmp_path} | awk -vRS="]" -vFS="[" '{print $2}'`
		echo "backup_scene_lock=${backup_scene_lock}"
	fi

	tmp_path='/sys/power/wake_lock'
	if [ -r "${tmp_path}" ] ; then
		backup_wake_lock=$(cat ${tmp_path})
		echo "backup_wake_lock=${backup_wake_lock}"
	fi

	return 0
}

function restore_env()
{
	Debug "restore env..."
	local tmp_path

	tmp_path='/sys/power/scene_lock'
	if [ -w "${tmp_path}" ]; then
		#restore scene_lock
		for scene in ${backup_scene_lock} ; do
			Debug "write scene(${scene}) to ${tmp_path}"
			echo $scene > ${tmp_path}
		done
	fi

	tmp_path='/sys/power/wake_lock'
	if [ -w "${tmp_path}" ] ; then
		for wake in ${backup_wake_lock} ; do
			Debug "write wake(${wake}) to ${tmp_path}"
			echo ${backup_wake_lock} > ${tmp_path}
		done
	fi

	tmp_path=${PATH_CONSOLE_SUSPEND}
	Debug "write ${backup_console_suspend} to ${tmp_path}"
	[ -w "${tmp_path}" ] && echo ${backup_console_suspend} > ${tmp_path} || \
		echo "Error: ${tmp_path} can not write."


	#restore initcall_debug setting value
	tmp_path=${PATH_INITCALL_DEBUG}
	Debug "write ${backup_initcall_debug} to ${tmp_path}"
	[ -w "${tmp_path}" ] && echo ${backup_initcall_debug} > ${tmp_path} || \
		echo "Error: ${tmp_path} can not write."

	#restore pm_print_times setting value
	tmp_path=${PATH_PM_PRINT_TIMES}
	Debug "write ${backup_pm_print_times} to ${tmp_path}"
	[ -w "${tmp_path}" ] && echo ${backup_pm_print_times} > ${tmp_path} || \
		echo "Error: ${tmp_path} can not write."

	tmp_path=${PATH_PRINTK_LEVEL}
	Debug "write ${backup_printk_level} to ${tmp_path}"
	[ -w "${tmp_path}" ] && echo ${backup_printk_level} > ${tmp_path} || \
		echo "Error: ${tmp_path} can not write."


	#restore the system auto wakeup para
	case $TARGET in
		r7-* | r11-* | r7s-* | r332-* | r331-* | r16-* | r58-* | r6-*)
			tmp_path='/sys/module/pm_tmp/parameters/time_to_wakeup'
			;;
		r311-* | r30-* | t7-* | mr133-*)
			tmp_path='/sys/module/pm/parameters/time_to_wakeup_ms'
			;;
		r18-*)
			tmp_path='/sys/power/sunxi/time_to_wakeup_ms'
			;;
		r328s2-* | r328s3-* | mr112-*)
			tmp_path='/sys/power/sunxi_debug/time_to_wakeup_ms'
			;;
		*)
			echo "use alarm to wakeup, so nothing to do now."
			return 0
			;;
	esac
	Debug "write 0 to ${tmp_path}."
	echo 0 > ${tmp_path}

	return 0
}

function set_auto_wakeup()
{
	Debug "set wakeup time..."
	local tmp_path=''

	[ -z "$1" ] && echo "args is null." && return -1

	if [ "$1" -gt 0 ] ; then
		local tmp=$1
		Debug "arg is number ${tmp}."
	else
		echo "arg is not number, it is $1."
		exit -1
	fi


	tmp_path=${PATH_CONSOLE_SUSPEND}
	[ -w "${tmp_path}" ] && echo ${CONFIG_CONSOLE_SUSPEND} > ${tmp_path} \
		|| echo "${tmp_path} can\'t write."

	tmp_path=${PATH_INITCALL_DEBUG}
	[ -w "${tmp_path}" ] && echo ${CONFIG_INITCALL_DEBUG} > ${tmp_path} \
		|| echo "${tmp_path} can\'t write."

	tmp_path=${PATH_PM_PRINT_TIMES}
	[ -w "${tmp_path}" ] && echo ${CONFIG_PM_PRINT_TIMES} > ${tmp_path} \
		|| echo "${tmp_path} can\'t write."

	tmp_path=${PATH_PRINTK_LEVEL}
	[ -w "${tmp_path}" ] && echo ${CONFIG_PRINTK_LEVEL} > ${tmp_path} \
		|| echo "${tmp_path} can\'t write."

	case ${TARGET} in
		r7-* | r11-* | r7s-* | r332-* | r331-* | r16-* | r58-*)
			time_to_wakeup=$(busybox expr $1 \* 1000)
			time_path='/sys/module/pm_tmp/parameters/time_to_wakeup'
			;;
		r6-*)
			time_to_wakeup=$1
			time_path='/sys/module/pm_tmp/parameters/time_to_wakeup'
			;;
		r311-* | r30-* | t7-* | mr133-*)
			time_to_wakeup=$(busybox expr $1 \* 1000)
			time_path='/sys/module/pm/parameters/time_to_wakeup_ms'
			;;
		r18-*)
			time_to_wakeup=$1
			time_path='/sys/power/sunxi/time_to_wakeup_ms'
			;;
		r328s2-* | r328s3-* | mr112-*)
			time_to_wakeup=$(busybox expr $1 \* 200)
			time_path='/sys/power/sunxi_debug/time_to_wakeup_ms'
			;;
		*)
			time_to_wakeup="+$1"
			time_path='/sys/class/rtc/rtc0/wakealarm'
			;;
	esac

	[ ! -w "${time_path}" ] && echo "Error: ${time_path} cant write." && exit -1
	Debug "write ${time_to_wakeup} to ${time_path}"
	echo ${time_to_wakeup} > ${time_path}
	return 0

}

function clear_all_wake_lock()
{
	Debug "clear all wake locks"

	local wake_lock_path='/sys/power/wake_lock'
	local wake_unlock_path='/sys/power/wake_unlock'
	if [ -r "${wake_lock_path}" -a -w "${wake_unlock_path}" ] ; then
		wake_lock_valid=$(${wakelock_path})
		echo "wake_lock_valid=${wake_lock_valid}"
		if [ x"$wake_lock_valid" != x"" ] ; then
			for wake_lock_tmp in ${wake_lock_valid} ; do
				Debug "write ${wake_lock_tmp} to ${wake_unlock_path}"
				echo ${wake_lock_tmp} > ${wake_unlock_path}
			done
		fi
	fi
}

function enter_standby()
{
	local tmp_path='/sys/power/state'
	[ ! -w "${tmp_path}" ] && exit -1
	Debug "write mem to ${tmp_path}"
	echo mem > ${tmp_path}
}

function suspend_resume_test()
{
	Debug "enter test..."

	echo "The test will begin in 5 seconds..."
	sleep 5

	num=1
	while [ "${num}" -le "${TEST_NUMBERS}" ]
	do
		echo -e "=======================================\n"
		echo -e "Begin: rounds No.${num} !\n"
		echo
		num=$((${num}+1))

		# set standby period
		set_auto_wakeup ${STANDBY_PERIOD}

		# Set standby type: normal, super or earlysuspend
		local tmp_path='/sys/power/scene_lock'
		if [ -w "${tmp_path}" -a -r "${tmp_path}" ] ; then
			grep "${STANDBY_TYPE}" "${tmp_path}" > /dev/null
			if [ $? -eq 0 ] ; then
				echo ${STANDBY_TYPE} > ${tmp_path}
			fi
		fi

		# for earlysuspend, remove the wake_tmp wake lock.
		if [ x"${STANDBY_TYPE}" == x"earlysuspend" ] ; then
			clear_all_wake_lock
		fi

		# enter standby
		echo "===>enter standby<==="
		echo ""
		enter_standby
		echo ""
		echo "===>exit  standby<==="
		sleep ${RUNNING_PERIOD}

		# for earlysuspend, enter_standby is non-blocking, so we must ensure:
		# 1. It must have entered standby.
		# 2. Preventing the system from entering standby again after it resumed.
		if [ x"${STANDBY_TYPE}" == x"earlysuspend" ] ; then
			if [ -w "/sys/power/wake_lock" ] ; then
				sleep 1
				tmp_path='/sys/power/state'
				[ ! -w ${tmp_path} ] && exit -1
				Debug "write mem to ${tmp_path}"
				echo none > ${tmp_path}
			fi
		fi
		echo "=====================resume ok  $(($num-1))!!!===================="
		echo
	done
}

#===================================main function===============================
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
	# set default STANDBY_PERIOD, when exec by console.
	# there will return 1, when $2 is number, or return 0.
	echo "$2" | grep [^0-9] > /dev/null
	[ $? -eq 1 ] && STANDBY_PERIOD=$2 && \
	echo "STANDBY_PERIOD(from shell): ${STANDBY_PERIOD}"

fi

if [ -n "$3" ] ; then
	# set default RUNNING_PERIOD, when exec by console.
	# there will return 1, when $3 is number, or return 0.
	echo "$3" | grep [^0-9] > /dev/null
	[ $? -eq 1 ] && RUNNING_PERIOD=$3 && \
	echo "RUNNING_PERIOD(from shell): ${RUNNING_PERIOD}"

fi


temp=$4
if [ -n "$temp" ] ; then
	CONFIG_PRINTK_LEVEL=${temp:0:1}
	CONFIG_CONSOLE_SUSPEND=${temp:1:1}
	CONFIG_PM_PRINT_TIMES=${temp:2:1}
	CONFIG_INITCALL_DEBUG=${temp:3:1}
	echo "CONFIG_PRINTK_LEVEL(from shell): ${CONFIG_PRINTK_LEVEL}"
	echo "CONFIG_CONSOLE_SUSPEND(from shell): ${CONFIG_CONSOLE_SUSPEND}"
	echo "CONFIG_PM_PRINT_TIMES(from shell): ${CONFIG_PM_PRINT_TIMES}"
	echo "CONFIG_INITCALL_DEBUG(from shell): ${CONFIG_INITCALL_DEBUG}"
fi


# check parameter
[ -z "${TEST_NUMBERS}" ] && echo "ERROR: TEST_NUMBERS isn't defined." && \
	TEST_NUMBERS=10000 && echo "use default value: ${TEST_NUMBERS}"

[ -z "${STANDBY_PERIOD}" ] && echo "ERROR: STANDBY_PERIOD isn't defined." && \
	STANDBY_PERIOD=5 && echo "use default value: ${STANDBY_PERIOD}"

[ -z "${RUNNING_PERIOD}" ] && echo "ERROR: RUNNING_PERIOD isn't defined." && \
	RUNNING_PERIOD=5 && echo "use default value: ${RUNNING_PERIOD}"

[ -z "${CONFIG_PRINTK_LEVEL}" ] && echo "ERROR: CONFIG_PRINTK_LEVEL isn't defined." && \
	CONFIG_PRINTK_LEVEL=4 && echo "use default value: ${CONFIG_PRINTK_LEVEL}"

[ -z "${CONFIG_CONSOLE_SUSPEND}" ] && echo "ERROR: CONFIG_CONSOLE_SUSPEND isn't defined." && \
	CONFIG_CONSOLE_SUSPEND="Y" && echo "use default value: ${CONFIG_CONSOLE_SUSPEND}"

[ -z "${CONFIG_PM_PRINT_TIMES}" ] && echo "ERROR: CONFIG_PM_PRINT_TIMES isn't defined." && \
	CONFIG_PM_PRINT_TIMES=0 && echo "use default value: ${CONFIG_PM_PRINT_TIMES}"

[ -z "${CONFIG_INITCALL_DEBUG}" ] && echo "ERROR: CONFIG_INITCALL_DEBUG isn't defined." && \
	CONFIG_INITCALL_DEBUG="N" && echo "use default value: ${CONFIG_INITCALL_DEBUG}"

save_env
auto_get_target
suspend_resume_test $1 $2 $3
restore_env

[ $(($num-1)) -lt $TEST_NUMBERS ] && echo "exit -1" && exit -1
echo "exit 0" && exit 0

