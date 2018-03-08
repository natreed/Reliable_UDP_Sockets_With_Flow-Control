#include "utilities.h"



void write_data(std::mutex & m, std::list<packet> & packet_list, int sockid, 
    sockaddr_in s_addr, int & max_packet_num, int last_packet_num,  std::ofstream & outfile, bool & all_done) 
{
  int previous_packet_num = -1;
  while (true) 
  {

    if (packet_list.empty())
    {
      continue;
    }
    if ((packet_list.front().packet_num == previous_packet_num +1 ||packet_list.front().packet_num == 0) && packet_list.front().status == ACKED)
    {
      if (packet_list.front().msg_type == 'D')
      {
      packet p = packet_list.front(); 
      m.lock();
      packet_list.pop_front();
      m.unlock();
      outfile.write(p.data, p.msg_size);
      
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

void send_acks(std::mutex & m, std::list<packet> & packetlist, int sockid, sockaddr_in s_addr, int & max_packet_num, int window_size, bool & all_done) 
{
   char buffer[PACK_SZ];
   set_null(buffer);
   while(!all_done)
   {
     rcv_msg(buffer, sockid, &s_addr);
     if (packetlist.size() <= window_size)
     {
        packet p;
        deserialize(&p, buffer);
        if (p.packet_num > max_packet_num)
        {
          continue;
        }
        p.status = ACKED;
        m.lock();
        insert_packet(packetlist, p);
        m.lock();
        packet p_ack('A', p.packet_num, "\0");
        send_packet(p_ack, sockid, s_addr);
     }
   }
}

int client_handshake (struct sockaddr_in serv_addr, int sockid) 
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
  status = sendto(sockid, data_packet, sizeof(data_packet), 0, (struct sockaddr *) &serv_addr, length);
  check_retval(status, "handshake failed in Sendto()");

  set_timeout(sockid);	 
  rcv_msg (buffer, sockid, &serv_addr);

  struct packet handshake;
  deserialize(&handshake, buffer);
  return status;

}
