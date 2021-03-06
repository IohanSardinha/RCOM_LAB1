#include "ll.h"

int role, Ns = 0;
struct termios oldtio;
bool connected = false;


void handle_alarm()
{
	if(!connected)
	{	
		fprintf(stderr,"Could not open port\n");
		exit(1);
	}
}

int llopen(int portN, int role_)
{
	
	struct termios newtio;
	char port[255];
	
	int fd;

	role = role_;

	sprintf(port, "/dev/ttyS%d", portN);

	if(!debug) printf("Connecting to %s...\n", port);

	(void) signal(SIGALRM, handle_alarm);
	alarm(read_time_out);

	fd = open(port, O_RDWR | O_NOCTTY);
	if (fd <0) {
		perror(port); 
		return -1; 
	}
	connected = true;

	if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
		perror("tcgetattr");
		return -1;
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE0 | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	newtio.c_lflag = 0;
	newtio.c_cc[VTIME]    = 1;
	newtio.c_cc[VMIN]     = 0;


	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
		perror("tcsetattr");
		return -1;
	}

	if(!debug) printf("New termios structure set\n");

	switch(role_)
	{
		case TRANSMITTER:
			
			if(send_s_frame_with_response(fd,A_TR,C_SET,C_UA, A_TR) != OK) return -1;
			break;

		case RECIEVER:
			if(read_s_frame(fd, A_TR, C_SET) != OK)return -1;
    		if(send_s_frame(fd, A_TR, C_UA) != OK )return -1;
    		break;	
	}

	return fd;
}


int llread(int fd, unsigned char* packetbuff){

	int res = read_i_frame_with_response(fd,packetbuff);
		
	return res;
}

int llwrite(int fd, unsigned char* buffer, int lenght)
{
	int res = send_i_frame_with_response(fd,A_TR, (Ns == 0)?C_I_0:C_I_1 , buffer, lenght, Ns);

	if(res < 0)
		return res;
	
	Ns = (Ns +1) % 2;

	return res;
}


int llclose(int fd){
	switch(role)
	{
		case TRANSMITTER:
			if(send_s_frame_with_response(fd,A_TR,C_DISC,C_DISC, A_RC) != OK) return -1;
			if(send_s_frame(fd, A_RC, C_UA) < 0) return -1;
			break;
		case RECIEVER:
			if(read_s_frame(fd,A_TR,C_DISC) < 0) return -1;
			if(send_s_frame_with_response(fd,A_RC,C_DISC, C_UA, A_RC) != OK) return -1;
			break;
	}

	sleep(1);

	if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    	perror("tcsetattr");
    	return -1;
  	}
    return close(fd);
}
