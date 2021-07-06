#include <syslog.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <signal.h> 
#include <string.h>
#include <errno.h>
#include <time.h>

#define LOCKFILE "var/run/LinuxMonitorDaemon.pid"
#define MEM_INFO "/LinuxMonitor.json"
#define TEMPFILE "/CommandOutputForLinuxMonitor.txt"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

int lockfile(int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    fl.l_pid = getpid();

    return fcntl(fd, F_SETLK, &fl);
}

int already_running()
{
    int fd;
    char totalMem[16];

    fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
    if (fd < 0)
    {
        syslog(LOG_ERR, "LinuxMonitorDaemon: can't open %s - %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    if (lockfile(fd) < 0)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return 1;
        }
        syslog(LOG_ERR, "LinuxMonitorDaemon: can't block %s - %s", LOCKFILE, strerror(errno));
        exit(1);
    }

    ftruncate(fd, 0);
    sprintf(totalMem, "%ld", (long)getpid());
    write(fd, totalMem, strlen(totalMem) + 1);
    return 0;
}

void daemonize(const char *cmd)
{
    int fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    // Сброс маски режима создания файла

    umask(0);

    // Получить максимально возможный номер дескриптора файла

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        perror("LinuxMonitorDaemon: can't get the max discriptor");

    // Стать лидером сессии

    if ((pid = fork()) < 0)
        perror("LinuxMonitorDaemon: fork error");
    else if (pid)
        exit(0);
    setsid();

    // Обеспечить невозможность обретения управляющего терминала

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
        perror("LinuxMonitorDaemon: can't ignore SIGHUP ");

    // Назначение корневого каталога текущим рабочим каталогом

    if (chdir("/") < 0)
        perror("LinuxMonitorDaemon: can't change catalog");

    // Закрыть все открытые файловые дескрипторы

    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (int i = 0; i < rl.rlim_max; i++)
        close(i);

    // Присоеденить файловые дескрипторы 0, 1 и 2 к /dev/null

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    // Инициализировать файл журнала

    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        syslog(LOG_ERR, "LinuxMonitorDaemon: wrong file discriptor %d, %d, %d", fd0, fd1, fd2);
        exit(1);
    }
}

void runCommand(char *comm, int size, char *path)
{
    char *command;
    command = calloc(size + strlen(path), sizeof(char));
    sprintf(command, "%s%s", comm, path);
    system(command);
    free(command);
}

char * readBuf(char *path)
{
    FILE *file;
    char * buf;
    file = fopen(path, "r");
    
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    buf = calloc(fsize + 1, sizeof(char));
    fread(buf, 1, fsize, file);
    fclose(file);
    return buf;
}

char * removeWhiteSpaces(char* source)
{
    char *blank = calloc(strlen(source), sizeof(char));
    int c = 0, d = 0;

    while (source[c] != '\0') 
    {
        if (source[c] == ' ') 
        {
            int temp = c + 1;
            if (source[temp] != '\0') 
            {
                while (source[temp] == ' ' && source[temp] != '\0') 
                {
                    if (source[temp] == ' ') 
                        c++;
                    temp++;
                }
            }
        }
        blank[d] = source[c];
        c++;
        d++;
    }
    blank[d] = '\0';
    return blank;
}

void convertToJSON(char *buf, char *top)
{
    time_t rawtime = time(NULL);
    struct tm *ptm = localtime(&rawtime);

    char* time = calloc(35, sizeof(char));
    char* tmp = calloc(15, sizeof(char));
    strftime(time, 35, "%d/%m/%Y ", ptm);
    sprintf(tmp,"%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    strcat(time, tmp);

    strcpy(&time[strlen(time)],"\0");
    char *jsonBuf = calloc(99999, sizeof(char));
    char *total = removeWhiteSpaces(buf);
    char *indv = removeWhiteSpaces(top);
    char *totalSep[2], *indvSep[5];
    totalSep[0] = strtok(total, "\n");
    totalSep[1] = strtok(NULL, "\n");

    indvSep[0] = strtok(indv, "\n");
    for (int i = 1; i < 5; i++)
        indvSep[i] = strtok(NULL, "\n");

    strcpy(jsonBuf,"{\n\t");
    strcat(jsonBuf, "\"time\":\"");
    strcat(jsonBuf, time);
    strcat(jsonBuf, "\",\n\t\"memoryUsage\":\n\t\t{\n\t\t\t\"");

    char *memline[5];
    memline[0] = strtok(totalSep[0], ": ");
    for (int i = 0; i < 2; i++)
    {        
        memline[1] = strtok(NULL, " ");
        memline[2] = strtok(NULL, "\n");
        strcat(jsonBuf, memline[0]);
        strcat(jsonBuf, "\":");
        strcat(jsonBuf, memline[1]);
        if (i + 1 < 2)
        {
            memline[0] = strtok(totalSep[i + 1], ": "); 
            strcat(jsonBuf, ",\n\t\t\t\""); 
        }
    }
    char *proc = calloc(8, sizeof(char));
    strcat(jsonBuf, ",\n\t\t\t\"process0\":");
    memline[0] = strtok(indvSep[0], " ");
    for (int i = 0; i < 5; i++)
    {
        memline[1] = strtok(NULL, " ");
        memline[2] = strtok(NULL, " ");
        memline[3] = strtok(NULL, "\n");
        if (memline[3] == NULL)
            memline[3] = memline[2];
        else
            strcat(memline[1], memline[2]);
        strcat(jsonBuf, "\n\t\t\t\t{\n\t\t\t\t\t\"pid\":");
        strcat(jsonBuf, memline[0]);
        strcat(jsonBuf, ",\n\t\t\t\t\t\"comm\":\"");
        strcat(jsonBuf, memline[1]);
        strcat(jsonBuf, "\",\n\t\t\t\t\t\"mem\":");
        strcat(jsonBuf, memline[3]);
        strcat(jsonBuf, "\n\t\t\t\t}"); 
        if (i + 1 < 5) 
        {
            memline[0] = strtok(indvSep[i + 1], " ");
            strcat(jsonBuf, ",\n\t\t\t\"");
            sprintf(proc, "process%d", i+1);
            strcat(jsonBuf, proc);
            strcat(jsonBuf, "\":");
        }
    }
    strcat(jsonBuf, "\n\t\t}");
    strcat(jsonBuf, "\n}");
    FILE *output = fopen(MEM_INFO, "a");
    fprintf(output, "%s\n", jsonBuf);
    fclose(output);
}

void logData()
{
    char path[99999], *totalMem, *topFive;
    strcpy(path, TEMPFILE);

    runCommand("head -2 /proc/meminfo > ", 24, path);
    totalMem = readBuf(path);

    runCommand("ps -eo pid,cmd,%mem --sort=-%mem --no-headers | head -5 > ", 58, path);
    topFive = readBuf(path);

    convertToJSON(totalMem, topFive);

    free(totalMem);
    free(topFive);
}

int main()
{
    time_t current;
    daemonize("LinuxMonitorDaemon");

    if (already_running())
    {
        syslog(LOG_ERR, "LinuxMonitorDaemon: already running");
        exit(1);
    }

    FILE *out = fopen(MEM_INFO, "w");
    fclose(out);
    int count = 0;

    while (1)
    {    
        if (count > 60)
        {
            count = 0;
            FILE *out = fopen(MEM_INFO, "w");
            fclose(out);
        }    
        current = time(NULL);
        logData();
        syslog(LOG_INFO, "LinuxMonitorDaemon %lu logged data at %s\n", (long)getpid(),  ctime(&current));
        count++;        
        sleep(60);
    }   

    exit(0);
}
