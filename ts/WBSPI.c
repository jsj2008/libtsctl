#ifndef WBSPI_c
#define WBSPI_c
#include "WBSPI.h"
#include "Thread.h"
#include "Array.h"

void *WBSPIInit(WBSPI *spi,void *bus1,void *bus8,int offset,
		void (*cs)(WBSPI *,unsigned,int)) {
  Bus *bus = bus1;

  if (spi->InitStatus > 0) return spi;
  spi->Init = (void *)WBSPIInit;
  spi->Fini = (void *)WBSPIFini;
  spi->Lock = (void *)WBSPILock;
  spi->Unlock = (void *)WBSPIUnlock;
  spi->Preempt = (void *)WBSPIPreempt;
  spi->Write = (void *)WBSPIWrite;
  spi->Read = (void *)WBSPIRead;
  spi->ReadWrite = (void *)WBSPIReadWrite;
  spi->ClockSet = (void *)WBSPIClockSet;
  spi->EdgeSet = (void *)WBSPIEdgeSet;

  spi->LockNum = ThreadMutexAllocate(1);
  spi->bus = bus;
  spi->bus8 = bus;
  spi->offset = offset;
  spi->ChipSelect = cs;

  if (spi->bus->InitStatus <= 0
      || spi->bus8->InitStatus <= 0) {
    spi->Fini(spi);
    spi->InitStatus = -1;
    return spi;
  }

  spi->InitStatus = 1;
  return spi;
}

void WBSPIFini(WBSPI *spi) {
  spi->bus->Fini(spi->bus);
  if (spi->InitStatus > 0) spi->InitStatus = 0;
}

int WBSPILock(WBSPI *spi,unsigned num,int flags) {
  return ThreadMutexLock(spi->LockNum,flags);
}

int WBSPIUnlock(WBSPI *spi,unsigned num,int flags) {
  return ThreadMutexUnlock(spi->LockNum);
}

void WBSPIPreempt(WBSPI *spi) {
  /*
    WBSPIUnlock(spi,0,0);
    sched_yield();
    WBSPILock(spi,0,0);
  */
}


/*
0x40 is this reg:
* base + 0x0: LUN register (R/W)
*   bit 15: spi miso state (RO)
*   bit 14: spi clk state (RW)
*   bits 13-10: speed[3:0] (0- highest, 1- 1/2 speed, 2- 1/4, etc..) (RW)
*   bits 9-8: LUN (0-3 representing the 4 CS# signals) (RW)
*   bit 7: CS (1 - CS# is asserted) (RW)
*   bits 6-1: reserved
*   bit 0: speed[4] (RW)
 */

int WBSPIWrite(WBSPI *ob,int adrs,const char* buf) {
  int n = ArrayLength(buf),reg;
  int de_cs = 1;
  int maxspeed;

  if (adrs < 0) {
    adrs = -adrs;
    de_cs = 0;
  }
  adrs--;
  ob->ChipSelect(ob,adrs,1);
  ob->bus->Lock(ob->bus,0,0); 
  reg = ob->bus->Peek16(ob->bus,ob->offset);
  maxspeed = ((reg >> 10) & 0xF) == 0 && ((reg & 1) == 0);
  if (n > 16 && maxspeed) {
    if (n % 2 == 0) {
      //ob->bus->PokeStream(ob->bus,ob->offset+0xA,1,buf,(n-2)/2);
      n = 2;
    } else {
      //ob->bus->PokeStream(ob->bus,ob->offset+0xA,1,buf,(n-3)/2);
      n = 3;
    }
  }
  while (n >= 4) {
    //ob->bus->Poke16(ob->bus,ob->offset+8,buf[1]+(buf[0]<<8));
    ob->bus->Poke8(ob->bus,ob->offset+8,buf[0]);
    ob->bus->Poke8(ob->bus,ob->offset+8,buf[1]);
    buf += 2;
    n -= 2;
  }
  if (n > 2) { // n == 3
    //ob->bus->Poke16(ob->bus,ob->offset+8,buf[1]+(buf[0]<<8));
    ob->bus->Poke8(ob->bus,ob->offset+8,buf[0]);
    ob->bus->Poke8(ob->bus,ob->offset+8,buf[1]);
    buf += 2;
    n -= 2;
    ob->bus->Poke8(ob->bus,ob->offset+(de_cs?0xC:8),buf[0]);
  } else if (n == 2) {
    //ob->bus->Poke16(ob->bus,ob->offset+(de_cs?0xC:8),buf[1]+(buf[0]<<8));
    ob->bus->Poke8(ob->bus,ob->offset+(de_cs?0x8:8),buf[0]);
    ob->bus->Poke8(ob->bus,ob->offset+(de_cs?0xC:8),buf[1]);
    buf += 2;
    n -= 2;
  } else if (n == 1) {
    ob->bus8->Poke8(ob->bus,ob->offset+(de_cs?0xC:8),buf[0]);
  }
  ob->bus->Unlock(ob->bus,0,0);
  ob->ChipSelect(ob,adrs,0);
  return 1;
}

int WBSPIRead(WBSPI *ob,int adrs,char* buf) {
  unsigned s;
  int i,n = ArrayLength(buf),reg;
  int de_cs = 1, maxspeed;

  if (adrs < 0) {
    adrs = -adrs;
    de_cs = 0;
  }
  adrs--;
  //ob->ChipSelect(ob,adrs,1);
  ob->bus->Lock(ob->bus,0,0);
  reg = ob->bus->Peek16(ob->bus,ob->offset);
  maxspeed = ((reg >> 10) & 0xF) == 0 && ((reg & 1) == 0);
  if (n > 16 && maxspeed) {
    if (n % 2 == 0) {
      //ob->bus->PeekStream(ob->bus,ob->offset+0xA,1,buf,(n-2)/2);
      for (i=0;i<n-2;i+=2) { // swap byte order
	unsigned char b;

	b = buf[i];
	buf[i] = buf[i+1];
	buf[i+1] = b;
      }
      buf += (n-2);
      n = 2;
    } else {
      //ob->bus->PeekStream(ob->bus,ob->offset+0xA,1,buf,(n-3)/2);
      for (i=0;i<n-2;i+=2) { // swap byte order
	unsigned char b;

	b = buf[i];
	buf[i] = buf[i+1];
	buf[i+1] = b;
      }
      buf += (n-3);
      n = 3;
    }
  }
  while (n >= 4) {
    //s = ob->bus->Peek16(ob->bus,ob->offset+0xA); // 0xA pipelined read
    //*buf++ = s >> 8;
    //*buf++ = s & 0xff;
    *buf++ = ob->bus->Peek8(ob->bus,ob->offset+0x8); // 0xA pipelined read
    //ob->bus->Peek16(ob->bus,ob->offset); // TEMP
    *buf++ = ob->bus->Peek8(ob->bus,ob->offset+0x8); // 0xA pipelined read
    //ob->bus->Peek16(ob->bus,ob->offset); // TEMP
    n -= 2;
  }
  if (n > 2) { // n == 3
    //s = ob->bus->Peek16(ob->bus,ob->offset+8); // read, leave CS# asserted
    //*buf++ = s >> 8;
    //*buf++ = s & 0xff;
    *buf++ = ob->bus->Peek8(ob->bus,ob->offset+8); // read, leave CS# asserted
    *buf++ = ob->bus->Peek8(ob->bus,ob->offset+8); // read, leave CS# asserted
    n -= 2;
    ob->bus8->Poke8(ob->bus,ob->offset+(de_cs?0xC:8),0);
    *buf = ob->bus->Peek8(ob->bus,ob->offset+2) >> 8;
  } else if (n == 2) {
    //s = ob->bus->Peek16(ob->bus,ob->offset+(de_cs?0xC:8));
    //*buf++ = s >> 8;
    //*buf++ = s & 0xff;
    *buf++ = ob->bus->Peek8(ob->bus,ob->offset+(de_cs?0x8:8));
    *buf++ = ob->bus->Peek8(ob->bus,ob->offset+(de_cs?0xC:8));
    n -= 2;
  } else if (n == 1) {
    //ob->bus8->Poke8(ob->bus,ob->offset+(de_cs?0xC:8),0);
    //*buf = ob->bus->Peek8(ob->bus,ob->offset+2) >> 8;
    *buf = ob->bus8->Peek8(ob->bus,ob->offset+(de_cs?0xC:8));
  }  
  ob->bus->Peek16(ob->bus,ob->offset); // TEMP
  ob->bus->Unlock(ob->bus,0,0); 
  ob->ChipSelect(ob,adrs,0);
  return 1;
}

int WBSPIReadWrite(WBSPI *ob,int adrs,unsigned char* wbuf,unsigned char* rbuf) {
  unsigned s;
  int n, de_cs=1;

  if (ArrayLength(wbuf) > ArrayLength(rbuf)) {
    n = ArrayLength(wbuf);
  } else {
    n = ArrayLength(rbuf);
  }

  if (adrs < 0) {
    adrs = -adrs;
    de_cs = 0;
  }
  adrs--;
  ob->ChipSelect(ob,adrs,1);
  ob->bus->Lock(ob->bus,0,0);
  while (n >= 4) {
    //ob->bus->Poke16(ob->bus,ob->offset+8,wbuf[1]+(wbuf[0]<<8));
    ob->bus->Poke8(ob->bus,ob->offset+8,wbuf[0]);
    ob->bus->Poke8(ob->bus,ob->offset+8,wbuf[1]);
    //log9(LOG_SPI,"%4: 02X->[%02X]\n",wbuf[1]+(wbuf[0]<<8),ob->offset+8);
    wbuf += 2;
    //s = ob->bus->Peek16(ob->bus,ob->offset+2);
    //*rbuf++ = s >> 8;
    //*rbuf++ = s & 0xff;
    *rbuf++ = ob->bus->Peek8(ob->bus,ob->offset+2);
    *rbuf++ = ob->bus->Peek8(ob->bus,ob->offset+2);
    n -= 2;
  }
  if (n > 2) { // n == 3
    //ob->bus->Poke16(ob->bus,ob->offset+8,wbuf[1]+(wbuf[0]<<8));
    ob->bus->Poke8(ob->bus,ob->offset+8,wbuf[0]);
    ob->bus->Poke8(ob->bus,ob->offset+8,wbuf[1]);
    //log9(LOG_SPI,"3a: %02X->[%02X]\n",wbuf[1]+(wbuf[0]<<8),ob->offset+8);
    //s = ob->bus->Peek16(ob->bus,ob->offset+2);
    *rbuf++ = ob->bus->Peek8(ob->bus,ob->offset+2);
    *rbuf++ = ob->bus->Peek8(ob->bus,ob->offset+2);
    //s=0;
    wbuf += 2;
    //*rbuf++ = s >> 8;
    //*rbuf++ = s & 0xff;
    n -= 2;
    ob->bus8->Poke8(ob->bus,ob->offset+(de_cs?0xC:8),wbuf[0]);
    //log9(LOG_SPI,"3b: %02X->[%02X]\n",wbuf[1]+(wbuf[0]<<8),ob->offset+(de_cs?0xC:8));
    *rbuf=0;
    *rbuf = ob->bus8->Peek8(ob->bus,ob->offset+2);
  } else if (n == 2) {
    //ob->bus->Poke16(ob->bus,ob->offset+(de_cs?0xC:8),wbuf[1]+(wbuf[0]<<8));
    ob->bus->Poke8(ob->bus,ob->offset+(de_cs?0x8:8),wbuf[0]);
    ob->bus->Poke8(ob->bus,ob->offset+(de_cs?0xC:8),wbuf[1]);
    //log9(LOG_SPI,"2: %02X->[%02X]\n",wbuf[1]+(wbuf[0]<<8),ob->offset+(de_cs?0xC:8));
    *rbuf++ = ob->bus->Peek8(ob->bus,ob->offset+2);
    *rbuf++ = ob->bus->Peek8(ob->bus,ob->offset+3);
    //s = ob->bus->Peek16(ob->bus,ob->offset+2);
    //*rbuf++ = s >> 8;
    //*rbuf++ = s & 0xff;
    n -= 2;
  } else if (n == 1) {
    ob->bus8->Poke8(ob->bus,ob->offset+(de_cs?0xC:8),wbuf[0]);
    //log9(LOG_SPI,"1: %02X->[%02X]\n",wbuf[1]+(wbuf[0]<<8),ob->offset+(de_cs?0xC:8));
    *rbuf = ob->bus8->Peek8(ob->bus,ob->offset+2);
  }
  ob->bus->Unlock(ob->bus,0,0);
  ob->ChipSelect(ob,adrs,0);
  return 1;
}

static int spi_f[] = {
  75000000,37500000,18750000,12500000,9375000,
  7500000,6250000,5360000,4680000,4170000,
  3750000,3409090,3125000,2884615,2678571,
  2500000,2343750,2205882,2083333,1973684,
  1875000,1785714,1704545,1630434,1562500,
  1500000,1442307,1388888,1339285,1293103,
  1250000,0 // 1209677
};

int WBSPIClockSet(WBSPI *ob,unsigned hz) {
  short reg,reg2;
  int i;
  if (hz == 0) return;
  for (i=0;spi_f[i]>hz;i++);
  ob->bus->Lock(ob->bus,0,0);
  reg = reg2 = ob->bus->Peek16(ob->bus,ob->offset);
  reg2 &= ~(15<<10); // mask out existing speed[3:0] bits
  reg2 |= (i & 15) << 10; // or in new speed[3:0] bits
  reg2 &= ~(1 << 0); // mask out existing speed[4] bit
  reg2 |= ((i >> 4) & 1); // or in new speed[4] bit
  if (reg2 != reg) {
    ob->bus->Poke16(ob->bus,ob->offset,reg2);
  }
  ob->bus->Unlock(ob->bus,0,0);
  return 1;
}

int WBSPIEdgeSet(WBSPI *ob,int posedge) {
  short reg,reg2;
  int maxspeed;

  ob->bus->Lock(ob->bus,0,0);
  reg = reg2 = ob->bus->Peek16(ob->bus,ob->offset);
  maxspeed = ((reg >> 10) & 0xF) == 0 && ((reg & 1) == 0);
  if (maxspeed && posedge != 0) {
    return -5; // unsupported
  }
  reg &= ~(1<<14); // mask out existing clock polarity bit
  reg |= (((posedge?1:0)||maxspeed)?0:1) << 14; // or in new clock polarity bit
  //log9(LOG_SPI,"regs:%X,%X\n",reg,reg2);
  if (reg2 != reg) {
    ob->bus->Poke16(ob->bus,ob->offset,reg);
  }
  ob->bus->Unlock(ob->bus,0,0);
  return 1;
}


// Author: Michael Schmidt (michael@embeddedARM.com)
// Copyright (c) 2011, Technologic Systems, All Rights Reserved
// Refer to the COPYRIGHT file provided with this project for licensing terms.
#endif
