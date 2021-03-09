
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>

#define RED "\e[1;31m"
#define WHITE "\e[1;37m"

#define RESET "\x1b[0m"
#define TRUE 1
#define PORT 7777
extern int errno;

int main(int argc, char *argv[])
{
  int sd;
  struct sockaddr_in server;
  char msg_sent[1000], msg_received[4000], exiting[30];
  strcpy(exiting, RED "Exiting" RESET);

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Eroare la socket().\n");
    return errno;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_port = htons(PORT);

  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("Eroare la connect().\n");
    return errno;
  }

  if (read(sd, msg_received, 200) < 0)
  {
    perror("Eroare la citire de la server.\n");
    return errno;
  }
  printf("%s\n", msg_received);

  while (TRUE)
  {
    printf(WHITE "♫♫♫♫ ➤ ");
    fflush(stdout);
    bzero(msg_sent, 1000);
    read(0, msg_sent, 1000);
    int n = strlen(msg_sent);
    if (n != 1)
      n--;
    if (write(sd, msg_sent, n) <= 0)
    {
      perror("Eroare la scriere spre server.\n");
      return errno;
    }
    printf(RESET);
    fflush(stdout);
    bzero(msg_received, 4000);

    if (read(sd, msg_received, 4000) < 0)
    {
      perror("Eroare la citire de la server.\n");
      return errno;
    }
    printf("%s\n", msg_received);

    if (!strcmp(msg_received, exiting))
    {
      close(sd);
      exit(0);
    }
  }
  return 0;
}