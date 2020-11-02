#include "frame.h"

int tries;
enum s_frame_state_machine state_machine;
volatile bool TIME_OUT;

static int send_time_out = 3, read_time_out = 7, send_tries = 3; 

char* header_to_string(unsigned char C)
{
	switch(C)
	{
		case C_SET:
			return "SET";
		case C_UA:
			return "UA";
		case C_DISC:
			return "DISC";
		case C_I_0:
			return "C_I, Ns = 0";
		case C_I_1:
			return "C_I, Ns = 1";
		case C_RR_0:
			return "RR, Ns = 0";
		case C_RR_1:
			return "RR, Ns = 1";
		case C_REJ_0:
			return "REJ, Ns = 0";
		case C_REJ_1:
			return "REJ, Ns = 1";
	}
	return "NONE";
}

void handleAlarm(){
	TIME_OUT = true;
}

char* s_frame(char A, char C)
{
	char* frame = malloc(sizeof(char)*S_FRAME_SIZE);
	frame[0] = FLAG;
	frame[1] = A;
	frame[2] = C;
	frame[3] = A^C;
	frame[4] = FLAG;
	return frame;
}

int send_s_frame(int fd,char A, char C)
{

	unsigned char* frame = s_frame(A,C);

	if(write(fd,frame,S_FRAME_SIZE) < 0)
		return -1;

    free(frame);

    if(debug) printf("Sent: %s\n", header_to_string(C));

	return OK;
}

int send_s_frame_with_response(int fd, char A, char C, char response)
{
	int ret;
    unsigned char rcvd[1];
    unsigned char frame[5];

    tries = 0;
    state_machine = START_S;

    (void) signal(SIGALRM, handleAlarm);
    
    do
  	{
      	tries++;
	    TIME_OUT = false;

		if(send_s_frame(fd, A, C) != OK) return -1;

	    alarm(send_time_out);

	    state_machine = START_S;
	    
	    do{
	      ret = read(fd,rcvd,1);
	      if(TIME_OUT) 
	      	break;
	      if(ret == 0) continue;
	      change_s_frame_state(&state_machine, rcvd[0], frame, A, response);
	    }while(state_machine!=STOP_S);
	    
  	}while(state_machine != STOP_S && tries<send_tries);

  	if(tries >= send_tries) return -1;

    if(debug) printf("Recieved response: %s\n", header_to_string(response));
	
	return OK;
}

int read_s_frame(int fd, char A, char C)
{
	unsigned char rcvd[1];
    unsigned char frame[5];
    
    int res;

	state_machine = START_S;
	TIME_OUT = false;

    (void) signal(SIGALRM, handleAlarm);

    alarm(read_time_out);
    
    do
    {
      res = read(fd,rcvd,1);
      if(TIME_OUT) return -1;
      if(res == 0) continue;
      change_s_frame_state(&state_machine, rcvd[0], frame, A, C);
      alarm(read_time_out);
    }while(state_machine != STOP_S);


    if(debug) printf("Read: %s\n", header_to_string(C));
    
    return OK;
}

void change_s_frame_state(enum s_frame_state_machine* state, char rcvd, char* frame, char A, char C)
{
	switch(*state)
	{
		case START_S:
		
			if(rcvd == FLAG)
			{
				*state = FLAG_RCV;
				frame[0] = FLAG;
			}
			//else keep state
			
			break;
		
		case FLAG_RCV:
		
			if((rcvd == A_TR || rcvd == A_RC) && rcvd == A)
			{
				*state = A_RCV;
				frame[1] = A;
			}
			else if(rcvd != FLAG)
				*state = START_S;
			//else keep state
			
			break;

		case A_RCV:
		
			if((rcvd == C_SET || rcvd == C_UA || rcvd == C_DISC) && rcvd == C)
			{
				*state = C_RCV;
				frame[2] = C;
			}
			else if(rcvd == C_RR_0)
			{
				*state = C_RCV;
				frame[2] = C_RR_0;	
			}
			else if(rcvd == C_RR_1)
			{
				*state = C_RCV;
				frame[2] = C_RR_1;	
			}
			else if(rcvd == C_REJ_0)
			{
				*state = C_RCV;
				frame[2] = C_REJ_0;	
			}
			else if(rcvd == C_REJ_1)
			{
				*state = C_RCV;
				frame[2] = C_REJ_1;	
			}
			else if(rcvd == FLAG)
			{
				*state = FLAG_RCV;
			}
			else
				*state = START_S;
			
			break;

		case C_RCV:
		
			if(rcvd == frame[1]^frame[2])
			{
				*state = BCC_OK;
				frame[3] = rcvd;
			}
			else if(rcvd == FLAG)
			{
				*state = FLAG_RCV;
			}
			else
			{
				*state = START_S;
			}
			
			break;

		case BCC_OK:
		
			if(rcvd == FLAG)
			{
				*state = STOP_S;
				frame[4] = FLAG;
			}
			else *state=START_S;
			
			break;
	}
}

char* i_frame( char* data, char A, unsigned char C, int tamanho, int* frameSize){
	char parity=data[0];
	int oversize=0;
	
	for (int i=0; i<tamanho;i++){
		if (i!=0)parity=parity^data[i];
		if (data[i] == FLAG || data[i] == ESC){
			oversize++;		
		}
	}
	
	if (parity== FLAG || parity== ESC)oversize++;
	int size=sizeof(char)*(6+tamanho+oversize);
	
	char* frame= malloc (size);
	frame[0] = FLAG;
	frame[1] = A;
	frame[2] = C;
	frame[3] = A^C;
	
	int actual=4;
	//stuffing and counting parity
	for (int i=0; i<tamanho;i++){
		
		if (data[i] == FLAG){
			frame[i+actual]=0x7d;
			frame[i+actual+1]=0x5e;
			actual+=1;
			
		}else if (data[i]==ESC){
			frame[i+actual]=0x7d;
			frame[i+actual+1]=0x5d;
			actual+=1;
		
		}else{
			frame[i+actual]=data[i];
		}
		
	}
	if (parity==FLAG){
		frame[actual+tamanho]=0x7d;
		frame[actual+tamanho+1]=0x5e;
		actual+=1;
	}
	else if (parity==ESC){
		frame[actual+tamanho]=0x7d;
		frame[actual+tamanho+1]=0x5d;
		actual+=1;
	}
	else{frame[actual+tamanho]=parity;}
	
	frame[actual+tamanho+1]= FLAG;	
	
	*frameSize=6+tamanho+oversize;
	
	return frame;

}

int send_i_frame(int fd, char A, unsigned char C, char* data, int lenght)
{
	int frame_size, res;
	unsigned char* frame = i_frame(data, A, C, lenght, &frame_size);

	if(write(fd, frame, frame_size) < 0)
		return -1;

	free(frame);

	if(debug) printf("Sent: %s\n",header_to_string(C));

	return frame_size;
}

int send_i_frame_with_response(int fd, char A, char C, char* data, int lenght, int Ns)
{
	int ret, size;
    unsigned char rcvd[1];
	unsigned char frame[5];

    tries = 0;
    state_machine = START_S;

    (void) signal(SIGALRM, handleAlarm);
    
    do
  	{
	    tries++;
	    
	    TIME_OUT = false;

		if((size = send_i_frame(fd, A, C, data, lenght)) < 0) return -1;

	    alarm(send_time_out);

	    state_machine = START_S;
	    
	    do{
	      ret = read(fd,rcvd,1);
	      if(TIME_OUT)
	      	break;
	      if(ret == 0) continue;
	      change_s_frame_state(&state_machine, rcvd[0], frame, A, C_RET_I);
	    }while(state_machine!=STOP_S);

	    if((!(frame[2] == C_RR_0 && Ns == 1) || (frame[2] == C_RR_1 && Ns == 0)))
	    	state_machine = START_S;
	    
    	if(debug && !TIME_OUT) printf("Recieved response: %s\n", frame[2] == -1 ? "NONE" : header_to_string(frame[2]));
  	
  	}while(state_machine != STOP_S && tries<send_tries);

  	if(tries >= send_tries) return -1;

	
	return size;
}