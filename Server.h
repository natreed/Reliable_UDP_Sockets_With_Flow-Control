#include "utilities.h"

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
  catch(const std::exception & e)
  {

  }
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

