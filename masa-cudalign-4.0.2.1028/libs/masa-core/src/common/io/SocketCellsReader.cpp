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

#include "SocketCellsReader.hpp"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <errno.h>
#include <netdb.h> //hostent

SocketCellsReader::SocketCellsReader(string hostname, int port) {
    this->hostname = hostname;
    this->port = port;
    this->socketfd = -1;
    init();
}

SocketCellsReader::~SocketCellsReader() {
	close();
}

void SocketCellsReader::close() {
    fprintf(stderr, "SocketCellsReader::close(): %d\n", socketfd);
    if (socketfd != -1) {
        ::close(socketfd);
        socketfd = -1;
    }
}

int SocketCellsReader::getType() {
	return INIT_WITH_CUSTOM_DATA;
}

int SocketCellsReader::read(cell_t* buf, int len) {
    int pos = 0;
    while (pos < len*sizeof(cell_t)) {
    	int ret = recv(socketfd, (void*)(((unsigned char*)buf)+pos), len*sizeof(cell_t), 0);
        if (ret == -1) {
        	close();
            fprintf(stderr, "recv: Socket error -1\n");
            break;
        }
        pos += ret;
    }
    return pos/sizeof(cell_t);
}

void SocketCellsReader::init() {
    int rc;
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */



    /* Create a reliable, stream socket using TCP */
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "ERROR creating socket: %s\n", strerror(errno));
		sleep(1);
	}

	/* Resolving DNS */
	char ip[100];
	if (resolveDNS(hostname.c_str(), ip)) {
        fprintf(stderr, "FATAL: cannot resolve hostname: %s\n", hostname.c_str());
        exit(-1);
	}

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Zero out structure */
    echoServAddr.sin_family      = AF_INET;             /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(ip);   /* Server IP address */
    echoServAddr.sin_port        = htons(port); /* Server port */

    /* Establish the connection to the echo server */
    int max_retries = 3000;
    int retries = 0;
    int ok = 0;
    fprintf(stderr, "Listening on %s %d\n", hostname.c_str(), port);
    while ((retries < max_retries) && !ok) {
		if ((rc=connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr))) < 0) {
			if (retries % 100 == 0) {
				fprintf(stderr, "ERROR connecting to Server %s:%d [Retry %d/%d]. %s\n", ip, port,
						retries, max_retries, strerror(errno));
			}
			retries++;
			usleep(10000);
		} else {
			ok = 1;
		}
	}
	if (!ok) {
		fprintf(stderr, "ERROR connecting to Server. Aborting\n");
		exit(-1);
	}
    fprintf(stderr, "Connected to Server %s\n", inet_ntoa(echoServAddr.sin_addr));

    this->socketfd = sock;
}

int SocketCellsReader::resolveDNS(const char * hostname , char* ip) {
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    if ( (he = gethostbyname( hostname ) ) == NULL)
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }

    addr_list = (struct in_addr **) he->h_addr_list;

    for(i = 0; addr_list[i] != NULL; i++)
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }

    return 1;
}
