#! /bin/sh

# mx_irigb.sh          Start/Stop the irigb time sync daemon.
# chkconfig: 2345 90 60
# description: Moxa IRIG-B time sync daemon
# 
### BEGIN INIT INFO
# Provides:		ServiceSyncTime
# Required-Start:	$remote_fs $syslog
# Required-Stop:	$remote_fs $syslog
# Default-Start:	2 3 4 5
# Default-Stop:		0 1 6
# Short-Description:	Moxa IRIG-B time sync daemon
### END INIT INFO

set -e

umask 022

# The time sync daemon default configure wtih
#   -t 1 - Sync time in DIFF signal format
#   -i 10 - The time interval in 10 seconds to sync the IRIG-B time into system time.
#   -B - Run daemon in the background
#
MX_IRIGB_SERVICESYNCTIME_OPTS="-t 1 -i 10 -B"

# The IRIG-B utility default configure wtih
#   -f 15 - Set Output Interface
#   -p 1,1,2,0 - Output port 1 in DIFF signal from IRIG-B encode module and not inverse the signal
MX_IRIGB_UTIL_OPTS="-f 15 -p 1,1,2,0"

export PATH="${PATH:+$PATH:}/usr/sbin:/sbin"

load_irigb_driver() {

	if [ "`lsmod|grep -c moxa_irigb`" = "0" ]; then
		# If the driver has not been loaded, load it
		insmod /lib/modules/`uname -r`/kernel/drivers/misc/moxa_irigb.ko
	fi
}

case "$1" in
  start)

	load_irigb_driver

	if [ -e "/var/run/ServiceSyncTime.pid" ]; then
		echo "The ServiceSyncTime is running. You cannot run it twice."
	else
		/usr/sbin/ServiceSyncTime $MX_IRIGB_SERVICESYNCTIME_OPTS > /dev/null 2>&1
		# If you need the IRIG-B signal output, you should remove the # in from of the following line.
		#/usr/sbin/mxIrigUtil $MX_IRIGB_UTIL_OPTS > /dev/null 2>&1
	fi
	;;
  stop)
	if [ -e "/var/run/ServiceSyncTime.pid" ]; then
		pkill -F  /var/run/ServiceSyncTime.pid
		rm -rf /var/run/ServiceSyncTime.pid
	fi
	;;
  restart)
	$0 stop
	sleep 1
	$0 start
	;;
esac

exit 0
