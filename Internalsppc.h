// Copyright (c) 2019 Dennis van der Boon
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

void illegal(void) = "\t.long\t0\n";            //debug function

#define PPCFUNCTION __section ("functions","acrx") __entry

void    Reset  (void);
ULONG   ReadPVR(void);
ULONG   SetIdle(void);

ULONG getLeadZ(ULONG value)               = "\tcntlzw\tr3,r3\n";
ULONG getPVR(void)                        = "\tmfpvr\tr3\n";
ULONG getSDR1(void)                       = "\tmfsdr1\tr3\n";
void  setSDR1(ULONG value)                = "\tmtsdr1\tr3\n";
void  setSRIn(ULONG keyVal, ULONG segVal) = "\tmtsrin\tr3,r4\n";
ULONG getSRIn(ULONG address)              = "\tmfsrin\tr3,r3\n";
void  setMSR(ULONG value)                 = "\tsync\n\tmtmsr\tr3\n\tsync\n";
void  tlbSync(void)                       = "\ttlbsync\n";
void  tlbIe(ULONG value)                  = "\ttlbie\tr3\n";

void setBAT0(ULONG ibatl, ULONG ibatu, ULONG dbatl, ULONG dbatu)
        ="\tmtibatl\t0,r3\n\tmtibatu\t0,r4\n\tmtdbatl\t0,r5\n\tmtdbatu\t0,r6\n";
void setBAT1(ULONG ibatl, ULONG ibatu, ULONG dbatl, ULONG dbatu)
        ="\tmtibatl\t1,r3\n\tmtibatu\t1,r4\n\tmtdbatl\t1,r5\n\tmtdbatu\t1,r6\n";
void setBAT2(ULONG ibatl, ULONG ibatu, ULONG dbatl, ULONG dbatu)
        ="\tmtibatl\t2,r3\n\tmtibatu\t2,r4\n\tmtdbatl\t2,r5\n\tmtdbatu\t2,r6\n";
void setBAT3(ULONG ibatl, ULONG ibatu, ULONG dbatl, ULONG dbatu)
        ="\tmtibatl\t3,r3\n\tmtibatu\t3,r4\n\tmtdbatl\t3,r5\n\tmtdbatu\t3,r6\n";

void mSync(void)                          = "\tsync\n\tisync\n";


ULONG myRun68K(struct PPCBase* PowerPCBase, struct PPCArgs* PPStruct);
ULONG myWaitFor68K(struct PPCBase* PowerPCBase, struct PPCArgs* PPStruct);
VOID mySPrintF(struct PPCBase* PowerPCBase, STRPTR Formatstring, APTR Values);
APTR myAllocVecPPC(struct PPCBase* PowerPCBase, ULONG size, ULONG flags, ULONG align);
LONG myFreeVecPPC(struct PPCBase* PowerPCBase, APTR memblock);
struct TaskPPC* myCreateTaskPPC(struct PPCBase* PowerPCBase, struct TagItem* taglist);
VOID myDeleteTaskPPC(struct PPCBase* PowerPCBase, struct TaskPPC* PPCtask);
struct TaskPPC* myFindTaskPPC(struct PPCBase* PowerPCBase, STRPTR name);
LONG myInitSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
VOID myFreeSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
VOID myAddSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
VOID myRemSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
VOID myObtainSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
LONG myAttemptSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
VOID myReleaseSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
struct SignalSemaphorePPC* myFindSemaphorePPC(struct PPCBase* PowerPCBase, STRPTR name);
VOID myInsertPPC(struct PPCBase* PowerPCBase, struct List* list, struct Node* node, struct Node* pred);
VOID myAddHeadPPC(struct PPCBase* PowerPCBase, struct List* list, struct Node* node);
VOID myAddTailPPC(struct PPCBase* PowerPCBase, struct List* list, struct Node* node);
VOID myRemovePPC(struct PPCBase* PowerPCBase, struct Node* node);
struct Node* myRemHeadPPC(struct PPCBase* PowerPCBase, struct List* list);
struct Node* myRemTailPPC(struct PPCBase* PowerPCBase, struct Node* list);
VOID myEnqueuePPC(struct PPCBase* PowerPCBase, struct List* list, struct Node* node);
struct Node* myFindNamePPC(struct PPCBase* PowerPCBase, struct List* list, STRPTR name);
struct TagItem* myFindTagItemPPC(struct PPCBase* PowerPCBase, ULONG value, struct TagItem* taglist);
ULONG myGetTagDataPPC(struct PPCBase* PowerPCBase, ULONG value, ULONG d0arg, struct TagItem* taglist);
struct TagItem* myNextTagItemPPC(struct PPCBase* PowerPCBase, struct TagItem** tagitem);
LONG myAllocSignalPPC(struct PPCBase* PowerPCBase, LONG signum);
VOID myFreeSignalPPC(struct PPCBase* PowerPCBase, LONG signum);
ULONG mySetSignalPPC(struct PPCBase* PowerPCBase, ULONG signals, ULONG mask);
VOID mySignalPPC(struct PPCBase* PowerPCBase, struct TaskPPC* task, ULONG signals);
ULONG myWaitPPC(struct PPCBase* PowerPCBase, ULONG signals);
LONG mySetTaskPriPPC(struct PPCBase* PowerPCBase, struct TaskPPC* task, LONG pri);
VOID mySignal68K(struct PPCBase* PowerPCBase, struct Task* task, ULONG signals);
VOID mySetCache(struct PPCBase* PowerPCBase, ULONG flags, APTR start, ULONG length);
APTR mySetExcHandler(struct PPCBase* PowerPCBase, struct TagItem* taglist);
VOID myRemExcHandler(struct PPCBase* PowerPCBase, APTR xlock);
ULONG mySuper(struct PPCBase* PowerPCBase);
VOID myUser(struct PPCBase* PowerPCBase, ULONG key);
ULONG mySetHardware(struct PPCBase* PowerPCBase, ULONG flags, APTR param);
VOID myModifyFPExc(struct PPCBase* PowerPCBase, ULONG fpflags);
ULONG myWaitTime(struct PPCBase* PowerPCBase, ULONG signals, ULONG time);
struct TaskPtr* myLockTaskList(struct PPCBase* PowerPCBase);
VOID myUnLockTaskList(struct PPCBase* PowerPCBase);
VOID mySetExcMMU(struct PPCBase* PowerPCBase);
VOID myClearExcMMU(struct PPCBase* PowerPCBase);
VOID myChangeMMU(struct PPCBase* PowerPCBase, ULONG mode);
VOID myGetInfo(struct PPCBase* PowerPCBase, struct TagItem* taglist);
struct MsgPortPPC* myCreateMsgPortPPC(struct PPCBase* PowerPCBase);
VOID myDeleteMsgPortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port);
VOID myAddPortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port);
VOID myRemPortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port);
struct MsgPortPPC* myFindPortPPC(struct PPCBase* PowerPCBase, STRPTR port);
struct Message* myWaitPortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port);
VOID myPutMsgPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port, struct Message* message);
struct Message* myGetMsgPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port);
VOID myReplyMsgPPC(struct PPCBase* PowerPCBase, struct Message* message);
VOID myFreeAllMem(struct PPCBase* PowerPCBase);
VOID myCopyMemPPC(struct PPCBase* PowerPCBase, APTR source, APTR dest, ULONG size);
struct Message* myAllocXMsgPPC(struct PPCBase* PowerPCBase, ULONG length, struct MsgPortPPC* port);
VOID myFreeXMsgPPC(struct PPCBase* PowerPCBase, struct Message* message);
VOID myPutXMsgPPC(struct PPCBase* PowerPCBase, struct MsgPort* port, struct Message* message);
VOID myGetSysTimePPC(struct PPCBase* PowerPCBase, struct timeval* timeval);
VOID myAddTimePPC(struct PPCBase* PowerPCBase, struct timeval* dest, struct timeval* source);
VOID mySubTimePPC(struct PPCBase* PowerPCBase, struct timeval* dest, struct timeval* source);
LONG myCmpTimePPC(struct PPCBase* PowerPCBase, struct timeval* dest, struct timeval* source);
struct MsgPortPPC* mySetReplyPortPPC(struct PPCBase* PowerPCBase, struct Message* message, struct MsgPortPPC* port);
ULONG mySnoopTask(struct PPCBase* PowerPCBase, struct TagItem* taglist);
VOID myEndSnoopTask(struct PPCBase* PowerPCBase, ULONG id);
VOID myGetHALInfo(struct PPCBase* PowerPCBase, struct TagItem* taglist);
VOID mySetScheduling(struct PPCBase* PowerPCBase, struct TagItem* taglist);
struct TaskPPC* myFindTaskByID(struct PPCBase* PowerPCBase, LONG id);
LONG mySetNiceValue(struct PPCBase* PowerPCBase, struct TaskPPC* task, LONG nice);
LONG myTrySemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, ULONG timeout);
VOID myNewListPPC(struct PPCBase* PowerPCBase, struct List* list);
ULONG mySetExceptPPC(struct PPCBase* PowerPCBase, ULONG signals, ULONG mask, ULONG flag);
VOID myObtainSemaphoreSharedPPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
LONG myAttemptSemaphoreSharedPPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
VOID myProcurePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, struct SemaphoreMessage* SemaphoreMessage);
VOID myVacatePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, struct SemaphoreMessage* SemaphoreMessage);
VOID myCauseInterrupt(struct PPCBase* PowerPCBase);
APTR myCreatePoolPPC(struct PPCBase* PowerPCBase, ULONG flags, ULONG puddle_size, ULONG trehs_size);
VOID myDeletePoolPPC(struct PPCBase* PowerPCBase, APTR poolheader);
APTR myAllocPooledPPC(struct PPCBase* PowerPCBase, APTR poolheader, ULONG size);
VOID myFreePooledPPC(struct PPCBase* PowerPCBase, APTR poolheader, APTR ptr, ULONG size);
APTR myRawDoFmtPPC(struct PPCBase* PowerPCBase, STRPTR formatstring, APTR datastream, void (*putchproc)(), APTR putchdata);
LONG myPutPublicMsgPPC(struct PPCBase* PowerPCBase, STRPTR portname, struct Message* message);
LONG myAddUniquePortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port);
LONG myAddUniqueSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
BOOL myIsExceptionMode(struct PPCBase* PowerPCBase);
VOID myAllocPrivateMem(void);
VOID myFreePrivateMem(void);
VOID myResetPPC(void);
BOOL myChangeStack(struct PPCBase* PowerPCBase, ULONG NewStackSize);
ULONG myRun68KLowLevel(struct PPCBase* PowerPCBase, ULONG Code, LONG Offset, ULONG a0, ULONG a1, ULONG d0, ULONG d1);

