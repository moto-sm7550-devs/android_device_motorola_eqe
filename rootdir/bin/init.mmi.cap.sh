#!/vendor/bin/sh

PATH=/sbin:/vendor/sbin:/vendor/bin:/vendor/xbin
export PATH

scriptname=${0##*/}

notice()
{
	echo "$*"
	echo "$scriptname: $*" > /dev/kmsg
}

# Globals

cap_path=

setup_permissions()
{

	 chmod 0660 $cap_path/fac_enable
	 chmod 0660 $cap_path/fac_cal
	 notice "set permission "
	[ -e $cap_path/fac_detect ] && chown system:system $cap_path/fac_detect
	[ -e $cap_path/fac_enable ] && chown system:system $cap_path/fac_enable
	[ -e $cap_path/fac_cal ] && chown system:system $cap_path/fac_cal
	[ -e $cap_path/fac_irq_state ] && chown system:system $cap_path/fac_irq_state
	[ -e $cap_path/fac_compensation ] && chown system:system $cap_path/fac_compensation
	[ -e $cap_path/fac_raw ] && chown system:system $cap_path/fac_raw
	[ -e $cap_path/reset ] && chown system:system $cap_path/reset
}


# main
notice "cap sh start"
count=0
max=10
while [ ! -e /sys/class/capsense/capsense0 ]; do
	sleep 1
	count=$(($count+1))
	if [ $count -gt $max ]; then
		break
	fi
done

if [ -e /sys/class/capsense/capsense0 ]; then
	cap_path=/sys/class/capsense/capsense0
	setup_permissions
fi
if [ -e /sys/class/capsense/capsense1 ]; then
	cap_path=/sys/class/capsense/capsense1
	setup_permissions
fi
