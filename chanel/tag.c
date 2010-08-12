/*
 * Copyright (c) 2007 NAKATA, Junya (jnakata@nict.go.jp)
 * All rights reserved.
 *
 * $Id: tag.c,v 1.1 2007/11/26 08:55:54 jnakata Exp $
 */

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <runecommon.h>
#include <runeservice.h>

#include "tag_public.h"
#include "tag_private.h"

#include "pic16f648.h"

#define nn2ni(n) ((uint32_t)(1 << (n)))

entryPoints ep = {
	.init = tag_init,
	.step = tag_step,
	.fin = tag_fin,
	.read = tag_read,
	.write = tag_write
};

void
timevalSub(struct timeval *a, struct timeval *b)
{

	a->tv_usec -= b->tv_usec;
	if(a->tv_usec < 0) {
		a->tv_sec--;
		a->tv_usec += 1000000;
	}
	a->tv_sec -= b->tv_sec;
}

void *
tag_sender(void *p)
{
	tagstruct *s = p;

pthread_setcancelstate(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	while(s->running != 1)
		releaseExec();
	for(;;) {
		while(s->cpu->sndstat != READY)
			releaseExec();
#ifdef GATEWAY_SLEEP
if((s->cpu->m_step > 60000000) && (s->cpu->m_step < 120000000))
goto skip;
#endif /* GATEWAY_SLEEP */
		s->frame.len = htonl(8);
		bcopy(s->cpu->sndbuf, s->frame.payload, 8);
		runeWrite(s->parent, 0, (condData *)&s->frame, NULL);
if(s->cpu->rcvstat == PENDING){fprintf(s->cpu->logfd, "%2d: packet collide with sending packet\n", s->cpu->gsid);fflush(s->cpu->logfd);}
		s->cpu->rcvscnt = 15808;
		s->cpu->rcvstat = CONFLICT;
//		fprintf(s->cpu->logfd, "s -t %f -Ni %d -Nl MAC -Ms %d -It AODV\n", (double)s->cpu->m_step / 1000000, s->gsid, s->gsid); fflush(s->cpu->logfd);
		fprintf(s->cpu->logfd,
		    "%16.8f:\t[ %4d ->    * ]\t"
		    "%02x %02x %02x %02x %02x %02x %02x %02x\n", 
		    (double)s->cpu->m_step / 1000000, s->gsid,
		    s->frame.payload[0], s->frame.payload[1],
		    s->frame.payload[2], s->frame.payload[3],
		    s->frame.payload[4], s->frame.payload[5],
		    s->frame.payload[6], s->frame.payload[7]);
		fflush(s->cpu->logfd);
#ifdef GATEWAY_SLEEP
skip:
#endif /* GATEWAY_SLEEP */
		s->cpu->sndstat = NODATA;
//fprintf(s->cpu->logfd, "%2d: packet sent\n", s->gsid); fflush(s->cpu->logfd);
	}
	return s;
}

void *
tag_init(int gsid)
{
	FILE *fd;
	tagstruct *p;

	if((p = (tagstruct *)malloc(sizeof(*p))) == NULL)
		return NULL;
	p->gsid = gsid;
	p->running = 0;
	if((fd = fopen("SmartTag.HEX", "r")) == NULL) {
		perror("fopen()");
		return NULL;
	}
#ifdef	SCALING_FACTOR
	if((p->cpu = pic16f648Alloc(4000000.0 / SCALING_FACTOR)) == NULL) {
		fprintf(stderr, "failed to allocate instance\n");
		return NULL;
	}
#else	/* !SCALING_FACTOR */
	if((p->cpu = pic16f648Alloc(4000000)) == NULL) {
		fprintf(stderr, "failed to allocate instance\n");
		return NULL;
	}
#endif
	p->cpu->sndstat = NODATA;
	p->cpu->sndcnt = 0;
	p->cpu->gsid = gsid;
	if(pic16f648LoadHex(p->cpu, fd) < 0) {
		fprintf(stderr, "failed to load binary\n");
		return NULL;
	}
	fclose(fd);
	if(gsid < 16) /* MOBILE */
		p->cpu->eeprom[0x10] = 0x03;
	else if(gsid < 20) /* FIXED */
		p->cpu->eeprom[0x10] = 0x02;
	else if(gsid < 24) /* GATEWAY */
		p->cpu->eeprom[0x10] = 0x01;
	else
		return NULL;
	p->cpu->eeprom[0x11] = (nn2ni(gsid) & 0x00ff0000) >> 16;
	p->cpu->eeprom[0x12] = (nn2ni(gsid) & 0x0000ff00) >> 8;
	p->cpu->eeprom[0x13] = (nn2ni(gsid) & 0x000000ff) >> 0;
	p->cpu->eeprom[0x14] = gsid;
	p->cpu->eeprom[0x15] = 0x02;
	p->cpu->eeprom[0x16] = 0x0f;
	p->cpu->eeprom[0x17] = 0x1f;

	if(pthread_create(&p->cpu->sender, NULL, tag_sender, p) < 0) {
		fprintf(stderr, "failed to create thread\n");
		return NULL;
	}
	return p;
}

int
tag_step(void *p)
{
	char filename[MAXPATHLEN];
	tagstruct *s = getStorage(p);
	
	s->parent = p;
	snprintf(filename, MAXPATHLEN, DTAG_LOGFILE, s->parent->name);
//printf("log file: %s\n", filename);
	if((s->cpu->logfd = fopen(filename, "w")) == NULL) {
		perror("fopen()");
		return -1;
	}
//	fprintf(s->cpu->logfd, "log started\n"); fflush(s->cpu->logfd);
	if(s->running == 0) {
		s->running = 1;
		if(pic16f648Start(s->cpu) < 0) {
			fprintf(stderr,
			    "failed to start processor instance\n");
			return -1;
		}
	}
	for(;;)
		releaseExec();
	return 0; 
}

void
tag_fin(void *p)
{
	tagstruct *s = getStorage(p);

	fclose(s->cpu->logfd);
	free(s);
}

void *
tag_read(void *p, void *a)
{

	return NULL;
}

void *
tag_write(void *p, void *a)
{
	condPacket *packet = (condPacket *)a;
	tagstruct *s = getStorage(p);

//	fprintf(s->cpu->logfd, "r -t %f -Ni %d -Nl MAC -Ms %d -Md %d -It AODV\n", (double)s->cpu->m_step / 1000000, s->gsid, ntohl(packet->sgsid), ntohl(packet->dgsid)); fflush(s->cpu->logfd);
	while(s->running == 0)
		goto out;
if(ntohl(packet->data.len) != 8)fprintf(stderr, "packet size (%d) incorrect\n", ntohl(packet->data.len));
	if(s->cpu->rcvstat == NODATA) {
		s->cpu->rcvscnt = 15808;
		s->cpu->rcvstat = PENDING;
		bcopy(packet->data.payload, s->cpu->rcvbuf, 8);
	} else if(s->cpu->rcvstat == PENDING) {
		s->cpu->rcvscnt = 15808;
		s->cpu->rcvstat = CONFLICT;
fprintf(s->cpu->logfd, "%2d: packet collide with receiving packet\n", s->cpu->gsid);fflush(s->cpu->logfd);
	} else if(s->cpu->rcvstat == CONFLICT) {
		s->cpu->rcvscnt = 15808;
		s->cpu->rcvstat = CONFLICT;
fprintf(s->cpu->logfd, "%2d: more packet dropped due to collision\n", s->cpu->gsid);fflush(s->cpu->logfd);
	}
/*
	fprintf(s->cpu->logfd,
	    "%16.8f:\t[ %4d -> %4d ]\t"
	    "%02x %02x %02x %02x %02x %02x %02x %02x\n", 
	    (double)s->cpu->m_step / 1000000,
	    ntohl(packet->sgsid), ntohl(packet->dgsid),
	    packet->data.payload[0], packet->data.payload[1],
	    packet->data.payload[2], packet->data.payload[3],
	    packet->data.payload[4], packet->data.payload[5],
	    packet->data.payload[6], packet->data.payload[7]);
	fflush(s->cpu->logfd);
 */
#ifdef GATEWAY_SLEEP
	if((s->cpu->m_step > 60000000) && (s->cpu->m_step < 160000000)) {
	s->cpu->rcvscnt = -1;
	s->cpu->rcvstat = NODATA;
}
#endif /* GATEWAY_SLEEP */
out:
	s->resp.status = 0;
	s->resp.packet.data.len = htonl(sizeof(int));
	return &s->resp;
}

