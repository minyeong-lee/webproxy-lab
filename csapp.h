/*
 * csapp.h - prototypes and definitions for the CS:APP3e book
 */
/* $begin csapp.h */
#ifndef __CSAPP_H__
#define __CSAPP_H__

/* 다양한 표준 라이브러리 헤더 파일 포함 */
#include <stdio.h>  //파일 입출력
#include <stdlib.h>  //메모리 관리
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h> //errno 전역 변수
#include <math.h>
#include <pthread.h>  //멀티스레딩
#include <semaphore.h>
#include <sys/socket.h>  //소켓 프로그래밍
#include <netdb.h>  //네트워크 통신
#include <netinet/in.h>
#include <arpa/inet.h>  //소켓 프로그래밍

/* Default file permissions are DEF_MODE & ~DEF_UMASK */
/* $begin createmasks */
/* 기본 파일 권한 설정 */
#define DEF_MODE   S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH
#define DEF_UMASK  S_IWGRP|S_IWOTH
/* $end createmasks */

/* Simplifies calls to bind(), connect(), and accept() */
/* $begin sockaddrdef */
/* 소켓 주소 구조체 정의 */
typedef struct sockaddr SA;
/* $end sockaddrdef */

/* Persistent state for the robust I/O (Rio) package */
/* $begin rio_t */
/* Rio 패키지용 버퍼 구조체 */
/* I/O 작업을 좀 더 안전하게 하기 위해 정의된 Robust I/O 패키지의 일환 */
#define RIO_BUFSIZE 8192
typedef struct {
    int rio_fd;                /* Descriptor for this internal buf | 소켓 파일 디스크립터 */ 
    int rio_cnt;               /* Unread bytes in internal buf | 현재 버퍼에 남아 있는 읽지 않은 데이터의 양(바이트 수) */
    char *rio_bufptr;          /* Next unread byte in internal buf | 버퍼 내에서 다음에 읽을 위치를 가리키는 포인터 (현재 읽기 위치를 나타내는 가변 포인터임) */
    char rio_buf[RIO_BUFSIZE]; /* Internal buffer | 배열 이름 자체가 배열의 시작 주소를 나타냄  */
} rio_t;
/* $end rio_t */

/* External variables */
/* 외부 변수 */
extern int h_errno;    /* Defined by BIND for DNS errors */ 
extern char **environ; /* Defined by libc */

/* Misc constants */
/* 기타 상수 */
#define	MAXLINE	 8192  /* Max text line length */
#define MAXBUF   8192  /* Max I/O buffer size */
#define LISTENQ  1024  /* Second argument to listen() */

/* Our own error-handling functions */
/* 에러 핸들링 함수 */
void unix_error(char *msg);
void posix_error(int code, char *msg);
void dns_error(char *msg);
void gai_error(int code, char *msg);
void app_error(char *msg);

/* Process control wrappers */
/* 프로세스 제어 래퍼 */
/* 프로세스 생성(Fork), 실행(Execve), 대기(Wait), 종료(Kill) 등 프로세스 제어에 필요한 함수들을 래핑하여 정의한 것 */
/* C의 기본 시스템 호출을 조금 더 간단하게 사용할 수 있도록 돕는다 */
pid_t Fork(void);
void Execve(const char *filename, char *const argv[], char *const envp[]);
pid_t Wait(int *status);
pid_t Waitpid(pid_t pid, int *iptr, int options);
void Kill(pid_t pid, int signum);
unsigned int Sleep(unsigned int secs);
void Pause(void);
unsigned int Alarm(unsigned int seconds);
void Setpgid(pid_t pid, pid_t pgid);
pid_t Getpgrp();

/*
pid_t 타입은 C에서 공식적으로 정의된 데이터 타입
프로세스 ID를 저장하기 위해 사용되며, 시스템마다 구현이 다를 수 있음
일반적으로 int 또는 short 등의 기본형을 기반으로 하지만, 시스템마다 최적화되어 있음
*/

/* Signal wrappers */
/* 시그널 처리 함수 */
/* 시그널을 등록하거나, 시그널 마스크를 조작하는 함수들 */
/* Signal을 통해 특정 시그널에 대해 처리할 함수를 지정할 수 있다 */
/* 시그널은 운영체제가 프로세스에 보내는 알림으로,
   특정 이벤트가 발생했을 때 프로세스가 이에 반응할 수 있도록 해 준다.
*/
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
void Sigemptyset(sigset_t *set);
void Sigfillset(sigset_t *set);
void Sigaddset(sigset_t *set, int signum);
void Sigdelset(sigset_t *set, int signum);
int Sigismember(const sigset_t *set, int signum);
int Sigsuspend(const sigset_t *set);

/* Sio (Signal-safe I/O) routines */
/* 시그널 처리 중에도 안전하게 입출력 할 수 있는 함수들 */
ssize_t sio_puts(char s[]);
ssize_t sio_putl(long v);
void sio_error(char s[]);

/* Sio wrappers */
/* 위에서 정의한 함수의 래퍼로, 함수 이름에 대문자 Sio_ 붙여 구분할 수 있게 하였다 */
ssize_t Sio_puts(char s[]);
ssize_t Sio_putl(long v);
void Sio_error(char s[]);

/* Unix I/O wrappers */
/* Unix 시스템 호출을 래핑한 함수들 */
/* 파일과 파일 디스크립터를 다루기 위해 사용된다 */
/* 파일 및 디스크립터 작업을 좀 더 간단하고 오류 줄여서 처리할 수 있도록 래핑한 함수들 */
int Open(const char *pathname, int flags, mode_t mode);  //파일 열거나 새로 생성
ssize_t Read(int fd, void *buf, size_t count);  //파일 디스크립터를 통해 데이터를 읽는 함수
ssize_t Write(int fd, const void *buf, size_t count); //파일 디스크립터에 데이터를 쓴다
off_t Lseek(int fildes, off_t offset, int whence); //파일 디스크립터가 가리키는 파일의 위치를 이동한다
void Close(int fd);  //열려 있는 파일 디스크립터를 닫아준다
int Select(int  n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, 
	   struct timeval *timeout);  //여러 파일 디스크립터에서 입출력 이벤트를 기다릴 때 사용된다
void Stat(const char *filename, struct stat *buf);  //파일의 정보를 가져온다
void Fstat(int fd, struct stat *buf);  //열려 있는 파일 디스크립터로부터 파일 정보를 가져온다

/* Directory wrappers */
/* 디렉토리 작업을 위한 함수들(파일을 조직화하는 폴더. 파일에 접근하는 경로) */
/*
Opendir로 디렉토리를 열어 DIR 포인터를 받고, 
Readdir로 디렉토리 내 항목들을 순차적으로 읽습니다. 작업이 끝나면 Closedir로 디렉토리를 닫아줍니다.
*/
DIR *Opendir(const char *name);  //지정된 디렉토리 열어 디렉토리 스트림 포인터 반환하는 함수
struct dirent *Readdir(DIR *dirp);  //열린 디렉토리 스트림(dirp)에서 다음 항목 읽어오는 함수
int Closedir(DIR *dirp);  //열린 디렉토리 스트림을 닫는 함수

/* Memory mapping wrappers */
/* 메모리 매핑을 위한 함수들 */
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset); //파일을 메모리에 매핑하여 프로세스의 주소 공간에서 파일을 다룰 수 있게 한다
void Munmap(void *start, size_t length);  //매핑된 메모리를 해제한다

/* Standard I/O wrappers */
/* 표준 I/O 함수 래퍼 */
/* C의 표준 파일 입출력을 위한 함수들 */
void Fclose(FILE *fp);
FILE *Fdopen(int fd, const char *type);
char *Fgets(char *ptr, int n, FILE *stream);
FILE *Fopen(const char *filename, const char *mode);
void Fputs(const char *ptr, FILE *stream);
size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/* 동적 메모리 할당을 위한 함수들 */
/* Dynamic storage allocation wrappers */
void *Malloc(size_t size);
void *Realloc(void *ptr, size_t size);
void *Calloc(size_t nmemb, size_t size);
void Free(void *ptr);

/* 소켓 인터페이스를 위한 래퍼 함수들 */
/*
    네트워크 프로그래밍에서 서버와 클라이언트 간 통신을 설정하고 관리하는 데 사용된다
*/
/* Sockets interface wrappers */
int Socket(int domain, int type, int protocol);  //새로운 소켓 생성
void Setsockopt(int s, int level, int optname, const void *optval, int optlen);  //소켓의 옵션 설정
void Bind(int sockfd, struct sockaddr *my_addr, int addrlen); //소켓을 특정 IP 주소와 포트에 연결
void Listen(int s, int backlog); //소켓을 수신 대기 상태로 설정하여, 연결 요청을 대기하도록 함
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);  //대기 중인 연결 요청을 수락하고, 새로운 소켓을 생성하여 클라이언트와 통신할 수 있게 한다
void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen);  //클라이언트가 서버에 연결을 요청할 때 사용된다

/* 프로토콜 독립적인 네트워크 주소 처리를 위한 래퍼 함수들 */
/* Protocol independent wrappers */
void Getaddrinfo(const char *node, const char *service, 
                 const struct addrinfo *hints, struct addrinfo **res); //호스트 이름과 서비스 이름을 소켓 주소로 변환하는 함수
void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, 
                 size_t hostlen, char *serv, size_t servlen, int flags); //소켓 주소를 호스트 이름과 서비스 이름으로 변환하는 함수
void Freeaddrinfo(struct addrinfo *res); //Getaddrinfo로 할당된 메모리를 해제하는 함수
void Inet_ntop(int af, const void *src, char *dst, socklen_t size);  //이진 형식의 IP 주소를 문자열 형식으로 변환
void Inet_pton(int af, const char *src, void *dst); //문자열 형식의 IP 주소를 이진 형식으로 변환

/* DNS wrappers */
struct hostent *Gethostbyname(const char *name);
struct hostent *Gethostbyaddr(const char *addr, int len, int type);

/* Pthreads thread control wrappers */
void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp, 
		    void * (*routine)(void *), void *argp);
void Pthread_join(pthread_t tid, void **thread_return);
void Pthread_cancel(pthread_t tid);
void Pthread_detach(pthread_t tid);
void Pthread_exit(void *retval);
pthread_t Pthread_self(void);
void Pthread_once(pthread_once_t *once_control, void (*init_function)());

/* POSIX semaphore wrappers */
void Sem_init(sem_t *sem, int pshared, unsigned int value);
void P(sem_t *sem);
void V(sem_t *sem);

/* Rio (Robust I/O) package */
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
void rio_readinitb(rio_t *rp, int fd); 
ssize_t	rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t	rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

/* Wrappers for Rio package */
ssize_t Rio_readn(int fd, void *usrbuf, size_t n);
void Rio_writen(int fd, void *usrbuf, size_t n);
void Rio_readinitb(rio_t *rp, int fd); 
ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

/* Reentrant protocol-independent client/server helpers */
int open_clientfd(char *hostname, char *port);
int open_listenfd(char *port);

/* Wrappers for reentrant protocol-independent client/server helpers */
int Open_clientfd(char *hostname, char *port);
int Open_listenfd(char *port);


#endif /* __CSAPP_H__ */
/* $end csapp.h */
