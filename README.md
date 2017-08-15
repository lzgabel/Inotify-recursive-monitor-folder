## Inotiry 递归监控目录

inotify只能监控单层目录变化，不能监控子目录中的变化情况。所以只能遍历目录，将所有子目录添加入监控列表
当所监控目录超过8192时，导致too many open files, 两种解决方案:

### 1:
  * 需要更改下列文件数值大小:
  *    /proc/sys/fs/inotify/max_user_watches
  *    This specifies an upper limit on the number of watches that
  *    can be created per real user ID.
 但是这种方式在系统重启后会被重置为8192
### 2: [推荐]
   *   vim /etc/sysctl.conf
   *   添加　fs.inotify.max_user_watches=[max_watch_number]
## 开机自启：
####    添加:
   *   cp monitor_file_system.sh /etc/init.d
   *   cd /etc/init.d
   *   sudo update-rc.d monitor_file_system.sh defaults'
 ####   删除:
   *    sudo update-rc.d -f monitor_file_system.sh remove
## 使用:
   *    service monitor_file_system { start | stop | restart | status }
   *    /etc/init.d/monitor_file_system { start | stop | restart | status }
