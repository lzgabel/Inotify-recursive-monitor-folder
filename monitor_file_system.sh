#!/bin/bash
### BEGIN INIT INFO
#
# Provides:	 lz19960321lz@163.com
# Required-Start:	$local_fs  $remote_fs
# Required-Stop:	$local_fs  $remote_fs
# Default-Start: 	2 3 4 5
# Default-Stop: 	0 1 6
# Short-Description:	initscript
# Description: 	删除监控文件夹中文件所带\r
#
### END INIT INFO

#Author: lzgabel

PATH=/sbin:/usr/sbin:/bin:/usr/bin
DAEMON=/usr/sbin/monitor_file_system/monitor_file_systemd
. /lib/lsb/init-functions

start() {
    status
        if [[ $? -ne 0 ]]; then
            return 1
        else 
            echo "Starting monitor_file_systemd..."
            $DAEMON
        fi
        return $?
}

stop() {
    status
        if [[ $? -eq 0 ]]; then
            return 1
        else 
            echo "Stopping monitor_file_systemd..."
            `ps -ef | grep "$DAEMON" | grep -v "grep" | awk '{print $2}' | xargs kill -9`
            if [[ $? -eq 0 ]]; then
                echo "Done."
                return 0
            else
                return 1
            fi
        fi
        
}

restart() {
    status
        if [[ $? -eq 0 ]]; then
            return 1
        else 
            echo "Stopping monitor_file_systemd..."
            stop 
            echo "Restart monitor_file_systemd..."
            start
            return 0
        fi
}

status() {
        ProCount=`ps -ef | grep "$DAEMON" | grep -v "grep" | awk '{print $2}' |wc -l`
        if [[ $ProCount -le 0 ]]; then
            echo "* : monitor_file_systemd is not running."
            return 0
        else 
            echo "* : monitor_file_systemd is running.";
            return 1
        fi 
}


case "$1" in 
    'start')
        start
        ;;
    'stop')
        stop
        ;;
    'restart')
        restart
        ;;
    'status')
        status
        ;;
    *)
        echo "Usage: "$DAEMON" {start|stop|restart|status}"
        exit 1
        ;;
esac
exit 0
