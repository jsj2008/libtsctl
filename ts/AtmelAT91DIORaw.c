#ifndef AtmelAT91DIORaw_c
#define AtmelAT91DIORaw_c
#include "AtmelAT91DIORaw.h"

#define I(x) ( Num<0 ? ~(x) : (x))

#define LOCK_TYPE_SHARED_DEFERRED
#define LOCK_CLASS AtmelAT91DIORaw
#include "TSLock.h"

static DIORawAPI AtmelAT91DIORawAPI = {
  .Init = (void *)AtmelAT91DIORawInit,
  .Fini = (void *)AtmelAT91DIORawFini,
  .Lock = (void *)AtmelAT91DIORawLock,
  .Unlock = (void *)AtmelAT91DIORawUnlock,
  .Preempt = (void *)AtmelAT91DIORawPreempt,
  .DirSet = (void *)AtmelAT91DIORawDirSet,
  .DataSet = (void *)AtmelAT91DIORawDataSet,
  .DirGet = (void *)AtmelAT91DIORawDirGet,
  .DataGet = (void *)AtmelAT91DIORawDataGet,
  .Count = (void *)AtmelAT91DIORawCount
};

void *AtmelAT91DIORawInit(AtmelAT91DIORaw *dio,void *bus1) {
  Bus *bus = bus1;

  if (dio->InitStatus > 0) return dio;

  dio->Init = (void *)AtmelAT91DIORawInit;
  dio->Fini = (void *)AtmelAT91DIORawFini;
  dio->Lock = (void *)AtmelAT91DIORawLock;
  dio->Unlock = (void *)AtmelAT91DIORawUnlock;
  dio->Preempt = (void *)AtmelAT91DIORawPreempt;
  dio->DirSet = (void *)AtmelAT91DIORawDirSet;
  dio->DataSet = (void *)AtmelAT91DIORawDataSet;
  dio->DirGet = (void *)AtmelAT91DIORawDirGet;
  dio->DataGet = (void *)AtmelAT91DIORawDataGet;
  dio->Count = (void *)AtmelAT91DIORawCount;

  AtmelAT91DIORawLockInit(dio);
  dio->sub = bus;
  if (dio->sub->InitStatus <= 0) {
    dio->InitStatus = -1;
    return dio;
  }
  dio->InitStatus = 1;
  return dio;
}

void AtmelAT91DIORawFini(AtmelAT91DIORaw *dio) {
  dio->sub->Fini(dio->sub);
  if (dio->InitStatus > 0) dio->InitStatus = 0;
}

void AtmelAT91DIORawDirSet(AtmelAT91DIORaw *dio,int Num,int asOutput) {
  if (I(Num) > 31) return;
  AtmelAT91DIORawLockReal(dio,0);
  dio->sub->BitSet32(dio->sub,I(0),I(Num));
  dio->sub->BitSet32(dio->sub,asOutput ? I(0x10) : I(0x14),I(Num));
}

void AtmelAT91DIORawDataSet(AtmelAT91DIORaw *dio,int Num,int asHigh) {
  if (I(Num) > 31) return;
  AtmelAT91DIORawLockReal(dio,0);
  dio->sub->BitSet32(dio->sub,asHigh?I(0x30):I(0x34),I(Num));
}

int AtmelAT91DIORawDirGet(AtmelAT91DIORaw *dio,int Num) {
  if (I(Num) > 31) return;
  AtmelAT91DIORawLockReal(dio,SHARED);
  return dio->sub->BitGet32(dio->sub,I(0x18),I(Num));
}

int AtmelAT91DIORawDataGet(AtmelAT91DIORaw *dio,int Num) {
  if (I(Num) > 31) return;
  AtmelAT91DIORawLockReal(dio,SHARED);
  return dio->sub->BitGet32(dio->sub,I(0x3C),I(Num));
}

unsigned AtmelAT91DIORawCount(AtmelAT91DIORaw *dio) {
  return 32;
}

// Author: Michael Schmidt (michael@embeddedARM.com)
// Copyright (c) 2011, Technologic Systems, All Rights Reserved
// Refer to the COPYRIGHT file provided with this project for licensing terms.
#endif