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
#include "constants.h"
#include "libstructs.h"
#include "Internalsppc.h"

#define function 0 //debug

/********************************************************************************************
*
*        LONG = Run68K(struct Library* PowerPCBase, struct PPCArgs* PPStruct)
*
*********************************************************************************************/

PPCFUNCTION LONG myRun68K(struct PrivatePPCBase* PowerPCBase, struct PPCArgs* PPStruct)
{
    return 0;
}

/********************************************************************************************
*
*        LONG = WaitFor68K(struct Library*, struct PPCArgs*)
*
*********************************************************************************************/


PPCFUNCTION LONG myWaitFor68K(struct PrivatePPCBase* PowerPCBase, struct PPCArgs* PPStruct)
{
    return 0;
}

/********************************************************************************************
*
*        VOID mySPrintF (struct Library*, STRPTR, APTR)
*
*********************************************************************************************/

PPCFUNCTION VOID mySPrintF(struct PrivatePPCBase* PowerPCBase, STRPTR Formatstring, APTR Values)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION APTR myAllocVecPPC(struct PrivatePPCBase* PowerPCBase, ULONG size, ULONG flags, ULONG align)
{
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myFreeVecPPC(struct PrivatePPCBase* PowerPCBase, APTR memblock)
{
	printDebugEntry(PowerPCBase, function, (ULONG)memblock, 0, 0, 0);

	if (memblock)
	{
		ULONG* myMemblock = (ULONG*)memblock;
		myFreePooledPPC(PowerPCBase, (APTR)myMemblock[-2], (APTR)myMemblock[-3], myMemblock[-1]);
	}

	printDebugExit(PowerPCBase, function, 0);
	
    return MEMERR_SUCCESS;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct TaskPPC* myCreateTaskPPC(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist)
{
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myDeleteTaskPPC(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* PPCtask)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct TaskPPC* myFindTaskPPC(struct PrivatePPCBase* PowerPCBase, STRPTR name)
{
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myInitSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
	printDebugEntry(PowerPCBase, function, (ULONG)SemaphorePPC, 0, 0, 0);

	LONG result = 0;
    APTR reserved;

	myNewListPPC(PowerPCBase, (struct List*)&SemaphorePPC->ssppc_SS.ss_WaitQueue);
	SemaphorePPC->ssppc_SS.ss_Owner = 0;
	SemaphorePPC->ssppc_SS.ss_NestCount = 0;
	SemaphorePPC->ssppc_SS.ss_QueueCount = -1;

	if (reserved = AllocVec68K(PowerPCBase, 32, MEMF_PUBLIC|MEMF_CLEAR|MEMF_PPC)) //sizeof?
	{
		SemaphorePPC->ssppc_reserved = reserved;
		result = -1;
	}

	printDebugExit(PowerPCBase, function, (ULONG)result);

	return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myFreeSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
	printDebugEntry(PowerPCBase, function, (ULONG)SemaphorePPC, 0, 0, 0);

	if (SemaphorePPC)
	{
		FreeVec68K(PowerPCBase, SemaphorePPC->ssppc_reserved);
	}

	printDebugExit(PowerPCBase, function, 0);
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAddSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
	LONG result = 0;
	printDebugEntry(PowerPCBase, function, (ULONG)SemaphorePPC, 0, 0, 0);

	if (result = myInitSemaphorePPC(PowerPCBase, SemaphorePPC))
	{
		myObtainSemaphorePPC(PowerPCBase, &PowerPCBase->pp_SemSemList);

		myEnqueuePPC(PowerPCBase, (struct List*)&PowerPCBase->pp_Semaphores, (struct Node*)SemaphorePPC);

		myReleaseSemaphorePPC(PowerPCBase, &PowerPCBase->pp_SemSemList);

	}
    printDebugExit(PowerPCBase, function, (ULONG)result);

	return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myRemSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
	printDebugEntry(PowerPCBase, function, (ULONG)SemaphorePPC, 0, 0, 0);

	myObtainSemaphorePPC(PowerPCBase, &PowerPCBase->pp_SemSemList);

	myRemovePPC(PowerPCBase, (struct Node*)SemaphorePPC);

	myReleaseSemaphorePPC(PowerPCBase, &PowerPCBase->pp_SemSemList);

	myFreeSemaphorePPC(PowerPCBase, SemaphorePPC);

    printDebugExit(PowerPCBase, function, 0);
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myObtainSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
	printDebugEntry(PowerPCBase, function, (ULONG)SemaphorePPC, 0, 0, 0);

	while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

	SemaphorePPC->ssppc_SS.ss_QueueCount += 1;

	struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

	if ((SemaphorePPC->ssppc_SS.ss_QueueCount) && (!(SemaphorePPC->ssppc_SS.ss_Owner == (struct Task*)myTask)))
	{
		struct SemWait myWait;
		myWait.sw_Task = myTask;
		myWait.sw_Semaphore = SemaphorePPC;
		myTask->tp_Task.tc_SigRecvd |= SIGF_SINGLE;
		myTask->tp_Task.tc_SigRecvd ^= SIGF_SINGLE;

		myAddTailPPC(PowerPCBase, (struct List*)&SemaphorePPC->ssppc_SS.ss_WaitQueue, (struct Node*)&myWait);

		FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

		myWaitPPC(PowerPCBase, SIGF_SINGLE);
	}
	else
	{
		FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
	}

	SemaphorePPC->ssppc_SS.ss_NestCount += 1;

	printDebugExit(PowerPCBase, function, 0);

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAttemptSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
	printDebugEntry(PowerPCBase, function, (ULONG)SemaphorePPC, 0, 0, 0);
	LONG result = 0;

	while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

	LONG testSem = SemaphorePPC->ssppc_SS.ss_QueueCount + 1;

	struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

	if ((testSem) && (!(SemaphorePPC->ssppc_SS.ss_Owner == (struct Task*)myTask)))
	{
		result = ATTEMPT_FAILURE;
	}
	else
	{
		SemaphorePPC->ssppc_SS.ss_Owner = (struct Task*)myTask;
		SemaphorePPC->ssppc_SS.ss_QueueCount += 1;
		SemaphorePPC->ssppc_SS.ss_NestCount += 1;
		result = ATTEMPT_SUCCESS;
	}

	FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

	printDebugExit(PowerPCBase, function, (ULONG)result);

	return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myReleaseSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct SignalSemaphorePPC* myFindSemaphorePPC(struct PrivatePPCBase* PowerPCBase, STRPTR name)
{
	printDebugEntry(PowerPCBase, function, (ULONG)name, 0, 0, 0);

	myObtainSemaphorePPC(PowerPCBase, &PowerPCBase->pp_SemSemList);

	struct SignalSemaphorePPC* mySem = (struct SignalSemaphorePPC*)myFindNamePPC(PowerPCBase, (struct List*)&PowerPCBase->pp_SemSemList, name);

	myReleaseSemaphorePPC(PowerPCBase, &PowerPCBase->pp_SemSemList);

    printDebugExit(PowerPCBase, function, (ULONG)mySem);

	return mySem;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myInsertPPC(struct PrivatePPCBase* PowerPCBase, struct List* list, struct Node* node, struct Node* pred)
{
	if (pred == 0)
	{
		myAddHeadPPC(PowerPCBase, list, node);
        return;
	}

	struct Node* succ = pred->ln_Succ;

	if (succ)
	{
		node->ln_Succ = succ;
		node->ln_Pred = pred;
		succ->ln_Pred = node;
		pred->ln_Succ = node;
		return;
	}

	struct Node* predPred = pred->ln_Pred;

	node->ln_Succ = pred;
	pred->ln_Pred = node;
	predPred->ln_Succ = node;

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myAddHeadPPC(struct PrivatePPCBase* PowerPCBase, struct List* list, struct Node* node)
{
	struct Node* succ = list->lh_Head;
	list->lh_Head = node;
	node->ln_Succ = succ;
	node->ln_Pred = (struct Node*)list;
	succ->ln_Pred = node;

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myAddTailPPC(struct PrivatePPCBase* PowerPCBase, struct List* list, struct Node* node)
{
	struct Node* tailpred = list->lh_TailPred;
	list->lh_TailPred = node;
	node->ln_Succ = (struct Node*)&list->lh_Tail;
	node->ln_Pred = tailpred;
	tailpred->ln_Succ = node;

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myRemovePPC(struct PrivatePPCBase* PowerPCBase, struct Node* node)
{
	struct Node* succ = node->ln_Succ;
	struct Node* pred = node->ln_Pred;
	succ->ln_Pred = pred;
	pred->ln_Succ = succ;

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct Node* myRemHeadPPC(struct PrivatePPCBase* PowerPCBase, struct List* list)
{
	struct Node* firstNode = list->lh_Head;
	struct Node* succ = firstNode->ln_Succ;
	if (succ)
	{
		list->lh_Head = succ;
		succ->ln_Pred = (struct Node*)list;
		return firstNode;
	}
	return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct Node* myRemTailPPC(struct PrivatePPCBase* PowerPCBase, struct List* list)
{
	struct Node* tailpred = list->lh_TailPred;
	struct Node* pred = tailpred->ln_Pred;
	if (pred)
	{
		list->lh_TailPred = pred;
		pred->ln_Succ = (struct Node*)&list->lh_Tail;
		return tailpred;
	}
	return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myEnqueuePPC(struct PrivatePPCBase* PowerPCBase, struct List* list, struct Node* node)
{
	LONG myPrio = (LONG)node->ln_Pri;
	struct Node* nextNode = list->lh_Head;

	while(nextNode->ln_Succ)
	{
		LONG cmpPrio = (LONG)nextNode->ln_Pri;

		if (cmpPrio > myPrio)
		{
			break;
		}
		nextNode = nextNode->ln_Succ;
	}
	struct Node* pred = nextNode->ln_Pred;
	nextNode->ln_Pred = node;
	node->ln_Succ = nextNode;
	node->ln_Pred = pred;
	pred->ln_Succ = node;

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct Node* myFindNamePPC(struct PrivatePPCBase* PowerPCBase, struct List* list, STRPTR name)
{
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct TagItem* myFindTagItemPPC(struct PrivatePPCBase* PowerPCBase, ULONG tagvalue, struct TagItem* taglist)
{
	struct TagItem* myTagItem = NULL;

    struct TagItemPtr tagPtr;
    tagPtr.tip_TagItem = taglist;

	while (myTagItem = myNextTagItemPPC(PowerPCBase, (struct TagItem**)&tagPtr))
	{
		if (myTagItem->ti_Tag == tagvalue)
		{
			break;
		}
	}
	return myTagItem;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG myGetTagDataPPC(struct PrivatePPCBase* PowerPCBase, ULONG tagvalue, ULONG tagdefault, struct TagItem* taglist)
{
    ULONG result;
    struct TagItem* myTagItem;

    if (myTagItem = myFindTagItemPPC(PowerPCBase, tagvalue, taglist))
    {
        result = myTagItem->ti_Data;
    }
    else
    {
        result = tagdefault;
    }
    return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct TagItem* myNextTagItemPPC(struct PrivatePPCBase* PowerPCBase, struct TagItem** tagitem)
{
	struct TagItemPtr* tagitemptr = (struct TagItemPtr*)tagitem;
	struct TagItem* myTagItem;
    LONG myTag;

	if (myTagItem = tagitemptr->tip_TagItem)
    {
	    while (myTag = (LONG)myTagItem->ti_Tag)
	    {
            if ((myTag < 0) || (myTag > 3))
            {
                tagitemptr->tip_TagItem = myTagItem + 1;
                return myTagItem;
            }
            else if (myTag == 2)
            {
                if (!(myTagItem = (struct TagItem*)myTagItem->ti_Data))
                {
                    break;
                }
            }
            else if (myTag == 1)
            {
                myTagItem += 1;
            }
            else    //my Tag == 3
            {
                 ULONG skipValue = 1 + myTagItem->ti_Data;
                 myTagItem += skipValue;
            }
        }
        myTagItem = NULL;
        tagitemptr->tip_TagItem = myTagItem;
    }
    return myTagItem;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAllocSignalPPC(struct PrivatePPCBase* PowerPCBase, LONG signum)
{
    	printDebugEntry(PowerPCBase, function, (ULONG)signum, 0, 0, 0);

	LONG resAlloc, testAlloc;

	ULONG myAlloc = PowerPCBase->pp_ThisPPCProc->tp_Task.tc_SigAlloc;

	if (!((BYTE)signum == -1))
	{
		ULONG testAlloc = 1 << signum;
		resAlloc = signum;
		if (testAlloc & myAlloc)
		{
			resAlloc = -1;
		}
	}
	else
	{
		resAlloc = 0x1f;
        do
        {
			testAlloc = 1 << resAlloc;
			if (!(testAlloc & myAlloc))
			{
				break;
			}
        resAlloc--;
		} while(resAlloc > -1);
	}

	printDebugExit(PowerPCBase, function, 0);
	return resAlloc;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myFreeSignalPPC(struct PrivatePPCBase* PowerPCBase, LONG signum)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG mySetSignalPPC(struct PrivatePPCBase* PowerPCBase, ULONG signals, ULONG mask)
{
    return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySignalPPC(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* task, ULONG signals)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG myWaitPPC(struct PrivatePPCBase* PowerPCBase, ULONG signals)
{
    return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG mySetTaskPriPPC(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* task, LONG pri)
{
    return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySignal68K(struct PrivatePPCBase* PowerPCBase, struct Task* task, ULONG signals)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySetCache(struct PrivatePPCBase* PowerPCBase, ULONG flags, APTR start, ULONG length)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION APTR mySetExcHandler(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist)
{
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myRemExcHandler(struct PrivatePPCBase* PowerPCBase, APTR xlock)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG mySuper(struct PrivatePPCBase* PowerPCBase)
{
    ULONG result;

    storeR0(-1);

    getPVR();

    result = getR0();

    return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myUser(struct PrivatePPCBase* PowerPCBase, ULONG key)
{
    if (!(key))
    {
        ULONG msrbits = getMSR();
        msrbits |= PSL_PR;
        setMSR(msrbits);
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG mySetHardware(struct PrivatePPCBase* PowerPCBase, ULONG flags, APTR param)
{
    return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myModifyFPExc(struct PrivatePPCBase* PowerPCBase, ULONG fpflags)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG myWaitTime(struct PrivatePPCBase* PowerPCBase, ULONG signals, ULONG time)
{
    return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct TaskPtr* myLockTaskList(struct PrivatePPCBase* PowerPCBase)
{
	myObtainSemaphorePPC(PowerPCBase, &PowerPCBase->pp_SemTaskList);
	return ((struct TaskPtr*)&PowerPCBase->pp_AllTasks.mlh_Head);
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myUnLockTaskList(struct PrivatePPCBase* PowerPCBase)
{
	myReleaseSemaphorePPC(PowerPCBase, &PowerPCBase->pp_SemTaskList);
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySetExcMMU(struct PrivatePPCBase* PowerPCBase)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myClearExcMMU(struct PrivatePPCBase* PowerPCBase)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myChangeMMU(struct PrivatePPCBase* PowerPCBase, ULONG mode)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myGetInfo(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct MsgPortPPC* myCreateMsgPortPPC(struct PrivatePPCBase* PowerPCBase)
{
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myDeleteMsgPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myAddPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myRemPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct MsgPortPPC* myFindPortPPC(struct PrivatePPCBase* PowerPCBase, STRPTR port)
{
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct Message* myWaitPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myPutMsgPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port, struct Message* message)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct Message* myGetMsgPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myReplyMsgPPC(struct PrivatePPCBase* PowerPCBase, struct Message* message)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myFreeAllMem(struct PrivatePPCBase* PowerPCBase)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myCopyMemPPC(struct PrivatePPCBase* PowerPCBase, APTR source, APTR dest, ULONG size)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct Message* myAllocXMsgPPC(struct PrivatePPCBase* PowerPCBase, ULONG length, struct MsgPortPPC* port)
{
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myFreeXMsgPPC(struct PrivatePPCBase* PowerPCBase, struct Message* message)
{
	myFreeVecPPC(PowerPCBase, message);

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myPutXMsgPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPort* port, struct Message* message)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myGetSysTimePPC(struct PrivatePPCBase* PowerPCBase, struct timeval* timeval)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myAddTimePPC(struct PrivatePPCBase* PowerPCBase, struct timeval* dest, struct timeval* source)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySubTimePPC(struct PrivatePPCBase* PowerPCBase, struct timeval* dest, struct timeval* source)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myCmpTimePPC(struct PrivatePPCBase* PowerPCBase, struct timeval* dest, struct timeval* source)
{
    return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct MsgPortPPC* mySetReplyPortPPC(struct PrivatePPCBase* PowerPCBase, struct Message* message, struct MsgPortPPC* port)
{
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG mySnoopTask(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist)
{
    return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myEndSnoopTask(struct PrivatePPCBase* PowerPCBase, ULONG id)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myGetHALInfo(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySetScheduling(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct TaskPPC* myFindTaskByID(struct PrivatePPCBase* PowerPCBase, LONG id)
{
    printDebugEntry(PowerPCBase, function, (LONG)id, 0, 0, 0);

    struct TaskPtr* myPtr = myLockTaskList(PowerPCBase);
    struct TaskPPC* foundTask = NULL;

    while (myPtr->tptr_Node.ln_Succ)
    {
        struct TaskPPC* myTask = (struct TaskPPC*)myPtr->tptr_Task;
        if (id == myTask->tp_Id)
        {
            foundTask = myTask;
            break;
        }
        myPtr = (struct TaskPtr*)myPtr->tptr_Node.ln_Succ;
    }

    myUnLockTaskList(PowerPCBase);

    printDebugExit(PowerPCBase, function, (ULONG)foundTask);

    return foundTask;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG mySetNiceValue(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* task, LONG nice)
{
    printDebugEntry(PowerPCBase, function, (ULONG)task, (ULONG)nice, 0, 0);

    LONG oldNice;

    if (oldNice = (LONG)task)
    {
        if (nice < -20)
        {
            nice = -20;
        }
        else if (nice > 20)
        {
            nice = 20;
        }

        myLockTaskList(PowerPCBase);

        oldNice = task->tp_Nice;
        task->tp_Nice = nice;

        myUnLockTaskList(PowerPCBase);
    }

    printDebugExit(PowerPCBase, function, oldNice);

    return oldNice;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myTrySemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, ULONG timeout)
{
    return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myNewListPPC(struct PrivatePPCBase* PowerPCBase, struct List* list)
{
	list->lh_TailPred = (struct Node*)list;
	list->lh_Tail = 0;
	list->lh_Head = (struct Node*)((ULONG)list+4);

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG mySetExceptPPC(struct PrivatePPCBase* PowerPCBase, ULONG signals, ULONG mask, ULONG flag)
{
	printDebugEntry(PowerPCBase, function, signals, mask, flag, 0);

    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

    while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

    ULONG OldSignals = myTask->tp_Task.tc_SigExcept;
    ULONG newSignals = (signals & mask) | (OldSignals & ~mask);
    myTask->tp_Task.tc_SigExcept = newSignals;
    if (flag)
    {
       myTask->tp_Task.tc_ExceptData = (APTR)getR2();
    }

    FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

    CheckExcSignal(PowerPCBase, myTask, 0);

    printDebugExit(PowerPCBase, function, OldSignals);

    return OldSignals;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myObtainSemaphoreSharedPPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAttemptSemaphoreSharedPPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myProcurePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, struct SemaphoreMessage* SemaphoreMessage)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myVacatePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, struct SemaphoreMessage* SemaphoreMessage)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myCauseInterrupt(struct PrivatePPCBase* PowerPCBase)
{
    if (!(PowerPCBase->pp_ExceptionMode))
    {
        ULONG key = mySuper(PowerPCBase);

        PowerPCBase->pp_ExternalInt = -1;

		setDEC(10);
		
        myUser(PowerPCBase, key);
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION APTR myCreatePoolPPC(struct PrivatePPCBase* PowerPCBase, ULONG flags, ULONG puddle_size, ULONG thres_size)
{
	struct poolHeader* myPoolHeader = 0;

	printDebugEntry(PowerPCBase, function, flags, puddle_size, thres_size, 0);

	if (puddle_size >= thres_size)
	{
		if (myPoolHeader = AllocVec68K(PowerPCBase, sizeof(struct poolHeader), flags))
		{
			puddle_size = (puddle_size + 31) & -32;
			myNewListPPC(PowerPCBase, (struct List*)&myPoolHeader->ph_PuddleList);
			myNewListPPC(PowerPCBase, (struct List*)&myPoolHeader->ph_BlockList);

			myPoolHeader->ph_requirements = flags;
			myPoolHeader->ph_PuddleSize = puddle_size;
			myPoolHeader->ph_ThresholdSize = thres_size;

			myAddHeadPPC(PowerPCBase, &PowerPCBase->pp_ThisPPCProc->tp_TaskPools, (struct Node*)&myPoolHeader->ph_Node);
		}
	}
	printDebugExit(PowerPCBase, function, (ULONG)myPoolHeader);
	return myPoolHeader;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myDeletePoolPPC(struct PrivatePPCBase* PowerPCBase, APTR poolheader)
{
	printDebugEntry(PowerPCBase, function, (ULONG)poolheader, 0, 0, 0);

	if (poolheader)
	{
		myObtainSemaphorePPC(PowerPCBase, &PowerPCBase->pp_SemMemory);
		myRemovePPC(PowerPCBase, (struct Node*)poolheader);

        struct poolHeader* myheader = (struct poolHeader*)poolheader;

        struct List* puddleList = (struct List*)(&(myheader->ph_PuddleList));
        struct List* blockList = (struct List*)(&(myheader->ph_BlockList));
		struct Node* currNode;

		while (currNode = myRemHeadPPC(PowerPCBase, puddleList))
		{
			FreeVec68K(PowerPCBase, currNode);
		}
		while (currNode = myRemHeadPPC(PowerPCBase, blockList))
		{
			FreeVec68K(PowerPCBase, currNode);
		}

		FreeVec68K(PowerPCBase, poolheader);

		myReleaseSemaphorePPC(PowerPCBase, &PowerPCBase->pp_SemMemory);
	}
	printDebugExit(PowerPCBase, function, 0);
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION APTR myAllocPooledPPC(struct PrivatePPCBase* PowerPCBase, APTR poolheader, ULONG size)
{
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myFreePooledPPC(struct PrivatePPCBase* PowerPCBase, APTR poolheader, APTR ptr, ULONG size)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION APTR myRawDoFmtPPC(struct PrivatePPCBase* PowerPCBase, STRPTR formatstring, APTR datastream, void (*putchproc)(), APTR putchdata)
{
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myPutPublicMsgPPC(struct PrivatePPCBase* PowerPCBase, STRPTR portname, struct Message* message)
{
    return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAddUniquePortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAddUniqueSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION BOOL myIsExceptionMode(struct PrivatePPCBase* PowerPCBase)
{
    BOOL status = PowerPCBase->pp_ExceptionMode;
    return status;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myAllocPrivateMem(void)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myFreePrivateMem(void)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myResetPPC(void)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION BOOL myChangeStack(struct PrivatePPCBase* PowerPCBase, ULONG NewStackSize)
{
    return FALSE;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG myRun68KLowLevel(struct PrivatePPCBase* PowerPCBase, ULONG Code, LONG Offset, ULONG a0,
                                 ULONG a1, ULONG d0, ULONG d1)
{
    return 0;
}
