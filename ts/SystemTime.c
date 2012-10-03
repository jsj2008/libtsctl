#ifndef SystemTime_c
#define SystemTime_c
#include <sys/time.h>
#include "SystemTime.h"

static TimeAPI SystemTimeAPI = {
  .Init = (void *)SystemTimeInit,
  .Fini = (void *)SystemTimeFini,
  .Wait = (void *)SystemTimeWait,
  .Delay = (void *)SystemTimeDelay,
  .Tick = (void *)SystemTimeTick,
  .usElapsed = (void *)SystemTimeUSElapsed,
  .usFuture = (void *)SystemTimeUSFuture,
  .TimeoutQ = (void *)SystemTimeTimeoutQ,
  .TPS = (void *)SystemTimeTPS
};

static inline unsigned long long TimeDifference(struct timeval t0,struct timeval t1) {
  return (t1.tv_usec - t0.tv_usec) + 1000000 * (t1.tv_sec - t0.tv_sec);
}

unsigned SystemMicroSecondWait(void *time0,unsigned us) {
  unsigned start,t1=0,t2,end;
  Time *time = time0;

  // system wait
  if (us >= 10000) {
    struct timeval start,end;

    gettimeofday(&start,0);
    usleep(us - us % 10000);
    gettimeofday(&end,0);
    t1 = TimeDifference(start,end);
    if (t1 > us) return t1;
    us -= t1;
  }
  start = time->Tick(time);
  end = time->usFuture(time,start,us);
  while (!time->TimeoutQ(time,start,end));
  //while ((t2=time->usElapsed(time,start)) < us);
  t2=time->usElapsed(time,start);
  return t1+t2;
}

void *SystemTimeInit(SystemTime *me,...) {
  if (me->InitStatus > 0) return me;

  me->Init = (void *)SystemTimeInit;
  me->Fini = (void *)SystemTimeFini;
  me->Wait = (void *)SystemTimeWait;
  me->Delay = (void *)SystemTimeDelay;
  me->Tick = (void *)SystemTimeTick;
  me->usElapsed = (void *)SystemTimeUSElapsed;
  me->usFuture = (void *)SystemTimeUSFuture;
  me->TimeoutQ = (void *)SystemTimeTimeoutQ;
  me->TPS = (void *)SystemTimeTPS;

  me->InitStatus = 1;
  return me;
}

void SystemTimeFini(SystemTime *me) {
  if (me->InitStatus > 0) me->InitStatus = 0;
}

unsigned SystemTimeWait(SystemTime *me,unsigned microseconds) {
  return SystemMicroSecondWait(me,microseconds);
}

void SystemTimeDelay(SystemTime *me,unsigned microseconds) {
  SystemMicroSecondWait(me,microseconds);
}

unsigned SystemTimeTick(SystemTime *me) {
  struct timeval tv;
  gettimeofday(&tv,0);
  return (unsigned)((unsigned long long)tv.tv_sec * 1000000 + tv.tv_usec);
}

unsigned SystemTimeUSElapsed(SystemTime *time,unsigned start) {
  unsigned ret,now = SystemTimeTick(time);
  if (now < start) {
    ret = ((unsigned)(-start)+now);
    if (ret > 1000000000) {
      //fprintf(stderr,"debug: %u %u %u %u\n",ret,now,start,start-now);
    }
    return ret;
  } else {
    return now - start;
  }
}

unsigned SystemTimeUSFuture(SystemTime *time,unsigned start,unsigned us) {
  return start + us;
}

int SystemTimeTimeoutQ(SystemTime *time,unsigned start,unsigned end) {
  unsigned now = SystemTimeTick(time);
  if (end > start) {
    return now >= end || now < start;
  } else if (end == start) {
    return 1;
  } else {
    return !(now >= start || now < end);
  }
}

unsigned SystemTimeTPS(SystemTime *me) {
  return 1000000;
}
// Author: Michael Schmidt (michael@embeddedARM.com)
// Copyright (c) 2011, Technologic Systems, All Rights Reserved
// Refer to the COPYRIGHT file provided with this project for licensing terms.
#endif