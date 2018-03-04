//Nathan and Randy
#include "utilities.h"

int main (int argc, char * argv[]) 
{
  struct hostent  server_ip;
  int sockid, portno, status, packet_num = 0, file_size;
  unsigned int length;
  server_ip = *gethostbyname(argv[1]);
  //char filepath[100] = argv[2];
  portno = atoi(argv[2]);
  char * fp_get = argv[3];
  
  struct sockaddr_in serv_addr;
  char buffer [PACK_SZ];
  packet p;
  set_null(buffer);
  length = sizeof(struct sockaddr_in);

  sockid = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  check_retval(sockid, "error opening socket");
  
  //set up server sockaddr_in
  serv_addr.sin_family = PF_INET;
  serv_addr.sin_port = ntohs(portno);
  bcopy((char*) server_ip.h_addr, (char*) &serv_addr.sin_addr, server_ip.h_length);

  printf("%s","Initiate handshake with server. . .\n");
  status = client_handshake(&serv_addr, sockid);
  printf("%s", "Acknowledgement received from server . . .\n");
  
  //args are (packet number, sockid, sockaddr_in, filepath)
  packet_num++;
  printf("Requesting file %s from sever", argv[3]);
  packet p_request_file('R', packet_num, argv[3]);
  status = send_packet (p_request_file, sockid, serv_addr);
  if (!(status < 0)) 
  {
    printf("%s","Request sent successfully . . .\n");
  }
  
  set_timeout(sockid);
  rcv_msg (buffer, sockid, &serv_addr);
  printf("Msg size %s received . . .\n", p.data);
  deserialize(&p, buffer);
  file_size = atoi(p.data);
  packet p_ack(ACK, packet_num, "ack");
  //send file size acknowlegement
  packet_num++;
  status = send_packet(p_ack, sockid, serv_addr); 
  
  std::ofstream outfile ("download.txt",std::ofstream::binary);
  int prev_packet_num = packet_num-1;
  

	printf("Beginning data transmission . . .\n");

/*
  while(p.msg_type != 'C')
  {
    packet p;
    try
    {
      rcv_msg(buffer, sockid, &serv_addr);
      deserialize(p, buffer);
    }
    catch (const std::exception& e)    
    {
      close(sockid);
      perror("Error: maximum attempts made to receive! Terminating Connection . . .\n");
    }

    if(prev_packet_num+1 != p.packet_num)
    {
      
      status = send_packet(packet p(buffer), sockid, serv_addr);
      set_timeout(sockid);
      while(true)
      {  
        if((rcv_msg (buffer, sockid, serv_addr)))
        {      
          status = send_packet(&prev_packet_num, sockid, serv_addr, "ACK", 'A');
        }
        else
        {
           break;
        }
      }
    }
    else
    {
      outfile.write(p.data, p.msg_size);
      status = send_packet(&p.packet_num, sockid, serv_addr, "ACK", 'A');
      printf("ACK packet %d  . . .\n", p.packet_num);
    } 

    packet_num = p.packet_num;
    prev_packet_num++;
  }
  */

  printf("Data transmission complete . . .\n");
  close(sockid);
  return 0;
}


