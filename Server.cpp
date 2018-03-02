//Nathan and Randy
#include "utilities.h"

int main(int argc, char *argv[]) 
{

  int sockid, portno, status, size_sent, size_received, pack_num = 0;
  unsigned int client_addr_length = sizeof(struct sockaddr_in);
  portno = atoi(argv[1]);
  int win_sz = 4;
  int max_win_sz = 32;
  

  if (argv[2]) 
  {
    max_win_sz = atoi(argv[2]);
  }
  

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
  set_null(buffer);
  rcv_msg(buffer, sockid, &client_addr);
  printf("Connection made, sending ack...\n");
  //send ack back
 
  packet hs_ack(ACK, pack_num, handshake_msg);
  status = send_packet(hs_ack, sockid, serv_addr);
  set_timeout(sockid);
  size_sent = 0;
    
  //receive filepath from client

  rcv_msg (buffer, sockid, &client_addr);
  packet p_filepath;
  deserialize(&p_filepath, buffer);
  packet p_fp_ack(ACK, pack_num, '\0');
  size_sent = send_packet(p_fp_ack, sockid, client_addr); 

  //deserialize data stream to  packet
  deserialize(&p_fp_ack, buffer);

  //Message recieved here is path of file to retrieve
  const char * fp = (const char*)&p_filepath.data;
  packet p;
  
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
  //printf("%s\n", p.data);

  printf("Sending file length: %ld\n", size);
  //sleep(2);
  pack_num++;
  packet p_size(SIZE, pack_num, str_to_chr);
  size_sent = send_packet(p_size, sockid, client_addr);
  set_timeout(sockid); 

  //receive message before beginning transmission
  rcv_msg (buffer, sockid, &client_addr);
 
  deserialize(&p, buffer);

  //reset pack num to zero to send data
  pack_num = 0;
  printf("Sending data...");
  
  //step one: send first win_sz packets and add to ctrl window
  ctrl_win cw(4, fp);
  cw.init(4, sockid, client_addr);
  cw.shift_win(sockid, client_addr);

  while (true) 
  {
    packet p;
    rcv_msg(buffer, sockid, &serv_addr);
    deserialize(&p, buffer);
    cw.log_ack(p.packet_num);
    if (!cw.shift_win(sockid, client_addr))
    {
      close(sockid);
      break;
    }

  }
  return 0;
}


/*  
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


*/



