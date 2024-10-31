/* 
 * csapp.c - Functions for the CS:APP3e book
 *
 * Updated 10/2016 reb:
 *   - Fixed bug in sio_ltoa that didn't cover negative numbers
 *
 * Updated 2/2016 droh:
 *   - Updated open_clientfd and open_listenfd to fail more gracefully
 *
 * Updated 8/2014 droh: 
 *   - New versions of open_clientfd and open_listenfd are reentrant and
 *     protocol independent.
 *
 *   - Added protocol-independent inet_ntop and inet_pton functions. The
 *     inet_ntoa and inet_aton functions are obsolete.
 *
 * Updated 7/2014 droh:
 *   - Aded reentrant sio (signal-safe I/O) routines
 * 
 * Updated 4/2013 droh: 
 *   - rio_readlineb: fixed edge case bug
 *   - rio_readnb: removed redundant EINTR check
 */
/* $begin csapp.c */
#include "csapp.h"

/************************** 
 * Error-handling functions
 **************************/
/* $begin errorfuns */
/* $begin unixerror */
void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
/* $end unixerror */

void posix_error(int code, char *msg) /* Posix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(0);
}

void gai_error(int code, char *msg) /* Getaddrinfo-style error */
{
    fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
    exit(0);
}

void app_error(char *msg) /* Application error */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}
/* $end errorfuns */

void dns_error(char *msg) /* Obsolete gethostbyname error */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}


/*********************************************
 * Wrappers for Unix process control functions
 ********************************************/

/* $begin forkwrapper */
pid_t Fork(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork error");
    return pid;
}
/* $end forkwrapper */

void Execve(const char *filename, char *const argv[], char *const envp[]) 
{
    if (execve(filename, argv, envp) < 0)
	unix_error("Execve error");
}

/* $begin wait */
pid_t Wait(int *status) 
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
	unix_error("Wait error");
    return pid;
}
/* $end wait */

pid_t Waitpid(pid_t pid, int *iptr, int options) 
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0) 
	unix_error("Waitpid error");
    return(retpid);
}

/* $begin kill */
void Kill(pid_t pid, int signum) 
{
    int rc;

    if ((rc = kill(pid, signum)) < 0)
	unix_error("Kill error");
}
/* $end kill */

void Pause() 
{
    (void)pause();
    return;
}

unsigned int Sleep(unsigned int secs) 
{
    unsigned int rc;

    if ((rc = sleep(secs)) < 0)
	unix_error("Sleep error");
    return rc;
}

unsigned int Alarm(unsigned int seconds) {
    return alarm(seconds);
}
 
void Setpgid(pid_t pid, pid_t pgid) {
    int rc;

    if ((rc = setpgid(pid, pgid)) < 0)
	unix_error("Setpgid error");
    return;
}

pid_t Getpgrp(void) {
    return getpgrp();
}

/************************************
 * Wrappers for Unix signal functions 
 ***********************************/

/* $begin sigaction */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* Block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}
/* $end sigaction */

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    if (sigprocmask(how, set, oldset) < 0)
	unix_error("Sigprocmask error");
    return;
}

void Sigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0)
	unix_error("Sigemptyset error");
    return;
}

void Sigfillset(sigset_t *set)
{ 
    if (sigfillset(set) < 0)
	unix_error("Sigfillset error");
    return;
}

void Sigaddset(sigset_t *set, int signum)
{
    if (sigaddset(set, signum) < 0)
	unix_error("Sigaddset error");
    return;
}

void Sigdelset(sigset_t *set, int signum)
{
    if (sigdelset(set, signum) < 0)
	unix_error("Sigdelset error");
    return;
}

int Sigismember(const sigset_t *set, int signum)
{
    int rc;
    if ((rc = sigismember(set, signum)) < 0)
	unix_error("Sigismember error");
    return rc;
}

int Sigsuspend(const sigset_t *set)
{
    int rc = sigsuspend(set); /* always returns -1 */
    if (errno != EINTR)
        unix_error("Sigsuspend error");
    return rc;
}

/*************************************************************
 * The Sio (Signal-safe I/O) package - simple reentrant output
 * functions that are safe for signal handlers.
 *************************************************************/

/* Private sio functions */

/* $begin sioprivate */
/* sio_reverse - Reverse a string (from K&R) */
static void sio_reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* sio_ltoa - Convert long to base b string (from K&R) */
static void sio_ltoa(long v, char s[], int b) 
{
    int c, i = 0;
    int neg = v < 0;

    if (neg)
	v = -v;

    do {  
        s[i++] = ((c = (v % b)) < 10)  ?  c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);

    if (neg)
	s[i++] = '-';

    s[i] = '\0';
    sio_reverse(s);
}

/* sio_strlen - Return length of string (from K&R) */
static size_t sio_strlen(char s[])
{
    int i = 0;

    while (s[i] != '\0')
        ++i;
    return i;
}
/* $end sioprivate */

/* Public Sio functions */
/* $begin siopublic */

ssize_t sio_puts(char s[]) /* Put string */
{
    return write(STDOUT_FILENO, s, sio_strlen(s)); //line:csapp:siostrlen
}

ssize_t sio_putl(long v) /* Put long */
{
    char s[128];
    
    sio_ltoa(v, s, 10); /* Based on K&R itoa() */  //line:csapp:sioltoa
    return sio_puts(s);
}

void sio_error(char s[]) /* Put error message and exit */
{
    sio_puts(s);
    _exit(1);                                      //line:csapp:sioexit
}
/* $end siopublic */

/*******************************
 * Wrappers for the SIO routines
 ******************************/
ssize_t Sio_putl(long v)
{
    ssize_t n;
  
    if ((n = sio_putl(v)) < 0)
	sio_error("Sio_putl error");
    return n;
}

ssize_t Sio_puts(char s[])
{
    ssize_t n;
  
    if ((n = sio_puts(s)) < 0)
	sio_error("Sio_puts error");
    return n;
}

void Sio_error(char s[])
{
    sio_error(s);
}

/********************************
 * Wrappers for Unix I/O routines
 ********************************/

int Open(const char *pathname, int flags, mode_t mode) 
{
    int rc;

    if ((rc = open(pathname, flags, mode))  < 0)
	unix_error("Open error");
    return rc;
}

ssize_t Read(int fd, void *buf, size_t count) 
{
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0) 
	unix_error("Read error");
    return rc;
}

ssize_t Write(int fd, const void *buf, size_t count) 
{
    ssize_t rc;

    if ((rc = write(fd, buf, count)) < 0)
	unix_error("Write error");
    return rc;
}

off_t Lseek(int fildes, off_t offset, int whence) 
{
    off_t rc;

    if ((rc = lseek(fildes, offset, whence)) < 0)
	unix_error("Lseek error");
    return rc;
}

void Close(int fd) 
{
    int rc;

    if ((rc = close(fd)) < 0)
	unix_error("Close error");
}

int Select(int  n, fd_set *readfds, fd_set *writefds,
	   fd_set *exceptfds, struct timeval *timeout) 
{
    int rc;

    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
	unix_error("Select error");
    return rc;
}

int Dup2(int fd1, int fd2) 
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
	unix_error("Dup2 error");
    return rc;
}

void Stat(const char *filename, struct stat *buf) 
{
    if (stat(filename, buf) < 0)
	unix_error("Stat error");
}

void Fstat(int fd, struct stat *buf) 
{
    if (fstat(fd, buf) < 0)
	unix_error("Fstat error");
}

/*********************************
 * Wrappers for directory function
 *********************************/

DIR *Opendir(const char *name) 
{
    DIR *dirp = opendir(name); 

    if (!dirp)
        unix_error("opendir error");
    return dirp;
}

struct dirent *Readdir(DIR *dirp)
{
    struct dirent *dep;
    
    errno = 0;
    dep = readdir(dirp);
    if ((dep == NULL) && (errno != 0))
        unix_error("readdir error");
    return dep;
}

int Closedir(DIR *dirp) 
{
    int rc;

    if ((rc = closedir(dirp)) < 0)
        unix_error("closedir error");
    return rc;
}

/***************************************
 * Wrappers for memory mapping functions
 ***************************************/
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) 
{
    void *ptr;

    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1))
	unix_error("mmap error");
    return(ptr);
}

void Munmap(void *start, size_t length) 
{
    if (munmap(start, length) < 0)
	unix_error("munmap error");
}

/***************************************************
 * Wrappers for dynamic storage allocation functions
 ***************************************************/

void *Malloc(size_t size) 
{
    void *p;

    if ((p  = malloc(size)) == NULL)
	unix_error("Malloc error");
    return p;
}

void *Realloc(void *ptr, size_t size) 
{
    void *p;

    if ((p  = realloc(ptr, size)) == NULL)
	unix_error("Realloc error");
    return p;
}

void *Calloc(size_t nmemb, size_t size) 
{
    void *p;

    if ((p = calloc(nmemb, size)) == NULL)
	unix_error("Calloc error");
    return p;
}

void Free(void *ptr) 
{
    free(ptr);
}

/******************************************
 * Wrappers for the Standard I/O functions.
 ******************************************/
void Fclose(FILE *fp) 
{
    if (fclose(fp) != 0)
	unix_error("Fclose error");
}

FILE *Fdopen(int fd, const char *type) 
{
    FILE *fp;

    if ((fp = fdopen(fd, type)) == NULL)
	unix_error("Fdopen error");

    return fp;
}

char *Fgets(char *ptr, int n, FILE *stream) 
{
    char *rptr;

    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
	app_error("Fgets error");

    return rptr;
}

FILE *Fopen(const char *filename, const char *mode) 
{
    FILE *fp;

    if ((fp = fopen(filename, mode)) == NULL)
	unix_error("Fopen error");

    return fp;
}

void Fputs(const char *ptr, FILE *stream) 
{
    if (fputs(ptr, stream) == EOF)
	unix_error("Fputs error");
}

size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream) 
{
    size_t n;

    if (((n = fread(ptr, size, nmemb, stream)) < nmemb) && ferror(stream)) 
	unix_error("Fread error");
    return n;
}

void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) 
{
    if (fwrite(ptr, size, nmemb, stream) < nmemb)
	unix_error("Fwrite error");
}


/**************************** 
 * Sockets interface wrappers
 ****************************/

int Socket(int domain, int type, int protocol) 
{
    int rc;

    if ((rc = socket(domain, type, protocol)) < 0)
	unix_error("Socket error");
    return rc;
}
/*
   domain : 소켓의 주소 체계 (ex. AF_INET은 IPv4, AF_INET은 IPv6)
   type : 소켓의 타입 (ex. SOCKET_STREAM은 TCP, SOCKET_DGRAM은 UDP)
   protocol : 프로토콜을 명시 (대부분 0으로 설정하여 기본 프로토콜 사용)
   
   반환값 : 생성된 소켓의 파일 디스크립터 반환하며, 실패 시 -1 반환한다
   사용 목적 : 소켓 기반 통신을 시작하기 위해 소켓을 열어준다
*/

void Setsockopt(int s, int level, int optname, const void *optval, int optlen) 
{
    int rc;

    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
	unix_error("Setsockopt error");
}
/*
    s : 소켓 파일 디스크립터
    level : 옵션의 적용 수준 (ex. SQL_SOCKET)
    optname : 설정할 옵션 이름 (ex. SO_REUSEADDR)
    optval: 옵션의 값이 저장된 포인터
    optlen: optval의 크기

    반환값 : 없음. 실패 시 오류 반환
    목적 : 소켓이 동작하는 방식을 세부적으로 조정할 때 사용된다. (ex. 포트 재사용 설정 등)
*/

void Bind(int sockfd, struct sockaddr *my_addr, int addrlen) 
{
    int rc;

    if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
	unix_error("Bind error");
}
/*
    sockfd : 바인딩할 소켓 파일 디스크립터
    my_addr : 소켓에 연결할 주소 정보를 담은 sockaddr 구조체
    addrlen : 주소 구조체의 크기
    
    반환값 : 없음. 실패 시 오류 반환
    사용 목적 : 서버가 수신할 IP와 포트를 지정할 때 사용된다
*/

void Listen(int s, int backlog) 
{
    int rc;

    if ((rc = listen(s,  backlog)) < 0)
	unix_error("Listen error");
}
/*
    s : 대기 상태로 설정할 소켓 파일 디스크립터
    backlog : 대기열에 쌓을 수 있는 최대 연결 요청 수

    반환값 : 없음. 실패 시 오류 반환
    사용 목적 : 서버가 클라이언트의 연결 요청을 수락하기 위해 대기 상태로 전환할 때 사용된다
*/

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen) 
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
	unix_error("Accept error");
    return rc;
}
/*
    s : 수락할 소켓 파일 디스크립터
    addr : 클라이언트 주소 정보가 저장될 구조체
    addrlen : 주소 구조체의 크기

    반환값 : 새로 생성된 소켓의 파일 디스크립터를 반환하며, 실패 시 -1 반환
    사용 목적 : 서버가 클라이언트 연결을 수락하여 개별 통신을 시작할 수 있게 한다
*/

void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen) 
{
    int rc;

    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
	unix_error("Connect error");
}
/*
    sockfd : 연결할 소켓 파일 디스크립터
    serv_addr : 연결할 서버의 주소 정보가 저장된 sockaddr 구조체
    addrlen : 주소 구조체의 크기

    반환값 : 없음. 실패 시 오류 반환
    사용 목적 : 클라이언트가 서버에 연결 요청을 보내기 위해 사용된다
*/

/*******************************
 * Protocol-independent wrappers
 *******************************/
/* $begin getaddrinfo */
void Getaddrinfo(const char *node, const char *service, 
                 const struct addrinfo *hints, struct addrinfo **res)
{
    int rc;

    if ((rc = getaddrinfo(node, service, hints, res)) != 0) 
        gai_error(rc, "Getaddrinfo error");
}
/* $end getaddrinfo */
/*
    node : 변환할 호스트 이름 또는 IP 주소 (ex. localhost 또는 192.168.1.1)
    service : 변환할 서비스 이름 또는 포트 번호 (ex. http 또는 80)
    hints : 변환 방식을 지정하는 addrinfo 구조체 (NULL로 설정 가능)
    res : 결과로 반환되는 소켓 주소 리스트의 포인터
    
    반환값 : 성공 시 0 반환, 실패 시 오류 코드 반환
    사용 목적: 특정 호스트와 포트에 맞는 주소 정보 가져올 때 사용함
               socket이나 connect와 함께 사용된다
*/


void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, 
                 size_t hostlen, char *serv, size_t servlen, int flags)
{
    int rc;

    if ((rc = getnameinfo(sa, salen, host, hostlen, serv, 
                          servlen, flags)) != 0) 
        gai_error(rc, "Getnameinfo error");
}
/*
sa: 변환할 소켓 주소.
salen: 소켓 주소의 길이.
host: 변환된 호스트 이름을 저장할 버퍼.
hostlen: 호스트 이름을 저장할 버퍼의 크기.
serv: 변환된 서비스 이름을 저장할 버퍼.
servlen: 서비스 이름을 저장할 버퍼의 크기.
flags: 변환 방식 설정 (예: NI_NUMERICHOST는 숫자 형식의 주소를 사용).
반환값: 성공 시 0을 반환하고, 실패 시 오류 코드를 반환합니다.
사용 목적: IP 주소와 포트 번호를 호스트와 서비스 이름으로 변환할 때 사용합니다. 네트워크 로깅이나 디버깅에 유용합니다.
*/

void Freeaddrinfo(struct addrinfo *res)
{
    freeaddrinfo(res);
}
/*
res: 해제할 addrinfo 구조체 포인터.
반환값: 없음.
사용 목적: Getaddrinfo로 할당된 메모리 블록을 사용 후에 해제하여 메모리 누수를 방지합니다.
*/

void Inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if (!inet_ntop(af, src, dst, size))
        unix_error("Inet_ntop error");
}
/*
af: 주소 패밀리 (예: AF_INET은 IPv4, AF_INET6은 IPv6).
src: 변환할 이진 형식의 IP 주소.
dst: 변환된 문자열 형식의 IP 주소를 저장할 버퍼.
size: dst 버퍼의 크기.
반환값: 성공 시 dst 포인터를 반환하며, 실패 시 NULL을 반환합니다.
사용 목적: 네트워크 주소를 사람이 읽을 수 있는 문자열 형식으로 변환할 때 사용합니다.
*/

void Inet_pton(int af, const char *src, void *dst) 
{
    int rc;

    rc = inet_pton(af, src, dst);
    if (rc == 0)
	app_error("inet_pton error: invalid dotted-decimal address");
    else if (rc < 0)
        unix_error("Inet_pton error");
}
/*
파라미터:
af: 주소 패밀리 (예: AF_INET은 IPv4, AF_INET6은 IPv6).
src: 변환할 문자열 형식의 IP 주소.
dst: 변환된 이진 형식의 IP 주소를 저장할 메모리 위치.
반환값: 성공 시 1을 반환하고, 실패 시 0(유효하지 않은 IP) 또는 -1(에러)를 반환합니다.
사용 목적: 사람이 읽을 수 있는 IP 주소를 네트워크에서 사용할 수 있는 이진 형식으로 변환할 때 사용합니다.
*/

/*******************************************
 * DNS interface wrappers. 
 *
 * NOTE: These are obsolete because they are not thread safe. Use
 * getaddrinfo and getnameinfo instead
 ***********************************/

/* $begin gethostbyname */
struct hostent *Gethostbyname(const char *name) 
{
    struct hostent *p;

    if ((p = gethostbyname(name)) == NULL)
	dns_error("Gethostbyname error");
    return p;
}
/* $end gethostbyname */

struct hostent *Gethostbyaddr(const char *addr, int len, int type) 
{
    struct hostent *p;

    if ((p = gethostbyaddr(addr, len, type)) == NULL)
	dns_error("Gethostbyaddr error");
    return p;
}

/************************************************
 * Wrappers for Pthreads thread control functions
 ************************************************/

void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp, 
		    void * (*routine)(void *), void *argp) 
{
    int rc;

    if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
	posix_error(rc, "Pthread_create error");
}

void Pthread_cancel(pthread_t tid) {
    int rc;

    if ((rc = pthread_cancel(tid)) != 0)
	posix_error(rc, "Pthread_cancel error");
}

void Pthread_join(pthread_t tid, void **thread_return) {
    int rc;

    if ((rc = pthread_join(tid, thread_return)) != 0)
	posix_error(rc, "Pthread_join error");
}

/* $begin detach */
void Pthread_detach(pthread_t tid) {
    int rc;

    if ((rc = pthread_detach(tid)) != 0)
	posix_error(rc, "Pthread_detach error");
}
/* $end detach */

void Pthread_exit(void *retval) {
    pthread_exit(retval);
}

pthread_t Pthread_self(void) {
    return pthread_self();
}
 
void Pthread_once(pthread_once_t *once_control, void (*init_function)()) {
    pthread_once(once_control, init_function);
}

/*******************************
 * Wrappers for Posix semaphores
 *******************************/

void Sem_init(sem_t *sem, int pshared, unsigned int value) 
{
    if (sem_init(sem, pshared, value) < 0)
	unix_error("Sem_init error");
}

void P(sem_t *sem) 
{
    if (sem_wait(sem) < 0)
	unix_error("P error");
}

void V(sem_t *sem) 
{
    if (sem_post(sem) < 0)
	unix_error("V error");
}

/****************************************
 * The Rio package - Robust I/O functions
 ****************************************/

/*
 * rio_readn - Robustly read n bytes (unbuffered)
 */
/* $begin rio_readn */
ssize_t rio_readn(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nread = read(fd, bufp, nleft)) < 0) {
	    if (errno == EINTR) /* Interrupted by sig handler return */
		nread = 0;      /* and call read() again */
	    else
		return -1;      /* errno set by read() */ 
	} 
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* Return >= 0 */
}
/* $end rio_readn */

/*
 * rio_writen - Robustly write n bytes (unbuffered)
 */
/* $begin rio_writen */
/* fd에 n 바이트의 데이터를 쓰는 함수 */
ssize_t rio_writen(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;  //nleft는 아직 쓰지 않은 남은 바이트 수를 의미 (처음에는 쓰고자 하는 총 바이트 수 n으로 초기화된다)
    ssize_t nwritten;  //write 함수가 한 번에 성공적으로 쓴 바이트 수를 저장할 변수
    char *bufp = usrbuf;  //write 함수 호출 시 쓸 데이터의 시작 위치 가리킴

    while (nleft > 0) {  //nleft가 0이 되면 종료
	if ((nwritten = write(fd, bufp, nleft)) <= 0) {
	    if (errno == EINTR)  /* Interrupted by sig handler return */
		nwritten = 0;    /* and call write() again */
	    else
		return -1;       /* errno set by write() */
	}
	nleft -= nwritten; //nleft(남은 바이트 수)에서 방금 쓴 바이트 수(nwritten)를 빼서 업데이트함
	bufp += nwritten;  //포인터 'bufp'를 nwritten만큼 이동시켜 다음에 쓸 데이터의 위치를 가리키게 한다. 즉, 이미 쓴 부분 건너뛰고 남은 데이터 위치로 이동하는 것
    }
    return n;
    /*
    [write 함수의 반환값 의미]
    1. 양수 값 : write 함수가 반환하는 양수 값은 성공적으로 쓴 바이트 수를 의미한다.
                - 예를 들어, write 호출에서 fd에 10바이트 쓰기를 요청했는데 write가 10을 반환하면, 요청한 10바이트가 모두 성공적으로 쓰였다는 뜻입니다.
                - 네트워크 소켓이나 파일 디스크립터에서는 경우에 따라 요청한 바이트 수보다 적은 바이트가 쓰일 수도 있습니다. 
                  이 경우 write는 실제로 쓴 바이트 수를 반환하므로, 요청한 바이트가 모두 쓰일 때까지 write를 다시 호출하는 것이 일반적입니다.
    2. 0 (EOF) : 파일에 대한 write 호출에서 0이 반환될 일은 거의 없습니다. 그러나 일반적으로 소켓이나 파이프에서 상대방이 연결을 종료한 상태라면 0을 반환할 수 있습니다.
    3. 음수 값(-1) : 오류 발생
    */
   .0
   20.
}
/* $end rio_writen */


/* 
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
/* 시스템 호출을 사용하여 데이터를 읽어와 버퍼링하는 역할! 버퍼에 채워 읽기 위한 준비 과정 */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)  //n은 사용자가 요청한 읽기 크기, 즉 읽고자 하는 최대 바이트 수
{
    int cnt;  //실제로 읽어온 바이트 수 저장하는 변수

    while (rp->rio_cnt <= 0) {  /* Refill if buf is empty */ //rp->rio_cnt<=0 이면 버퍼에 읽을 데이터가 없음
	rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
			   sizeof(rp->rio_buf));  //read 시스템 호출을 사용해 rp->rio_fd(소켓 파일디스크립터)에서 rio_buf(내부 버퍼 주소)로 최대 sizeof(rp->rio_buf) 바이트를 읽어온다
                                      //왜 rio_cnt로 저장할까? 현재 버퍼에 남아 있는 데이터의 바이트 수를 명시적으로 추적하기 위해서
	if (rp->rio_cnt < 0) {  //rio_cnt는 읽어온 데이터의 바이트 수 저장하는 역할이므로, 정상적인 상황에서는 0 또는 양수여야 한다
	    if (errno != EINTR) /* Interrupted by sig handler return */ //신호 처리로 인해 중단된 것이 아니라면
		return -1;
	}
	else if (rp->rio_cnt == 0)  /* EOF (End of File)*/ //EOF는 파일이나 입력 스트림의 끝에 도달했다는 표시 의미 (읽어올 데이터가 없거나 더 이상 읽을 데이터가 없다)
	    return 0;  //0을 반환하여 호출한 쪽에서 EOF임을 알 수 있게 해 준다
	else 
	    rp->rio_bufptr = rp->rio_buf; /* Reset buffer ptr */ //데이터를 성공적으로 읽었다면, rio_bufptr을 rio_buf의 시작 위치로 초기화하여 새로 채워진 버퍼의 첫 위치 가리키도록 함
        /*
        rio_buf는 데이터를 저장하는 실제 내부 버퍼 배열이다. (이동하지 않고, 고정된 메모리 주소를 갖는다)
        rio_buf는 주소 상수로, 배열 이름 그 자체가 배열의 시작 주소를 나타내므로, rio_buf 자체는 이동하지 않고 항상 고정된 메모리 위치를 가리킨다
        */
    }
    /*
    [주요 과정]
    버퍼가 비어 있을 때만 read 시스템 호출로 새 데이터를 채워 넣는 과정

    [매개변수]
    - n : 사용자는 특정한 바이트 수만큼 데이터를 읽고자 할 수 있다. n이 바로 이 크기
          (rio_read 함수는 n 바이트를 초과해서는 읽지 않도록, 내부 버퍼에 있는 데이터 양(rp->rio_cnt)과 n 중 작은 값을 선택하여 실제 읽을 양 결정한다)

    [rio_cnt에 read 함수 시스템 콜 반환값 저장하는 이유]
    - read 시스템 호출로 데이터를 가져올 때마다 rp->rio_cnt에 그 값을 저장하여,
      이후 함수가 얼마나 많은 데이터를 버퍼에서 읽어야 하는지를 알 수 있게 한다

    [rp->rio_cnt < 0]
    에러 발생 의미
    - EOF(End of File)에 도달하면 read는 0을 반환하므로, rio_cnt가 0이 된다
    - 에러가 발생한 경우에만 read가 -1 반환한다
    - 즉, 정상적인 데이터 읽기나 EOF가 아닌 명백한 오류가 발생한 것임
    */

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;  // 읽을 바이트 수로 설정(사용자가 읽고자 하는 바이트 크기)
    if (rp->rio_cnt < n)  //n과 rio_cnt 중 더 작은 값을
	cnt = rp->rio_cnt;  //cnt에 할당하여 요청한 바이트 수가 버퍼에 남아 있는 바이트 수보다 클 경우 남아 있는 바이트 수만큼만 복사한다
    memcpy(usrbuf, rp->rio_bufptr, cnt);  //memcpy를 사용하여 내부 버퍼(rio_bufptr)에서 usrbuf로 cnt 바이트를 복사한다
    //포인터와 카운터 업데이트
    rp->rio_bufptr += cnt;  //내부 버퍼(rio_bufptr)를 cnt만큼 이동시켜 다음 읽기 위치 가리키도록 한다 (포인터 연산) cnt 바이트만큼 오른쪽으로 이동한다
    rp->rio_cnt -= cnt;  //남은 바이트 수에서 cnt만큼 빼서 내부 버퍼에서 읽어야 할 바이트가 얼마나 남았는지 추적한다
    return cnt;  //실제로 복사된 바이트 수 반환 (호출한 함수가 얼마나 많은 바이트를 성공적으로 읽었는지 확인할 수 있도록 한다)
}
/* $end rio_read */
/*
[이 함수 역할]
rio_read 함수는 rp가 가리키는 rio_t 구조체의 내부 버퍼에서 최대 n 바이트를 읽어와 usrbuf에 저장합니다. 
리턴 값은 읽은 바이트 수(ssize_t 타입)입니다.


[read 함수]
read는 운영체제 커널에서 제공하는 시스템 호출 (저수준 입출력 함수로, 파일 디스크립터를 통해 데이터 직접 읽어온다)
- read 함수는 커널에서 실제 데이터를 읽어오는 시스템 호출이다
- unistd.h 파일에서 구현부를 볼 수 없고, 커널의 파일 시스템이나 네트워크 계층을 통해 데이터를 읽어온다
*/



/*
 * rio_readinitb - Associate a descriptor with a read buffer and reset buffer
 */
/* $begin rio_readinitb */
/* rio_t 구조체를 초기화하여 버퍼링된 입출력을 준비하는 함수 */
void rio_readinitb(rio_t *rp, int fd) 
{
    rp->rio_fd = fd;  //rio_t 구조체(rp)의 rio_fd 필드를 fd로 설정한다 (예. 클라이언트와 연결된 소켓인 connfd 일 수 있다)
    rp->rio_cnt = 0;  //현재 버퍼에 남아 있는 데이터의 바이트 수. 초기화 시 버퍼 비어 있음을 0으로 설정하여 나타낸다
    rp->rio_bufptr = rp->rio_buf;  //rio_bufptr 포인터가 rio_buf 배열의 시작 주소를 가리키도록 설정하는 것
}
/* $end rio_readinitb */
/*
rp : rio_t 구조체
fd : 특정 파일 디스크립터

rio_bufptr이 rio_buf 배열의 첫 번째 요소를 가리키며, 이후 rio_bufptr++ 를 통해 배열의 다음 요소로 이동할 수 있다
*/



/*
 * rio_readnb - Robustly read n bytes (buffered)
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;
    
    while (nleft > 0) {
	if ((nread = rio_read(rp, bufp, nleft)) < 0) 
            return -1;          /* errno set by read() */ 
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readnb */

/* 
 * rio_readlineb - Robustly read a text line (buffered)
 */
/* $begin rio_readlineb */
/* rio_t 구조체를 통해 한 줄씩 데이터를 읽어오는 함수 */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n, rc;  //n은 읽은 바이트 수 기록하는 변수 | rc는 rio_read 함수의 반환 값 저장한다
    char c, *bufp = usrbuf;  //bufp는 usrbuf를 가리키는 포인터 (데이터 순차적으로 저장함)

    for (n = 1; n < maxlen; n++) { 
        if ((rc = rio_read(rp, &c, 1)) == 1) {  //rc == 1 은 1바이트를 성공적으로 읽었다는 것
	    *bufp++ = c;
	    if (c == '\n') {
                n++;
     		break;
            }
	} else if (rc == 0) {
	    if (n == 1)
		return 0; /* EOF, no data read */
	    else
		break;    /* EOF, some data was read */
	} else
	    return -1;	  /* Error */
    }
    *bufp = 0;  //모든 반복이 끝나면, bufp가 가리키는 위치에 0(널 문자)을 추가하여 문자열 종료
    return n-1;  //최종적으로, 실제 읽은 바이트 수를 반환한다.
}
/* $end rio_readlineb */
/*
[역할]
rio_readlineb 함수는 rio_t 구조체를 통해 한 줄씩 데이터를 읽어오는 함수입니다. 
데이터를 한 글자씩 읽으면서 줄 바꿈 문자(\\n)가 나타나면 읽기를 중단하고, 해당 줄을 버퍼에 저장합니다.

이 함수는 rp를 통해 데이터를 한 글자씩 읽어 usrbuf에 저장하고, 줄 바꿈 문자가 나올 때까지 반복하여 한 줄을 읽어 반환합니다. 
EOF에 도달하면 0을, 오류가 발생하면 -1을 반환합니다


*/



/**********************************
 * Wrappers for robust I/O routines
 **********************************/
ssize_t Rio_readn(int fd, void *ptr, size_t nbytes) 
{
    ssize_t n;
  
    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
	unix_error("Rio_readn error");
    return n;
}

void Rio_writen(int fd, void *usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n)
	unix_error("Rio_writen error");
}

void Rio_readinitb(rio_t *rp, int fd)
{
    rio_readinitb(rp, fd);
} 

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
	unix_error("Rio_readnb error");
    return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
	unix_error("Rio_readlineb error");
    return rc;
} 

/******************************** 
 * Client/server helper functions
 ********************************/
/*
 * open_clientfd - Open connection to server at <hostname, port> and
 *     return a socket descriptor ready for reading and writing. This
 *     function is reentrant and protocol-independent.
 *
 *     On error, returns: 
 *       -2 for getaddrinfo error
 *       -1 with errno set for other errors.
 */
/* $begin open_clientfd */
int open_clientfd(char *hostname, char *port) {
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV;  /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG;  /* Recommended for connections */
    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", hostname, port, gai_strerror(rc));
        return -2;
    }
  
    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue; /* Socket failed, try the next */

        /* Connect to the server */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) 
            break; /* Success */
        if (close(clientfd) < 0) { /* Connect failed, try another */  //line:netp:openclientfd:closefd
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            return -1;
        } 
    } 

    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* All connects failed */
        return -1;
    else    /* The last connect succeeded */
        return clientfd;  //정수형임. 파일 디스크립터 반환됨.
}
/* $end open_clientfd */

/*  
 * open_listenfd - Open and return a listening socket on port. This
 *     function is reentrant and protocol-independent.
 *
 *     On error, returns: 
 *       -2 for getaddrinfo error
 *       -1 with errno set for other errors.
 */
/* $begin open_listenfd */
int open_listenfd(char *port) 
{
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval=1;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             /* Accept connections */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* ... on any IP address */
    hints.ai_flags |= AI_NUMERICSERV;            /* ... using port number */
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (port %s): %s\n", port, gai_strerror(rc));
        return -2;
    }

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue;  /* Socket failed, try the next */

        /* Eliminates "Address already in use" error from bind */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,    //line:netp:csapp:setsockopt
                   (const void *)&optval , sizeof(int));

        /* Bind the descriptor to the address */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; /* Success */
        if (close(listenfd) < 0) { /* Bind failed, try the next */
            fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
            return -1;
        }
    }


    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* No address worked */
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
	return -1;
    }
    return listenfd;
}
/* $end open_listenfd */

/****************************************************
 * Wrappers for reentrant protocol-independent helpers
 ****************************************************/
int Open_clientfd(char *hostname, char *port) 
{
    int rc;

    if ((rc = open_clientfd(hostname, port)) < 0) 
	unix_error("Open_clientfd error");
    return rc;
}

int Open_listenfd(char *port) 
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
	unix_error("Open_listenfd error");
    return rc;
}

/* $end csapp.c */




