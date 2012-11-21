#ifndef __TSTime_h
#define __TSTime_h
#undef FUNC
#ifndef SWIGGY
#define FUNC(x) (*x)
#else
#define FUNC(x) x
#endif

typedef struct TSTime TSTime;
struct TSTime {
	void *FUNC(Init)(TSTime *me,void *bus,int adrsMSB,int adrsLSB,int Hz);
	void FUNC(Fini)(TSTime *me);
	unsigned FUNC(Wait)(TSTime *me,unsigned microseconds);
	void FUNC(Delay)(TSTime *me,unsigned microseconds);
	unsigned FUNC(Tick)(TSTime *me);
	unsigned FUNC(usElapsed)(TSTime *me,unsigned start);
	unsigned FUNC(usFuture)(TSTime *me,unsigned start,unsigned microseconds);
	int FUNC(TimeoutQ)(TSTime *me,unsigned start,unsigned end);
	unsigned FUNC(TPS)(TSTime *me);
	int InitStatus;
	Bus *bus;
	int adrsMSB;
	int adrsLSB;
	int Hz;
};

void *TSTimeInit(TSTime* ob,void *bus,int adrsMSB,int adrsLSB,int Hz);
void TSTimeFini(TSTime* ob);
unsigned TSTimeWait(TSTime* ob,unsigned microseconds);
void TSTimeDelay(TSTime* ob,unsigned microseconds);
unsigned TSTimeTick(TSTime* ob);
unsigned TSTimeusElapsed(TSTime* ob,unsigned start);
unsigned TSTimeusFuture(TSTime* ob,unsigned start,unsigned microseconds);
int TSTimeTimeoutQ(TSTime* ob,unsigned start,unsigned end);
unsigned TSTimeTPS(TSTime* ob);
#endif

// Author: Michael Schmidt (michael@embeddedARM.com)
// Copyright (c) 2012, Technologic Systems, All Rights Reserved
// Refer to the COPYRIGHT file provided with this project for licensing terms.
