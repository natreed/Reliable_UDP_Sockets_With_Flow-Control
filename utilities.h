//Useful functions
#ifndef UTILITIES_H
#define UTILITIES_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include<sys/time.h>
#include <vector>
#include <array>
#include <list>

const int DATA_SZ = 1024;
const int PACK_SZ = sizeof(char) + sizeof(int)*2 + DATA_SZ;
const int ATTEMPTS = 5;

//packet
typedef struct packet 
{
  char msg_type;
  int packet_num;
  int msg_size;
  char data[DATA_SZ];
} packet;

//++++++++++++++++++++++++++++++++++++++++++=
//Control list node
typedef struct cntrl_node
{
  packet p;
  int packet_num;
} cntrl_node;

//control window
typedef std::list <cntrl_node> cntrl_window;

//++++++++++++++++++++++++++++++++++++++++++=

//Error msg Function
int check_retval (int ret_val,const char msg[]) {
  if (ret_val < 0) 
  {
    perror(msg);
    return -1;
  }
}

void set_null(char * buffer)
{
  for (int i = 0; i < PACK_SZ; ++i)  
  {
    buffer[i] = 0;
  }
}


void serialize (struct packet* p,char * serialized_packet, int buffer_size) 
{
  //build packet
  int offset = 0;
  memcpy(serialized_packet, &p->msg_type, sizeof(char));
  offset += sizeof(char);
  memcpy(serialized_packet + offset, &p->packet_num, sizeof(int)); 
  offset += sizeof(int);
  memcpy(serialized_packet + offset, &p->msg_size, sizeof(int)); 
  offset += sizeof(int);
  memcpy(serialized_packet + offset, &p->data, buffer_size); 

}

void deserialize (struct packet* p, char * buffer) 
{
 int offset = 0;
 memcpy(&p->msg_type, buffer, sizeof(char));
 offset += sizeof(char);
 memcpy(&p->packet_num, buffer + offset, sizeof(int));
 offset += sizeof(int);
 memcpy(&p->msg_size, buffer + offset, sizeof(int));
 offset += sizeof(int);
 memcpy(&p->data, buffer + offset, p->msg_size);

}

void set_timeout (int sockid) {

  //Set rcvtimeout option on socket
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;

	if (setsockopt(sockid, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    perror("Error");
	}
}

bool rcv_msg (int sockid, sockaddr_in * s_addr, char * buffer, int max_attempt)
{   
    unsigned int s_addr_len = sizeof(struct sockaddr_in);
    int status = recvfrom (sockid, buffer, PACK_SZ, 0, (struct sockaddr *) s_addr, &s_addr_len);
    check_retval(status, "msg not received"); 
    //check to see if timeout occured and send again if necessary
    while (status < 0 && max_attempt > 0) 
    {
      printf("%s", "Timeout occurred: resending\n");
      status = recvfrom (sockid, buffer, PACK_SZ, 0, 
          (struct sockaddr *) s_addr, &s_addr_len);
      max_attempt --;
    }
    
    //return false if message not received. This will allow caller to resend packet.
    if (max_attempt <= 0) {
      return false;
    }
    else {
      return true;  
    }
}


int client_handshake (struct sockaddr_in *serv_addr, int sockid) 
{
  int status = 0;
  char buffer[PACK_SZ];
  unsigned int length = sizeof(struct sockaddr_in);
  struct packet p;
  
  p.msg_type = 'H';
  p.packet_num = 0;
  const char myData[6] = "Hello";
  strcpy(p.data, myData);
  p.msg_size = sizeof(p.data);

  char data_packet[sizeof(char) + sizeof(int)*2 + sizeof(buffer)];
  serialize(&p, data_packet, sizeof(buffer));

  //send handshake  packet
  status = sendto(sockid, data_packet, sizeof(data_packet), 0, (struct sockaddr *) serv_addr, length);
  check_retval(status, "handshake failed in Sendto()");
 
  set_timeout(sockid);	 
  rcv_msg (sockid, serv_addr, buffer, 5);
  
  struct packet handshake;
  deserialize(&handshake, buffer);
  return status;

}

int server_handshake () 
{
  
}



//initialize a packet to send
int send_packet (int * pack_num, int sockid, sockaddr_in s_addr, const char * msg, char msg_type) 
{
  int status;
  struct packet to_send;
  unsigned int length = sizeof(struct sockaddr_in);
  to_send.msg_type = msg_type;
  to_send.packet_num  = *pack_num;
  strcpy(to_send.data, msg);
  to_send.msg_size = strlen(msg);
  char bytes_to_send[PACK_SZ];
  serialize(&to_send, bytes_to_send, PACK_SZ);
  
  status = sendto(sockid, bytes_to_send, sizeof(bytes_to_send), 0, (struct sockaddr *) &s_addr, length);
  check_retval(status, "failed in Sendto()");
  return status;
}

//deserialize new msg
int fetch_packet (int &pack_num, int sockid, sockaddr_in s_addr, char msg_type, char * buffer)
{
  
}



#endif
