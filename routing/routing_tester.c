
/*
 * Copyright (c) 2006-2009 The StarBED Project  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/************************************************************************
 *
 * QOMET Emulator Implementation
 *
 * File name: routing_tester.c
 * Function: Source file of the routing control tester application
 *
 * Authors: Takashi Okada, Razvan Beuran
 *
 *   $Revision: 128 $
 *   $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#include "routing.h"

/* #define RTCTL2_TEST */

void usage(char *argv0)
{
	fprintf(stderr, "->%s -d [destination IP address] "
		"-g [gateway IP address] "
		"-m [subnet mask] "
		"-i [network interface name] "
		"-c [command name]\n", argv0);
}


int main(int argc, char *argv[])
{
	char		*daddr, *gaddr, *maddr, *ifname, *argv0;
	char		 c, *cmp_cmd;
	//	uint32_t 	mask;
	u_char		cmd;
	char		cmd_add[] = "add";
	char		cmd_delete[] = "delete";
	char		cmd_change[] = "change";
#ifdef RTCTL2_TEST
	in_addr_t	dstaddr, gateaddr, maskaddr;
	in_addr_t	*dstp, *gatep, *maskp;
	dstp = gatep = maskp = NULL;
#endif

	argv0 = argv[0];
	daddr = gaddr = maddr = ifname = NULL;
	cmd = 0;
	opterr = 0;
	while ((c = getopt(argc, argv, "d:g:m:i:c:")) != -1) {
		switch (c) {
		case 'd':
			daddr = optarg;
			printf("optarg=%s\n", optarg);
			break;
		case 'g':
			gaddr = optarg;
			printf("optarg=%s\n", optarg);
			break;
		case 'm':
			maddr = optarg;
			printf("optarg=%s\n", optarg);
			break;
		case 'i':
			ifname = optarg;
			printf("optarg=%s\n", optarg);
			break;
		case 'c':
			cmp_cmd = optarg;
			printf("optarg=%s\n", optarg);
			if (!strncmp(cmp_cmd, cmd_add, sizeof(cmd_add))) {
				cmd = (u_char) RTM_ADD;
			} else if (!strncmp(cmp_cmd, cmd_delete, 
				sizeof(cmd_delete)))
				cmd = RTM_DELETE;
			else if (!strncmp(cmp_cmd, cmd_change, 
				sizeof(cmd_change)))
				cmd = RTM_CHANGE;
			break;
		case '?':
		default:
			usage(argv0);
			exit(1);
		}
	}
#if 0
	/* check the inputed commands */
	fprintf(stderr, "dst:%s, gate:%s, mask:%d, dev:%s, cmd:%d\n",
		daddr, gaddr, maskaddr, ifname, cmd);
#endif
#ifdef RTCTL2_TEST
	if (daddr != NULL) {
		if ((dstaddr = inet_addr(daddr)) < 0) {
			fprintf(stderr, "Invalid dst address.\n");
			exit(1);
		}
		dstp = &dstaddr;
	}
	if (gaddr != NULL) {
		if ((gateaddr = inet_addr(gaddr)) < 0) {
			fprintf(stderr, "Invalid gate address.\n");
			exit(1);
		}
		gatep = &gateaddr;
	}
	if (maddr != NULL) {
		if ((maskaddr = inet_addr(maddr)) < 0) {
			fprintf(stderr, "Invalid mask address.\n");
			exit(1);
		}
		maskp = &maskaddr;
	}
	if (rtctl2(dstp, gatep, maskp, ifname, cmd) < 0) 
		exit(1);
#else
	if (rtctl(daddr, gaddr, maddr, ifname, cmd) < 0) 
		exit(1);
#endif

	return 0;
}

