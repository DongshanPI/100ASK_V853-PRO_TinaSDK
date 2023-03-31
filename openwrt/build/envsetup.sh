#!/bin/bash

function _load_config()
{
	local cfgkey=$1
	local cfgfile=$2
	local defval=$3
	local val=""

	[ -f "$cfgfile" ] && val="$(sed -n "/^\s*export\s\+$cfgkey\s*=/h;\${x;p}" $cfgfile | sed -e 's/^[^=]\+=//g' -e 's/^\s\+//g' -e 's/\s\+$//g')"
	eval echo "${val:-"$defval"}"
}

# cd openwrt dir
function cop()
{
	cd ${TINA_TOPDIR}/openwrt
}

# cd openwrt src dir
function copsrc()
{
	cd ${TINA_TOPDIR}/openwrt/openwrt
}

function copdl()
{
	cd ${TINA_TOPDIR}/openwrt/dl
}

function copbuild()
{
	cd ${TINA_TOPDIR}/openwrt/build
}


function cgeneric()
{
	local dkey="LICHEE_IC"
	local dval=$(_load_config $dkey ${TINA_TOPDIR}/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd ${TINA_TOPDIR}/openwrt/target/$dval/generic
}


function ccommon()
{
	local dkey="LICHEE_IC"
	local dval=$(_load_config $dkey ${TINA_TOPDIR}/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd ${TINA_TOPDIR}/openwrt/target/$dval/$dval-common
}

function cplat()
{
	local dkey1="LICHEE_IC"
	local dkey2="LICHEE_BOARD"
	local dval1=$(_load_config $dkey1 ${TINA_TOPDIR}/.buildconfig)
	local dval2=$(_load_config $dkey2 ${TINA_TOPDIR}/.buildconfig)
	[ -z "$dval1" ] && echo "ERROR: $dkey1 not set in .buildconfig" && return 1
	[ -z "$dval2" ] && echo "ERROR: $dkey2 not set in .buildconfig" && return 1

	cd ${TINA_TOPDIR}/openwrt/target/$dval1/$dval1-$dval2
}

function ctoolchain()
{
	cd ${TINA_TOPDIR}/prebuilt/
}

function chostbin()
{
	local dkey1="LICHEE_IC"
	local dkey2="LICHEE_BOARD"
	local dkey3="LICHEE_LINUX_DEV"
	local dval1=$(_load_config $dkey1 ${TINA_TOPDIR}/.buildconfig)
	local dval2=$(_load_config $dkey2 ${TINA_TOPDIR}/.buildconfig)
	local dval3=$(_load_config $dkey3 ${TINA_TOPDIR}/.buildconfig)
	[ -z "$dval1" ] && echo "ERROR: $dkey1 not set in .buildconfig" && return 1
	[ -z "$dval2" ] && echo "ERROR: $dkey2 not set in .buildconfig" && return 1
	[ -z "$dval3" ] && echo "ERROR: $dkey3 not set in .buildconfig" && return 1

	cd ${TINA_TOPDIR}/out/${dval1}/${dval2}/${dval3}/staging_dir/host/bin
}

function crootfs()
{
	local dkey1="LICHEE_IC"
	local dkey2="LICHEE_BOARD"
	local dkey3="LICHEE_LINUX_DEV"
	local dval1=$(_load_config $dkey1 ${TINA_TOPDIR}/.buildconfig)
	local dval2=$(_load_config $dkey2 ${TINA_TOPDIR}/.buildconfig)
	local dval3=$(_load_config $dkey3 ${TINA_TOPDIR}/.buildconfig)
	[ -z "$dval1" ] && echo "ERROR: $dkey1 not set in .buildconfig" && return 1
	[ -z "$dval2" ] && echo "ERROR: $dkey2 not set in .buildconfig" && return 1
	[ -z "$dval3" ] && echo "ERROR: $dkey3 not set in .buildconfig" && return 1

	cd ${TINA_TOPDIR}/out/${dval1}/${dval2}/${dval3}/build_dir/target/root-${dval1}-${dval2}
}

# out target
function ctarget()
{
	local dkey1="LICHEE_IC"
	local dkey2="LICHEE_BOARD"
	local dkey3="LICHEE_LINUX_DEV"
	local dval1=$(_load_config $dkey1 ${TINA_TOPDIR}/.buildconfig)
	local dval2=$(_load_config $dkey2 ${TINA_TOPDIR}/.buildconfig)
	local dval3=$(_load_config $dkey3 ${TINA_TOPDIR}/.buildconfig)
	[ -z "$dval1" ] && echo "ERROR: $dkey1 not set in .buildconfig" && return 1
	[ -z "$dval2" ] && echo "ERROR: $dkey2 not set in .buildconfig" && return 1
	[ -z "$dval3" ] && echo "ERROR: $dkey3 not set in .buildconfig" && return 1

	cd ${TINA_TOPDIR}/out/${dval1}/${dval2}/${dval3}/build_dir/target
}

### check parameter
function check_tina_topdir()
{
	[ -z "${TINA_TOPDIR}" -o ! -e "${TINA_TOPDIR}/build/envsetup.sh" ] \
		&& echo "Couldn't locate the top of the tree.  Try setting TOP." \
		&& return -1

	return 0
}

# check to see if the supplied product is one we can build
function check_platform()
{
	local tinadir_openwrt_target="${TINA_TOPDIR}/openwrt/target"
	local tina_platform_choices="$(ls ${tinadir_openwrt_target} 2>/dev/null | sort | uniq)"

	local v
	for v in ${tina_platform_choices}
	do
		[ "$v" = "$1" -a -n "$v" ] && return 0
	done

	return -1
}

# check to see if the supplied product is one we can build
# $1 is ic name, $platform in func envsetup.
# $2 is board name, $board in func envsetup.
function check_board()
{
	[ -z "$1" -o -z "$2" ] && return -1
	local tinadir_openwrt_target="${TINA_TOPDIR}/openwrt/target"
	local tina_board_choices="$(ls ${tinadir_openwrt_target}/$1 2>/dev/null | sort | uniq)"

	local v=
	for v in ${tina_board_choices}
	do
		[ "$v" = "$1-$2" -a -n "$v" ] && return 0
	done

	return -1
}

function get_storage_type()
{
	check_tina_topdir || return -1
	[ -z "${TINA_TARGET_PLAT}" ] && return -1
	[ -z "${TINA_TARGET_BOARD}" ] && return -1
	return -1
}

function print_lunch_menu()
{
	local uname=$(uname)
	echo
	echo "You're building on" $uname
	echo
	echo "Lunch menu... pick a combo:"

	local tinafile_openwrt_target=${TINA_TOPDIR}/openwrt/.openwrt_targets
	cat -n ${tinafile_openwrt_target}
}

# Tab completion for lunch.
function _lunch()
{
	local cur prev
	local tinafile_openwrt_target=${TINA_TOPDIR}/openwrt/.openwrt_targets

	COMPREPLY=()
	cur="${COMP_WORDS[COMP_CWORD]}"
	prev="${COMP_WORDS[COMP_CWORD-1]}"
	COMPREPLY=( $(compgen -W "$(cat ${tinafile_openwrt_target})" -- ${cur}) )
	return 0
}


function lunch
{
	check_tina_topdir || return -1

	# get last platfrom as default platform
	local confile="${TINA_TOPDIR}/openwrt/openwrt/.config"
	local last
	if [ -f "${confile}" ]; then
		#last="$(awk -F[=_] '/CONFIG_TARGET_[a-z_0-9]*[^_]=y/{print $3 "_" $4; exit}' ${T}/.config)"
		last="$(sed -n -r '/^CONFIG_TARGET_BOARD=.*$/{s/^CONFIG_TARGET_BOARD=\"(.*)\"$/\1/;p;q}' ${confile})"
	fi

	# select platform
	local select
	if [ "$1" ] ; then
		select=$1
	else
		print_lunch_menu
		echo -n "Which would you like?"
		[ -n "${last}" ] && echo -n " [Default ${last}]"
		echo -n ": "
		read select
	fi

	# format $select
	select=$(echo ${select} |sed 's/-tina//g')

	local tinafile_openwrt_target=${TINA_TOPDIR}/openwrt/.openwrt_targets
	if [ -z "${select}" ]; then
		select="${last}-tina"
	elif (echo -n $select | grep -q -e "^[0-9][0-9]*$"); then
		select=$(eval "awk 'NR==${select}' ${tinafile_openwrt_target}")
	elif (echo -n $select | grep -q -e "^[a-z0-9]*-[^\-][^\-]*$"); then
		select="$select-tina"
	else
		echo
		echo "Invalid lunch combo: $select" >&2
		return -1
	fi

	# check platform
	local platform=$(echo -n $select | awk -F- '{print $1}')
	check_platform ${platform}
	if [ $? -ne 0 ]; then
		echo
		echo "** Don't have a platform spec for: '$platform'" >&2
		echo "** Do you have the right repo manifest?" >&2
		platform=
		return -1
	fi

	# check board
	local board=$(echo -n $select | awk -F- '{print $2}')
	check_board ${platform} ${board}
	if [ $? -ne 0 ]; then
		echo
		echo "** Don't have a board spec for: '$board'" >&2
		echo "** Do you have the right repo manifest?" >&2
		board=
		return -1
	fi

	local variant=$(echo -n $select | awk -F- '{print $3}')
	if [ $? -ne 0 ]
	then
		echo
		echo "** Invalid variant: '$variant'" >&2
		echo "** Must be one of ${VARIANT_CHOICES}" >&2
		variant=
		return -1
	fi

	# phase sysconfig.fex
	local tinafile_sysconfig="${TINA_TOPDIR}/device/config/chips/${platform}/configs/${board}/sys_config.fex"

	local storage_type=
	storage_type=$([ -e "$tinafile_sysconfig" ] && awk -F"=" '/^storage_type/{print $2}' $tinafile_sysconfig | sed 's/^[ \t]*//g' )
	if [ x"$storage_type" = x"3" ]; then
		local flash="nor"
	else
		local flash="default"
	fi

	echo "Jump to longan autoconfig"
	echo "${TINA_TOPDIR}/build.sh autoconfig -o openwrt -i $platform -b $board \
		-n $flash "

	${TINA_TOPDIR}/build.sh autoconfig -o openwrt -i $platform -b $board \
		-n $flash

	return  0
	# Provided for openwrt's output
	#export TARGET_OUT_DIR=${TINA_TOPDIR}/out/${TINA_TARGET_PLAT}-${TINA_TARGET_BOARD}
	#[ ! -d "${TARGET_OUT_DIR}" ] && mkdir -p ${TARGET_OUT_DIR}

	# load platform script
	#[ -e ${TINA_TOPDIR}/openwrt/target/allwinner/${TINA_TARGET_PLAT}-${TINA_TARGET_BOARD}/${TINA_TARGET_PLAT}_${TINA_TARGET_BOARD}-setup.sh ] \
	#    && source ${TINA_TOPDIR}/target/allwinner/${TINA_TARGET_PLAT}-${TINA_TARGET_BOARD}/${TINA_TARGET_PLAT}_${TINA_TARGET_BOARD}-setup.sh
}

function add_lunch_combo()
{
	[ -z "$@" ] && return -1;

	local tinafile_openwrt_target=${TINA_TOPDIR}/openwrt/.openwrt_targets
	local all=$(cat ${tinafile_openwrt_target})
	echo ${all} $@ | xargs -n1 | sort | uniq > ${tinafile_openwrt_target}
}

function make()
{
	check_tina_topdir || return -1

	if [ x"${TINA_TOPDIR}" = x"$(pwd)" ] ; then
		echo "===There is tina environment.==="
		echo " Note: make is the shell functon in envsetup.sh. "
	else
		echo "===There isn't tina environment.==="
		echo " Note: will use shell command origin rather than the functon. "
		command make $@
		return $?
	fi
	echo ""

	local allargs=$@
	local action action_args
	action=openwrt_build
	while [ $# -gt 0 ]; do
		case "$1" in
		kernel_menuconfig|menuconfig|openwrt_rootfs|openwrt_build|clean|distclean)
			action=$1
			shift;
			action_args="$@"
			break;
			;;
		*)
			# default
			action=openwrt_build
			action_args="$@"
			break;
		esac
	done
	echo "== action: $action, action_args: $action_args =="

	if [ x"$action" = x"kernel_menuconfig" ] ; then
		${TINA_TOPDIR}/build.sh loadconfig
		[ $? -ne 0 ] && echo "Error: when exec loadconfig" && return -1;
		${TINA_TOPDIR}/build.sh menuconfig
		[ $? -ne 0 ] && echo "Error: when exec menuconfig" && return -1;
		${TINA_TOPDIR}/build.sh saveconfig
		[ $? -ne 0 ] && echo "Error: when exec saveconfig" && return -1;
		return 0;
	fi

	if [ x"$action" = x"menuconfig" ] ; then
		${TINA_TOPDIR}/build.sh openwrt_menuconfig $@
		return $?;
	fi

	if [ x"$action" = x"openwrt_rootfs" ] ; then
		${TINA_TOPDIR}/build.sh openwrt_rootfs $@
		return $?;
	fi

	if [ x"$action" = x"openwrt_build" ] ; then
		${TINA_TOPDIR}/build.sh tina $@
		return $?;
	fi

	if [ x"$action" = x"clean" ] ; then
		${TINA_TOPDIR}/build.sh $action $@
		return $?;
	fi

	if [ x"$action" = x"distclean" ] ; then
		${TINA_TOPDIR}/build.sh $action $@
		return $?;
	fi

	return $?;
}

function mm
{
    local T=${TINA_TOPDIR}/
    local orgin=`pwd`
    local path=`pwd`
    local target=
    local cmd=

    #find target makefile
    #trap 'echo $orgin >> ~/mm; trap - SIGINT; ' SIGINT
    while [ x`pwd` != x$T ] && [ x`pwd` != x"/" ]
    do
        find  -maxdepth 1 -name Makefile | xargs cat | grep "define Package" > /dev/null
        is_package=$?
        find  -maxdepth 1 -name Makefile | xargs cat | grep "define KernelPackage" > /dev/null
        is_kernel_package=$?
        if [ $is_package -eq 1 ] && [ $is_kernel_package -eq 1 ]; then
            cd ../
        else
            path=`pwd`
            target=${path##*/}
            cd $T
            cmd="compile V=s"
            for i in $*; do
                [ x$i = x"-B" ] && {
                    # -B clean the package
                    print_red "make openwrt_rootfs package/$target/clean V=s"
                    make openwrt_rootfs package/$target/clean V=s
                }
                [ x${i:0:2} = x"-j" ] && cmd=$cmd" "$i
            done
            print_red "make openwrt_rootfs package/$target/$cmd"
            make openwrt_rootfs package/$target/$cmd
            cd $orgin
            #trap - SIGINT
            return
        fi
    done
    cd $orgin
    #trap - SIGINT
    print_red "Can't not find Tina Package Makefile!"
}

function mmo
{
    local T=${TINA_TOPDIR}/
    local orgin=`pwd`
    local target=$1
    local cmd=

    [ x$target = x ] && {
        print_red "mmo need pkg name."
        return
    }
    cd $T
    cmd="compile V=s"
    for i in $*; do
        [ x$i = x"-B" ] && {
            # -B clean the package
            print_red "make openwrt_rootfs package/$target/clean V=s"
            make openwrt_rootfs package/$target/clean V=s
        }
        [ x${i:0:2} = x"-j" ] && cmd=$cmd" "$i
    done
    print_red "make openwrt_rootfs package/$target/$cmd"
    make openwrt_rootfs package/$target/$cmd
    cd $orgin
}

function _m
{
    [ -z "$TINA_TOPDIR" ] && return
    [ -f "$TINA_TOPDIR/openwrt/openwrt/include/toplevel.mk" ] || return
    local cur prev list
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    list="$(grep "[^\.].*config:" $TINA_TOPDIR/openwrt/openwrt/include/toplevel.mk | awk -F: '{print $1}')"
    COMPREPLY=( $(compgen -W "${list}" -- ${cur}) )
}


function m
{
    (\cd ${TINA_TOPDIR} && make $@)
}

function p
{
    (\cd ${TINA_TOPDIR} && pack)
}

function mp
{
   m $@ && p
}

function envsetup_openwrt()
{
	# check bash
	if [ "x$SHELL" != "x/bin/bash" ]; then
		case `ps -o command -p $$` in
			*bash*)
				;;
			*)
				echo -n "WARNING: Only bash is supported, "
				echo "use of other shell would lead to erroneous results"
				;;
		esac
	fi

	# reset these variables.
	local tinafile_openwrt_target=${TINA_TOPDIR}/openwrt/.openwrt_targets
	[ -e ${tinafile_openwrt_target} ] && rm ${tinafile_openwrt_target}
	touch ${tinafile_openwrt_target}

	# Execute the contents of any vendorsetup.sh files we can find.
	local vendors vendor
	local tinadir_openwrt_target="${TINA_TOPDIR}/openwrt/target"
	verdors="$(find -L ${tinadir_openwrt_target} -maxdepth 4 -name 'vendorsetup.sh' 2>/dev/null | sort)"

	for verdor in ${verdors}
	do
		source ${verdor}
	done

	# completion
	complete -F _lunch lunch
	complete -F _m m
}


### MAIN ###

# check top of SDK
if [ ! -f "${TINA_TOPDIR}/build/.tinatopdir" ]; then
	echo "ERROR: Not found .tinatopdir"
	return -1;
fi


envsetup_openwrt


