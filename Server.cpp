#include "utilities.h"
#include "ctrl_win.h"
#include "Server.h"

int main(int argc, char *argv[]) 
{

  int sockid, portno, status, size_sent, size_received, pack_num = 0;
  unsigned int client_addr_length = sizeof(struct sockaddr_in);
  portno = atoi(argv[1]);
  int win_sz = 4;
  int max_win_sz = 32;
  char buffer[DATA_SZ];
  int last_packet_num = 0;
  std::mutex list_lock;

  

  if (argv[2]) 
  {
    max_win_sz = atoi(argv[2]);
  }

  //struct that holds necessary socket parameters
  struct sockaddr_in client_addr, serv_addr;
  //Assign values to serv_addr socket fields
  serv_addr.sin_family  = PF_INET;          //address family
  serv_addr.sin_port = htons(portno);       //port in network byte order  
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //internet address

  //Sockid is handle for socket. (family, type, protocol)
  sockid = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  check_retval(sockid, "could not open socket");

  //Bind socket
  status = bind(sockid, (struct sockaddr*) &serv_addr, 
      sizeof(serv_addr));
  check_retval(status, "status, binding socket");

  
  //create new socket for receiver thread
  struct sockaddr_in rcv_addr;
  //Assign values to serv_addr socket fields
  rcv_addr.sin_family  = PF_INET;          //address family
  rcv_addr.sin_port = htons(portno +1);       //port in network byte order  
  rcv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //internet address

  //Sockid is handle for socket. (family, type, protocol)
  int rcv_sockid = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  check_retval(rcv_sockid, "could not open socket");

  //Bind socket
  status = bind(rcv_sockid, (struct sockaddr*) &rcv_addr, 
      sizeof(rcv_addr));
  check_retval(status, "status, binding socket");







  //wait for handshake msg
  //TODO: what to do here so that multiple connections can
  //be initiated
  hs_server(serv_addr, sockid);
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
  last_packet_num =ceil(size  / DATA_SZ);
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

  //start receiving thread
  std::thread rcv_thread(rcv_loop, std::ref(list_lock), std::ref(cw), rcv_sockid, rcv_addr, last_packet_num);
  cw.init(&list_lock, 4, sockid, client_addr);
  cw.shift_win(&list_lock, sockid, client_addr);

  while (true) 
  {
    if (!cw.shift_win(&list_lock, sockid, client_addr))
    {
      close(sockid);
      break;
    }

  }
  rcv_thread.join();
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

