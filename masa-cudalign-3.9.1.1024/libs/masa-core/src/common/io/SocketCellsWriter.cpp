/*******************************************************************************
 *
 * Copyright (c) 2010-2015   Edans Sandes
 *
 * This file is part of MASA-Core.
 * 
 * MASA-Core is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * MASA-Core is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MASA-Core.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include "SocketCellsWriter.hpp"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <errno.h>

#define DEBUG (0)

SocketCellsWriter::SocketCellsWriter(string hostname, int port) {
    this->hostname = hostname;
    this->port = port;
    this->socketfd = -1;
    init();
}

SocketCellsWriter::~SocketCellsWriter() {
	close();
}

void SocketCellsWriter::close() {
    fprintf(stderr, "SocketCellsWriter::close(): %d\n", socketfd);
    if (socketfd != -1) {
        ::close(socketfd);
        socketfd = -1;
    }
}

int SocketCellsWriter::write(const cell_t* buf, int len) {
    int ret = send(socketfd, buf, len*sizeof(cell_t), 0);
    if (ret == -1) {
        fprintf(stderr, "send: Socket error: -1\n");
        return 0;
    }
    return ret;
}

void SocketCellsWriter::init() {
    int rc;
    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int clntLen;            /* Length of client address data structure */

    if (DEBUG) printf("SocketCellsWriter: create socket\n");
    /* Create socket for incoming connections */
    if ((servSock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "ERROR creating server socket; return code from socket() is %d\n", servSock);
        exit(-1);
    }

    int optval = 1;
    setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));



    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(port);      /* Local port */

    /* Bind to the local address */
    if (DEBUG) printf("SocketCellsWriter: Bind to local address %d\n", port);
    if ((rc = bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr))) < 0) {
        fprintf(stderr, "ERROR; return code from bind() is %d\n", rc);
        exit(-1);
    }

    /* Mark the socket so it will listen for incoming connections */
    if (DEBUG) printf("SocketCellsWriter: Listening on port %d\n", port);
    if ((rc=listen(servSock, 1)) < 0) {
        fprintf(stderr, "ERROR; return code from listen() is %d\n", rc);
        exit(-1);
    }

    /* Set the size of the in-out parameter */
    clntLen = sizeof(echoClntAddr);

    /* Wait for a client to connect */
    if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0){
        fprintf(stderr, "ERROR; return code from accept() is %d\n", clntSock);
        exit(-1);
    }

    /* clntSock is connected to a client! */

    fprintf(stderr, "Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

    //HandleTCPClient(clntSock);

    ::close(servSock);

    this->socketfd = clntSock;
}
