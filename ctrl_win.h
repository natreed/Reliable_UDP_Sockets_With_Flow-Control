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
    void check_time_resend(int sockid, struct sockaddr_in client_addr);
    void pop_cnode();
    //function to shift window when ack is recieved
    //for first element
    int win_mgr(std::mutex * m, int sockid, struct sockaddr_in client_addr);
    void shift_win (std::mutex * m, int sockid, sockaddr_in client_addr, char type, std::list<packet> data);
  private:
    int pack_num; 
    std::mutex pack_num_mutex;
    int w_size;
    std::ifstream fs;
    ctrl_list cl;
    std::mutex ctrl_list_mutex;
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

void ctrl_win::check_time_resend(int sockid, struct sockaddr_in client_addr)
{
  //return if no nodes in list
  if(cl.empty())
  {
    return;
  }
  //get the current time and iterate over nodes
  std::list<ctrl_node>::iterator cl_iterator = cl.begin();
  std::chrono::high_resolution_clock::time_point current_time = std::chrono::high_resolution_clock::now();
  for(int i = 0; i < cl.size(); i++)
  {
    //if the duration of the current time and the time that the node was send is greater than 200 milliseconds, resend
    if(std::chrono::duration_cast<std::chrono::milliseconds>(current_time - cl_iterator->time_started).count() > 200)
    {
      //send out expired packet and reset time
      send_packet(cl_iterator->get_packet(), sockid, client_addr);
      cl_iterator->time_started = std::chrono::high_resolution_clock::now();
      //cut window in half
      pack_num_mutex.lock();
      pack_num = (cl.back().get_packet().packet_num -cl.front().get_packet().packet_num)/2 + cl.front().get_packet().packet_num;
      pack_num_mutex.unlock();
      printf("Resending expired packet, Packet number: %d\n", cl_iterator->get_pack_num());
    }
    cl_iterator++;
  }
}

//TODO: This function could run on a thread
//return value of 0 indicates that all acks have been receive/
int ctrl_win::win_mgr(std::mutex * m, int sockid, struct sockaddr_in client_addr)
{
  bool flagForDone = true;
  while (!cl.empty() && flagForDone)
  {
    if (cl.front().get_status() != ACKED) 
    {
      continue;
    }
    //to move window pop first node. create and add last nodae.
    
    std::list<packet> p_list;
    char  data[DATA_SZ]; 
    pack_num_mutex.lock();
    
    for(int i = 0; i < 2; i++)
    {
      if(!fs.eof())
      {    
        set_null(data);
        fs.read(data, DATA_SZ);  //get data to send
        pack_num +=1;
        if(!fs.eof())
        {
          packet p(DATA, pack_num, data);
          p_list.push_back(p);
        }
        else
        {
          packet p(CLOSE, pack_num, data);
          p_list.push_back(p);
          break;
        }
      }
      pack_num_mutex.unlock();
    }
    if (!fs.eof())  //check for end of file
    {
      printf("Shifting window...(not end of file)\n");
      shift_win(m, sockid, client_addr, DATA, p_list);
    }
    else 
    {
      printf("Shifting window...(end of file, sending close packet)\n");
      shift_win(m, sockid, client_addr, CLOSE, p_list);
      flagForDone = false;
    }
  }
  while(true)
  {
    if(cl.empty())
    {
      break;
    }
    if(cl.front().get_status() == ACKED)
    {
      m->lock();
      pop_cnode();
      m->unlock();
    }
  }
  if (cl.empty() && !flagForDone) {
    return 0;
  }   
  return 1;
}


void ctrl_win::shift_win (std::mutex * m, int sockid, sockaddr_in client_addr, 
char type, std::list<packet> data)
{
      int status;
      try 
      {
        std::list<packet>::iterator data_iterator = data.begin();
        for(data_iterator; data_iterator != data.end(); data_iterator++)
        {
        //TODO: This needs to be done more than once as window grows
          status = send_packet(*data_iterator, sockid, client_addr);
           
          if(type == CLOSE)
          {
            m->lock();
            pop_cnode();
            m->unlock();
          }
          else
          {
            //if not end of file pop from front and append new packet
            ctrl_node cn(*data_iterator, sockid, client_addr);    //initialize control node w/ packet
            cn.set_status(SENT);
            cn.time_started = std::chrono::high_resolution_clock::now();
            m->lock();
            pop_cnode();            //pop front of ctrl list
            append_cnode(cn);  //append to end of ctrl list
            m->unlock();
          }
          printf("Sending new packet in Shift:window and popping first packet. Packet num: %d\n", data_iterator->packet_num);
          w_size++;
        }
      }
      catch (const std::exception& e) {}
}



void ctrl_win::log_ack(int pack_number)
{
  //iterate to the sequential node of the index
  std::list<ctrl_node>::iterator cl_iterator = cl.begin();
  for (int i = 0; i < cl.size(); ++i) 
  {
    if (cl_iterator->get_pack_num() == pack_number)
    {
      printf("Logging ack, packet number: %d\n", pack_number);
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
    //printf("DATA : %s\n", data);
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



