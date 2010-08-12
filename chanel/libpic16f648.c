/*
 * Copyright (c) 2007 NAKATA, Junya (jnakata@nict.go.jp)
 * All rights reserved.
 *
 * $Id: libpic16f648.c,v 1.1 2007/12/09 20:30:38 jnakata Exp $
 */

#include <sys/types.h>
#include <sys/sysctl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <unistd.h>
#include "pic16f648.h"

#ifdef MEASUREMENT
void
pic16f648ShowInfo(pic16f648_t *p)
{

	rdtsc(p->m_crtm);
/*
 */
	fprintf(p->logfd, "pc: %04x\toperation: %04x\tstep: %8llu\n",
	    p->pc, p->memory[p->pc], p->m_step);
	fprintf(p->logfd, "int0: %8llu\tint1: %8llu\tint2: %8llu\n",
	    p->m_int0, p->m_int1, p->m_int2);
	fprintf(p->logfd, "%s context\n", p->intr ? "Interrupt" : "Normal");
	fprintf(p->logfd, "GIE: %s, T0IE: %s, TMR1E: %s, TMR2E: %s\n",
	(p->reg.m[INTCON] & GIE) ? "Enabled " : "Disabled",
	(p->reg.m[INTCON] & T0IE) ? "Enabled " : "Disabled",
	(p->reg.m[PIE1] & TMR1IE) ? "Enabled " : "Disabled",
	(p->reg.m[PIE1] & TMR2IE) ? "Enabled " : "Disabled");
	fprintf(p->logfd, "%8llu cycl\t%16lf sec\n", p->m_crtm
	    - p->m_sttm, (double)(p->m_crtm - p->m_sttm) / p->m_freq);
	fprintf(p->logfd, "%16lf step/usec\t%16lf usec/step\n",
	    (double)p->m_step * p->m_freq / ((p->m_crtm - p->m_sttm))
	    / 1000000, (double)(p->m_crtm - p->m_sttm) / (p->m_step
	    * p->m_freq) * 1000000);
	fprintf(p->logfd, "%16lf int0/msec\t%16lf msec/int0\n",
	    (double)p->m_int0 * p->m_freq / ((p->m_crtm - p->m_sttm))
	    / 1000, (double)(p->m_crtm - p->m_sttm) / (p->m_int0
	    * p->m_freq) * 1000);
	fprintf(p->logfd, "%16lf int1/msec\t%16lf msec/int1\n",
	    (double)p->m_int1 * p->m_freq / ((p->m_crtm - p->m_sttm))
	    / 1000, (double)(p->m_crtm - p->m_sttm) / (p->m_int1
	    * p->m_freq) * 1000);
	fprintf(p->logfd, "%16lf int2/msec\t%16lf msec/int2\n",
	    (double)p->m_int2 * p->m_freq / ((p->m_crtm - p->m_sttm))
	    / 1000, (double)(p->m_crtm - p->m_sttm) / (p->m_int2
	    * p->m_freq) * 1000);
/*
 */
	fprintf(p->logfd, "\t%16lf MHz\n", (double)p->m_step * p->m_freq
	    / ((p->m_crtm - p->m_sttm)) / 1000000 * 4);
	fflush(p->logfd);
}
#endif

inline static int
pic16f648Lock(pic16f648_t *p)
{

	return(pthread_mutex_lock(&p->lock));
}

inline static int
pic16f648Unlock(pic16f648_t *p)
{

	return(pthread_mutex_unlock(&p->lock));
}

static uint64_t
getCpuFreq(void)
{
        uint8_t f[8];
	uint64_t hz;
	size_t s = sizeof(hz);

#if defined(__APPLE__) && defined(__MACH__)
	sysctlbyname("hw.cpufrequency", NULL, &s, NULL, 0);
	sysctlbyname("hw.cpufrequency", f, &s, NULL, 0);
#elif defined(__FreeBSD__)
	sysctlbyname("machdep.tsc_freq", NULL, &s, NULL, 0);
	sysctlbyname("machdep.tsc_freq", f, &s, NULL, 0);
#endif
	if(s == 4)
		hz = *(uint32_t *)f;
	else if(s == 8)
		hz = *(uint64_t *)f;
	else
		hz = 0;
	return hz;
}

static int
pic16f648Wait(pic16f648_t *p)
{
#ifndef BESTEFFORT
	uint64_t current;

	COUNTSTEP(p);
	rdtsc(current);
/*
	if(current > p->next + p->tick / 2) {
#if 0
		DEBUGMSG(p, "deadline missed!\n"
		    "curr: \t%8lld\nnext: \t%8lld\n", current, p->next);
#endif
		p->next += p->tick;
		return -1;
	}
 */
	while(current < p->next - p->tick / 2) {
		sched_yield();
		rdtsc(current);
	}
	p->next += p->tick;
#endif
	pthread_testcancel();
	return 0;
}

inline void
addWait(pic16f648_t *p)
{

	p->extracycle = 1;
}

inline static uint8_t
pic16f684ReadReg(pic16f648_t *p, uint16_t a)
{
	uint8_t v;
	uint16_t addr;

	addr = ((a & 0x007f) > 0x0000)
	    ? (p->reg.m[STATUS] & 0x0060) << 2 | (a & 0x007f)
	    : ((p->reg.m[STATUS] & IRP) << 1) | p->reg.m[FSR];
#ifdef ACTIVETAG
//if(addr == 0x0069)printf("%04x: BK1_WRITE_PTR =\t%02x\n", p->pc, p->reg.m[addr]);
//if(addr == 0x006a)printf("%04x: BK1_READ_PTR =\t%02x\n", p->pc, p->reg.m[addr]);
//if(((a & 0x007f) == 0x0000) && (p->pc >= 0x05b7) && (p->pc <= 0x0612))printf("address %04x read(%02x). \tpc:%04x\n", addr, p->reg.m[addr], p->pc);
//if((addr >= 0x70) && (addr <= 0x77))printf("address %04x read(%02x). \tpc:%04x\n", addr, p->reg.m[addr], p->pc);
#endif /* ACTIVETAG */
	v = p->reg.m[addr];
	return v;
}

inline static void
pic16f648SetReg(pic16f648_t *p, uint16_t a, uint8_t v)
{
	uint16_t addr;

//printf("writing %04x(%02x:%02x)\n", ((p->reg.m[STATUS] & 0x60) << 2) | (a & 0x007f), ((p->reg.m[STATUS] & 0x60) >> 5), (a & 0x007f));
#if 0
	DEBUGMSG(p);
#endif
	addr = ((a & 0x007f) > 0x0000)
	    ? (p->reg.m[STATUS] & 0x0060) << 2 | (a & 0x007f)
	    : ((p->reg.m[STATUS] & IRP) << 1) | p->reg.m[FSR];
#ifdef ACTIVETAG
//if(addr == 0x0069)printf("%04x: BK1_WRITE_PTR <-\t%02x\n", p->pc, v);
//if(addr == 0x006a)printf("%04x: BK1_READ_PTR <-\t%02x\n", p->pc, v);
//if(addr == 0x0069)fprintf(p->logfd, "[%04x]\tBK1_READ_PTR: %02x ->", p->pc, p->reg.m[0x0069]);
//if(addr == 0x006a)fprintf(p->logfd, "[%04x]\tBK1_READ_PTR: %02x ->", p->pc, p->reg.m[0x006a]);
//if(addr == 0x0063)fprintf(p->logfd, "[%04x]\tSTFLAG: %02x -> ", p->pc, p->reg.m[0x0063]);
//if((addr & 0x007f) == INTCON)fprintf(p->logfd, "[%04x]\tINTCON: %02x\t", p->pc, p->reg.m[INTCON]);
//if((addr & 0x007f) == T1CON)fprintf(p->logfd, "[%04x]\tT1CON: %02x\t", p->pc, p->reg.m[T1CON]);
//if((addr & 0x007f) == T2CON)fprintf(p->logfd, "[%04x]\tT2CON: %02x\t", p->pc, p->reg.m[T2CON]);
#endif /* ACTIVETAG */
	if(((addr & 0x007f) > 0x001f) && ((addr & 0x007f) < 0x0070)) {
		REGD(p, addr) = v;
//if(addr == 0x0063)fprintf(p->logfd, "%02x\n", p->reg.m[0x0063]);
		return;
	} else if((addr & 0x007f) >= 0x0070) {
		REGD(p, (addr & 0x007f))
		    = REGD(p, (addr & 0x007f) + 0x0080)
		    = REGD(p, (addr & 0x007f) + 0x0100)
		    = REGD(p, (addr & 0x007f) + 0x0180) = v;
		return;
	}
	switch(addr) {
	case 0x0001:
	case 0x0101:
		/* TMR0 */
		REGD(p, 0x0001) = REGD(p, 0x0101) = v;
		if((p->reg.m[OPTREG] & 0x08) != 0)
			p->presc0 = 0xfe;
		else		/* prescaler in use */
			p->presc0 = 0xfe - (p->reg.m[OPTREG] & 0x07);
		break;
	case 0x0002:
	case 0x0082:
	case 0x0102:
	case 0x0182:
		/* PCL */
		REGD(p, 0x0002) = REGD(p, 0x0082) = REGD(p, 0x0102)
		    = REGD(p, 0x0182) = v;
		p->pc = ((p->reg.m[PCLATH] & 0x001f) << 8) | v;
		break;
	case 0x0003:
	case 0x0083:
	case 0x0103:
	case 0x0183:
		/* STATUS */
		REGD(p, 0x0003) = REGD(p, 0x0083) = REGD(p, 0x0103)
		    = REGD(p, 0x0183) = v;
		break;
	case 0x0004:
	case 0x0084:
	case 0x0104:
	case 0x0184:
		/* FSR */
		REGD(p, 0x0004) = REGD(p, 0x0084) = REGD(p, 0x0104)
		    = REGD(p, 0x0184) = v;
		break;
	case 0x0006:
	case 0x0106:
		/* PORTB */
		REGD(p, 0x0006) = REGD(p, 0x0106) = v;
		break;
#ifdef ACTIVETAG
	case 0x0007:
//printf("|| trying to send!! \t%02x ||\n", v);
		p->sndbuf[p->sndcnt++] = v;
		if(p->sndcnt == 8) {
			p->sndstat = READY;
			p->sndcnt = 0;
		}
		break;
#endif /* ACTIVETAG */
	case 0x000a:
	case 0x008a:
	case 0x010a:
	case 0x018a:
		/* PCLATH */
		REGD(p, 0x000a) = REGD(p, 0x008a) = REGD(p, 0x010a)
		    = REGD(p, 0x018a) = v;
		break;
	case 0x000b:
	case 0x008b:
	case 0x010b:
	case 0x018b:
		/* INTCON */
		REGD(p, 0x000b) = REGD(p, 0x008b) = REGD(p, 0x010b)
		    = REGD(p, 0x018b) = v;
		break;
	case 0x000e:
		/* TMR1L */
		REGD(p, 0x000e) = v;
		p->presc1 = 0x00;
		break;
	case 0x000f:
		/* TMR1H */
		REGD(p, 0x000f) = v;
		p->presc1 = 0x00;
		break;
	case 0x0011:
		/* TMR2 */
		REGD(p, 0x0011) = v;
		p->presc2 = 0x00;
		p->pstsc2 = 0x00;
		break;
	case 0x0012:
		/* T2CON */
		REGD(p, 0x0012) = v;
		p->presc2 = 0x00;
		p->pstsc2 = 0x00;
		break;
	case 0x009a:
		/* EEDATA not supported */
		break;
	case 0x009c:
		/* EECON1 */
//printf("EEPROM(%04x) is accessed.\n", p->reg.m[EEADR]);
		if((v & RD) != 0) {
			p->reg.m[EEDATA] = p->eeprom[p->reg.m[EEADR]];
			REGD(p, EECON1) = v & 0xfe;
		}
		break;
	case 0x009d:
		/* EECON2 not supported */
		break;
	case 0x0081:
	case 0x0181:
		/* OPTREG */
		REGD(p, 0x0081) = REGD(p, 0x0181) = v;
		break;
	case 0x0086:
	case 0x0186:
		/* TRISB */
		REGD(p, 0x0086) = REGD(p, 0x0186) = v;
		break;
	default:
		REGD(p, addr) = v;
		break;
	}
//if((addr & 0x007f) == INTCON)fprintf(p->logfd, "[%04x]\tINTCON: %02x\n", p->pc, p->reg.m[INTCON]);
//if((addr & 0x007f) == T1CON)fprintf(p->logfd, "[%04x]\tT1CON: %02x\n", p->pc, p->reg.m[T1CON]);
//if((addr & 0x007f) == T2CON)fprintf(p->logfd, "[%04x]\tT2CON: %02x\n", p->pc, p->reg.m[T2CON]);
}

static int
pic16f648OpAddlw(pic16f648_t *p, uint16_t op)
{
	uint32_t prev, tmp;

	DEBUGMSG(p, "%04x: %04x addlw %02x", PADDR(p), op, op & 0x00ff);
	prev = p->accumulator;
	tmp = prev + (op & 0x00ff);
	SETACC(p, tmp & 0x000000ff);
	if((tmp & 0xffffff00) != 0x00000000)
		SETCF(p);
	else
		CLRCF(p);
	if((tmp & 0xfffffff0) != (prev & 0xfffffff0))
		SETDC(p);
	else
		CLRDC(p);
	if(tmp == 0x00000000)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpAndlw(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x andlw %02x", PADDR(p), op, op & 0x00ff);
	SETACC(p, p->accumulator & (op & 0x00ff));
	if(p->accumulator == 0x00)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpAddwf(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp, prev;

	DEBUGMSG(p, "%04x: %04x addwf %02x, %c", PADDR(p), op, op & 0x007f,
	    DST(op));
	prev = pic16f684ReadReg(p, op);
	tmp = p->accumulator + prev;
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp);
	if((tmp & 0xffffff00) != 0x00000000)
		SETCF(p);
	else
		CLRCF(p);
	if((tmp & 0xfffffff0) != (prev & 0xfffffff0))
		SETDC(p);
	else
		CLRDC(p);
	if((tmp & 0x000000ff) == 0x00000000)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpAndwf(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp;

	DEBUGMSG(p, "%04x: %04x andfw %02x, %c", PADDR(p), op,
	    op & 0x007f, DST(op));
	tmp = p->accumulator & pic16f684ReadReg(p, op);
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp);
	if((tmp & 0x000000ff) == 0x00000000)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpBcf(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x bcf %02x, %1x", PADDR(p), op,
	    op & 0x007f, BIT(op));
	pic16f648SetReg(p, op, (pic16f684ReadReg(p, op)
	    & ~(1 << ((op & 0x0380) >> 7))) & 0xff);
	INCPC(p);
	return 0;
}

static int
pic16f648OpBsf(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x bsf %02x, %1x", PADDR(p), op,
	    op & 0x007f, BIT(op));
	pic16f648SetReg(p, op & 0x007f, (pic16f684ReadReg(p, op)
	    | (1 << ((op & 0x0380) >> 7))) & 0xff);
	INCPC(p);
	return 0;
}

static int
pic16f648OpBtfsc(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp;

	DEBUGMSG(p, "%04x: %04x btfsc %02x, %1x", PADDR(p), op,
		op & 0x007f, BIT(op));
	tmp = pic16f684ReadReg(p, op) & (1 << ((op & 0x0380) >> 7));
	if(tmp == 0) {
		addWait(p);
		INCPC(p);
	}
	INCPC(p);
	return 0;
}

static int
pic16f648OpBtfss(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp;

	DEBUGMSG(p, "%04x: %04x btfss %02x, %1x", PADDR(p), op,
		op & 0x007f, BIT(op));
	tmp = pic16f684ReadReg(p, op) & (1 << ((op & 0x0380) >> 7));
	if(tmp != 0) {
		addWait(p);
		INCPC(p);
	}
	INCPC(p);
	return 0;
}

static int
pic16f648OpCall(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x call %04x", PADDR(p), op, op & 0x07ff);
	PUSH(p, p->pc + 1);
	SETPC(p,
	    ((p->reg.bank0[PCLATH] & 0x0018) << 8 ) | (op & 0x07ff));
	addWait(p);
	return 0;
}

static int
pic16f648OpClrf(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x clrf %02x", PADDR(p), op, op & 0x007f);
	pic16f648SetReg(p, op, 0x00);
	SETZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpClrw(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x clrw", PADDR(p), op);
	SETACC(p, 0x00);
	SETZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpClrwdt(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x clrwdt", PADDR(p), op);
	p->WDT = 0x00;
	if((p->reg.m[OPTREG] & PSA) == 1)
		p->presc0 = 0;
	CLRWDTPS(p);
	SETTO(p);
	SETPD(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpComf(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp;

	DEBUGMSG(p, "%04x: %04x comf %02x, %c", PADDR(p), op,
	    op & 0x007f, DST(op));
	tmp = ~pic16f684ReadReg(p, op);
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp);
	if((tmp & 0x000000ff) == 0x00000000)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpDecf(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp;

	DEBUGMSG(p, "%04x: %04x decf %02x, %c", PADDR(p), op,
	    op & 0x007f, DST(op));
	tmp = pic16f684ReadReg(p, op) - 1;
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp);
	if((tmp & 0x000000ff) == 0x00000000)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpDecfsz(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp;

	DEBUGMSG(p, "%04x: %04x decfsz %02x, %c", PADDR(p), op,
		op & 0x007f, DST(op));
	tmp = pic16f684ReadReg(p, op) - 1;
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp);
	if((tmp & 0x000000ff) == 0x00000000) {
		addWait(p);
		INCPC(p);
	}
	INCPC(p);
	return 0;
}

static int
pic16f648OpGoto(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x goto %04x", PADDR(p), op, op & 0x07ff);
	SETPC(p,
	    ((p->reg.bank0[PCLATH] & 0x0018) << 8 ) | (op & 0x07ff));
	addWait(p);
	return 0;
}

static int
pic16f648OpIncf(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp;

	DEBUGMSG(p, "%04x: %04x incf %02x, %c", PADDR(p), op,
	    op & 0x007f, DST(op));
	tmp = pic16f684ReadReg(p, op) + 1;
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp);
	if((tmp & 0x000000ff) == 0x00000000)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpIncfsz(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp;

	DEBUGMSG(p, "%04x: %04x incszf %02x, %c", PADDR(p), op,
		op & 0x007f, DST(op));
	tmp = pic16f684ReadReg(p, op) + 1;
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp);
	if((tmp & 0x000000ff) == 0x00000000) {
		addWait(p);
		INCPC(p);
	}
	INCPC(p);
	return 0;
}

static int
pic16f648OpIorlw(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x iorlw %02x", PADDR(p), op, op & 0x00ff);
	SETACC(p, p->accumulator | (op & 0x00ff));
	if(p->accumulator == 0x00)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpIorwf(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp;

	DEBUGMSG(p, "%04x: %04x iorwf %02x, %c", PADDR(p), op,
	    op & 0x007f, DST(op));
	tmp = p->accumulator | pic16f684ReadReg(p, op);
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp);
	if((tmp & 0x000000ff) == 0x00000000)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpMovlw(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x movlw %02x", PADDR(p), op, op & 0x00ff);
	SETACC(p, op & 0x00ff);
	INCPC(p);
	return 0;
}

static int
pic16f648OpMovf(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp;

	DEBUGMSG(p, "%04x: %04x movf %02x, %c", PADDR(p), op,
	    op & 0x007f, DST(op));
	tmp = pic16f684ReadReg(p, op);
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp);
	if((tmp & 0x000000ff) == 0x00000000)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpMovwf(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x movwf %02x", PADDR(p), op, op & 0x007f);
	pic16f648SetReg(p, op, p->accumulator);
	INCPC(p);
	return 0;
}

static int
pic16f648OpNop(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x nop", PADDR(p), op);
	INCPC(p);
	return 0;
}

static int
pic16f648OpRetfie(pic16f648_t *p, uint16_t op)
{
	uint16_t addr;

	DEBUGMSG(p, "%04x: %04x retfie", PADDR(p), op);
//fprintf(p->logfd, "[%04x]\tINTCON: %02x\t", p->pc, p->reg.m[INTCON]);
	SETGIE(p);//fprintf(p->logfd, "[%04x]\tINTCON: %02x\n", p->pc, p->reg.m[INTCON]);
	POP(p, addr);
	SETPC(p, addr);
	p->intr = 0;
	addWait(p);
	return 0;
}

static int
pic16f648OpRetlw(pic16f648_t *p, uint16_t op)
{
	uint16_t addr;

	DEBUGMSG(p, "%04x: %04x retlw %02x", PADDR(p), op, op & 0x00ff);
	SETACC(p, op & 0x00ff);
	POP(p, addr);
	SETPC(p, addr);
	addWait(p);
	return 0;
}

static int
pic16f648OpReturn(pic16f648_t *p, uint16_t op)
{
	uint16_t addr;

	DEBUGMSG(p, "%04x: %04x return", PADDR(p), op);
	POP(p, addr);
	SETPC(p, addr);
	addWait(p);
	return 0;
}

static int
pic16f648OpRlf(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp;

	DEBUGMSG(p, "%04x: %04x rlf %02x, %c", PADDR(p), op,
	    op & 0x007f, DST(op));
	tmp = (pic16f684ReadReg(p, op) << 1) | GETCF(p);
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp);
	if((tmp & 0x00000100) == 0)
		CLRCF(p);
	else
		SETCF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpRrf(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp, lsb;

	DEBUGMSG(p, "%04x: %04x rrf %02x, %c", PADDR(p), op,
	    op & 0x007f, DST(op));
	lsb = pic16f684ReadReg(p, op) & 0x01;
	tmp = (pic16f684ReadReg(p, op) >> 1) | (GETCF(p) << 7);
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp);
	if(lsb == 0)
		CLRCF(p);
	else
		SETCF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpSleep(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x sleep", PADDR(p), op);
	WARNINGMSG(p, "sleep instruction is not supported.");
	p->WDT = 0x00;
	CLRWDTPS(p);
	SETTO(p);
	CLRPD(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpSublw(pic16f648_t *p, uint16_t op)
{
	uint32_t prev, tmp;

	DEBUGMSG(p, "%04x: %04x sublw %02x", PADDR(p), op, op & 0x00ff);
	prev = p->accumulator;
	tmp = (op & 0x00ff) - prev;
	SETACC(p, tmp & 0x000000ff);
	if((tmp & 0xffffff00) != 0x00000000)
		CLRCF(p);
	else
		SETCF(p);
	if((tmp & 0xfffffff0) != (prev & 0xfffffff0))
		CLRDC(p);
	else
		SETDC(p);
	if(tmp == 0x00000000)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpSubwf(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp, prev;

	DEBUGMSG(p, "%04x: %04x subwf %02x, %c", PADDR(p), op,
	    op & 0x007f, DST(op));
	prev = pic16f684ReadReg(p, op);
	tmp = prev - p->accumulator;
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp);
	if((tmp & 0xffffff00) != 0x00000000)
		CLRCF(p);
	else
		SETCF(p);
	if((tmp & 0xfffffff0) != (prev & 0xfffffff0))
		CLRDC(p);
	else
		SETDC(p);
	if((tmp & 0x000000ff) == 0x00000000)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpSwapf(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp;

	DEBUGMSG(p, "%04x: %04x swapf %02x, %c", PADDR(p), op,
	    op & 0x007f, DST(op));
	tmp = pic16f684ReadReg(p, op);
	tmp = ((tmp & 0x0000000f) << 4) | ((tmp & 0x000000f0) >> 4);
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp);
	INCPC(p);
	return 0;
}

static int
pic16f648OpXorlw(pic16f648_t *p, uint16_t op)
{

	DEBUGMSG(p, "%04x: %04x xorlw %02x", PADDR(p), op, op & 0x00ff);
	SETACC(p, p->accumulator ^ (op & 0x00ff));
	if(p->accumulator == 0x00)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648OpXorwf(pic16f648_t *p, uint16_t op)
{
	uint32_t tmp;

	DEBUGMSG(p, "%04x: %04x xorwf %02x, %c", PADDR(p), op,
	    op & 0x007f, DST(op));
	tmp = p->accumulator ^ pic16f684ReadReg(p, op);
	if((op & 0x0080) == 0x0000)
		SETACC(p, tmp & 0x000000ff);
	else
		pic16f648SetReg(p, op, tmp & 0x000000ff);
	if((tmp & 0x000000ff) == 0x00000000)
		SETZF(p);
	else
		CLRZF(p);
	INCPC(p);
	return 0;
}

static int
pic16f648ProcInt(pic16f648_t *p)
{

#if 0
	DEBUGMSG(p);
#endif
#ifdef ACTIVETAG
	if(p->rcvscnt > 0)
		p->rcvscnt--;
//if((p->rcvscnt == 0) && (p->rcvstat == NODATA)) fprintf(stderr, "%02d: timer expired, status = NODATA\n", p->gsid);
//if((p->rcvscnt == 0) && (p->rcvstat == PENDING)) fprintf(stderr, "%02d: timer expired, status = PENDING\n", p->gsid);
//if((p->rcvscnt == 0) && (p->rcvstat == CONFLICT)) fprintf(stderr, "%02d: timer expired, status = CONFLICT\n", p->gsid);
//if((p->rcvscnt == 0) && (p->rcvstat == READY)) fprintf(stderr, "%02d: timer expired, status = READY\n", p->gsid);
	if(p->rcvscnt == 0) {
		if(p->rcvstat == CONFLICT) {
			p->rcvscnt = -1;
			p->rcvstat = NODATA;
		} else if(p->rcvstat == PENDING) {
			bcopy(p->rcvbuf, &p->reg.m[0x0070], 8);
			bcopy(p->rcvbuf, &p->reg.m[0x00f0], 8);
			bcopy(p->rcvbuf, &p->reg.m[0x0170], 8);
			bcopy(p->rcvbuf, &p->reg.m[0x01f0], 8);
//fprintf(p->logfd, "0070: %02x %02x %02x %02x %02x %02x %02x %02x\n", p->reg.m[0x0070], p->reg.m[0x0071], p->reg.m[0x0072], p->reg.m[0x0073], p->reg.m[0x0074], p->reg.m[0x0075], p->reg.m[0x0076], p->reg.m[0x0077]);
			p->reg.bank0[INTCON] = p->reg.bank1[INTCON]
			    = p->reg.bank2[INTCON] = p->reg.bank3[INTCON]
			    |= T0IF;
#ifdef MEASUREMENT
			p->m_int0++;
#endif /* MEASUREMENT */
			if(p->intr == 0) {
				p->intr = 1;
				PUSH(p, p->pc);
				SETPC(p, INTVEC);
				CLRGIE(p);//fprintf(p->logfd, "[%04x]\tINTCON: %02x\n", p->pc, p->reg.m[INTCON]);
			}
//fprintf(p->logfd, "timer0 prescaled interrupt at %lf\n", (double)p->m_step / 1000000);
			p->rcvscnt = -1;
			p->rcvstat = NODATA;
/*
 */
			fprintf(p->logfd,
			    "%16.8f:\t[    * -> %4d ]\t"
			    "%02x %02x %02x %02x %02x %02x %02x %02x\n",
			    (double)p->m_step / 1000000, p->gsid,
			    p->rcvbuf[0], p->rcvbuf[1], p->rcvbuf[2],
			    p->rcvbuf[3], p->rcvbuf[4], p->rcvbuf[5],
			    p->rcvbuf[6], p->rcvbuf[7]);
			fflush(p->logfd);
/*
 */
/*
 *	LOG OUTPUT
 */
			if((p->gsid >= 20) && ((p->rcvbuf[0] & 0xc0) == 0x80)){
				int i;
				uint32_t tid, log, rid;

//				tid = 1 << p->rcvbuf[1];
				tid = p->rcvbuf[1] + 1;
				log = ((uint32_t)p->rcvbuf[4] << 16) | ((uint32_t)p->rcvbuf[5] << 8) | ((uint32_t)p->rcvbuf[6] << 0);
				for(i = 0; i < 24; i++) {
					rid = 1 << i;
					if((log & rid) != 0) {
						fprintf(p->logfd,
						    "%02X%02X%02X%02X%02X%02X"
						    "%02X%02X%02X%02X%02X%02X"
						    "%02X%02X%02X%02X%02X%02X"
						    "%02X\n",
						    0x24, 0x81, 0x13, 0xff,
						    0x00, 0x00,
						    p->rcvbuf[2], p->rcvbuf[3],
						    (tid & 0xff000000) >> 24,
						    (tid & 0x00ff0000) >> 16,
						    (tid & 0x0000ff00) >> 8,
						    tid & 0x000000ff,
						    ((i + 1) & 0xff000000) >> 24,
						    ((i + 1) & 0x00ff0000) >> 16,
						    ((i + 1) & 0x0000ff00) >> 8,
						    (i + 1) & 0x000000ff,
						    0x08, 0xff, 0x0d);
						fflush(p->logfd);
					}
				}
			}
		}
	}
#endif /* ACTIVETAG */
/*
 *	Timer0
 */
/*REMOVE!!*/goto timer1;/*REMOVE!!*/
	if((p->reg.m[OPTREG] & T0CS) == 1)
	    goto timer1;
	p->presc0++;
	if((p->reg.m[OPTREG] & PSA) == 0) {	/* prescaler in use */
		if(p->presc0
		    == (1 << ((p->reg.m[OPTREG] & 0x07) + 1))) {
			p->presc0 = 0;
			if((p->reg.bank0[TMR0] = p->reg.bank2[TMR0]++)
			    == 0xff) {
				p->reg.bank0[INTCON]
				    = p->reg.bank1[INTCON]
				    = p->reg.bank2[INTCON]
				    = p->reg.bank3[INTCON] |= T0IF;
//fprintf(p->logfd, "timer0 prescaled interrupt at %lf\n", (double)p->m_step / 1000000);
				if((p->reg.bank0[INTCON] & GIE)
				    && (p->reg.m[INTCON] & T0IE)) {
#ifdef MEASUREMENT
					p->m_int0++;
#endif /* MEASUREMENT */
					if(p->intr == 0) {
						p->intr = 1;
						PUSH(p, p->pc);
						SETPC(p, INTVEC);
						CLRGIE(p);//fprintf(p->logfd, "[%04x]\tINTCON: %02x\n", p->pc, p->reg.m[INTCON]);
					}
				}
			}
		}
	} else {	/* prescaler not in use */
		if((p->reg.bank0[TMR0] = p->reg.bank2[TMR0]++)
		    == 0xff) {
			p->reg.bank0[INTCON] = p->reg.bank1[INTCON]
			    = p->reg.bank2[INTCON]
			    = p->reg.bank3[INTCON] |= T0IF;
//fprintf(p->logfd, "timer0 interrupt at %lf\n", (double)p->m_step / 1000000);
			if((p->reg.m[INTCON] & GIE)
			    && (p->reg.m[INTCON] & T0IE)) {
#ifdef MEASUREMENT
				p->m_int0++;
#endif /* MEASUREMENT */
				if(p->intr == 0) {
					p->intr = 1;
					PUSH(p, p->pc);
					SETPC(p, INTVEC);
					CLRGIE(p);//fprintf(p->logfd, "[%04x]\tINTCON: %02x\n", p->pc, p->reg.m[INTCON]);
				}
			}
		}
	}
/*
 *	Timer1
 */
timer1:
	if((p->reg.m[T1CON] & TMR1ON) == 0)
		goto timer2;
	if((p->reg.m[T1CON] & TMR1CS) == 1)
		goto timer2;
//printf("p->presc1 = %d, p->reg.m[TMR1L] = %d, p->reg.m[TMR1H] = %d\n", p->presc1, p->reg.m[TMR1L], p->reg.m[TMR1H]);
//printf("p->reg.m[INTCON] = %d, p->reg.m[PIE1] = %d\n", p->reg.m[INTCON], p->reg.m[PIE1]);
	p->presc1++;
	if(p->presc1 == (1 << ((p->reg.m[T1CON] & 0x30) >> 4))) {
		p->presc1 = 0;
		if(p->reg.m[TMR1L]++ == 0xff) {
//printf("p->reg.m[INTCON]<GIE>: %s, p->reg.m[PIE1]<TMR1E>: %s\n", (p->reg.m[INTCON] & GIE) ? "set" : "clear", (p->reg.m[PIE1] & TMR1IE) ? "set" : "clear");
			if(p->reg.m[TMR1H]++ == 0xff) {
				p->reg.m[PIR1] |= TMR1IF;
//fprintf(p->logfd, "timer1 interrupt at %lf\n", (double)p->m_step / 1000000);
				if((p->reg.m[INTCON] & GIE)
				    && (p->reg.m[PIE1] & TMR1IE)) {
#ifdef MEASUREMENT
					p->m_int1++;
#endif /* MEASUREMENT */
					if(p->intr == 0) {
						p->intr = 1;
						PUSH(p, p->pc);
						SETPC(p, INTVEC);
						CLRGIE(p);//fprintf(p->logfd, "[%04x]\tINTCON: %02x\n", p->pc, p->reg.m[INTCON]);
					}
				}
			}
		}
	}
/*
 *	Timer2
 */
timer2:
	if((p->reg.m[T2CON] & TMR2ON) == 0)
		goto skip;
	p->presc2++;
	if(((p->reg.m[T2CON] & T2CKPS1) != 0) && (p->presc2 == 16)) {
#ifdef INTLOG
fprintf(p->logfd, "%16llu: timer2 prescaler overflow.\n", p->m_step);
#endif	/* INTLOG */
	/* x16 prescaler */
		p->presc2 = 0;
		if(p->reg.m[TMR2]++ == p->reg.m[PR2]) {
#ifdef INTLOG
fprintf(p->logfd, "%16llu: timer2 counter overflow.\n", p->m_step);
#endif	/* INTLOG */
			p->reg.m[TMR2] = 0;
			if(p->pstsc2++
			    == ((p->reg.m[T2CON] & 0x78) >> 3)) {
#ifdef INTLOG
fprintf(p->logfd, "%16llu: timer2 postscaler overflow.\n", p->m_step);
#endif	/* INTLOG */
				p->pstsc2 = 0;
				p->reg.m[PIR1] |= TMR2IF;
//fprintf(p->logfd, "timer2 interrupt at %lf\n", (double)p->m_step / 1000000);
				if((p->reg.m[INTCON] & GIE)
				    && (p->reg.m[PIE1] & TMR2IE)) {
#ifdef INTLOG
fprintf(p->logfd, "%16llu: timer2 interrupt generated.\n", p->m_step);
#endif	/* INTLOG */
#ifdef MEASUREMENT
					p->m_int2++;
#endif /* MEASUREMENT */
					if(p->intr == 0) {
						p->intr = 1;
						PUSH(p, p->pc);
						SETPC(p, INTVEC);
						CLRGIE(p);//fprintf(p->logfd, "[%04x]\tINTCON: %02x\n", p->pc, p->reg.m[INTCON]);
					}
				}
			}
		}
	} else if(p->presc2
		    == (1 << ((p->reg.m[T2CON] & T2CKPS0) * 2)) - 1) {
#ifdef INTLOG
fprintf(p->logfd, "%16llu: timer2 prescaler overflow.\n", p->m_step);
#endif	/* INTLOG */
	/* x1/x4 prescaler */
		p->presc2 = 0;
		if(p->reg.m[TMR2]++ == p->reg.m[PR2]) {
#ifdef INTLOG
fprintf(p->logfd, "%16llu: timer2 counter overflow.\n", p->m_step);
#endif	/* INTLOG */
			p->reg.m[TMR2] = 0;
			if(p->pstsc2++
			    == ((p->reg.m[T2CON] & 0x78) >> 3)) {
#ifdef INTLOG
fprintf(p->logfd, "%16llu: timer2 postscaler overflow.\n", p->m_step);
#endif	/* INTLOG */
				p->pstsc2 = 0;
				p->reg.m[PIR1] |= TMR2IF;
//fprintf(p->logfd, "timer2 interrupt at %lf\n", (double)p->m_step / 1000000);
				if((p->reg.m[INTCON] & GIE)
				    && (p->reg.m[PIE1] & TMR2IE)) {
#ifdef INTLOG
fprintf(p->logfd, "%16llu: timer2 interrupt generated.\n", p->m_step);
#endif	/* INTLOG */
#ifdef MEASUREMENT
					p->m_int2++;
#endif /* MEASUREMENT */
					if(p->intr == 0) {
						p->intr = 1;
						PUSH(p, p->pc);
						SETPC(p, INTVEC);
						CLRGIE(p);//fprintf(p->logfd, "[%04x]\tINTCON: %02x\n", p->pc, p->reg.m[INTCON]);
					}
				}
			}
		}
	}
skip:
//fprintf(p->logfd, "interrupt flags: TMR0IF=%d, TMR1IF=%d, TMR2IF=%d\n", (p->reg.bank3[INTCON] & T0IF) / T0IF, (p->reg.m[PIR1] & TMR1IF) / TMR1IF, (p->reg.m[PIR1] & TMR2IF) / TMR2IF);
	return 0;
}

static int
pic16f648ExecOp(pic16f648_t *p)
{
	uint16_t op = p->memory[p->pc];

#if 0
	DEBUGMSG(p, "%04x: %04x", p->pc, op);
#endif
	switch((op & 0x3000) >> 12) {
	case 0x00:
		switch((op & 0x0c00) >> 10) {
		case 0x00:
			switch((op & 0x0380) >> 7) {
			case 0x00:
				switch(op & 0x007f) {
				case 0x00:
				case 0x20:
				case 0x40:
				case 0x60:
					return pic16f648OpNop(p, op);
				case 0x08:
					return pic16f648OpReturn(p, op);
				case 0x09:
					return pic16f648OpRetfie(p, op);
				case 0x63:
					return pic16f648OpSleep(p, op);
				case 0x64:
					return pic16f648OpClrwdt(p, op);
				default:
					return -1;
				}
				break;
			case 0x01:
				return pic16f648OpMovwf(p, op);
			case 0x02:
				return pic16f648OpClrw(p, op);
			case 0x03:
				return pic16f648OpClrf(p, op);
			case 0x04:
			case 0x05:
				return pic16f648OpSubwf(p, op);
			case 0x06:
			case 0x07:
				return pic16f648OpDecf(p, op);
			default:
				return -1;
			}
			break;
		case 0x01:
			switch((op & 0x0300) >> 8) {
			case 0x00:
				return pic16f648OpIorwf(p, op);
			case 0x01:
				return pic16f648OpAndwf(p, op);
			case 0x02:
				return pic16f648OpXorwf(p, op);
			case 0x03:
				return pic16f648OpAddwf(p, op);
			default:
				return -1;
			}
			break;
		case 0x02:
			switch((op & 0x0300) >> 8) {
			case 0x00:
				return pic16f648OpMovf(p, op);
			case 0x01:
				return pic16f648OpComf(p, op);
			case 0x02:
				return pic16f648OpIncf(p, op);
			case 0x03:
				return pic16f648OpDecfsz(p, op);
			default:
				return -1;
			}
			break;
		case 0x03:
			switch((op & 0x0300) >> 8) {
			case 0x00:
				return pic16f648OpRrf(p, op);
			case 0x01:
				return pic16f648OpRlf(p, op);
			case 0x02:
				return pic16f648OpSwapf(p, op);
			case 0x03:
				return pic16f648OpIncfsz(p, op);
			default:
				return -1;
			}
			break;
		default:
			return -1;
		}
		break;
	case 0x01:
		switch((op & 0x0c00) >> 10) {
		case 0x00:
			return pic16f648OpBcf(p, op);
		case 0x01:
			return pic16f648OpBsf(p, op);
		case 0x02:
			return pic16f648OpBtfsc(p, op);
		case 0x03:
			return pic16f648OpBtfss(p, op);
		default:
			return -1;
		}
		break;
	case 0x02:
		switch((op & 0x0800) >> 11) {
		case 0x00:
			return pic16f648OpCall(p, op);
		case 0x01:
			return pic16f648OpGoto(p, op);
		default:
			return -1;
		}
		break;
	case 0x03:
		switch((op & 0x0c00) >> 10) {
		case 0x00:
			return pic16f648OpMovlw(p, op);
		case 0x01:
			return pic16f648OpRetlw(p, op);
		case 0x02:
			switch((op & 0x0300) >> 8) {
			case 0x00:
				return pic16f648OpIorlw(p, op);
			case 0x01:
				return pic16f648OpAndlw(p, op);
			case 0x02:
				return pic16f648OpXorlw(p, op);
			default:
				return -1;
			}
			break;
		case 0x03:
			switch((op & 0x0200) >> 9) {
			case 0x00:
				return pic16f648OpSublw(p, op);
			case 0x01:
				return pic16f648OpAddlw(p, op);
			default:
				return -1;
			}
			break;
		default:
			return -1;
		}
		break;
	default:
		return -1;
	}
	return -1;
}

static void *
pic16f648Emul(void *arg)
{
	pic16f648_t *p = arg;
#ifndef BESTEFFORT
	uint64_t current;
#endif

#ifdef MEASUREMENT
	p->m_freq = getCpuFreq();
	rdtsc(p->m_sttm);
	p->m_int0 = 0;
	p->m_int2 = 0;
#endif /* MEASUREMENT */
	DEBUGMSG(p);
	if(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0) {
		WARNINGMSG(p, "failed to set cancel state\n");
		pthread_exit(NULL);
	}
	if(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL)
	    != 0) {
		WARNINGMSG(p, "failed to set cancel type\n");
		pthread_exit(NULL);
	}
#ifdef ACTIVETAG
	srandomdev();
#endif /* ACTIVETAG */
	while(p->running == 0)
		sched_yield();
#ifndef BESTEFFORT
	rdtsc(current);
	p->next = current + p->tick;
#endif
	for(;;) {
//if(p->m_step % 4000000 == 0)pic16f648ShowInfo(p);
//printf("timer1: T1CON = %d, presc = %d, TMR1 = %d\n", p->reg.m[T1CON], p->presc1, p->reg.m[TMR1H] * 256 + p->reg.m[TMR1L]);
//printf("timer2: T2CON = %02x, presc = %d, pstsc = %d, TMR2 = %d\n", p->reg.m[T2CON], p->presc2, p->pstsc2, p->reg.m[TMR2]);
#ifdef ACTIVETAG
//if(p->pc == 0x00a5){printf("0043: %02x %02x %02x %02x %02x %02x %02x %02x\n", p->reg.m[0x0043], p->reg.m[0x0044], p->reg.m[0x0045], p->reg.m[0x0046], p->reg.m[0x0047], p->reg.m[0x0048], p->reg.m[0x0049], p->reg.m[0x004a]);}
//if(p->pc == 0x00a5){printf("0070: %02x %02x %02x %02x %02x %02x %02x %02x\n", p->reg.m[0x0070], p->reg.m[0x0071], p->reg.m[0x0072], p->reg.m[0x0073], p->reg.m[0x0074], p->reg.m[0x0075], p->reg.m[0x0076], p->reg.m[0x0077]);}
//if(p->pc == 0x00be){printf("0043: %02x %02x %02x %02x %02x %02x %02x %02x\n", p->reg.m[0x0043], p->reg.m[0x0044], p->reg.m[0x0045], p->reg.m[0x0046], p->reg.m[0x0047], p->reg.m[0x0048], p->reg.m[0x0049], p->reg.m[0x004a]);}
//if(p->pc == 0x00be){printf("0070: %02x %02x %02x %02x %02x %02x %02x %02x\n", p->reg.m[0x0070], p->reg.m[0x0071], p->reg.m[0x0072], p->reg.m[0x0073], p->reg.m[0x0074], p->reg.m[0x0075], p->reg.m[0x0076], p->reg.m[0x0077]);}
//if((p->pc >= 0x00be) && (p->pc == 0x0104)){printf("parity: %02x\n", p->reg.m[0x0039]);}
//if(p->pc == 0x0107){printf("parity matched! keep processing ...\n");}
//if(p->pc == 0x04c9){printf("RCVRFDATA_M\n");}
//if(p->pc == 0x0481){fprintf(p->logfd, "recieved packet has slot number %d\n", p->reg.m[0x0044]);}

if(p->pc == 0x03be){p->reg.m[TXSTA] |= 0x02; /*fprintf(p->logfd, "TXSTA = %02x\n", p->reg.m[0x0098]);*/}
//if(p->pc == 0x01b5){p->reg.m[0x0061] = (int)((float)random() / RAND_MAX * 8) + 2;}
//if(p->pc == 0x01b5){p->reg.m[0x0061] = (int)((float)((random() & 0xffff0000) >> 16) / 65536 * 9) + 2;}
if(p->pc == 0x01b5){p->reg.m[0x0061] = ((random() & 0xffff0000) >> 16) % 9 + 2;}

//if(p->pc == 0x04c9){fprintf(p->logfd, "04c9:	RCVRFDATA_M, STFLAG = %02x\n", p->reg.m[0x0063]);}
//if(p->pc == 0x0119){fprintf(p->logfd, "%16.8f: CHECK_NG_END\n", (double)p->m_step / 1000000);}
#endif /* ACTIVETAG */
		if(pic16f648ExecOp(p) != 0) {
			ERROR(p, "fatal error\nabort\n");
			return NULL;
		}
		p->extracycle++;
		do {
			if(pic16f648ProcInt(p) != 0) {
				ERROR(p, "fatal error\nabort\n");
				return NULL;
			}
			if(pic16f648Wait(p) != 0) {
#if 0
				WARNINGMSG(p, "deadline missed!\n");
#endif
			}
			p->extracycle--;
		} while(p->extracycle > 0);
	}
	return NULL;
}

pic16f648_t *
pic16f648Alloc(uint32_t Hz)
{
	uint64_t freq;
	pic16f648_t *p;

	if((p = (pic16f648_t *)malloc(sizeof(pic16f648_t))) == NULL) {
		perror("malloc()");
		return NULL;
	}
	bzero(p, sizeof(pic16f648_t));
	p->reg.bank0[STATUS] = p->reg.bank1[STATUS] = 
	    p->reg.bank2[STATUS] = p->reg.bank3[STATUS] = 0x18;
	p->reg.bank1[OPTREG] = p->reg.bank3[OPTREG] = 0xff;
	p->reg.m[TRISA] = 0xff;
	p->reg.m[TRISB] = 0xff;
	p->reg.m[PCON] = 0x08;
	p->reg.m[PR2] = 0xff;
	p->reg.m[TXSTA] = 0x02;
#ifdef ACTIVETAG
	p->rcvscnt = -1;
	p->rcvstat = NODATA;
	p->sndstat = NODATA;
#endif /* ACTIVETAG */
	p->logfd = stderr;
	DEBUGMSG(p);
	SETPC(p, RSTVEC);
	if(pthread_mutex_init(&p->lock, NULL) != 0) {
		ERROR(p, "failed to initialize mutex\n");
		return NULL;
	}
	if((freq = getCpuFreq()) == 0) {
		WARNINGMSG(p, "cannot obtain processor clock");
		exit(1);
	}
	p->tick = freq / Hz * 4;
	DEBUGMSG(p, "tick = %d", p->tick);
	if(pthread_create(&p->thread, NULL, pic16f648Emul, p) != 0)
		return NULL;
	return p;
}

int
pic16f648Start(pic16f648_t *p)
{
	time_t ct;

	DEBUGMSG(p);
	if((p->loaded == 0) || (p->running != 0))
		return -1;
#ifdef ACTIVETAG /* REMOVE */
	ct = time(NULL);
fprintf(p->logfd, "start time: 0x%08x\n", ct);
	p->reg.m[0x0064] = p->reg.m[0x0065] = 0x00;
#endif /* ACTIVETAG */
	p->running = 1;
	return 0;
}

static uint8_t
hexStrToByte(char *str)
{
	char byte[3];
	uint8_t val;

	strncpy(byte, str, 2);
	byte[2] = '\0';
	val = strtoul(byte, NULL, 16);
	return val;
}

static uint16_t
hexStrToWord(char *str)
{
	char word[3];
	uint16_t val;

	strncpy(word, str, 4);
	word[4] = '\0';
	val = strtoul(word, NULL, 16);
	return val;
}

int
pic16f648LoadHex(pic16f648_t *p, FILE *fd)
{
	char buf[BUFSIZ];
	int i, end;
	uint8_t h, l, len, tag, sum, tmp;
	uint16_t off;

	DEBUGMSG(p);
	end = 0;
	while(fgets(buf, BUFSIZ, fd) != NULL) {
		if(*buf != ':') {
			ERROR(p, "unknown hex format\n%s", buf);
			return -1;
		}
		len = hexStrToByte(buf + 1);
		off = hexStrToWord(buf + 3);
		tag = hexStrToByte(buf + 7);
		sum = len + ((off >> 8) & 0xff) + (off & 0xff) + tag;
		switch(tag) {
		case 0x00:
			for(i = 0; i < len / 2; i++) {
				h = hexStrToByte(buf + 9 + i * 4);
				l = hexStrToByte(buf + 9 + i * 4 + 2);
				sum += h + l;
				p->memory[off / 2 + i] = l * 0x100 + h;
			}
			tmp = hexStrToByte(buf + 9 + len * 2);
			sum += tmp;
			if(sum != 0x00) {
				ERROR(p, "checksum error\n%s", buf);
				goto out;
			}
			break;
		case 0x01:
			end = 1;
			break;
		case 0x04:
			break;
		default:
			ERROR(p, "invalid record\n%s", buf);
			goto out;
		}
	}
out:
	if(end != 1) {
		ERROR(p, "end directive missing");
		return -1;
	}
#ifdef ACTIVETAG /* REMOVE */
//	p->memory[0x0105] = 0x1838;
	p->memory[0x049a] = 0x0000;
#endif /* ACTIVETAG */
	p->loaded = 1;
	return 0;
}

void
pic16f648Release(pic16f648_t *p)
{

	DEBUGMSG(p);
	if(pthread_join(p->thread, NULL) != 0)
		ERROR(p, "failed to join thread\n");
	if(pthread_mutex_destroy(&p->lock) != 0)
		ERROR(p, "failed to release mutex\n");
	free(p);
}

