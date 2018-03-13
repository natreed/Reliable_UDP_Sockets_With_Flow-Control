#include "utilities.h"
#include "Server.h"

bool SERVER_READY;

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
  bool exit_timer_thread = true;
 
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
  hs_server(client_addr, sockid);

  set_timeout(sockid);
  size_sent = 0;

  //receive filepath from client
  rcv_msg (buffer, sockid, &client_addr);
  packet p_filepath;
  deserialize(&p_filepath, buffer);

  //Message recieved here is path of file to retrieve
  const char * fp = (const char*)&p_filepath.data;

  printf("File path received: %s\n", fp);
  std::ifstream infile (fp, std::ifstream::binary);
  if (!infile) 
  {
    perror("FILE NOT FOUND");
  }


  //get the size of the file
  infile.seekg (0, infile.end);
  long size = infile.tellg();
  infile.seekg(0);
  last_packet_num =size  / DATA_SZ;
  //configure size as char *
  //use string to convert from integer
  std::string sz = std::to_string(size);
  //convert from string to char *
  const char * str_to_chr = sz.c_str();

  printf("Sending file length: %ld\n", size);
  pack_num++;
  packet p_size(SIZE, pack_num, str_to_chr);
  size_sent = send_packet(p_size, sockid, client_addr);
  set_timeout(sockid); 

  //receive message before beginning transmission
  rcv_msg (buffer, sockid, &client_addr);
  packet p;
  deserialize(&p, buffer);

  //reset pack num to zero to send data
  pack_num = 0;

  //step one: send first win_sz packets and add to ctrl window
  ctrl_win cw(1, fp);

  cw.init(&list_lock, 1, sockid, client_addr);

  
  //connect with rcv_addr of client
  status = hs_server(rcv_addr, rcv_sockid);
  printf("\nBeginning data transmission. . .\n");
  
  //start receiving thread
  std::thread rcv_thread(rcv_acks, std::ref(list_lock), std::ref(cw), rcv_sockid, rcv_addr, last_packet_num);
  std::thread timer_thread(resend_timer, std::ref(list_lock), std::ref(cw), std::ref(exit_timer_thread), sockid, client_addr);
  if (!cw.win_mgr(&list_lock, sockid, client_addr))
  {
    close(sockid);
  }
  exit_timer_thread = false;
  close(sockid);
  close(rcv_sockid);
  rcv_thread.join();
  timer_thread.join();
  printf("\nData transmisssion complete. . .\n\n");
  return 0;
}


