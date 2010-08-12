/*
 * Copyright (c) 2007 NAKATA, Junya (jnakata@nict.go.jp)
 * All rights reserved.
 *
 * $Id: tag_private.h,v 1.1 2007/11/26 08:55:54 jnakata Exp $
 */

#include <sys/param.h>
#include <sys/time.h>
#include "pic16f648.h"

#define DTAG_LOGFILE "./%s.log"

void *tag_init(int gsid);
int tag_step(void *p);
void tag_fin(void *p);
void *tag_read(void *p, void *a);
void *tag_write(void *p, void *a);

typedef struct {
	int gsid;
	int running;
	localSpaceList *parent;
	pic16f648_t *cpu;
	tagframe frame;
	tagresp resp;
} tagstruct;

