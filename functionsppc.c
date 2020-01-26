// Copyright (c) 2019, 2020 Dennis van der Boon
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
	if (PPStruct->PP_Offset)
	{
		if ((PPStruct->PP_Code) && (PPStruct->PP_Code == PowerPCBase->pp_UtilityBase) && (PPStruct->PP_Offset == _LVOStricmp))
		{
			PPStruct->PP_Regs[0] = (ULONG)StricmpPPC((STRPTR)PPStruct->PP_Regs[8], (STRPTR)PPStruct->PP_Regs[9]);
			return PPERR_SUCCESS;
		}
	}

	printDebugEntry(PowerPCBase, function, (ULONG)PPStruct->PP_Code, (ULONG)PPStruct->PP_Offset, 0, 0);

	if (!(PPStruct->PP_Code))
	{
		return PPERR_MISCERR;
	}
	if (PPStruct->PP_Flags)
	{
		storeR0(ERR_ESNC);
		HaltTask();
	}

	PowerPCBase->pp_NumRun68k += 1;
	dFlush((ULONG)&PowerPCBase->pp_NumRun68k);

	struct MsgFrame* myFrame = CreateMsgFramePPC(PowerPCBase);

    myCopyMemPPC(PowerPCBase, (APTR)PPStruct, (APTR)&myFrame->mf_PPCArgs, sizeof(struct PPCArgs));

	struct PrivateTask* privTask = (struct PrivateTask*)PowerPCBase->pp_ThisPPCProc;
    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

	myFrame->mf_Identifier = ID_T68K;
	myFrame->mf_PPCTask = (struct TaskPPC*)myTask;
	myFrame->mf_MirrorPort = privTask->pt_MirrorPort;
	myFrame->mf_Arg[0] = (ULONG)myTask->tp_Task.tc_Node.ln_Name;
	myFrame->mf_Arg[1] = myTask->tp_Task.tc_SigAlloc;
	myFrame->mf_Arg[2] = 0;

	while(!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

	myFrame->mf_Signals = myTask->tp_Task.tc_SigRecvd & ~0xfff;
	myTask->tp_Task.tc_SigRecvd &= 0xfff;

	FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

	if (PPStruct->PP_Stack)
	{
		if(PPStruct->PP_StackSize)
		{
			mySetCache(PowerPCBase, CACHE_DCACHEFLUSH, PPStruct->PP_Stack, PPStruct->PP_StackSize);
		}
		else
		{
			myFrame->mf_PPCArgs.PP_Stack = NULL;
		}
	}

	myFrame->mf_Message.mn_Node.ln_Type = NT_MESSAGE;
	myFrame->mf_Message.mn_Length = sizeof(struct MsgFrame);

	if (PowerPCBase->pp_PowerPCBase.PPC_SysLib == PPStruct->PP_Code)
	{
		if((PPStruct->PP_Offset == _LVOAllocMem) || (PPStruct->PP_Offset == _LVOAllocVec))
		{
			if(!(PPStruct->PP_Regs[1] & MEMF_CHIP))
			{
				myFrame->mf_PPCArgs.PP_Regs[1] |= MEMF_PPC;
			}
		}
	}

	SendMsgFramePPC(PowerPCBase, myFrame);

	myWaitFor68K(PowerPCBase, PPStruct);

	printDebugExit(PowerPCBase, function, PPStruct->PP_Regs[0]);

	return PPERR_SUCCESS;
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
    	myRun68KLowLevel(PowerPCBase, (ULONG)PowerPCBase, (LONG)_LVOSprintF68K,
                        (ULONG)Formatstring, (ULONG)Values, 0, 0);
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
    	printDebugEntry(PowerPCBase, function, (ULONG)name, 0, 0, 0);

	if(!(name))
	{
		return PowerPCBase->pp_ThisPPCProc;
	}

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemTaskList);

	struct TaskPPC* fndTask;
	struct TaskPtr* taskPtr = (struct TaskPtr*)myFindNamePPC(PowerPCBase,
                              (struct List*)&PowerPCBase->pp_AllTasks, name);
	if (taskPtr)
	{
		fndTask = taskPtr->tptr_Task;
	}
	else
	{
		fndTask = NULL;
	}
	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemTaskList);

	printDebugExit(PowerPCBase, function, (ULONG)fndTask);

	return fndTask;
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
		myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

		myEnqueuePPC(PowerPCBase, (struct List*)&PowerPCBase->pp_Semaphores, (struct Node*)SemaphorePPC);

		myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

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

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

	myRemovePPC(PowerPCBase, (struct Node*)SemaphorePPC);

	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

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
    	printDebugEntry(PowerPCBase, function, (ULONG)SemaphorePPC, 0, 0, 0);

	while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

	SemaphorePPC->ssppc_SS.ss_NestCount -= 1;

	if (SemaphorePPC->ssppc_SS.ss_NestCount < 0)
	{
		storeR0(ERR_ESEM);
        HaltTask();
	}

	if (SemaphorePPC->ssppc_SS.ss_NestCount > 0)
	{
		SemaphorePPC->ssppc_SS.ss_QueueCount -= 1;
		FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
	}
	else
	{
		SemaphorePPC->ssppc_SS.ss_Owner = 0;
		SemaphorePPC->ssppc_SS.ss_QueueCount -=1;
		if (SemaphorePPC->ssppc_SS.ss_QueueCount < 0)
		{
			FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
		}
		else
		{
			SemaphorePPC->ssppc_lock = 1;
			FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
			struct SemWait* myWait = (struct SemWait*)myRemHeadPPC(PowerPCBase, (struct List*)&SemaphorePPC->ssppc_SS.ss_WaitQueue);
			if (myWait)
			{
				struct TaskPPC* currTask = myWait->sw_Task;
                struct TaskPPC* sigTask = (struct TaskPPC*)((ULONG)currTask & ~SM_SHARED);
				if (!(SM_SHARED & (ULONG)currTask))
				{
					if (sigTask)
					{
						SemaphorePPC->ssppc_SS.ss_Owner = (struct Task*)sigTask;
						SemaphorePPC->ssppc_SS.ss_NestCount += 1;
						mySignalPPC(PowerPCBase, sigTask, SIGF_SINGLE);
					}
					else
					{
						struct SemaphoreMessage* mySemMsg = (struct SemaphoreMessage*)myWait;
						SemaphorePPC->ssppc_SS.ss_Owner = (struct Task*)mySemMsg->ssm_Semaphore;
						mySemMsg->ssm_Semaphore = (struct SignalSemaphore*)SemaphorePPC;
						SemaphorePPC->ssppc_SS.ss_NestCount += 1;
						myReplyMsgPPC(PowerPCBase, (struct Message*)mySemMsg);

                        myWait = (struct SemWait*)SemaphorePPC->ssppc_SS.ss_WaitQueue.mlh_Head;
                        struct SemWait* nxtWait;

                        while (nxtWait = (struct SemWait*)myWait->sw_Node.ln_Succ)
                        {
                            if (!(myWait->sw_Task))
                            {
                                if (myWait->sw_Semaphore)
                                {
                                    break;
                                }
                                struct Node* predWait = myWait->sw_Node.ln_Pred;
                                predWait->ln_Succ = (struct Node*)nxtWait;
                                nxtWait->sw_Node.ln_Pred = predWait;

                                SemaphorePPC->ssppc_SS.ss_NestCount += 2;
						        mySemMsg = (struct SemaphoreMessage*)myWait;
                                mySemMsg->ssm_Semaphore = (struct SignalSemaphore*)SemaphorePPC;

                                myReplyMsgPPC(PowerPCBase, (struct Message*)mySemMsg);
                            }
                            else if (myWait->sw_Task == (struct TaskPPC*)SemaphorePPC->ssppc_SS.ss_Owner)
                            {
                                struct Node* predWait = myWait->sw_Node.ln_Pred;
                                predWait->ln_Succ = (struct Node*)nxtWait;
                                nxtWait->sw_Node.ln_Pred = predWait;

                                SemaphorePPC->ssppc_SS.ss_NestCount += 1;
						        mySignalPPC(PowerPCBase, myWait->sw_Task, SIGF_SINGLE);

                                break;
                            }
                            myWait = nxtWait;
                        }
					}
				}
				else
				{
                    struct SemWait* nxtWait = (struct SemWait*)myWait->sw_Node.ln_Succ;
                    do
                    {
                        SemaphorePPC->ssppc_SS.ss_NestCount += 1;
                        if (sigTask)
                        {
                           mySignalPPC(PowerPCBase, sigTask, SIGF_SINGLE);
                        }
                        else
                        {
                            struct SemaphoreMessage* mySemMsg = (struct SemaphoreMessage*)myWait;
                            mySemMsg->ssm_Semaphore = (struct SignalSemaphore*)SemaphorePPC;
                            mySemMsg->ssm_Message.mn_ReplyPort = 0;  //wrong

                            myReplyMsgPPC(PowerPCBase, (struct Message*)mySemMsg);
                        }
                        if (!(nxtWait))
                        {
                            break;
                        }
                        sigTask = (struct TaskPPC*)((ULONG)myWait->sw_Task & ~ SM_SHARED);
                        while (nxtWait)
                        {
                            if ((ULONG)myWait->sw_Task & SM_SHARED)
                            {
                                struct Node* predWait = myWait->sw_Node.ln_Pred;
                                predWait->ln_Succ = (struct Node*)nxtWait;
                                nxtWait->sw_Node.ln_Pred = predWait;
                                myWait = nxtWait;
                                nxtWait = (struct SemWait*)myWait->sw_Node.ln_Succ;
                                break;
                            }
                            else
                            {
                                myWait = nxtWait;
                                nxtWait = (struct SemWait*)myWait->sw_Node.ln_Succ;
                            }
                        }
                    } while (1);
				}
			}
			SemaphorePPC->ssppc_lock = 0;
        }
	}
	printDebugExit(PowerPCBase, function, 0);
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

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

	struct SignalSemaphorePPC* mySem = (struct SignalSemaphorePPC*)myFindNamePPC(PowerPCBase, (struct List*)&PowerPCBase->pp_SemSemList, name);

	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

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
    struct Node* nxtNode;
	struct Node* curNode;

	curNode = list->lh_Head;
	while (nxtNode = curNode->ln_Succ)
	{
		STRPTR nodeName = curNode->ln_Name;
		ULONG offset = 0;

		while (nodeName[offset] == name[offset])
		{
			if (!(nodeName[offset]))
			{
				return curNode;
			}
		}
	curNode = nxtNode;
	}
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
    BYTE testSig = (BYTE)signum;
	if (!(testSig == -1))
	{
		PowerPCBase->pp_ThisPPCProc->tp_Task.tc_SigAlloc &= ~(1<signum);
		struct TaskLink* taskLink = &PowerPCBase->pp_ThisPPCProc->tp_Link;
		taskLink->tl_Sig &= ~(1<signum);
	}
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG mySetSignalPPC(struct PrivatePPCBase* PowerPCBase, ULONG signals, ULONG mask)
{
    printDebugEntry(PowerPCBase, function, signals, mask, 0, 0);

	struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;
	while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

	ULONG oldSig = myTask->tp_Task.tc_SigRecvd;
	myTask->tp_Task.tc_SigRecvd = (signals &= mask) | (oldSig &= ~mask);

	FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

	CheckExcSignal(PowerPCBase, myTask, 0);

	printDebugExit(PowerPCBase, function, oldSig);

	return oldSig;
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
	printDebugEntry(PowerPCBase, function, signals, 0, 0, 0);

	struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

	while(!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

	myTask->tp_Task.tc_SigWait = signals;
	ULONG maskSig;

	while(!(maskSig = myTask->tp_Task.tc_SigRecvd & signals))
	{
		FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
		myTask->tp_Task.tc_State = TS_CHANGING;

		CauseDECInterrupt(PowerPCBase);

		while(myTask->tp_Task.tc_State != TS_RUN);

		while(!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

		signals = myTask->tp_Task.tc_SigWait;
	}

	myTask->tp_Task.tc_SigRecvd ^= maskSig;

	FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

	printDebugExit(PowerPCBase, function, maskSig);

	return maskSig;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG mySetTaskPriPPC(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* task, LONG pri)
{
	printDebugEntry(PowerPCBase, function, (ULONG)task, (ULONG)pri, 0, 0);

	while(!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

	LONG oldPri = task->tp_Task.tc_Node.ln_Pri;
	task->tp_Task.tc_Node.ln_Pri = (BYTE)pri;

	if (PowerPCBase->pp_ThisPPCProc != task)
	{
		if((task->tp_Task.tc_State != TS_REMOVED) && (task->tp_Task.tc_State != TS_WAIT))
		{
			myRemovePPC(PowerPCBase, (struct Node*)task);
			task->tp_Task.tc_State = TS_READY;
			InsertOnPri(PowerPCBase, (struct List*)&PowerPCBase->pp_ReadyTasks, task);
			if(task == (struct TaskPPC*)PowerPCBase->pp_ReadyTasks.mlh_Head)
			{
				FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
				CauseDECInterrupt(PowerPCBase);
			}
			else
			{
				FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
			}
		}
		else
		{
			FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
		}
	}
	else
	{
		FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
		CauseDECInterrupt(PowerPCBase);
	}

	printDebugExit(PowerPCBase, function, oldPri);

	return oldPri;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySignal68K(struct PrivatePPCBase* PowerPCBase, struct Task* task, ULONG signals)
{
	printDebugEntry(PowerPCBase, function, (ULONG)task, signals, 0, 0);

	struct MsgFrame* myFrame = CreateMsgFramePPC(PowerPCBase);

	myFrame->mf_Identifier = ID_SIG;
	myFrame->mf_Signals = signals;
	myFrame->mf_Arg[0] = (ULONG)task;

	SendMsgFramePPC(PowerPCBase, myFrame);

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
	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemTaskList);
	return ((struct TaskPtr*)&PowerPCBase->pp_AllTasks.mlh_Head);
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myUnLockTaskList(struct PrivatePPCBase* PowerPCBase)
{
	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemTaskList);
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
	printDebugEntry(PowerPCBase, function, 0, 0, 0, 0);

	struct MsgPortPPC* myPort;
	LONG signal, result;

	if (myPort = AllocVec68K(PowerPCBase, sizeof(struct MsgPortPPC), MEMF_PUBLIC | MEMF_CLEAR | MEMF_PPC))
	{
		if ((signal = myAllocSignalPPC(PowerPCBase, -1) != -1))
		{
			if (result = myInitSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&myPort->mp_Semaphore))
			{
				myNewListPPC(PowerPCBase, (struct List*)&myPort->mp_IntMsg);
				myNewListPPC(PowerPCBase, (struct List*)&myPort->mp_Port.mp_MsgList);
				myPort->mp_Port.mp_SigBit = signal;
				myPort->mp_Port.mp_SigTask = PowerPCBase->pp_ThisPPCProc;
				myPort->mp_Port.mp_Flags = PA_SIGNAL;
				myPort->mp_Port.mp_Node.ln_Type = NT_MSGPORTPPC;
			}
			else
			{
				myFreeSignalPPC(PowerPCBase, signal);
			}
		}
		else
		{
			FreeVec68K(PowerPCBase, myPort);
		}
	}
	printDebugExit(PowerPCBase, function, (ULONG)myPort);
	return myPort;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myDeleteMsgPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
	printDebugEntry(PowerPCBase, function, (ULONG)port, 0, 0, 0);

	if(port)
	{
		myFreeSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&port->mp_Semaphore);
		myFreeSignalPPC(PowerPCBase, port->mp_Port.mp_SigBit);
		FreeVec68K(PowerPCBase, port);
	}
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myAddPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
	printDebugEntry(PowerPCBase, function, (ULONG)port, 0, 0, 0);

	myNewListPPC(PowerPCBase, (struct List*)&port->mp_Port.mp_MsgList);

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

	myEnqueuePPC(PowerPCBase, (struct List*)&PowerPCBase->pp_Ports, (struct Node*)port);

	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

	printDebugExit(PowerPCBase, function, 0);

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myRemPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
	printDebugEntry(PowerPCBase, function, (ULONG)port, 0, 0, 0);

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

	myRemovePPC(PowerPCBase, (struct Node*)port);

	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

	printDebugExit(PowerPCBase, function, 0);

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct MsgPortPPC* myFindPortPPC(struct PrivatePPCBase* PowerPCBase, STRPTR name)
{
	printDebugEntry(PowerPCBase, function, (ULONG)name, 0, 0, 0);

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

	struct MsgPortPPC* myPort = (struct MsgPortPPC*)myFindNamePPC(PowerPCBase, (struct List*)&PowerPCBase->pp_Ports, name);

	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

	printDebugExit(PowerPCBase, function, (ULONG)myPort);

	return myPort;
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
	printDebugEntry(PowerPCBase, function, length, (ULONG)port, 0, 0);

	length = (length + sizeof(struct Message) + 31) & -32;

	struct Message* myXMessage;

	if (myXMessage = myAllocVecPPC(PowerPCBase, length, MEMF_PUBLIC | MEMF_CLEAR | MEMF_PPC, 32L))
	{
		myXMessage->mn_ReplyPort = (struct MsgPort*)port;
		myXMessage->mn_Length = length;
	}

	printDebugExit(PowerPCBase, function, (ULONG)myXMessage);

	return myXMessage;
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
	printDebugEntry(PowerPCBase, function, (ULONG)port, (ULONG)message, 0, 0);

	message->mn_Node.ln_Type = NT_XMSGPPC;

	mySetCache(PowerPCBase, CACHE_DCACHEFLUSH, message, message->mn_Length);

	struct MsgFrame* myFrame = CreateMsgFramePPC(PowerPCBase);

	myFrame->mf_Identifier = ID_XMSG;
	myFrame->mf_Arg[0] = (ULONG)port;
	myFrame->mf_Arg[1] = (ULONG)message;
	myFrame->mf_Message.mn_Node.ln_Type = NT_MESSAGE;

	SendMsgFramePPC(PowerPCBase, myFrame);

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
	printDebugEntry(PowerPCBase, function, (ULONG)message, (ULONG)port, 0, 0);

	struct MsgPortPPC* oldPort = (struct MsgPortPPC*)message->mn_ReplyPort;
	message->mn_ReplyPort = (struct MsgPort*)port;

	return oldPort;
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
		myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemMemory);
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

		myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemMemory);
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
	printDebugEntry(PowerPCBase, function, (ULONG)port, 0, 0, 0);

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

	struct MsgPortPPC* myPort;
	LONG result;

	if (myPort = myFindPortPPC(PowerPCBase, port->mp_Port.mp_Node.ln_Name))
	{
		result = UNIPORT_NOTUNIQUE;
	}
	else
	{
		result = UNIPORT_SUCCESS;
		myAddPortPPC(PowerPCBase, port);
	}
	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);
	return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAddUniqueSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
	printDebugEntry(PowerPCBase, function, (ULONG)SemaphorePPC, 0, 0, 0);

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

	struct SignalSemaphorePPC* mySem;
	LONG result;

	if (mySem = myFindSemaphorePPC(PowerPCBase, SemaphorePPC->ssppc_SS.ss_Link.ln_Name))
	{
		result = UNISEM_NOTUNIQUE;
	}
	else
	{
		result = UNISEM_SUCCESS;
		myAddSemaphorePPC(PowerPCBase, SemaphorePPC);
	}
	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);
	return result;
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
	struct MsgFrame* myFrame = CreateMsgFramePPC(PowerPCBase);

	myFrame->mf_Message.mn_Node.ln_Type = NT_MESSAGE;
	myFrame->mf_Message.mn_Length = sizeof(struct MsgFrame);
	myFrame->mf_Identifier = ID_LL68;
	myFrame->mf_PPCTask = PowerPCBase->pp_ThisPPCProc;
	myFrame->mf_PPCArgs.PP_Regs[0] = Code;
	myFrame->mf_PPCArgs.PP_Regs[1] = Offset;
	myFrame->mf_PPCArgs.PP_Regs[2] = a0;
	myFrame->mf_PPCArgs.PP_Regs[3] = a1;
	myFrame->mf_PPCArgs.PP_Regs[4] = d0;
	myFrame->mf_PPCArgs.PP_Regs[5] = d1;
	myFrame->mf_Arg[0] = 0;
	myFrame->mf_Arg[1] = 0;
	myFrame->mf_Arg[2] = 0;

	SendMsgFramePPC(PowerPCBase, myFrame);

	ULONG result = myWaitFor68K(PowerPCBase, (struct PPCArgs*)&myFrame->mf_PPCArgs);

	return result;
}
