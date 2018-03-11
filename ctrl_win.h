#include "utilities.h"




//ctrl window holds list list of ctrl nodes + vector<bool>
class ctrl_win 
{
  public:
    //constructor takes a size argment to initialize vector
    ctrl_win(int s, const char * f_p) : pack_num(0)
  {
    fs.open(f_p, std::ifstream::binary);
    if (!fs)
    {
      perror("FILE NOT FOUND");
    }
  }
    void init(std::mutex * m, int win_sz, int sockid, struct sockaddr_in client_addr);
    void append_cnode(ctrl_node cn);
    void log_ack(int);
    void pop_cnode();
    //function to shift window when ack is recieved
    //for first element
    int win_mgr(std::mutex * m, int sockid, struct sockaddr_in client_addr);
    void shift_win (std::mutex * m, int sockid, sockaddr_in client_addr, char type, char * data);
  private:
    int pack_num;  
    int w_size;
    std::ifstream fs;
    ctrl_list cl;
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
int ctrl_win::win_mgr(std::mutex * m, int sockid, struct sockaddr_in client_addr)
{
  while (!cl.empty())
  {
    if (cl.front().get_status() != ACKED) 
    {
      continue;
    }
    //to move window pop first node. create and add last nodae.
    char  data[DATA_SZ]; 
    fs.read(data, DATA_SZ);  //get data to send

    if (!fs.eof())  //check for end of file
    {
      shift_win(m, sockid, client_addr, DATA, data);
    }
    else 
    {
      shift_win(m, sockid, client_addr, CLOSE, data);
    }
  }
  if (cl.empty()) {
    return 0;
  }   
  return 1;
}

void ctrl_win::shift_win (std::mutex * m, int sockid, sockaddr_in client_addr, 
char type, char * data)
{
      int status;
      pack_num += 1;  //increment packet number
      packet p(type, pack_num, data);
      try 
      {
        //TODO: This needs to be done more than once as window grows
        status = send_packet(p, sockid, client_addr);
      }
      catch (const std::exception& e) {}
      //if not end of file pop from front and append new packet
      ctrl_node cn(p, sockid, client_addr);    //initialize control node w/ packet
      cn.set_status(SENT);
      m->lock();
      pop_cnode();            //pop front of ctrl list
      append_cnode(cn);  //append to end of ctrl list
      m->unlock();
      //start timer thread
}



void ctrl_win::log_ack(int pack_num)
{
  //iterate to the sequential node of the index
  std::list<ctrl_node>::iterator cl_iterator = cl.begin();
  for (int i = 0; i < cl.size(); ++i) 
  {
    if (cl_iterator->get_pack_num() == pack_num)
    {
      cl_iterator->set_status(ACKED);
      break;
    }

    cl_iterator++;
  }
}

//send first win_size messages and initialize window list
void ctrl_win::init (std::mutex * m, int win_sz, int sockid, struct sockaddr_in client_addr) 
{
  w_size = win_sz;
  for (int i = 0; i < win_sz; i++) 
  { 
    char data[DATA_SZ];
    fs.read(data, DATA_SZ);
    printf("DATA : %s\n", data);
    if (!fs.eof()) 
    {
      //if not end of file pop from front and append new packet
      packet p('D', pack_num, data); //initialize data packet
      try 
      {
        send_packet(p, sockid, client_addr);
      }
      catch (const std::exception& e) {}
      ctrl_node cn(p, sockid, client_addr);    //initialize control node w/ packet
      cn.set_status(SENT);
      m->lock();
      append_cnode(cn);  //append to end of ctrl list
      m->unlock();
    }
    else 
    {
      packet p('C', pack_num, data); //initialize data packet
      try
      {
        send_packet(p, sockid, client_addr);
        ctrl_node cn(p, sockid, client_addr);
        cn.set_status(SENT);
        m->lock();
        append_cnode(cn);  //append to end of ctrl list
        m->unlock();
      }
      catch (const std::exception& e) {}
      break;
    }
    pack_num++;
  }
  pack_num--;
}



