#include "csapp.h"

//echo 함수 프로토타입 선언
void echo(int connfd);  //클라이언트로부터 데이터를 읽고, 받은 데이터를 그대로 다시 클라이언트로 보내는 '에코' 기능 수행

void echo(int connfd)  //confd : 클라이언트와 연결된 소켓 파일 디스크립터
{
    size_t n;  //읽은 바이트 수를 저장할 변수
    char buf[MAXLINE];  //클라이언트로부터 받은 데이터를 저장할 버퍼
    rio_t rio;  //리오(buffered I/O) 구조체로, connfd를 통해 데이터 읽고 쓸 수 있게 초기화된다

    Rio_readinitb(&rio, connfd);  //connfd 소켓 파일 디스크립터를 이용해 rio 구조체 초기화한다 => 이는 소켓을 통한 데이터를 버퍼링하여 읽을 수 있도록 준비한다
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        printf("server recieved %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
    /*
    while 루프는 클라이언트로부터 데이터를 한 줄씩(Rio_readlineb 사용) 읽어와 buf에 저장한다
    읽은 바이트 수가 0이 아닐 때만 반복된다
    */
}

/* 클라이언트가 연결되면, 클라이언트가 보낸 메시지를 그대로 되돌려주는 역할 */
int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_homename[MAXLINE], client_port[MAXLINE];

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, client_homename, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_homename, client_port);
        echo(connfd);
        Close(connfd);
    }
    exit(0);
}