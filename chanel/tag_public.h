/*
 * Copyright (c) 2007 NAKATA, Junya (jnakata@nict.go.jp)
 * All rights reserved.
 *
 * $Id: tag_public.h,v 1.1 2007/12/09 20:30:38 jnakata Exp $
 */

#ifndef _TAG_PRIVATE_H_
#define _TAG_PRIVATE_H_

#include <sys/types.h>

typedef struct {
	uint32_t len;
	uint8_t payload[8];
} tagframe;

typedef struct {
	condPacket packet;
	int status;
} tagresp;

#endif /* _TAG_PRIVATE_H_ */

