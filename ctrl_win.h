#include "utilities.h"




//ctrl window holds list list of ctrl nodes + vector<bool>
class ctrl_win 
{
  public:
    //constructor takes a size argment to initialize vector
    ctrl_win(int s, const char * f_p) : rcvd_vec(s, 0), pack_num(0)
  {
    fs.open(f_p, std::ifstream::binary);
    if (!fs)
    {
      perror("FILE NOT FOUND");
    }
  }
    void init(int win_sz, int sockid, struct sockaddr_in client_addr);
    int rcv_loop(sockaddr_in * s_addr, int max_attempt, int sockid);
    void append_cnode(ctrl_node cn);
    void log_ack(int);
    void pop_cnode();
    //function to shift window when ack is recieved
    //for first element
    int shift_win(int sockid, struct sockaddr_in client_addr);
  private:
    int pack_num;  
    int w_size;
    std::ifstream fs;
    ctrl_list cl;
    std::vector<bool> rcvd_vec;

}; 

void ctrl_win::append_cnode (ctrl_node cn) 
{
  //push new control node to end of list
  cl.push_back(cn);
}

void ctrl_win::pop_cnode()
{
  cl.pop_front();
}

//TODO: This function could run on a thread
//return value of 0 indicates that all acks have been receive/
int ctrl_win::shift_win(int sockid, struct sockaddr_in client_addr)
{
  while (!cl.empty() && cl.front().rcvd_ack)
  {
    //to move window pop first node. create and add last nodae.
    pack_num += 1;  //increment packet number
    char * data; 
    fs.read(data, DATA_SZ);  //get data to send

    if (!fs.eof())  //check for end of file
    {
      //TODO: This needs to be done more than once as window grows
      packet p('D', pack_num, data);
      try 
      {
        send_packet(p, sockid, client_addr);
      }
      catch (const std::exception& e) {}
      //if not end of file pop from front and append new packet
      ctrl_node cn(p, sockid, client_addr);    //initialize control node w/ packet
      pop_cnode();            //pop front of ctrl list
      append_cnode(cn);  //append to end of ctrl list
      //start timer thread
    }
    else 
    {
      packet p('C', pack_num, data);
      try
      {
        send_packet(p, sockid, client_addr);     
      }
      catch (const std::exception& e) {}   
      //end of file has been reached so check last acks till end of list

      pop_cnode();
    }
  }
  if (cl.empty()) {
    return 0;
  }   
  return 1;
}


//function to run in thread from main for receiving messages
int ctrl_win::rcv_loop( sockaddr_in * s_addr, int max_sz, int sockid)
{
  while(true)
  {
    char buffer[DATA_SZ];
    rcv_msg(buffer, sockid, s_addr);
    packet p;
    deserialize(&p, buffer);
    log_ack(p.packet_num);
    //must be ack
    if (p.msg_type == 'A')
    {
      return 1;
    }
  }
  return -1;
}

void ctrl_win::log_ack(int pack_num)
{
  int windex = (pack_num + cl.size()) % cl.size();
  //iterate to the sequential node of the index
  std::list<ctrl_node>::iterator cl_iterator = cl.begin();
  for (int i = 0; i <= windex; ++i) 
  {
    cl_iterator++;
  }
  cl_iterator->rcvd_ack = true;
}

//send first win_size messages and initialize window list
void ctrl_win::init (int win_sz, int sockid, struct sockaddr_in client_addr) 
{
  w_size = win_sz;
  for (int i = 0; i < win_sz; i++) 
  { 
    char * data;
    fs.read(data, DATA_SZ);
    if (!fs.eof()) 
    {
      //if not end of file pop from front and append new packet
      packet p('D', pack_num, data); //initialize data packet
      ctrl_node cn(p, sockid, client_addr);    //initialize control node w/ packet
      append_cnode(cn);  //append to end of ctrl list
      try 
      {
        send_packet(p, sockid, client_addr);
      }
      catch (const std::exception& e) {} 
    }
    else 
    {
      packet p('C', pack_num, data); //initialize data packet
      try
      {
        send_packet(p, sockid, client_addr);
        ctrl_node cn(p, sockid, client_addr);
        append_cnode(cn);
      }
      catch (const std::exception& e) {}
      break;
    }
    pack_num++;
  }
}



