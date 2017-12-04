#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define QSIZE 5
#define BSIZE 256
#define SOCKET_ADDRESS "mysock"
#define NSTRS 3
/*
 * Convert a null-terminated sting (one whose end is denoted by a byte
 * containing '\0') to all upper case letters by starting at the
 * beginning and going until the null byte is found.
 */
void
convert_string (char *cp)
{
  char *currp;	/* pointer to current position in the input string */
  int c;        /* return value of toupper is the converted letter */

  for (currp = cp; *currp != '\0'; currp++) {
    c = toupper (*currp);
    *currp = (char) c;
  }
}

char *strs[NSTRS] = {
  "THIS IS THE FIRST STRING FROM THE CLIENT\n",
  "this is the second string from the client\n",
  "this is the third string from the client\n"
};


int main(int argc, char *argv[])
{
  int handshake_sockfd, session_sockfd, ret;
  struct sockaddr_un saun;
  struct sockaddr *addr; 
  char buf[BSIZE];
  
  
  

  /* Add Code: Populate the sockaddr_un struct */
  
  saun.sun_family = AF_UNIX;
  strncpy(saun.sun_path, SOCKET_ADDRESS, sizeof(saun.sun_path) - 1);
  /* Add Code: Create the handshake socket */
  
  handshake_sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (handshake_sockfd < 0) {
    fprintf(stderr, "\nError Opening Socket, ERROR#%d\n", errno);
    return EXIT_FAILURE;
  }
 
  
  /* 
   * We need to unlink the address before binding to ensure the
   * address is free before attempting to bind.
   */
  unlink(saun.sun_path);
  addr = (struct sockaddr *)&saun;


  /* Add Code: Bind the handshake socket to the sockaddr. */ 
  ret = bind(handshake_sockfd, addr, sizeof(saun));
  if (ret < 0) {
    fprintf(stderr, "\nError Binding Socket, ERROR#%d\n", errno);
    return EXIT_FAILURE;
  }
  
  ret = listen(handshake_sockfd, 1);
  /* Add Code: Make the handshake socket a listening socket, with a
   * specified Queue Size
   */
  if (ret < 0) {
    fprintf(stderr, "\nError Listening on Socket, ERROR#%d\n", errno);
    return EXIT_FAILURE;
  }
  
  session_sockfd = accept(handshake_sockfd, NULL, NULL);
  /* Add Code: Accept a connection on the handshake socket,
   * giving the session socket as the return value.
   */
  if (session_sockfd < 0) {
    fprintf(stderr, "\nError Accepting Socket, ERROR#%d\n", errno);
    return EXIT_FAILURE;
  }

  /* Add Code: Read lines one at a time from the connected session
   * socket. Convert each line to uppercase using convert_string, and
   * write the line back to the client. Continue until there are no
   * more lines to read.
   */
   int  bytes;
  while (1) {
		
		bytes = read(session_sockfd, buf,BSIZE);
    if (bytes<0)
       fprintf(stderr,"\n Error receiving data at server, ERROR %d\n",errno);
    
    if(bytes == 0)
      break;

    buf[bytes-1]='\0';
    printf("RECEIVED:\n%s\n", buf);
    convert_string(buf);
    bytes = write(session_sockfd,buf,BSIZE);
    if (bytes<0)
      fprintf(stderr,"\n Error sending data at server, ERROR %d\n",errno);

    printf("SENDING:\n%s\n", buf);
  }

  close(session_sockfd);
  close(handshake_sockfd);

}
