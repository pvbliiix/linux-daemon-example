#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>


static void demon()
{
    pid_t pid;

    pid = fork();

    if (pid < 0)
        exit(EXIT_FAILURE);

    if (pid > 0)
        exit(EXIT_SUCCESS);

    if (setsid() < 0)
        exit(EXIT_FAILURE);

    pid = fork();

    if (pid < 0)
        exit(EXIT_FAILURE);

    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    chdir("/");

    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }
}

int main(int argc, char* argv[])
{

  int port, opt;

  int total_length = 32;
  bool hasport = false;

  char line[total_length];
  char cmd[32] = "pidof example-server -o ";
  char ownpid[5];
  sprintf(ownpid, "%d", getpid());
  strcat(cmd, ownpid);
  FILE * command = popen(cmd,"r");

  fgets(line,total_length,command);

  pid_t pid = strtoul(line,NULL,10);

  pclose(command);

  if(argc < 2)
  {
    printf("Uzycie: -p <port> lub -q aby wylaczyc serwer\n");
    return 0;
  }

  while((opt = getopt(argc, argv, "p:q")) != -1)
  {
    switch(opt)
    {
      case 'p':
        if(pid != 0)
        {
          printf("Serwer jest juz uruchomiony, uzyj parametru -q aby go wylaczyc\n");
          return 0;
        }
        port = atoi(optarg);
        hasport = true;
        break;
      case 'q':
        kill(pid, SIGKILL);
        printf("Wylaczono instancje serwera\n");
        return 0;
        break;
    }
  }

  if(hasport == false)
    return 0;
    demon();

    while (1)
    {
      int    len, rc, on = 1;
      int    listen_sd = -1, new_sd = -1;
      int    desc_ready, end_server = 0;
      int    close_conn;
      char   buffer[80];
      struct sockaddr_in6   addr;
      int    timeout;
      struct pollfd fds[200];
      int    nfds = 1, current_size = 0, i, j;

      listen_sd = socket(AF_INET6, SOCK_STREAM, 0);

      if (listen_sd < 0)
        exit(-1);

      rc = setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR,
                      (char *)&on, sizeof(on));
      if (rc < 0)
      {
        close(listen_sd);
        exit(-1);
      }

      rc = ioctl(listen_sd, FIONBIO, (char *)&on);
      if (rc < 0)
      {
        close(listen_sd);
        exit(-1);
      }

      memset(&addr, 0, sizeof(addr));
      addr.sin6_family      = AF_INET6;
      memcpy(&addr.sin6_addr, &in6addr_any, sizeof(in6addr_any));
      addr.sin6_port        = htons(port);
      rc = bind(listen_sd,
                (struct sockaddr *)&addr, sizeof(addr));
      if (rc < 0)
      {
        close(listen_sd);
        exit(-1);
      }

      rc = listen(listen_sd, 32);
      if (rc < 0)
      {
        close(listen_sd);
        exit(-1);
      }

      memset(fds, 0 , sizeof(fds));

      fds[0].fd = listen_sd;
      fds[0].events = POLLIN;

      timeout = (3 * 60 * 1000);

      do
      {
        rc = poll(fds, nfds, timeout);

        if (rc <= 0)
          break;

        current_size = nfds;
        for (i = 0; i < current_size; i++)
        {
          if(fds[i].revents == 0)
            continue;

          if(fds[i].revents != POLLIN)
          {
            end_server = 1;
            break;

          }
          if (fds[i].fd == listen_sd)
          {
            do
            {
              new_sd = accept(listen_sd, NULL, NULL);
              if (new_sd < 0)
              {
                if (errno != EWOULDBLOCK)
                  end_server = 1;

                break;
              }

              fds[nfds].fd = new_sd;
              fds[nfds].events = POLLIN;
              nfds++;

            } while (new_sd != -1);
          }

          else
          {
            close_conn = 0;

            do
            {
              rc = recv(fds[i].fd, buffer, sizeof(buffer), 0);
              if (rc < 0)
              {
                if (errno != EWOULDBLOCK)
                  close_conn = 1;

                break;
              }
              else if (rc == 0)
              {
                close_conn = 1;
                break;
              }

              len = rc;

              char operation[8];
              unsigned char string[80];

              sscanf(buffer, "%s %s", operation, string);

              bzero(buffer, sizeof(buffer));

              if(strcmp(operation, "tolower") == 0)
              {
                for(int i = 0; i < strlen(string); i++)
                  string[i] = tolower(string[i]);
              }
              else if(strcmp(operation, "toupper") == 0)
              {
                for(int i = 0; i < strlen(string); i++)
                  string[i] = toupper(string[i]);
              }
              else if(strcmp(operation, "invert") == 0)
              {
                char temp[80];
                for(int i = 0; i < strlen(string); i++)
                    temp[i] = string[strlen(string)-(1+i)];

                temp[strlen(string)] = '\0'; 

                memcpy(&string, &temp, sizeof(string));
              }

              if(send(fds[i].fd, string, sizeof(string), 0) < 0)
              {
                close_conn = 1;
                break;
              }

            } while(1);

            if (close_conn)
            {
              close(fds[i].fd);
              fds[i].fd = -1;
            }


          }
        }

      } while (end_server == 0);

      for (i = 0; i < nfds; i++)
      {
        if(fds[i].fd >= 0)
          close(fds[i].fd);
      }
        break;
    }
    return EXIT_SUCCESS;
}
