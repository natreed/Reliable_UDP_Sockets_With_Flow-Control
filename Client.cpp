//Nathan and Randy
#include "Client.h"
int main (int argc, char * argv[]) 
{
  struct hostent  server_ip = *gethostbyname(argv[1]);
  int sockid, rcv_sockid, portno, status, packet_num = 0, file_size;
  unsigned int length;
  int window_size = 2;
  //char filepath[100] = argv[2];
  portno = atoi(argv[2]);
  char * fp_get = argv[3];

  struct sockaddr_in serv_addr, rcv_addr;
  char buffer [PACK_SZ];
  packet p;
  set_null(buffer);
  std::list<packet> ctrl_list;
  std::mutex list_lock;
  bool all_done = false;

  int max_packet = window_size - 1;
  length = sizeof(struct sockaddr_in);

  sockid = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  check_retval(sockid, "error opening socket");
  
  //set up server sockaddr_in
  serv_addr.sin_family = PF_INET;
  serv_addr.sin_port = ntohs(portno);
  bcopy((char*) server_ip.h_addr, (char*) &serv_addr.sin_addr, server_ip.h_length);

  //New socket for receiver thread in Client
  rcv_sockid = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  check_retval(rcv_sockid, "error opening socket");
  
  //set up server sockaddr_in
  rcv_addr.sin_family = PF_INET;
  rcv_addr.sin_port = ntohs(portno + 1);
  bcopy((char*) server_ip.h_addr, (char*) &rcv_addr.sin_addr, server_ip.h_length);
/*
  status = bind(sockid, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
  check_retval(status, "status, binding socket");

  status = bind(rcv_sockid, (struct sockaddr*) &rcv_addr, sizeof(rcv_addr));
  check_retval(status, "status, binding socket");
 */ 
  printf("%s","Initiate handshake with server. . .\n");
  //handshake for send_ack thread
  status = client_handshake(serv_addr, sockid);

  printf("%s", "Acknowledgement received from server . . .\n");
 
  int previous_packet_num = 0;
  
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
  deserialize(&p, buffer);
  file_size = atoi(p.data);
  printf("Msg size %s received . . .\n", p.data);
  int last_packet_num = file_size / DATA_SZ;
  printf("last_packet_num: %d\n", last_packet_num);
  packet p_ack(ACK, packet_num, "ack");
  //send file size acknowlegement
  packet_num++;
  status = send_packet(p_ack, sockid, serv_addr); 
  
  std::ofstream outfile ("download.txt",std::ofstream::binary);
  int prev_packet_num = packet_num-1;
 
  //open new socket for send_ack thread
  status = client_handshake(rcv_addr, rcv_sockid);

	printf("Beginning receiving data . . .\n");
  std::thread rcv_and_insert(rcv_insert, std::ref(list_lock), std::ref(ctrl_list), sockid, serv_addr,
    std::ref(max_packet), window_size, std::ref(all_done));

  std::thread write_data_thread(write_data, std::ref(list_lock), std::ref(ctrl_list), std::ref(max_packet), last_packet_num, 
    std::ref(outfile), std::ref(all_done));
  
  send_acks(list_lock, ctrl_list, rcv_sockid, rcv_addr, max_packet, window_size, all_done);
  printf("Data transmission complete . . .\n");
  close(sockid);
  return 0;
}


