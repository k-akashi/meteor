/*
 * Copyright (c) 2007 NAKATA, Junya (jnakata@nict.go.jp)
 * All rights reserved.
 *
 * $Id: pic16f648.h,v 1.1 2007/12/09 20:30:38 jnakata Exp $
 */

#ifndef _LIBPIC16F648_H_
#define _LIBPIC16F648_H_

#include <sys/types.h>
#include <stdio.h>

#ifdef ACTIVETAG
enum stat {NODATA, PENDING, CONFLICT, READY};
#endif /* ACTIVETAG */

#ifdef __FreeBSD__
#define rdtsc(t) asm volatile("rdtsc" : "=A" (t))
#elif __linux
#define rdtsc(t) asm volatile("rdtsc" : "=A" (t))
#endif
#define RSTVEC	0x0000
#define INTVEC	0x0004
#define TMR0	0x0001
#define PCL	0x0002
#define STATUS	0x0003
#define FSR	0x0004
#define PORTA	0x0005
#define PORTB	0x0006
#define PCLATH	0x000a
#define INTCON	0x000B
#define PIR1	0x000c
#define TMR1L	0x000e
#define TMR1H	0x000f
#define T1CON	0x0010
#define TMR2	0x0011
#define T2CON	0x0012
#define CCPR1L	0x0015
#define CCPR1H	0x0016
#define CCP1CON	0x0017
#define RCSTA	0x0018
#define TXREG	0x0019
#define RCREG	0x001a
#define CMCON	0x001f
#define OPTREG	0x0081
#define TRISA	0x0085
#define TRISB	0x0086
#define PIE1	0x008c
#define PCON	0x008e
#define PR2	0x0092
#define TXSTA	0x0098
#define SPBRG	0x0099
#define EEDATA	0x009a
#define EEADR	0x009b
#define EECON1	0x009c
#define EECON2	0x009d
#define VRCON	0x009f
#define IRP	0x80
#define GIE	0x80
#define PSA	0x08
#define T0CS	0x20
#define T0IE	0x20
#define T0IF	0x04
#define TMR1CS	0x02
#define TMR1IE	0x01
#define TMR1IF	0x01
#define TMR1ON	0x01
#define TMR2IE	0x02
#define TMR2IF	0x02
#define TMR2ON	0x04
#define T2CKPS1	0x02
#define T2CKPS0	0x01
#define RD	0x01
#define REG(p, a) /* access register on current bank */		\
((p)->reg.m[(((p)->reg.m[STATUS] & 0x60) << 2) + ((a) & 0x7f)])
#define REGD(p, a) /* access register on all banks directly*/	\
((p)->reg.m[(a)])
#define PADDR(p)						\
((p)->pc)
#if 0
#define SETPC(p, a) do {					\
(p)->pc = ((a) & 0x07ff)					\
	| (((p)->reg.bank0[PCLATH] & 0x0018) << 8);		\
(p)->reg.bank0[PCL] = (p)->reg.bank1[PCL] = 			\
(p)->reg.bank2[PCL] = (p)->reg.bank3[PCL] = (p)->pc & 0xff;	\
} while(0)
#endif
#define SETPC(p, a) do {					\
(p)->pc = (a);							\
(p)->reg.bank0[PCL] = (p)->reg.bank1[PCL] = 			\
(p)->reg.bank2[PCL] = (p)->reg.bank3[PCL] = (p)->pc & 0xff;	\
} while(0)
#define SETACC(p, a)						\
(p)->accumulator = (a) & 0x00ff
#define INCPC(p) do {						\
(p)->pc++;							\
(p)->reg.bank0[PCL] = (p)->reg.bank1[PCL] = 			\
(p)->reg.bank2[PCL] = (p)->reg.bank3[PCL] = (p)->pc & 0xff;	\
} while(0)
#define GETCF(p) ((p)->reg.bank0[STATUS] & 0x01)
#define SETCF(p) ((p)->reg.bank0[STATUS]			\
= (p)->reg.bank1[STATUS] = (p)->reg.bank2[STATUS]		\
= (p)->reg.bank3[STATUS] |= 0x01)
#define CLRCF(p) ((p)->reg.bank0[STATUS]			\
= (p)->reg.bank1[STATUS] = (p)->reg.bank2[STATUS]		\
= (p)->reg.bank3[STATUS] &= 0xfe)
#define GETDC(p) (((p)->reg.bank0[STATUS] & 0x02) >> 1)
#define SETDC(p) ((p)->reg.bank0[STATUS]			\
= (p)->reg.bank1[STATUS] = (p)->reg.bank2[STATUS]		\
= (p)->reg.bank3[STATUS] |= 0x02)
#define CLRDC(p) ((p)->reg.bank0[STATUS]			\
= (p)->reg.bank1[STATUS] = (p)->reg.bank2[STATUS]		\
= (p)->reg.bank3[STATUS] &= 0xfd)
#define GETZF(p) (((p)->reg.bank0[STATUS] & 0x04) >> 2)
#define SETZF(p) ((p)->reg.bank0[STATUS]			\
= (p)->reg.bank1[STATUS] = (p)->reg.bank2[STATUS]		\
= (p)->reg.bank3[STATUS] |= 0x04)
#define CLRZF(p) ((p)->reg.bank0[STATUS]			\
= (p)->reg.bank1[STATUS] = (p)->reg.bank2[STATUS]		\
= (p)->reg.bank3[STATUS] &= 0xfb)
#define SETGIE(p) ((p)->reg.bank0[INTCON]			\
= (p)->reg.bank1[INTCON] = (p)->reg.bank2[INTCON]		\
= (p)->reg.bank3[INTCON] |= GIE)
#define CLRGIE(p) ((p)->reg.bank0[INTCON]			\
= (p)->reg.bank1[INTCON] = (p)->reg.bank2[INTCON]		\
= (p)->reg.bank3[INTCON] &= ~GIE)
#define SETTO(p) ((p)->reg.bank0[STATUS]			\
= (p)->reg.bank1[STATUS] = (p)->reg.bank2[STATUS]		\
= (p)->reg.bank3[STATUS] |= 0x10)
#define SETPD(p) ((p)->reg.bank0[STATUS]			\
= (p)->reg.bank1[STATUS] = (p)->reg.bank2[STATUS]		\
= (p)->reg.bank3[STATUS] |= 0x08)
#define CLRTO(p) ((p)->reg.bank0[STATUS]			\
= (p)->reg.bank1[STATUS] = (p)->reg.bank2[STATUS]		\
= (p)->reg.bank3[STATUS] &= 0xef)
#define CLRPD(p) ((p)->reg.bank0[STATUS]			\
= (p)->reg.bank1[STATUS] = (p)->reg.bank2[STATUS]		\
= (p)->reg.bank3[STATUS] &= 0xf7)
#define CLRWDTPS(p) ((p)->reg.bank1[OPTREG]			\
= (p)->reg.bank3[OPTREG] &= 0xf8)
#define DST(p) (((p) & 0x0080) ? 'F' : 'W')
#define BIT(p) (((p) & 0x0380) >> 7)
#define PUSH(p, v) do {						\
(p)->stack[(p)->sp] = v;					\
(p)->sp = ((p)->sp + 1) & 0x07;					\
} while(0)
#define POP(p, v) do {						\
(p)->sp = ((p)->sp - 1) & 0x07;					\
v = (p)->stack[(p)->sp];					\
} while(0)
#ifdef MEASUREMENT
#define COUNTSTEP(p) ((p)->m_step++)
#else /* !MEASUREMENT */
#define COUNTSTEP(p) /* */
#endif /* MEASUREMENT */

#define SHOWMSG(p, ...) do {					\
fprintf((p)->logfd, "[%s:%6d] ", __FILE__, __LINE__);		\
fprintf((p)->logfd, "%s(): \t", __func__);			\
fprintf((p)->logfd, __VA_ARGS__);					\
fprintf((p)->logfd, "\n");						\
} while(0)

#ifdef VERBOSE
#define DEBUGMSG(p, ...) SHOWMSG((p), "debug \t" __VA_ARGS__)
#define WARNINGMSG(p, ...) SHOWMSG((p), "warning \t" __VA_ARGS__)
#else /* VERBOSE */
#define DEBUGMSG(p, ...) /* __VA_ARGS__ */
#define WARNINGMSG(p, ...) /* __VA_ARGS__ */
#endif /* !VERBOSE */

#define ERROR(p, ...) SHOWMSG((p), "error " __VA_ARGS__)

typedef struct {
	union {
		uint8_t	m[0x0200];
		struct {
			uint8_t bank0[0x80];
			uint8_t bank1[0x80];
			uint8_t bank2[0x80];
			uint8_t bank3[0x80];
		};
	} reg;
	uint16_t memory[0x1000];
	uint8_t eeprom[0x0100];
	uint16_t pc;
	uint16_t stack[8];
	uint32_t sp;
	uint16_t config;
	uint8_t accumulator;
	uint8_t WDT;
	uint8_t presc0;
	uint8_t presc1;
	uint8_t presc2;
	uint8_t pstsc2;
	uint16_t tmr2eq;
	uint32_t tick;
	uint64_t next;
	int extracycle;
	int loaded;
	int running;
	int intr;
	pthread_mutex_t lock;
	pthread_t thread;
	uint64_t m_freq;
	uint64_t m_sttm;
	uint64_t m_crtm;
	uint64_t m_step;
	uint64_t m_int0;
	uint64_t m_int1;
	uint64_t m_int2;
#ifdef ACTIVETAG
	pthread_t sender;
	uint32_t gsid;
	enum stat rcvstat;
	uint8_t rcvbuf[8];
	int32_t rcvscnt;
	enum stat sndstat;
	int sndcnt;
	uint8_t sndbuf[8];
#endif /* ACTIVETAG */
	FILE *logfd;
} pic16f648_t;

void pic16f648ShowInfo(pic16f648_t *p);
pic16f648_t *pic16f648Alloc(uint32_t Hz);
void pic16f648Release(pic16f648_t *p);
int pic16f648LoadHex(pic16f648_t *p, FILE *fd);
int pic16f648Start(pic16f648_t *p);

#endif /* LIBPIC16F648_H_ */

