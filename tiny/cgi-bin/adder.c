#include "csapp.h"

int main(void)
{
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    p = strchr(buf, '&'); // '&' 위치 찾기
    if (p != NULL)
    {
      *p = '\0'; // '&'를 '\0'로 바꿔서 첫 번째 숫자를 추출할 준비

      // '&'를 기준으로 숫자 두 개를 추출
      strcpy(arg1, buf);   // 첫 번째 숫자 추출
      strcpy(arg2, p + 1); // 두 번째 숫자 추출

      n1 = atoi(arg1); // 문자열을 정수로 변환
      n2 = atoi(arg2);
    }
  }

  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s", getenv("QUERY_STRING"));
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>",
          content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);

  exit(0);
}