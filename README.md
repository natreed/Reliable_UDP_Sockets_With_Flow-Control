# AugmentedUDP - Project for Internetworking Protocols
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

## To run:
### Start the Server :
```
- From Linux terminal, enter 'Make' to create executables (Server, Client)
- Start server listening on the port of your choice.
-- i.e ./Server 8000
```
### Start the Client - Download file from server:
```
- ./Client <server IP i.e. localhost> <port i.e 8000> <absolute path to file on server>
- The data will be saved to filename.out in the client
```
## Implementation Overview and Key Concepts


***Three-way handshake*** similar to TCP’s handshake. As part of the handshake, the client must request a file to be sent by the server requireing a file request packet the handshake implements Stop-and-Wait ARQ flow control protocol. 

***Stop-and-Wait ARQ flow control protocol.*** The Stop-and-Wait protocol sends on packet of data at a time. After a data packet is sent to the client, the server waits until an acknowledgment packet (ACK) is received by the server. If an ACK is not received by the server before a timeout in milliseconds expires, the server assumes that the packet was lost and resends the data packet. Only after the ACK has been received the server can continue sending the next packet. 

***File request packet***. The file request packet specifies the exact path of the file that the client wants to retrieve (Conceptually, it is similar to a SYN packet in TCP). The server must acknowledge the request and notify the file size to the client (Similar to a SYN+ACK packet in TCP). If the file does not exist in the server the client must be notified and the connection terminated. Finally the client must acknowledge the reception of the file size and signal the server to start sending the file. (This is similar to the final ACK of the three-way handshake in TCP). After the request is acknowledged, the server can start sending the data to the client. See below for packet types.

***Flow Control- Go-Back N***. Go-Back-N is a sliding-window based protocol, that is, the protocol operates over a limited but configurable number of packets. In Go-Back-N the server sends up to N packets, for a window of size N, before waiting for an ACK from the client. Furthermore,in Go-Back-N acknowledgments are cumulative, hence a single ACK can mark several packets as successfully received. More specifically an Acknowledgement for packet N+1, will also acknowledge packet N.  This opens the possibility that the client might decide to send only acknowledgments for some of the packets. In this implementation, the client still acknowledges every single packet. Once the ACK from a data packet at the beginning of the window is received, the server can slide the window allowing for an additional packet to be sent. 

***Packet Loss***. To recover from packet loses, Go-Back-N uses a timer mechanism in which the oldest packet in the window must be acknowledged before the timer expiries. In case that the oldest packet in the window expires the server will re-send every single packet in the window. If no ACK has been received before the timer expires it means the packet was lost in transit and must be resent.

***Concurrency*** The Server sends data and receives acknowledgementsin a concurrent fashion. The mainthread in the server is responsible for sending packets.) Additionally there is a sliding window/ queue that allows the server to keep the state of each of the data packets (i.e.Available for Sending, Sent, Acknowledged, Expired). The queue holds at most N packets, where N is the size of the sliding window specified by the user. A third execution flow in the server is waiting for a timer to expire in case the packet is lost.

***Packet Structure***
```
- Connection Packet (SYN): Type[1]=S PktLen[2] Filename[PktLen]
- Connection Reply Packet (SYN+ACK): Type[1]=R FileSize[8]
- Connection Reply ACK Packet: Type[1]=W
- DATA Packet: Type[1]=D SeqNum[4] PktLen[2] Data[PktLen]
- DATA ACK Packet: Type[1]=A SeqNum[4]
- Close Connection: Type[1]=C SeqNum[4]
