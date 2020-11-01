/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include "API.h"

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int main(int argc, char** argv)
{ 
  int fd,c, res;
  struct termios oldtio,newtio;
  char buf[255];

  if ( (argc < 2) || 
       ((strcmp("/dev/ttyS0", argv[1])!=0) && 
        (strcmp("/dev/ttyS1", argv[1])!=0) )) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }


/*
  Open serial port device for reading and writing and not as controlling tty
  because we don't want to get killed if linenoise sends CTRL-C.
*/

  
  fd = open(argv[1], O_RDWR | O_NOCTTY );
  if (fd <0) {perror(argv[1]); exit(-1); }

  if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */



  tcflush(fd, TCIOFLUSH);

  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n\n");
  
  enum i_frame_state_machine state_machine = START_I;
    char rcvd[1];
    char *frame= malloc (sizeof(char)*(9));
    char data[4];
    int n=0;
    int d=0;
    	    
    do
    {
      res = read(fd,rcvd,1);
      printf("%x\n",rcvd[0]);
      change_I_frame_state(&state_machine, rcvd[0], frame, n);
      printf("%d",n);
      printStateI(state_machine);
      if (state_machine==BCC_OKI){
	      	data[d]=rcvd[0];
	      	d++;
      }
      n++;
    }while(state_machine != STOP_I);
    
    
    printf("Received: Data\n");
    
        for (int i=0; i<10;i++){
	printf("%x\n",frame[i]);}
	
	printf("data\n");
	
	    for (int i=0; i<6;i++){
	printf("%x\n",data[i]);}

    
/*
  do{
    
    enum s_frame_state_machine state_machine = START_S;
    char rcvd[1];
    char frame[5];
    
    do
    {
      res = read(fd,rcvd,1);
      change_s_frame_state(&state_machine, rcvd[0], frame);
    }while(state_machine != STOP_S);

    printf("Received: SET\n");
    
    char* UA = s_frame(A_EM,C_UA);
    res = write(fd,UA,5);
    printf("Sent: UA\n");
    free(UA);

  }while(buf[0] != 'z');

	printf("aqui");
	    enum i_frame_state_machine state_machine = START_I;
	    char rcvd[1];
	    char frame[9];
	    char parity=0x01;
	    int n=0;
	    printf("aqui");
	    do
	    {
	    printf("Entra aqui");
	      res = read(fd,rcvd,1);
	      printf("%x",rcvd[0]);
	      change_I_frame_state(&state_machine, rcvd[0], frame,parity,n);
	      printStateI(state_machine);
	      n++;
	    }while(state_machine != STOP_I);
	    
	    
	    printf("Received: Data\n");



    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o 
  */


  tcsetattr(fd,TCSANOW,&oldtio);
  close(fd);
  return 0;
}