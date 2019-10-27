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

#include <exec/types.h>
#include <devices/timer.h>
#include <powerpc/powerpc.h>
#include <powerpc/tasksPPC.h>
#include <powerpc/memoryPPC.h>
#include <powerpc/semaphoresPPC.h>
#include <powerpc/portsPPC.h>
#include "Internalsppc.h"

PPCFUNCTION ULONG myRun68K(struct PPCBase* PowerPCBase, struct PPCArgs* PPStruct)
{
    return 0;
}

PPCFUNCTION ULONG myWaitFor68K(struct PPCBase* PowerPCBase, struct PPCArgs* PPStruct)
{
    return 0;
}

PPCFUNCTION VOID mySPrintF(struct PPCBase* PowerPCBase, STRPTR Formatstring, APTR Values)
{
    return;
}

PPCFUNCTION APTR myAllocVecPPC(struct PPCBase* PowerPCBase, ULONG size, ULONG flags, ULONG align)
{
    return NULL;
}

PPCFUNCTION LONG myFreeVecPPC(struct PPCBase* PowerPCBase, APTR memblock)
{
    return 0;
}

PPCFUNCTION struct TaskPPC* myCreateTaskPPC(struct PPCBase* PowerPCBase, struct TagItem* taglist)
{
    return NULL;
}

PPCFUNCTION VOID myDeleteTaskPPC(struct PPCBase* PowerPCBase, struct TaskPPC* PPCtask)
{
    return;
}

PPCFUNCTION struct TaskPPC* myFindTaskPPC(struct PPCBase* PowerPCBase, STRPTR name)
{
    return NULL;
}

PPCFUNCTION LONG myInitSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return 0;
}

PPCFUNCTION VOID myFreeSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

PPCFUNCTION VOID myAddSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

PPCFUNCTION VOID myRemSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

PPCFUNCTION VOID myObtainSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

PPCFUNCTION LONG myAttemptSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return 0;
}

PPCFUNCTION VOID myReleaseSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

PPCFUNCTION struct SignalSemaphorePPC* myFindSemaphorePPC(struct PPCBase* PowerPCBase, STRPTR name)
{
    return NULL;
}

PPCFUNCTION VOID myInsertPPC(struct PPCBase* PowerPCBase, struct List* list, struct Node* node, struct Node* pred)
{
    return;
}

PPCFUNCTION VOID myAddHeadPPC(struct PPCBase* PowerPCBase, struct List* list, struct Node* node)
{
    return;
}

PPCFUNCTION VOID myAddTailPPC(struct PPCBase* PowerPCBase, struct List* list, struct Node* node)
{
    return;
}

PPCFUNCTION VOID myRemovePPC(struct PPCBase* PowerPCBase, struct Node* node)
{
    return;
}

PPCFUNCTION struct Node* myRemHeadPPC(struct PPCBase* PowerPCBase, struct List* list)
{
    return NULL;
}

PPCFUNCTION struct Node* myRemTailPPC(struct PPCBase* PowerPCBase, struct Node* list)
{
    return NULL;
}

PPCFUNCTION VOID myEnqueuePPC(struct PPCBase* PowerPCBase, struct List* list, struct Node* node)
{
    return;
}

PPCFUNCTION struct Node* myFindNamePPC(struct PPCBase* PowerPCBase, struct List* list, STRPTR name)
{
    return NULL;
}

PPCFUNCTION struct TagItem* myFindTagItemPPC(struct PPCBase* PowerPCBase, ULONG value, struct TagItem* taglist)
{
    return NULL;
}

PPCFUNCTION ULONG myGetTagDataPPC(struct PPCBase* PowerPCBase, ULONG value, ULONG d0arg, struct TagItem* taglist)
{
    return 0;
}

PPCFUNCTION struct TagItem* myNextTagItemPPC(struct PPCBase* PowerPCBase, struct TagItem** tagitem)
{
    return NULL;
}

PPCFUNCTION LONG myAllocSignalPPC(struct PPCBase* PowerPCBase, LONG signum)
{
    return 0;
}

PPCFUNCTION VOID myFreeSignalPPC(struct PPCBase* PowerPCBase, LONG signum)
{
    return;
}

PPCFUNCTION ULONG mySetSignalPPC(struct PPCBase* PowerPCBase, ULONG signals, ULONG mask)
{
    return 0;
}

PPCFUNCTION VOID mySignalPPC(struct PPCBase* PowerPCBase, struct TaskPPC* task, ULONG signals)
{
    return;
}

PPCFUNCTION ULONG myWaitPPC(struct PPCBase* PowerPCBase, ULONG signals)
{
    return 0;
}

PPCFUNCTION LONG mySetTaskPriPPC(struct PPCBase* PowerPCBase, struct TaskPPC* task, LONG pri)
{
    return 0;
}

PPCFUNCTION VOID mySignal68K(struct PPCBase* PowerPCBase, struct Task* task, ULONG signals)
{
    return;
}

PPCFUNCTION VOID mySetCache(struct PPCBase* PowerPCBase, ULONG flags, APTR start, ULONG length)
{
    return;
}

PPCFUNCTION APTR mySetExcHandler(struct PPCBase* PowerPCBase, struct TagItem* taglist)
{
    return NULL;
}

PPCFUNCTION VOID myRemExcHandler(struct PPCBase* PowerPCBase, APTR xlock)
{
    return;
}

PPCFUNCTION ULONG mySuper(struct PPCBase* PowerPCBase)
{
    return 0;
}

PPCFUNCTION VOID myUser(struct PPCBase* PowerPCBase, ULONG key)
{
    return;
}

PPCFUNCTION ULONG mySetHardware(struct PPCBase* PowerPCBase, ULONG flags, APTR param)
{
    return 0;
}

PPCFUNCTION VOID myModifyFPExc(struct PPCBase* PowerPCBase, ULONG fpflags)
{
    return;
}

PPCFUNCTION ULONG myWaitTime(struct PPCBase* PowerPCBase, ULONG signals, ULONG time)
{
    return 0;
}

PPCFUNCTION struct TaskPtr* myLockTaskList(struct PPCBase* PowerPCBase)
{
    return NULL;
}

PPCFUNCTION VOID myUnLockTaskList(struct PPCBase* PowerPCBase)
{
    return;
}

PPCFUNCTION VOID mySetExcMMU(struct PPCBase* PowerPCBase)
{
    return;
}

PPCFUNCTION VOID myClearExcMMU(struct PPCBase* PowerPCBase)
{
    return;
}

PPCFUNCTION VOID myChangeMMU(struct PPCBase* PowerPCBase, ULONG mode)
{
    return;
}

PPCFUNCTION VOID myGetInfo(struct PPCBase* PowerPCBase, struct TagItem* taglist)
{
    return;
}

PPCFUNCTION struct MsgPortPPC* myCreateMsgPortPPC(struct PPCBase* PowerPCBase)
{
    return NULL;
}

PPCFUNCTION VOID myDeleteMsgPortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return;
}

PPCFUNCTION VOID myAddPortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return;
}

PPCFUNCTION VOID myRemPortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return;
}

PPCFUNCTION struct MsgPortPPC* myFindPortPPC(struct PPCBase* PowerPCBase, STRPTR port)
{
    return NULL;
}

PPCFUNCTION struct Message* myWaitPortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return NULL;
}

PPCFUNCTION VOID myPutMsgPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port, struct Message* message)
{
    return;
}

PPCFUNCTION struct Message* myGetMsgPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return NULL;
}

PPCFUNCTION VOID myReplyMsgPPC(struct PPCBase* PowerPCBase, struct Message* message)
{
    return;
}

PPCFUNCTION VOID myFreeAllMem(struct PPCBase* PowerPCBase)
{
    return;
}

PPCFUNCTION VOID myCopyMemPPC(struct PPCBase* PowerPCBase, APTR source, APTR dest, ULONG size)
{
    return;
}

PPCFUNCTION struct Message* myAllocXMsgPPC(struct PPCBase* PowerPCBase, ULONG length, struct MsgPortPPC* port)
{
    return NULL;
}

PPCFUNCTION VOID myFreeXMsgPPC(struct PPCBase* PowerPCBase, struct Message* message)
{
    return;
}

PPCFUNCTION VOID myPutXMsgPPC(struct PPCBase* PowerPCBase, struct MsgPort* port, struct Message* message)
{
    return;
}

PPCFUNCTION VOID myGetSysTimePPC(struct PPCBase* PowerPCBase, struct timeval* timeval)
{
    return;
}

PPCFUNCTION VOID myAddTimePPC(struct PPCBase* PowerPCBase, struct timeval* dest, struct timeval* source)
{
    return;
}

PPCFUNCTION VOID mySubTimePPC(struct PPCBase* PowerPCBase, struct timeval* dest, struct timeval* source)
{
    return;
}

PPCFUNCTION LONG myCmpTimePPC(struct PPCBase* PowerPCBase, struct timeval* dest, struct timeval* source)
{
    return 0;
}

PPCFUNCTION struct MsgPortPPC* mySetReplyPortPPC(struct PPCBase* PowerPCBase, struct Message* message, struct MsgPortPPC* port)
{
    return NULL;
}

PPCFUNCTION ULONG mySnoopTask(struct PPCBase* PowerPCBase, struct TagItem* taglist)
{
    return 0;
}

PPCFUNCTION VOID myEndSnoopTask(struct PPCBase* PowerPCBase, ULONG id)
{
    return;
}

PPCFUNCTION VOID myGetHALInfo(struct PPCBase* PowerPCBase, struct TagItem* taglist)
{
    return;
}

PPCFUNCTION VOID mySetScheduling(struct PPCBase* PowerPCBase, struct TagItem* taglist)
{
    return;
}

PPCFUNCTION struct TaskPPC* myFindTaskByID(struct PPCBase* PowerPCBase, LONG id)
{
    return NULL;
}

PPCFUNCTION LONG mySetNiceValue(struct PPCBase* PowerPCBase, struct TaskPPC* task, LONG nice)
{
    return 0;
}

PPCFUNCTION LONG myTrySemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, ULONG timeout)
{
    return 0;
}

PPCFUNCTION VOID myNewListPPC(struct PPCBase* PowerPCBase, struct List* list)
{
    return;
}

PPCFUNCTION ULONG mySetExceptPPC(struct PPCBase* PowerPCBase, ULONG signals, ULONG mask, ULONG flag)
{
    return 0;
}

PPCFUNCTION VOID myObtainSemaphoreSharedPPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

PPCFUNCTION LONG myAttemptSemaphoreSharedPPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return 0;
}

PPCFUNCTION VOID myProcurePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, struct SemaphoreMessage* SemaphoreMessage)
{
    return;
}

PPCFUNCTION VOID myVacatePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, struct SemaphoreMessage* SemaphoreMessage)
{
    return;
}

PPCFUNCTION VOID myCauseInterrupt(struct PPCBase* PowerPCBase)
{
    return;
}

PPCFUNCTION APTR myCreatePoolPPC(struct PPCBase* PowerPCBase, ULONG flags, ULONG puddle_size, ULONG trehs_size)
{
    return NULL;
}

PPCFUNCTION VOID myDeletePoolPPC(struct PPCBase* PowerPCBase, APTR poolheader)
{
    return;
}

PPCFUNCTION APTR myAllocPooledPPC(struct PPCBase* PowerPCBase, APTR poolheader, ULONG size)
{
    return NULL;
}

PPCFUNCTION VOID myFreePooledPPC(struct PPCBase* PowerPCBase, APTR poolheader, APTR ptr, ULONG size)
{
    return;
}

PPCFUNCTION APTR myRawDoFmtPPC(struct PPCBase* PowerPCBase, STRPTR formatstring, APTR datastream, void (*putchproc)(), APTR putchdata)
{
    return NULL;
}

PPCFUNCTION LONG myPutPublicMsgPPC(struct PPCBase* PowerPCBase, STRPTR portname, struct Message* message)
{
    return 0;
}

PPCFUNCTION LONG myAddUniquePortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return 0;
}

PPCFUNCTION LONG myAddUniqueSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return 0;
}

PPCFUNCTION BOOL myIsExceptionMode(struct PPCBase* PowerPCBase)
{
    return FALSE;
}

PPCFUNCTION VOID myAllocPrivateMem(void)
{
    return;
}

PPCFUNCTION VOID myFreePrivateMem(void)
{
    return;
}

PPCFUNCTION VOID myResetPPC(void)
{
    return;
}

PPCFUNCTION BOOL myChangeStack(struct PPCBase* PowerPCBase, ULONG NewStackSize)
{
    return FALSE;
}

PPCFUNCTION ULONG myRun68KLowLevel(struct PPCBase* PowerPCBase, ULONG Code, LONG Offset, ULONG a0,
                                 ULONG a1, ULONG d0, ULONG d1)
{
    return 0;
}
