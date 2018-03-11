#include "utilities.h"



void write_data(std::mutex & m, std::list<packet> & packet_list, 
    int & max_packet_num, int last_packet_num,  std::ofstream & outfile, bool & all_done) 
{
  int previous_packet_num = -1;
  while (true) 
  {

    if (packet_list.empty())
    {
      continue;
    }
    if ((packet_list.front().packet_num == previous_packet_num +1 ||
          packet_list.front().packet_num == 0) && packet_list.front().status == ACKED)
    {
      if (packet_list.front().msg_type == 'D')
      {
        packet p = packet_list.front(); 
        m.lock();
        packet_list.pop_front();
        m.unlock();
        outfile.write(p.data, p.msg_size);
		
        printf("Writing data, packet number: %d\n", p.packet_num);
        previous_packet_num = p.packet_num;
        max_packet_num++;
        if (p.packet_num == last_packet_num)
        {
          all_done = true;
          return;
        }
      }
    }
  }
}

//Thread receives and inserts into client packet list. 
void rcv_insert (std::mutex & m, std::list<packet> & packetlist, int sockid, sockaddr_in s_addr,
    int  & max_packet_num, int window_size, bool & all_done)
{
  while(!all_done)
  {
    char buffer[PACK_SZ];
    set_null(buffer);
    int status;
    status = rcv_msg(buffer, sockid, &s_addr);
      

    //TODO: REAL PATCHY DOESNT FIX RCV MSG FAILURE
    if (buffer[0] == '\0')
    {
      continue;
    }

    
    if (packetlist.size() <=  window_size)
    {
      packet p;
      deserialize(&p, buffer);

      printf("received packet, packet number: %d\n", p.packet_num);
      //if packet is outside the window, drop it
      if (p.msg_type != DATA && p.msg_type != CLOSE)
      {
        char msg[100]  = "msg_type: is not DATA or CLOSE";
        perror("msg_type: %s not data or close packet.\n");
      }
      if (p.packet_num > max_packet_num)
      {
        continue;
      }
      m.lock();
      insert_packet(packetlist, p);
      m.unlock();
    }
  }
}

void send_acks(std::mutex & m, std::list<packet> & pack_list, int sockid, sockaddr_in s_addr, 
    int & max_packet_num, int window_size, bool & all_done) 
{
  while(!all_done)
  {
    std::list<packet>::iterator pi = pack_list.begin();

    if (pack_list.empty())
    {
      continue;
    }


    for(pi; pi !=  pack_list.end(); pi++)
    {
      if (pi->status != ACKED)
      {
        packet p(ACK, pi->packet_num, "\0");
        send_packet(p, sockid, s_addr);
        pi->status = ACKED;
      }
    }
  }
}


int client_handshake (struct sockaddr_in serv_addr, int sockid) 
{
  //sleep to give server a chance to get ready
  //once we get the timer working, we can resend.
  sleep(1);
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
  status = sendto(sockid, data_packet, sizeof(data_packet), 0, (struct sockaddr *) &serv_addr, length);
  check_retval(status, "handshake failed in Sendto()");

  set_timeout(sockid);	 
  rcv_msg (buffer, sockid, &serv_addr);

  struct packet handshake;
  deserialize(&handshake, buffer);
  return status;

}

/*
   int new_socket_connection (struct sockaddr_in  serv_addr, int sockid)
   {
   int status = 0;
   char buffer[DATA_SZ];
//send_msg
//Wait for signal to connect from server
rcv_msg(buffer, sockid, &serv_addr);

check_retval(status, "Failed in new_socket_connection: rcv_msg");
packet p(ACK, 0, "\0");
send_packet(p, sockid, serv_addr);


}*/

