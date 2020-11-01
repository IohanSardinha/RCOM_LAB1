#include "ll.h"

int role;
struct termios oldtio;

int llopen(int portN, int role_)
{
	
	struct termios newtio;
	char port[255];
	
	int fd;

	role = role_;

	sprintf(port, "/dev/ttyS%d", portN);

	fd = open(port, O_RDWR | O_NOCTTY );
	if (fd <0) {
		perror(port); 
		return -1; 
	}

	if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
		perror("tcgetattr");
		return -1;
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
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

	printf("New termios structure set\n\n");

	switch(role_)
	{
		case TRANSMITTER:
			
			if(send_s_frame_with_response(fd,A_TR,C_SET,C_UA) != OK) return -1;
			break;

		case RECIEVER:
			
			if(read_s_frame(fd, A_TR, C_SET) != OK)return -1;
    		if(send_s_frame(fd, A_RC, C_UA) != OK )return -1;
    		break;	
	}

	return fd;
}

int llclose(int fd){
	switch(role)
	{
		case TRANSMITTER:
			if(send_s_frame_with_response(fd,A_TR,C_DISC, C_DISC) != OK) return -1;
			if(send_s_frame(fd, A_TR, C_UA) < 0) return -1;
			break;
		case RECIEVER:
			if(read_s_frame(fd,A_TR,C_DISC) < 0) return -1;
			if(send_s_frame_with_response(fd,A_RC,C_DISC, C_UA) != OK) return -1;
			break;
	}

	if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    	perror("tcsetattr");
    	return -1;
  	}
    return close(fd);
}