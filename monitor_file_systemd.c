/*************************************************************************
	> File Name: monitor_file_systemd.c
	> Author: lzgabel
	> Mail: lz19960321lz@163.com
	> Created Time: 2017年08月05日 星期六 20时19分55秒
        > 守护进程实时监听文件状态，删除文件中所有\r

 ************************************************************************/

#include <stdio.h>
#include <sys/resource.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/inotify.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define WD_SIZE 65535  //监听目录个数

struct wd_dir
{
    int     wd; //inotify_add_watch返回：Watch descriptor
    char    dirname[BUFSIZ]; //监控目录绝对路径
};

struct wd_dir wd_array[WD_SIZE];
int wd_count = 0;  //事件发生个数

void 
init_daemon()
{
    pid_t               pid;
    struct rlimit       rl;
    struct sigaction    sa;
    unsigned int        i;
    umask(0);

    //创建子进程
    if((pid = fork()) < 0) {
         perror("fork error");
         exit(EXIT_FAILURE);
    } else if(pid > 0) {
        exit(EXIT_SUCCESS); //使父进程退出
    } 

    setsid();  //创建会话

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGHUP|SIGCHLD,&sa,NULL);

    //创建子进程避免获取控制终端
    if((pid = fork()) < 0) {
         perror("fork error");
         exit(EXIT_FAILURE);
    } else if(pid > 0) {
        exit(EXIT_SUCCESS);
    }

    //修改目录
    chdir("/");
    //获取文件描述符最大值
    getrlimit(RLIMIT_NOFILE,&rl);
    //关闭不需要的文件描述符
    if(rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for(i = 0; i < rl.rlim_max; ++i)
        close(i);
}

/**
 *   将当前目录添加监听列表，遍历整个目录，将当前目录下子目录入监控列表 
 */

void
add_watch(int watch_fd, char *dir, int mask)
{
    DIR             *odir;
    int             wd;
    struct dirent   *dirent;
    if ( (odir = opendir(dir)) == NULL ) {
        perror("opendir error");
        exit(EXIT_FAILURE);
    }

    wd = wd_array[wd_count].wd = inotify_add_watch(watch_fd, dir, mask);
    strcpy(wd_array[wd_count++].dirname, dir);

    if (wd == -1) {
        perror("Couldn't add watch list");
        return;
    }

    while ((dirent = readdir(odir)) != NULL) {
        if (strcmp(dirent->d_name, "..") == 0 
            || strcmp(dirent->d_name, ".") == 0) {
                continue;
            }
        if (dirent->d_type == DT_DIR) {
            char *subdir = (char *)malloc(BUFSIZ*sizeof(char));
            sprintf(subdir, "%s/%s", dir, dirent->d_name);
            add_watch(watch_fd, subdir, mask);
            free(subdir);
        }
    }
    closedir(odir);
    return;
}

/**
 *  获取文件/目录的父目录绝对路径 
 */

char *
get_absolute_path(struct inotify_event *event)
{
    int     i;
    char    *dir = (char *)malloc(BUFSIZ*sizeof(char));

    for(i = 0; i < wd_count; i++) {
        if (wd_array[i].wd == event->wd) {
            strcpy(dir ,wd_array[i].dirname);
            break;
        }
    }
    return dir;
}

/**
 *　将子目录添加进入监听列表
 */

void 
append_subdir(int watch_fd, struct inotify_event *event, int mask)
{
    char    *dir = (char *)malloc(BUFSIZ*sizeof(char));
    char    *subdir = (char *)malloc(BUFSIZ*sizeof(char));

    dir = get_absolute_path(event);
    sprintf(subdir, "%s/%s", dir, event->name);
    add_watch(watch_fd, subdir, mask);
    free(dir);
    free(subdir);
    return;
}

/**
 *  将删除文件夹移除监听列表
 */

void 
remove_dir(int watch_fd, char *dir)
{
    int         i;
    for (i = 0; i < wd_count; i++) {
        if (strcmp(dir, wd_array[i].dirname) == 0) {
            inotify_rm_watch(watch_fd, wd_array[i].wd);
            return;
        }
            
    }
    return;
}

/**
 *  监听目录等待事件触发
 */

void
watch_dir(int watch_fd, int mask)
{
    int     length, i;
    char    buffer[BUFSIZ];
   /*
    * read从inotify的文件描述符中读取事件，从而判定发生了那些事件。若读取之时还没有发生任何事件，则read会阻塞指导事件发生
    */
    length = read(watch_fd, buffer, BUFSIZ);
    if (length == -1) {
        perror("read error !");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < length; ){
        struct inotify_event *event = (struct inotify_event *) &buffer[i];  //event->name是引发事件的文件名, 并不是完整路径
        if (event->len) {
            if (event->mask & IN_CREATE) {
                FILE * FP = fopen("/tmp/file.log" , "a+");
                if (event->mask & IN_ISDIR) {
                    char     *dir = (char *)malloc(BUFSIZ*sizeof(char));
                    dir = get_absolute_path(event);
                    fprintf(FP, "Directory [%s] was created in directory [%s]\n", event->name, dir);
                    append_subdir(watch_fd, event, mask);
                } else {
                    pid_t    cpid;
                    if ((cpid = fork()) < 0) {
                        perror("fork error");
                        exit(EXIT_FAILURE);
                    } else if (cpid == 0) {
                        int      fd_temp;
                        FILE     *fp;
                        char     *file_name = (char *)malloc(BUFSIZ*sizeof(char));
                        char     *cmd_string = (char *)malloc(BUFSIZ*sizeof(char));
                        char     *temp_file_name = (char *)malloc(BUFSIZ*sizeof(char));
                        char     *dir = (char *)malloc(BUFSIZ*sizeof(char));
                        dir = get_absolute_path(event);
                        sprintf(file_name, "%s/%s", dir, event->name);
                        sprintf(temp_file_name, "/tmp/%s.XXXXXX", event->name);
                        fd_temp = mkstemp(temp_file_name);
                        if (watch_fd == -1) {
                            perror("create temp file faile");
                            exit(EXIT_FAILURE);
                        }
                        sprintf(cmd_string, "cp %s  %s && tr -d \"\\r\" < %s > %s",
                                file_name, temp_file_name, temp_file_name, file_name);

                        fprintf(FP, "File [%s] was created in directory [%s]\n", event->name, dir);
                        free(dir);
                        free(file_name);
                        close(fd_temp);
                        fp = popen(cmd_string, "r");
                        pclose(fp);
                        unlink(temp_file_name);
                        free(temp_file_name);
                        free(cmd_string);
                        exit(EXIT_SUCCESS);
                    } else {
                        cpid = wait(NULL);
                        if (cpid == -1) {
                            perror("Child process exit failed");
                        }
                    }

                }
                fclose(FP);
            } else if (event->mask & IN_DELETE) {
                FILE * FP = fopen("/tmp/file.log" , "a+");
                if (event->mask & IN_ISDIR) {
                    char    *dir = (char *)malloc(BUFSIZ*sizeof(char));
                    char    *subdir = (char *)malloc(BUFSIZ*sizeof(char));
                    dir = get_absolute_path(event);
                    sprintf(subdir, "%s/%s", dir, event->name);
                    remove_dir(watch_fd, subdir);
                    fprintf(FP, "\033[41;37mDirectory [ %s ] was deleted from directory [ %s ] \033[0m\n", event->name, dir);
                    free(dir);
                } else {
                    char    *dir = (char *)malloc(BUFSIZ*sizeof(char));
                    dir = get_absolute_path(event);
                    fprintf(FP, "\033[41;37mFile [ %s ] was deleted from directory [ %s ] \033[0m\n", event->name, dir);
                    free(dir);
                }
                fclose(FP);
            }
        }
        i += EVENT_SIZE + event->len;
    }
    return;
}

/**
 *   监听初始化
 */

int 
watch_init(char *root, int mask) 
{
    int     fd;
    if ((fd = inotify_init()) < 0) {
        perror("inotify_init error");
        exit(EXIT_FAILURE);
    } 
    add_watch(fd, root, mask);
    return fd;
}


int
main(int argc,char *argv[])
{
    int                watch_fd;
    int                mask=IN_CREATE|IN_DELETE;
    char               *watch_directory=argv[1];
    init_daemon();
    // daemone(0,0); 系统调用
    watch_fd = watch_init(watch_directory, mask);
    for (;;) {
            watch_dir(watch_fd, mask);
    }
    exit(EXIT_SUCCESS);
}

