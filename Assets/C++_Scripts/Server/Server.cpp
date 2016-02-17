#include "Server.h"

using namespace Networking;

/****************************************************************************
Initialize socket, server address to lookup to, and connect to the server

@return: socket file descriptor
*****************************************************************************/
int Server::Init_TCP_Server_Socket(char* name, short port)
{
    int err = -1;
    std::cerr << "Init_TCP_Server_Socket" << std::endl;
    /* Create a stream socket */
    if ((_TCPAcceptingSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
    {
        fatal("Init_TCP_Server_Socket: socket() failed\n");
        return _TCPAcceptingSocket;
    }
    std::cerr << "socket finished" << std::endl;
    /* Fill in server address information */
    //bzero((char *)&_ServerAddress, sizeof(struct sockaddr_in));
    memset(&_ServerAddress, 0, sizeof(struct sockaddr_in));

    _ServerAddress.sin_family = AF_INET;
    _ServerAddress.sin_port = htons(port);
    _ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client
    std::cerr << "binding" << std::endl;

    /* bind server address to accepting socket */
    if ((err = bind(_TCPAcceptingSocket, (struct sockaddr *)&_ServerAddress, sizeof(_ServerAddress))) == -1)
    {
        fatal("Init_TCP_Server_Socket: bind() failed\n");
        return err;
    }
    std::cerr << "listening" << std::endl;

    /* Listen for connections */
    listen(_TCPAcceptingSocket, MAXCONNECTIONS);

    return 0;
}


/****************************************************************************
Initialize socket, server address to lookup to, and connect to the server

@return: socket file descriptor
*****************************************************************************/
int Server::TCP_Server_Accept()
{
    struct sockaddr_in  ClientAddress;
    unsigned int        ClientLen = sizeof(ClientAddress);
    Player              p;
    pthread_t		ClientThread;

    /* Accepts a connection from the client */
    if ((_TCPReceivingSocket = accept(_TCPAcceptingSocket, (struct sockaddr *)&ClientAddress, &ClientLen)) == -1)
    {
        fatal("TCP_Server_Accept: accept() failed\n");
        return -1;
    }

    /* Adds the address and socket to the vector list */
    _ClientAddresses.push_back(ClientAddress);
    _ClientSockets.push_back(_TCPReceivingSocket);

    /* Adds the player to a team */
    if((_TeamOne.size()+_TeamTwo.size()) < MAXCONNECTIONS) 
    {
        p.connection = ClientAddress;
        if(_TeamOne.size() <= _TeamTwo.size()) 
        {
            p.team = "Team One";
            _TeamOne.push_back(p);
	    	send (_TCPReceivingSocket, "a", 1, 0);
        } 
        else if(_TeamTwo.size() < _TeamOne.size()) 
        {
            p.team = "Team Two";
            _TeamTwo.push_back(p);
	    	send (_TCPReceivingSocket, "b", 1, 0);
        } 
        else 
        {
            std::cerr << "Unable to add player to team" << std::endl;
            return -1;
        }
    } 
    else 
    {
        std::cerr << "The lobby is full" << std::endl;
        return -1;
    }
    std::cerr << "The player is added to " << std::endl;
    
	pid_t ClientProcess;
	ClientProcess = fork();

	if(ClientProcess == 0)
	{
		TCP_Server_Listen();
	}    

    /***************************************************
    * Create a child process here to handle new client * ****************************************************/
    return 0;
}

/****************************************************************************
Infinite Loop for listening on a connect client's socket. This is used by
threads.

@return: NULL
*****************************************************************************/
void Server::TCP_Server_Listen()
{
	std::string 	packet; 				/* packet received from the Client */	

	/*
	TODO:
	-Create a method where the thread is able to leave the infinite loop
		and or kill the thread naturally.
	*/
	while(1)
	{ 
		packet = TCP_Server_Receive();
		if(packet.size() > 0)
		{
			std::cerr << "Got a packet." << std::endl;
		}
	}

	return;
}

/****************************************************************************
Recives packets from a specific socket, should be in a child proccess

@return: packet of size PACKETLEN
*****************************************************************************/
std::string Server::TCP_Server_Receive()
{
    std::string         packet;                             /* packet to be returned               */
    int                 BytesRead;                          /* bytes read from one recv call       */
    int                 BytesToRead;                        /* remaining bytes to read from socket */
    char *              buf = (char *)malloc(BUFSIZE);      /* buffer read from one recv call      */

    BytesToRead = PACKETLEN;

    while ((BytesRead = recv (_TCPReceivingSocket, buf, PACKETLEN, 0)) < PACKETLEN)
    {
        /* store buffer read into packet */
        packet += buf;

        /* decrement remaining bytes to read */
        BytesToRead -= BytesRead;
    }
    free(buf);
    return packet;
}

/****************************************************************************
prints the list of addresses currently stored

@return: void
*****************************************************************************/
void Server::PrintAddresses()
{
    std::cout << "List of addresses\n";
    /*for(auto x : _ClientAddresses)
    {
      std::cout << "scamaz\n";
    }*/
}

/****************************************************************************
Initialize server socket, fill in the server address, and binds the address
to the socket

@return: void
*****************************************************************************/
int Server::Init_UDP_Server_Socket(char* name, short port)
{
    int err;
    /* Create a file descriptor for the socket */

    if((err = _UDPReceivingSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        fatal("Init_UDP_Server_Socket: socket() failed\n");
        return err;
    }

    /* Fill in server socket address structure */
    memset((char *)&_ServerAddress, 0, sizeof(_ServerAddress));
    _ServerAddress.sin_family = AF_INET;
    _ServerAddress.sin_port = htons(port);
    _ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Bind server address to the socket */
    if((err = bind(_UDPReceivingSocket, (sockaddr *)&_ServerAddress, sizeof(_ServerAddress))) < 0)
    {
        fatal("Init_UDP_Server_Socket: bind() failed\n");
        return err;
    }

    return 0;

}

/****************************************************************************
Listen for incoming UDP traffics

@return: a packet
*****************************************************************************/
std::string Server::UDP_Server_Recv()
{
    int err;
    std::string packet;                     /* packet recieved  */
    struct sockaddr_in Client;              /* Incoming client's socket address information */
    unsigned ClientLen = sizeof(Client);
    char * buf = (char *)malloc(BUFSIZE);

    if((err = recvfrom(_UDPReceivingSocket, buf, BUFSIZE, 0, (sockaddr *)&Client, &ClientLen)) <= 0)
    {
        fatal("UDP_Server_Recv: recvfrom() failed\n");
        return NULL;
    }

    _ClientAddresses.push_back(Client);

    packet = buf;
    free(buf);
    return packet;
}

/****************************************************************************
Sends a message to all the clients

*****************************************************************************/
void Server::pingAll(char* message)
{
    for(int i = 0 ; 0 < _ClientSockets.size(); i++){
        send (_ClientSockets[i], message, sizeof(message), 0);
    }
}

void Server::fatal(char* error)
{
    std::cerr << error << std::endl;
}
