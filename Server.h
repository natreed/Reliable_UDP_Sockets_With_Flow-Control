#include "utilities.h"
#include "ctrl_win.h"
//handshake function for server
int hs_server (struct sockaddr_in  client_addr, int sockid)
{
  char buffer[PACK_SZ];
  printf("Awaiting connection...\n");
  //receive hello/handshake
  const char * handshake_msg = "ACK";
  packet hs_packet;
  try
  {
    rcv_msg(buffer, sockid, &client_addr);
  }
  catch(const std::exception & e){}
  printf("hs msg received");
  deserialize(&hs_packet, buffer);
  printf("hs_packet  deserialized");

  //check to see if incoming message is a handshake
  //if not ignore
  if (hs_packet.msg_type == 'H')
	{
  	printf("Connection made, sending ack...\n");
    packet hs_ack(ACK, hs_packet.packet_num, handshake_msg);
    send_packet(hs_ack, sockid, client_addr);
    return 1;
	} 
  else
  {
    printf("%s", "Initial contact not handshake");
    return 0;
  }
}



//function to run in thread from main for receiving messages
void  rcv_acks(std::mutex & m, ctrl_win & cw, int sockid, sockaddr_in s_addr, int last_packet_num)
{
  int i = 0;
  for (int i = 0; i < last_packet_num; i++)
  {
    char buffer[DATA_SZ];
    rcv_msg(buffer, sockid, &s_addr);
    packet p;
    deserialize(&p, buffer);
    printf("packet ack received, Packet number: %d\n", p.packet_num);
    if (p.msg_type != ACK) 
    {
      char  msg[100] = "not receiving ACK. Wrong msg type";
      perror(msg);
    }
    //lock 
    m.lock();
    cw.log_ack(p.packet_num);
    m.unlock();
  }
}

/*
int new_socket_connection (struct sockaddr_in  client_addr, int sockid)
{
  int status = 0;
  //Signal client that that we are waithing on another socket
  try
  {
    rcv_msg(buffer, sockid, &client_addr);
  }
  catch(const std::exception & e){}
  packet p(WAITING, 0, "\0");
  status = send_packet(p,  sockid, client_addr);
  check_retval(status, "Failed in new_socket_connection: rcv_msg");
  

}
*/


