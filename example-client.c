
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>

int main(int argc, char* argv[])
{
    char *ip;
    char *port;
    char *string;
    char *operation;
    int opt;

    bool hasip, hasport, hasstring, hasoper;

    while((opt = getopt(argc, argv, "a:p:s:o:")) != -1)
    {
      switch(opt)
      {
        case 'a':
          hasip = true;
          ip = optarg;
          break;
        case 'p':
          hasport = true;
          port = optarg;
          break;
        case 's':
          hasstring = true;
          string = optarg;
          break;
        case 'o':
          if(strcmp(optarg, "toupper") != 0 && strcmp(optarg, "tolower") != 0  && strcmp(optarg, "invert") != 0 )
          {
            printf("Niedozwolona operacja, dozwolone operacje: tolower, toupper, invert\n");
            return 0;
          }
          hasoper = true;
          operation = optarg;
          break;
      }
    }

    if(hasip == false || hasport == false || hasstring == false || hasoper == false)
    {
      printf("Uzycie: -a <adres ip> -p <port> -s <ciag znakow> -o <operacja>");
      return 0;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    int portnum = atoi(port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Nie udalo sie stworzyc socketa...\n");
        exit(0);
    }
    else
        printf("Socket stworzony pomyslnie..\n");
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(portnum);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Nie udalo polaczyc sie z serwerem...\n");
        exit(0);
    }
    else
        printf("Polaczono z serwerem..\n");

    char buff[80];
    int n;
    bzero(buff, sizeof(buff));

    strcat(buff, operation);
    strcat(buff, " ");
    strcat(buff, string);

    write(sockfd, buff, sizeof(buff));
    bzero(buff, sizeof(buff));
    read(sockfd, buff, sizeof(buff));
    printf("Wiadomosc od serwera: %s\n", buff);

    close(sockfd);
}
