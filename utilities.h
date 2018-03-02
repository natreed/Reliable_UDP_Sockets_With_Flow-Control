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
#include <bitset>
#include <algorithm>

const int DATA_SZ = 1024;
const int PACK_SZ = sizeof(char) + sizeof(int)*2 + DATA_SZ;
const int ATTEMPTS = 5;

//Message Codes:
char HANDSHAKE = 'H'; //H : handshake
char SIZE = 'S';      //S : size of file (server)
char REQUEST = 'R';   //R : Request (client)
char ACK = 'A';       //A : ACK
char DATA = 'D';      //D : Data

//prototypes
void set_null(char *);
struct packet;
int send_packet (packet, int sockid, sockaddr_in s_addr); 
void rcv_msg (char *  buffer, int sockid, sockaddr_in * s_addr);
void deserialize (struct packet* p, char * buffer); 

//packet
typedef struct packet 
{
  packet() : msg_type('0'), packet_num(0), msg_size(0) 
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
} packet; 

//++++++++++++++++++++++++++++++++++++++++++=
//Control list node
typedef struct ctrl_node
{
  ctrl_node(packet pack) : rcvd_ack(false), p(pack) {}
  packet p;
  bool rcvd_ack;
} ctrl_node;

//control window is a list of ctrl nodes
typedef std::list <ctrl_node> ctrl_list;

//ctrl window holds list list of ctrl nodes + vector<bool>
class ctrl_win 
{
  public:
    //constructor takes a size argment to initialize vector
    ctrl_win(int s, const char * f_p) : rcvd_vec(s, 0), pack_num(0)
  {
    fs.open(f_p, std::ifstream::binary);
    if (!fs)
    {
      perror("FILE NOT FOUND");
    }
  }
    void init(int win_sz, int sockid, struct sockaddr_in client_addr);
    int rcv_loop(sockaddr_in * s_addr, int max_attempt, int sockid);
    void append_cnode(ctrl_node);
    void log_ack(int);
    void pop_cnode();
    //function to shift window when ack is recieved
    //for first element
    int shift_win(int sockid, struct sockaddr_in client_addr);
  private:
    int pack_num;  
    int w_size;
    std::ifstream fs;
    ctrl_list cl;
    std::vector<bool> rcvd_vec;

}; 

void ctrl_win::append_cnode (ctrl_node cn) 
{
  //push new control node to end of list
  cl.push_back(cn);
}

void ctrl_win::pop_cnode()
{
  cl.pop_front();
}


//return value of 0 indicates that all acks have been received
int ctrl_win::shift_win(int sockid, struct sockaddr_in client_addr)
{
  while (!cl.empty() && cl.front().rcvd_ack)
  {
    //to move window pop first node. create and add last nodae.
    pack_num += 1;  //increment packet number
    char * data; 
    fs.read(data, DATA_SZ);  //get data to send

    if (!fs.eof())  //check for end of file
    {
      //TODO: This needs to be done more than once as window grows
      packet p('D', pack_num, data);
      try 
      {
        send_packet(p, sockid, client_addr);
      }
      catch (const std::exception& e) {}
      //if not end of file pop from front and append new packet
      ctrl_node cn(p);    //initialize control node w/ packet
      pop_cnode();            //pop front of ctrl list
      append_cnode(cn);  //append to end of ctrl list
    }
    else 
    {
      packet p('C', pack_num, data);
      try
      {
        send_packet(p, sockid, client_addr);     
      }
      catch (const std::exception& e) {}   
      //end of file has been reached so check last acks till end of list

      pop_cnode();
    }
  }
  if (cl.empty()) {
    return 0;
  }   
  return 1;
}


//function to run in thread from main for receiving messages
int ctrl_win::rcv_loop( sockaddr_in * s_addr, int max_sz, int sockid)
{
  while(true)
  {
    char buffer[DATA_SZ];
    rcv_msg(buffer, sockid, s_addr);
    packet p;
    deserialize(&p, buffer);
    log_ack(p.packet_num);
    //must be ack
    if (p.msg_type == 'A')
    {
      return 1;
    }
  }
  return -1;
}

void ctrl_win::log_ack(int pack_num)
{
  int windex = (pack_num + cl.size()) % cl.size();
  //iterate to the sequential node of the index
  std::list<ctrl_node>::iterator cl_iterator = cl.begin();
  for (int i = 0; i <= windex; ++i) 
  {
    cl_iterator++;
  }
  cl_iterator->rcvd_ack = true;
}


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

//send first win_size messages and initialize window list
void ctrl_win::init (int win_sz, int sockid, struct sockaddr_in client_addr) 
{
  w_size = win_sz;
  for (int i = 0; i < win_sz; i++) 
  { 
    char * data;
    fs.read(data, DATA_SZ);
    if (!fs.eof()) 
    {
      //if not end of file pop from front and append new packet
      packet p('D', pack_num, data); //initialize data packet
      ctrl_node cn(p);    //initialize control node w/ packet
      append_cnode(cn);  //append to end of ctrl list
      try 
      {
        send_packet(p, sockid, client_addr);
        ctrl_node cn(p);
        append_cnode(cn);
      }
      catch (const std::exception& e) {} 
    }
    else 
    {
      packet p('C', pack_num, data); //initialize data packet
      try
      {
        send_packet(p, sockid, client_addr);
        ctrl_node cn(p);
        append_cnode(cn);
      }
      catch (const std::exception& e) {}
      break;
    }
    pack_num++;
  }
}



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
  set_null(buffer);
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
  rcv_msg (buffer, sockid, serv_addr);

  struct packet handshake;
  deserialize(&handshake, buffer);
  return status;

}

//initialize a packet to send
int send_packet (packet to_send, int sockid, sockaddr_in s_addr) 
{
  int status;
  unsigned int length = sizeof(struct sockaddr_in);
  char bytes_to_send[PACK_SZ];
  serialize(&to_send, bytes_to_send, PACK_SZ);

  status = sendto(sockid, bytes_to_send, sizeof(bytes_to_send), 0, (struct sockaddr *) &s_addr, length);
  check_retval(status, "failed in Sendto()");
  return status;
}

#endif
//Timer Stuff
/*
   endifyProgram(double secondsToDelay)
   {
   clock_t startTime = clock(); //Start timer

   clock_t testTime;
   clock_t timePassed;
   double secondsPassed;

   while(true)
   {
   testTime = clock();
   timePassed = startTime - testTime;
   secondsPassed = timePassed / (double)CLOCKS_PER_SEC;

   if(secondsPassed >= secondsToDelay)
   {
   cout << secondsToDelay << "seconds have passed" << endl;
   break;
   }

   }


   }
 */
