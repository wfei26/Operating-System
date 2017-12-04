#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BSIZE 256
#define NSTRS 3
#define SOCKET_ADDRESS "mysock"

/*
 * This is the set of strings we will send through the socket for
 * conversion
 */
char *strs[NSTRS] = {
  "this is the first string from the client\n",
  "this is the second string from the client\n",
  "this is the third string from the client\n"
};

int main(int argc, char *argv[])
{
  int sockfd, ret, i;
  struct sockaddr_un saun;
 // struct sockaddr *addr;
  char buf[BSIZE];
    

  /* Add Code: Populate the sockaddr_un struct */
  
  saun.sun_family = AF_UNIX;
  strncpy(saun.sun_path, SOCKET_ADDRESS, sizeof(saun.sun_path) - 1);
  
  sockfd = socket(PF_UNIX, SOCK_STREAM, 0); 
 
  /* Add Code: Create the client session socket */
  if (sockfd < 0) {
    fprintf(stderr, "\nError Opening Socket, ERROR#%d\n", errno);
    return EXIT_FAILURE;
  }


  
  /* Add Code: Connect the session socket to the server */
  
  ret = connect(sockfd,(struct sockaddr *) &saun,sizeof(saun));
  if (ret < 0) {
    fprintf(stderr, "\nError Connecting Sockets, ERROR#%d\n", errno);
    return EXIT_FAILURE;
  }


  /* Add Code: Send the strs array, one string at a time, to the
   * server. Read the converted string and print it out before sending
   * the next string
   */

	int bytes;
  for (i = 0; i < NSTRS; i++) {
    
    write(sockfd, strs[i], strlen(strs[i]));
    printf("SENDING:\n%s", strs[i]);
    bytes = read(sockfd, buf, BSIZE);
    if (bytes<0)
       fprintf(stderr,"\n Error receiving data at server, ERROR %d\n",errno);
    buf[ret-1]='\0';
    printf("RECEIVED:\n%s\n", buf);
    
  }
  close(sockfd);

}
