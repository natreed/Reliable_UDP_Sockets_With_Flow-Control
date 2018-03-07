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
#include <sys/time.h>
#include <vector>
#include <array>
#include <list>
#include <bitset>
#include <algorithm>
#include <pthread.h>
#include <thread>
#include <mutex>
#include <math.h>

const int DATA_SZ = 1024;
const int PACK_SZ = sizeof(char) + sizeof(int)*2 + DATA_SZ;
const int MAX_ATTEMPTS = 5;
const int TIMEOUT = 2;
const int MAX_WIN_SZ = 30;

//Message Codes:
char HANDSHAKE = 'H'; //H : handshake
char SIZE = 'S';      //S : size of file (server)
char REQUEST = 'R';   //R : Request (client)
char ACK = 'A';       //A : ACK
char DATA = 'D';      //D : Data

enum packet_status :  short  {UNUSED, SENT, ACKED};

//prototypes
void set_null(char *);
struct packet;
int send_packet (packet, int sockid, sockaddr_in s_addr); 
void rcv_msg (char *   buffer, int sockid, sockaddr_in * s_addr);
void deserialize (struct packet* p, char * buffer);
class ctrl_node;
int st_wrapper(ctrl_node cn) ;
int  set_timeout_(); 


//packet
typedef struct packet 
{
  packet() : msg_type('0'), packet_num(0), msg_size(0), status(UNUSED) 
  {
    set_null(data);
  }

  packet (char type, int pnum, const char *msg_data) 
  {
    msg_type = type;
    packet_num = pnum;
    strcpy(data, msg_data);
    msg_size = strlen(data);
  }
  char msg_type;
  int packet_num;
  int msg_size;
  //all elements initialized to zero in c++
  char data[DATA_SZ];
  //Status
  int status;
} packet; 

//++++++++++++++++++++++++++++++++++++++++++=
//Control list node
class ctrl_node
{
  public:
  ctrl_node(packet pack, int sock_id, sockaddr_in s) : p(pack), attempt_ct(0), running(true)
  { 
    sockid = sock_id;
    client_addr = s;
  }
  int get_status() {return status;}
  void set_status(int s) {status = s;} 
  private:
    
    bool running;
    int sockid;
    sockaddr_in client_addr;
    int status;
    int max_ct;
    int attempt_ct;
    packet p;
} ;

//control window is a list of ctrl nodes
typedef std::list <ctrl_node> ctrl_list;

void insert_packet (std::list<packet>  & pack_list, packet p) 
{
  std::list<packet>::iterator pi = pack_list.begin();;
  for(pi; pi != pack_list.end(); pi++)
  {
    if (p.packet_num < pi->packet_num) {
      pi++;
    }
    else if (p.packet_num == pi->packet_num) {
      return;
    }
    else {
      pack_list.insert(pi, p);
    }
  }
}

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

void rcv_msg (char * buffer, int sockid, sockaddr_in * s_addr)
{ 
  set_null(buffer);
  unsigned int s_addr_len = sizeof(struct sockaddr_in);
  try
  {
    recvfrom (sockid, buffer, PACK_SZ, 0, (struct sockaddr *) s_addr, &s_addr_len);
  }
  catch(const std::exception& e)
  {
    printf("%s", "failed to receive message");
  }
}



//initialize a packet to send
int send_packet (packet to_send, int sockid, sockaddr_in s_addr) 
{
  int status;
  set_null(to_send.data);
  unsigned int length = sizeof(struct sockaddr_in);
  char bytes_to_send[PACK_SZ];
  serialize(&to_send, bytes_to_send, PACK_SZ);
  status = sendto(sockid, bytes_to_send, sizeof(bytes_to_send), 0, (struct sockaddr *) &s_addr, length);
  return status;
}


#endif
