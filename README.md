# AugmentedUDP
Add features to UDP socket (reliable data transfer)

## Requirements (C++11)

## Problem Description
Design and implement a simple File Transfer Protocol similar to TCP (but much simpler), using UDP sockets and datagrams. Modeled after TCP/IP, the is on implementing a reliable channel, flow control and retransmission.

## How does it work?

```
- Establish a connection between client and server.
- Specify the absolute path location within the server filesystem where the desired file is located
- Transfer the file to the Client
- Close the connection.
```
### Server:
```
- compile with 


Your server must use the command line parameters to allow the user specify the port to listen for connections from other clients. This is an example of the command a user might use to start the server listening on port 1080:
./cs494rcp_server 1080
Your client program must use the command line parameters to allow the user to specify the IP address and port of the server to connect. It should also allow the user to specify the file path to the requested file. This is an example of the command a user might use to download a file located in the server computer at 120.45.67.126 and listening on port 1080:
./cs494rcp_client 120.45.67.126 1080 /Docs/Files/myfile.txt
The client should save a copy of the file, however to avoid name collisions when the server and client are run on the same computer, you must save the sent file with a different name. To do so the client
must append the substring “.out” at the end of the filename. For the example above the resulting filename should be “myfile.txt.out”.
3. Implementation Overview
For the First Part of the Programming Project we will implement the Stop-and-Wait ARQ flow control protocol. The Stop-and-Wait protocol sends on packet of data at a time. After a data packet is sent to the client, the server waits until an acknowledgment packet (ACK) is received by the server. If an ACK is not received by the server before a timeout in milliseconds expires, the server assumes that the packet was lost and resends the data packet. Only after the ACK has been received the server can continue sending the next packet. Please note you are not allowed to use SO_RCVTIMEO for this project! Using SO_RCVTIMEO will make the second part of the project much more complicated
These are a few key concepts you must address as part of your implementation:
1) You must implement a protocol that establishes handshake between the client and the server. We will implement a three-way handshake similar to TCP’s handshake. As part of the handshake, the client must request a file to be sent by the server. To do this your protocol must implement a file request packet. A file request packet must specify the exact path of the file that the client wants to retrieve (Conceptually, this will be similar to a SYN packet in TCP). The server must acknowledge the request and notify the file size to the client (Similar to a SYN+ACK packet in TCP). If the file does not exist in the server the client must be notified and the connection terminated. Finally the client must acknowledge the reception of the file size and signal the server to start sending the file. (This is similar to the final ACK of the three-way handshake in TCP)
2) After the request is acknowledged, the server can start sending the data to the client, by sending only one packet at a time. Before sending the next packet the server must wait for an acknowledgement from the client. This method is not efficient but it will be the basis for more
efficient flow control mechanisms used in Part 2. It is recommended you keep your packets small, up to 1Kb maximum length, otherwise they can be dropped by intermediate routers.
3) Your server must wait for a timer to expire in case the packet is lost. If no ACK has been received before the timer expires it means the packet was lost in transit and must be resent.
4) After the file transfer completes your server must notify the client using a Close packet before closing the connection. The server must wait for the client to respond and only then close the connection. The client can terminate the connection as soon as he acknowledges the closing of the connection.
4. Packet Structure
As part of your project you will need to design and implement various packet types to handle the required requests and responses of your protocol. You are allowed to change or design your own packet as you see fit. However, we provide a possible design suitable for this project. Please note all the recommended sizes are specified in bytes
 Connection Packet (SYN)
Type[1]=S
PktLen[2]
Filename[PktLen]
 Connection Reply Packet (SYN+ACK)
Type[1]=R
FileSize[8]
 Connection Reply ACK Packet
Type[1]=W
 DATA Packet
Type[1]=D
SeqNum[4]
PktLen[2]
Data[PktLen]
 Data ACK Packet
Type[1]=A
SeqNum[4]
 Close Connection and Close Connection ACK Packet
Type[1]=C
SeqNum[4]
5. Testing Script
To aid in grading and testing your project, you must prepare a BASH script (test.sh) that starts the server on port 1080, then starts the client on port 1080 and send the file “test.jpg” provided as part of the project package. The script then must compare the input and output files for correctness. A good way to do this is by using md5sum.
The project package provides a sample BASH script you can use to write your own script.
6. Hand-In
For submission, you should provide only source code (*.c, *.cpp, *.h), a Makefile script that compiles both the Client and the Server code and a test script test.sh.
Please pack your files into a TAR file before submitting your solution. Do not include test.jpg or this PDF. We will use the D2L system for submission (https://d2l.pdx.edu/). Please remember that there is no late policy.

## Adding Flow Control to UDP
Go-Back-N is a sliding-window based protocol, that is, the protocol operates over a limited but configurable number of packets.
In Go-Back-N the server sends up to N packets, for a window of size N, before waiting for an ACK from the client. Furthermore,in Go-Back-N acknowledgments are cumulative,hence a single ACK can mark several packets as successfullyreceived.More specifically an Acknowledgement for packet N+1, will also acknowledge packet N.  Therefore this opens the possibility that the client might decide to sendonly acknowledgments for some of the packets. However, in our implementation, the client will still acknowledge every single packet.Once the ACK from a data packet at the beginningof thewindow is received, the server can slide the window allowing for an additional packet to be sent. Please note that Go-Back-N is a pipelined protocol and therefore the server must send data and receive acknowledgementsin a concurrent fashion.Therefore, is not sufficient to simply send N packets and then wait for N acknowledgmentsin the same thread. Instead your server implementation will need separate threads for sending data and receivingACKs.Go-Back-N still requires packets to be received in order. The client will drop any packets that are not theexpected packet.This makes the client behave very similar to Stop-And-Go and hence much of the work is performed by the server. To recover from packet loses, Go-Back-N uses atimer mechanism in which the oldest packet in the window must be acknowledged before the timer expiries.In case that the oldest packet in the window expires the serverwill re-send every single packet in the window. Please note that this might cause the client to receive several duplicated packets. This is normal and an accepted behavior in Go-Back-N. Go-Back-Ncan be used with a static window size or a dynamic window size. We will use a configurable static window sizespecified by the user when launching the server.4.Implementation OverviewYou are allowed to use only one socket connection between client and server to send and receive packets. This is important as our protocol is a pipelined protocol and not a parallel version of Stop and Go. Many ofthe components of Part 1 willremainunchanged:Your program still needs to performa three-way handshake to establish the connection before sending the fileand itmust perform a simple two-way handshake to close the connection after the file has been sent. Also most ofthe client will remain unchanged.However,data transmissionover the server will change significantly. These are a few key concepts you must address as part of your implementation:1)Your server must send data and receive acknowledgementsin a concurrent fashion. Therefore you need to add a separate thread to receive acknowledgements.The mainthread in the server will still beresponsible forsending packets.2)You must implement a sliding window/ queue that allows the server to keep the state of each of the data packets (i.e.Available for Sending, Sent,Acknowledged, Expired). Please note that the queue mustnot hold the entire file. Your queue must old at most N packets, where N is the size of the sliding window specified by the user.You are allowed touse one of several STL C++ containers including but not limited to maps and vectors, however, you can also implement your own circular queue. 
3)Similarly,toStop-Wait ARQfrom Project 1,your server must wait for a timer to expire in case the packet is lost. If no ACK has been received before the timer expires it means the packet was lost in transit and must be resent. 4)Our pipelined design,leavesour sending thread without work when thequeue is full. Your design must address this case by putting the thread to sleepuntil there is at least one slot available in the sliding window. For this purpose you can use a Conditional Variable, a mutex or a semaphore.However, youmust not spin while the sliding window / queue is full5)Finally, you must address the data races created by the 3 concurrent executionflows (i.e.Main/Sending Thread,Acknowledge Thread and Timer). You must use a lock(i.e.mutex)to protect thecritical section connection and exit.5.Unreliable Channel SimulatorTo aid in yourtesting we have provided you with a packet simulator that willdrop, delay and duplicate packets. The simulator works by interposing the POSIX sendto() and rcvfrom() functions during compile time. Therefore,to use the simulator, you must compilethe provided C++ source file unrel_sendto.cppas part of your project and link against both your client and server. Moreover, the linking process requires to pass the following flags to the -Wl,-wrap=sendto -Wl,-wrap=recvfromFor example to compile the program server.cpp and link the simulator you can use the following command:g++-Wl,-wrap=sendto -Wl,-wrap=recvfromserver.cpp unrel_sendto.cpp -o server As an indication that the simulator was properly enabled, your client and server should print a message with the parameters of the simulator as soon as they start.6.Testing ScriptTo aid in grading and testing your project, you must prepare a BASH script (test.sh) that starts the server on port 1080, then starts the client on port 1080 and send the file “test.jpg” provided as part of the project package. The script then must comparethe input and output files for correctness. A good way to do this is by using md5sum. The project package provides a sample BASH script you can use to write your own script. 7.Hand-InFor submission, you should provide only source code (*.c,*.cpp,*.h) and a Makefile script that compiles both the Client and the Server codeand a test script test.sh. Please pack your files into a TAR file before submitting your solution.Do not include test.jpg,this PDF, executables or object files.We will use the D2L system for submission (https://d2l.pdx.edu/). Please remember that there is no late policy.
