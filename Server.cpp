//Nathan and Randy
#include "utilities.h"

int main(int argc, char *argv[]) 
{
  //Message Codes:
  //H : handshake
  //S : size of file (server)
  //R : Request (client)
  //A : ACK
  //D : Data

  int sockid, portno, status, size_sent, size_received, pack_num = 0;
  unsigned int client_addr_length = sizeof(struct sockaddr_in);
  portno = atoi(argv[1]);

  //struct that holds necessary socket parameters
  struct sockaddr_in serv_addr, client_addr;
  char buffer [PACK_SZ];
  set_null(buffer);
  const char * handshake_msg = "ACK";
  //Assign values to serv_addr socket fields
  serv_addr.sin_family  = PF_INET;          //address family
  serv_addr.sin_port = htons(portno);       //port in network byte order  
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //internet address

  //Sockid is handle for socket. (family, type, protocol)
  sockid = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  check_retval(sockid, "could not open socket");
  check_retval (sizeof(&serv_addr), "sockaddr_in not set up");

  //Bind socket
  status = bind(sockid, (struct sockaddr*) &serv_addr, 
      sizeof(serv_addr));
  check_retval(status, "status, binding socket");

  printf("Awaiting connection...\n");
  //receive hello/handshake
  rcv_msg(sockid, &client_addr, buffer, 5);
  printf("Connection made, sending ack...\n");
  //send ack back
  status = send_packet(&pack_num, sockid, client_addr, handshake_msg, 'A');
  set_timeout(sockid);
  while(true)
  {
    //receive filepath from client
    if(!(rcv_msg (sockid, &client_addr, buffer, 5))){
      size_sent = send_packet(&pack_num, sockid, client_addr, handshake_msg, 'A'); 
    }
    else{
      break;
    }
  }

  struct packet p;
  deserialize(&p, buffer);

  //Message recieved here is file path
  const char * fp = (const char*)&p.data;
  
  printf("File path received: %s\n", p.data);
  std::ifstream infile (fp, std::ifstream::binary);
  if (!infile) 
  {
    perror("FILE NOT FOUND");
  }
  //get the size of the file
  infile.seekg (0, infile.end);
  long size = infile.tellg();
  infile.seekg(0);

  //configure size as char *
  //use string to convert from integer
  std::string sz = std::to_string(size);
  //convert from string to char *
  const char * str_to_chr = sz.c_str();
  strcpy(p.data, str_to_chr);
  //printf("%s\n", p.data);

  printf("Sending file length: %ld\n", size);
  //sleep(2);
  pack_num++;
  size_sent = send_packet(&pack_num, sockid, client_addr, p.data, 'S');
  set_timeout(sockid); 
  while(true){
    if(!(rcv_msg (sockid, &client_addr, buffer, 5))){
      size_sent = send_packet(&pack_num, sockid, client_addr, p.data, 'S'); 
    }
    else{
      break;
    }
  }
  deserialize(&p, buffer);
  pack_num = p.packet_num;
  printf("Sending data...");
  //send payload
  //read file till eof
  while (!infile.eof()) 
  {
    printf(" ... \n");
    set_null(p.data); 
    infile.read(p.data, DATA_SZ);
    while(true)
    {
      size_sent = send_packet(&pack_num, sockid, client_addr, p.data, 'D'); 
      set_timeout(sockid); 
      while(true)
      {
        if(!(rcv_msg (sockid, &client_addr, buffer, 5))){
          size_sent = send_packet(&pack_num, sockid, client_addr, p.data, 'D'); 
		}
        else{
          break;
        }
      }
    
      deserialize(&p, buffer);
      if (p.msg_type == 'A' && pack_num == p.packet_num) 
      {
        break;
      }
    }
    pack_num = p.packet_num + 1;
  }

  printf("Sending close packet...\n");
  //Send a close packet to client
  size_sent = send_packet(&pack_num, sockid, client_addr, "", 'C');

  close(sockid);

  return 0;
}



