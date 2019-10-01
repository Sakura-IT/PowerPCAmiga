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
#include <powerpc/powerpc.h>
#include <powerpc/tasksPPC.h>
#include <powerpc/memoryPPC.h>
#include <powerpc/semaphoresPPC.h>
#include <powerpc/portsPPC.h>


ULONG myRun68K(struct PPCBase* PowerPCBase, struct PPCArgs* PPStruct)
{
    return 0;
}

ULONG myWaitFor68K(struct PPCBase* PowerPCBase, struct PPCArgs* PPStruct)
{
    return 0;
}

VOID mySPrintF(struct PPCBase* PowerPCBase, STRPTR Formatstring, APTR Values)
{
    return;
}

APTR myAllocVecPPC(struct PPCBase* PowerPCBase, ULONG size, ULONG flags, ULONG align)
{
    return NULL;
}

LONG myFreeVecPPC(struct PPCBase* PowerPCBase, APTR memblock)
{
    return 0;
}

struct TaskPPC* myCreateTaskPPC(struct PPCBase* PowerPCBase, struct TagItem* taglist)
{
    return NULL;
}

VOID myDeleteTaskPPC(struct PPCBase* PowerPCBase, struct TaskPPC* PPCtask)
{
    return;
}

struct TaskPPC* myFindTaskPPC(struct PPCBase* PowerPCBase, STRPTR name)
{
    return NULL;
}

LONG myInitSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return 0;
}

VOID myFreeSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

VOID myAddSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

VOID myRemSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

VOID myObtainSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

LONG myAttemptSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return 0;
}

VOID myReleaseSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

struct SignalSemaphorePPC* myFindSemaphorePPC(struct PPCBase* PowerPCBase, STRPTR name)
{
    return NULL;
}

VOID myInsertPPC(struct PPCBase* PowerPCBase, struct List* list, struct Node* node, struct Node* pred)
{
    return;
}

VOID myAddHeadPPC(struct PPCBase* PowerPCBase, struct List* list, struct Node* node)
{
    return;
}

VOID myAddTailPPC(struct PPCBase* PowerPCBase, struct List* list, struct Node* node)
{
    return;
}

VOID myRemovePPC(struct PPCBase* PowerPCBase, struct Node* node)
{
    return;
}

struct Node* myRemHeadPPC(struct PPCBase* PowerPCBase, struct List* list)
{
    return NULL;
}

struct Node* myRemTailPPC(struct PPCBase* PowerPCBase, struct Node* list)
{
    return NULL;
}

VOID myEnqueuePPC(struct PPCBase* PowerPCBase, struct List* list, struct Node* node)
{
    return;
}

struct Node* myFindNamePPC(struct PPCBase* PowerPCBase, struct List* list, STRPTR name)
{
    return NULL;
}

struct TagItem* myFindTagItemPPC(struct PPCBase* PowerPCBase, ULONG value, struct TagItem* taglist)
{
    return NULL;
}

ULONG myGetTagDataPPC(struct PPCBase* PowerPCBase, ULONG value, ULONG d0arg, struct TagItem* taglist)
{
    return 0;
}

struct TagItem* myNextTagItemPPC(struct PPCBase* PowerPCBase, struct TagItem** tagitem)
{
    return NULL;
}

LONG myAllocSignalPPC(struct PPCBase* PowerPCBase, LONG signum)
{
    return 0;
}

VOID myFreeSignalPPC(struct PPCBase* PowerPCBase, LONG signum)
{
    return;
}

ULONG mySetSignalPPC(struct PPCBase* PowerPCBase, ULONG signals, ULONG mask)
{
    return 0;
}

VOID mySignalPPC(struct PPCBase* PowerPCBase, struct TaskPPC* task, ULONG signals)
{
    return;
}

ULONG myWaitPPC(struct PPCBase* PowerPCBase, ULONG signals)
{
    return 0;
}

LONG mySetTaskPriPPC(struct PPCBase* PowerPCBase, struct TaskPPC* task, LONG pri)
{
    return 0;
}

VOID mySignal68K(struct PPCBase* PowerPCBase, struct Task* task, ULONG signals)
{
    return;
}

VOID mySetCache(struct PPCBase* PowerPCBase, ULONG flags, APTR start, ULONG length)
{
    return;
}

APTR mySetExcHandler(struct PPCBase* PowerPCBase, struct TagItem* taglist)
{
    return NULL;
}

VOID myRemExcHandler(struct PPCBase* PowerPCBase, APTR xlock)
{
    return;
}

ULONG mySuper(struct PPCBase* PowerPCBase)
{
    return 0;
}

VOID myUser(struct PPCBase* PowerPCBase, ULONG key)
{
    return;
}

ULONG mySetHardware(struct PPCBase* PowerPCBase, ULONG flags, APTR param)
{
    return 0;
}

VOID myModifyFPExc(struct PPCBase* PowerPCBase, ULONG fpflags)
{
    return;
}

ULONG myWaitTime(struct PPCBase* PowerPCBase, ULONG signals, ULONG time)
{
    return 0;
}

struct TaskPtr* myLockTaskList(struct PPCBase* PowerPCBase)
{
    return NULL;
}

VOID myUnLockTaskList(struct PPCBase* PowerPCBase)
{
    return;
}

VOID mySetExcMMU(struct PPCBase* PowerPCBase)
{
    return;
}

VOID myClearExcMMU(struct PPCBase* PowerPCBase)
{
    return;
}

VOID myChangeMMU(struct PPCBase* PowerPCBase, ULONG mode)
{
    return;
}

VOID myGetInfo(struct PPCBase* PowerPCBase, struct TagItem* taglist)
{
    return;
}

struct MsgPortPPC* myCreateMsgPortPPC(struct PPCBase* PowerPCBase)
{
    return NULL;
}

VOID myDeleteMsgPortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return;
}

VOID myAddPortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return;
}

VOID myRemPortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return;
}

struct MsgPortPPC* myFindPortPPC(struct PPCBase* PowerPCBase, STRPTR port)
{
    return NULL;
}

struct Message* myWaitPortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return NULL;
}

VOID myPutMsgPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port, struct Message* message)
{
    return;
}

struct Message* myGetMsgPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return NULL;
}

VOID myReplyMsgPPC(struct PPCBase* PowerPCBase, struct Message* message)
{
    return;
}

VOID myFreeAllMem(struct PPCBase* PowerPCBase)
{
    return;
}

VOID myCopyMemPPC(struct PPCBase* PowerPCBase, APTR source, APTR dest, ULONG size)
{
    return;
}

struct Message* myAllocXMsgPPC(struct PPCBase* PowerPCBase, ULONG length, struct MsgPortPPC* port)
{
    return NULL;
}

VOID myFreeXMsgPPC(struct PPCBase* PowerPCBase, struct Message* message)
{
    return;
}

VOID myPutXMsgPPC(struct PPCBase* PowerPCBase, struct MsgPort* port, struct Message* message)
{
    return;
}

VOID myGetSysTimePPC(struct PPCBase* PowerPCBase, struct timeval* timeval)
{
    return;
}

VOID myAddTimePPC(struct PPCBase* PowerPCBase, struct timeval* dest, struct timeval* source)
{
    return;
}

VOID mySubTimePPC(struct PPCBase* PowerPCBase, struct timeval* dest, struct timeval* source)
{
    return;
}

LONG myCmpTimePPC(struct PPCBase* PowerPCBase, struct timeval* dest, struct timeval* source)
{
    return 0;
}

struct MsgPortPPC* mySetReplyPortPPC(struct PPCBase* PowerPCBase, struct Message* message, struct MsgPortPPC* port)
{
    return NULL;
}

ULONG mySnoopTask(struct PPCBase* PowerPCBase, struct TagItem* taglist)
{
    return 0;
}

VOID myEndSnoopTask(struct PPCBase* PowerPCBase, ULONG id)
{
    return;
}

VOID myGetHALInfo(struct PPCBase* PowerPCBase, struct TagItem* taglist)
{
    return;
}

VOID mySetScheduling(struct PPCBase* PowerPCBase, struct TagItem* taglist)
{
    return;
}

struct TaskPPC* myFindTaskByID(struct PPCBase* PowerPCBase, LONG id)
{
    return NULL;
}

LONG mySetNiceValue(struct PPCBase* PowerPCBase, struct TaskPPC* task, LONG nice)
{
    return 0;
}

LONG myTrySemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, ULONG timeout)
{
    return 0;
}

VOID myNewListPPC(struct PPCBase* PowerPCBase, struct List* list)
{
    return;
}

ULONG mySetExceptPPC(struct PPCBase* PowerPCBase, ULONG signals, ULONG mask, ULONG flag)
{
    return 0;
}

VOID myObtainSemaphoreSharedPPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

LONG myAttemptSemaphoreSharedPPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return 0;
}

VOID myProcurePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, struct SemaphoreMessage* SemaphoreMessage)
{
    return;
}

VOID myVacatePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, struct SemaphoreMessage* SemaphoreMessage)
{
    return;
}

VOID myCauseInterrupt(struct PPCBase* PowerPCBase)
{
    return;
}

APTR myCreatePoolPPC(struct PPCBase* PowerPCBase, ULONG flags, ULONG puddle_size, ULONG trehs_size)
{
    return NULL;
}

VOID myDeletePoolPPC(struct PPCBase* PowerPCBase, APTR poolheader)
{
    return;
}

APTR myAllocPooledPPC(struct PPCBase* PowerPCBase, APTR poolheader, ULONG size)
{
    return NULL;
}

VOID myFreePooledPPC(struct PPCBase* PowerPCBase, APTR poolheader, APTR ptr, ULONG size)
{
    return;
}

APTR myRawDoFmtPPC(struct PPCBase* PowerPCBase, STRPTR formatstring, APTR datastream, void (*putchproc)(), APTR putchdata)
{
    return NULL;
}

LONG myPutPublicMsgPPC(struct PPCBase* PowerPCBase, STRPTR portname, struct Message* message)
{
    return 0;
}

LONG myAddUniquePortPPC(struct PPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return 0;
}

LONG myAddUniqueSemaphorePPC(struct PPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return 0;
}

BOOL myIsExceptionMode(struct PPCBase* PowerPCBase)
{
    return FALSE;
}


