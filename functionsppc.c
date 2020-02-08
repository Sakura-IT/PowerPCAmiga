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

    struct DebugArgs args;
    args.db_Function = 0;
    args.db_Arg[0] = (ULONG)PPStruct->PP_Code;
    args.db_Arg[1] = PPStruct->PP_Offset;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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

    args.db_Arg[0] = PPStruct->PP_Regs[0];
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);
    APTR mem = 0;

    flags = (flags & ~MEMF_CHIP) | MEMF_PPC;

    struct poolHeader* currPool = (struct poolHeader*)PowerPCBase->pp_ThisPPCProc->tp_TaskPools.lh_Head;
    struct poolHeader* nextPool;

    while (nextPool = (struct poolHeader*)currPool->ph_Node.mln_Succ)
    {
        if ((currPool->ph_ThresholdSize == 0x80000) && (currPool->ph_Requirements == flags))
        {
            break;
        }
        currPool = nextPool;
    }

    if (!(align) || (align & 0x1f))
    {
        align = 32;
    }
    else
    {
        ULONG zeros = getLeadZ(align - 1);
        zeros = 32 - zeros;
        align = 1 << zeros;
    }
    if (!(currPool))
    {
        currPool = myCreatePoolPPC(PowerPCBase, flags, 0x100000, 0x80000);
    }
    if (currPool)
    {
        mem = myAllocPooledPPC(PowerPCBase, currPool, size + align +32);
    }
    if (mem)
    {
        ULONG origmem = (ULONG)mem;
        mem = (APTR)(((ULONG)mem + 31 + align) & ~align);
        if (flags & MEMF_CLEAR)
        {
            UBYTE* buffer = mem;
            for (int i=0; i < size; i++)
            {
                buffer[i] = 0;
            }
        }
        ULONG* values = mem;
        values[-1] = size;
        values[-2] = (ULONG)currPool;
        values[-3] = origmem;
    }

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return mem;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myFreeVecPPC(struct PrivatePPCBase* PowerPCBase, APTR memblock)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	if (memblock)
	{
		ULONG* myMemblock = memblock;
		myFreePooledPPC(PowerPCBase, (APTR)myMemblock[-2], (APTR)myMemblock[-3], myMemblock[-1]);
	}

    printDebug(PowerPCBase, (struct DebugArgs*)&args);
	
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
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);
    ULONG ownFlag = 0;

    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;
    if ((!(PPCtask)) || (PPCtask == myTask))
    {
        ownFlag = 1;
        PPCtask = myTask;
    }

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);

    struct SnoopData* currSnoop = (struct SnoopData*)PowerPCBase->pp_Snoop.mlh_Head;
    struct SnoopData* nextSnoop;

    while (nextSnoop = (struct SnoopData*)currSnoop->sd_Node.ln_Succ)
    {
        if (currSnoop->sd_Type == SNOOP_EXIT)
        {
            ULONG tempR2 = getR2();
            ULONG (*runSnoop)(__reg("r2") ULONG, __reg("r3") struct TaskPPC*) = currSnoop->sd_Code;
            runSnoop(currSnoop->sd_Data, PPCtask);
            storeR2(tempR2);
        }
        currSnoop = nextSnoop;
    }

    myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);

    if (PPCtask->tp_Msgport)
    {
        FreeVec68K(PowerPCBase, PPCtask->tp_Msgport->mp_Semaphore.ssppc_reserved);
        FreeVec68K(PowerPCBase, PPCtask->tp_Msgport);
    }

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemTaskList);

    if (PPCtask->tp_TaskPtr)
    {
        myRemovePPC(PowerPCBase, (struct Node*)PPCtask->tp_TaskPtr);
    }

    PowerPCBase->pp_NumAllTasks -= 1;

    myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemTaskList);

    struct MsgPort* mirrorPort;
    struct NewTask* PPCNewtask = (struct NewTask*)PPCtask;

    if (mirrorPort = PPCNewtask->nt_MirrorPort)
    {
        struct MsgFrame* myFrame = CreateMsgFramePPC(PowerPCBase);
        myFrame->mf_Identifier = ID_END;
        myFrame->mf_MirrorPort = mirrorPort;
        SendMsgFramePPC(PowerPCBase, myFrame);
    }

    if (ownFlag)
    {
        PPCtask->tp_Task.tc_State = TS_REMOVED;
        PowerPCBase->pp_FlagReschedule = -1;
        CauseDECInterrupt(PowerPCBase);
        while(1);  //Warning 208
    }
    else
    {
        while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));
        myRemovePPC(PowerPCBase, (struct Node*)PPCtask);
        myAddTailPPC(PowerPCBase, (struct List*)&PowerPCBase->pp_RemovedTasks, (struct Node*)PPCtask);
        PPCtask->tp_Task.tc_State = TS_REMOVED;
        FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct TaskPPC* myFindTaskPPC(struct PrivatePPCBase* PowerPCBase, STRPTR name)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return fndTask;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myInitSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myFreeSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	if (SemaphorePPC)
	{
		FreeVec68K(PowerPCBase, SemaphorePPC->ssppc_reserved);
	}
	
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAddSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);
	
    LONG result = 0;

	if (result = myInitSemaphorePPC(PowerPCBase, SemaphorePPC))
	{
		myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

		myEnqueuePPC(PowerPCBase, (struct List*)&PowerPCBase->pp_Semaphores, (struct Node*)SemaphorePPC);

		myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

	}

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myRemSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

	myRemovePPC(PowerPCBase, (struct Node*)SemaphorePPC);

	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

	myFreeSemaphorePPC(PowerPCBase, SemaphorePPC);
	
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myObtainSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAttemptSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myReleaseSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct SignalSemaphorePPC* myFindSemaphorePPC(struct PrivatePPCBase* PowerPCBase, STRPTR name)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

	struct SignalSemaphorePPC* mySem = (struct SignalSemaphorePPC*)myFindNamePPC(PowerPCBase, (struct List*)&PowerPCBase->pp_SemSemList, name);

	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;
	while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

	ULONG oldSig = myTask->tp_Task.tc_SigRecvd;
	myTask->tp_Task.tc_SigRecvd = (signals &= mask) | (oldSig &= ~mask);

	FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

	CheckExcSignal(PowerPCBase, myTask, 0);

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return oldSig;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySignalPPC(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* task, ULONG signals)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    switch (task->tp_Task.tc_Node.ln_Type)
    {
        case NT_TASK:
        {
            mySignal68K(PowerPCBase, (struct Task*)task, signals);
            break;
        }
        case NT_PPCTASK:
        {
            CheckExcSignal(PowerPCBase, task, signals);
            while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

            task->tp_Task.tc_SigRecvd |= signals;

            switch (task->tp_Task.tc_State)
            {
                case TS_WAIT:
                {
                    if (task->tp_Task.tc_SigWait & signals)
                    {
                        myRemovePPC(PowerPCBase, (struct Node*)task);
                        task->tp_Task.tc_State = TS_READY;
                        InsertOnPri(PowerPCBase, (struct List*)&PowerPCBase->pp_ReadyTasks, task);
                        FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
                        struct TaskPPC* topTask = (struct TaskPPC*)PowerPCBase->pp_ReadyTasks.mlh_Head;

                        if (topTask == task)
                        {
                            PowerPCBase->pp_FlagReschedule = -1;
                            CauseDECInterrupt(PowerPCBase);
                        }
                        return;
                    }
                    break;
                }
                case TS_CHANGING:
                {
                    task->tp_Task.tc_State = TS_RUN;
                    break;
                }
            }
            FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
        }
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG myWaitPPC(struct PrivatePPCBase* PowerPCBase, ULONG signals)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return maskSig;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG mySetTaskPriPPC(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* task, LONG pri)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return oldPri;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySignal68K(struct PrivatePPCBase* PowerPCBase, struct Task* task, ULONG signals)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    struct ExcInfo* excInfo;
    ULONG value;
    APTR excdata;

    if (!(excInfo = AllocVec68K(PowerPCBase, sizeof(struct ExcInfo), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        printDebug(PowerPCBase, (struct DebugArgs*)&args);
        return NULL;
    }

    if (!(value = myGetTagDataPPC(PowerPCBase, EXCATTR_CODE, 0, taglist)))
    {
        FreeVec68K(PowerPCBase, excInfo);
        printDebug(PowerPCBase, (struct DebugArgs*)&args);
        return NULL;
    }
    excInfo->ei_ExcData.ed_Code = (APTR)value;
    value = myGetTagDataPPC(PowerPCBase, EXCATTR_FLAGS, 0, taglist);
    if (!(value & (EXCF_SMALLCONTEXT | EXCF_LARGECONTEXT)))
    {
        FreeVec68K(PowerPCBase, excInfo);
        printDebug(PowerPCBase, (struct DebugArgs*)&args);
        return NULL;
    }           
    if (!(value & (EXCF_GLOBAL | EXCF_LOCAL)))
    {
        FreeVec68K(PowerPCBase, excInfo);
        printDebug(PowerPCBase, (struct DebugArgs*)&args);
        return NULL;
    }
    if (value & EXCF_GLOBAL)
    {
        excInfo->ei_ExcData.ed_Task = (struct TaskPPC*)myGetTagDataPPC(PowerPCBase, EXCATTR_TASK, 0, taglist);
    }
    else
    {
        excInfo->ei_ExcData.ed_Task = PowerPCBase->pp_ThisPPCProc;
    }
    excInfo->ei_ExcData.ed_Flags = value;
    excInfo->ei_ExcData.ed_Data = myGetTagDataPPC(PowerPCBase, EXCATTR_DATA, 0, taglist);
    excInfo->ei_ExcData.ed_Node.ln_Name = (APTR)myGetTagDataPPC(PowerPCBase, EXCATTR_NAME, 0, taglist);
    excInfo->ei_ExcData.ed_Node.ln_Pri = (BYTE)myGetTagDataPPC(PowerPCBase, EXCATTR_PRI, 0, taglist);
    excInfo->ei_ExcData.ed_Node.ln_Type = NT_INTERRUPT;
    excInfo->ei_ExcData.ed_RemovalTime = myGetTagDataPPC(PowerPCBase, EXCATTR_TIMEDREMOVAL, 0, taglist);
    value = myGetTagDataPPC(PowerPCBase, EXCATTR_EXCID, 0, taglist);
    excInfo->ei_ExcData.ed_ExcID = value;

    if (!(value))
    {
        FreeVec68K(PowerPCBase, excInfo);
        printDebug(PowerPCBase, (struct DebugArgs*)&args);
        return NULL;
    }

    ULONG flag = 0;

    while (1)
    {
        struct ExcData* newData;

        if (value & EXCF_MCHECK)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (struct Node*)&excInfo->ei_MachineCheck, EXCF_MCHECK);
        }
        if (value & EXCF_DACCESS)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (struct Node*)&excInfo->ei_DataAccess, EXCF_DACCESS);
        }
        if (value & EXCF_IACCESS)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (struct Node*)&excInfo->ei_InstructionAccess, EXCF_IACCESS);
        }
        if (value & EXCF_INTERRUPT)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (struct Node*)&excInfo->ei_Interrupt, EXCF_INTERRUPT);
        }
        if (value & EXCF_ALIGN)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (struct Node*)&excInfo->ei_Alignment, EXCF_ALIGN);
        }
        if (value & EXCF_PROGRAM)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (struct Node*)&excInfo->ei_Program, EXCF_PROGRAM);
        }
        if (value & EXCF_FPUN)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (struct Node*)&excInfo->ei_FPUnavailable, EXCF_FPUN);
        }
        if (value & EXCF_DECREMENTER)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (struct Node*)&excInfo->ei_Decrementer, EXCF_DECREMENTER);
        }
        if (value & EXCF_SC)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (struct Node*)&excInfo->ei_SystemCall, EXCF_SC);
        }
        if (value & EXCF_TRACE)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (struct Node*)&excInfo->ei_Trace, EXCF_TRACE);
        }
        if (value & EXCF_PERFMON)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (struct Node*)&excInfo->ei_PerfMon, EXCF_PERFMON);
        }
        if (value & EXCF_IABR)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (struct Node*)&excInfo->ei_IABR, EXCF_IABR);
        }
        break;
    }

    if (flag)
    {
        FreeAllExcMem(PowerPCBase, excInfo);
        FreeVec68K(PowerPCBase, excInfo);
        printDebug(PowerPCBase, (struct DebugArgs*)&args);
        return NULL;
    }

    CauseDECInterrupt(PowerPCBase);

    while (!(excInfo->ei_ExcData.ed_Flags & EXCF_ACTIVE));

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return excInfo;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myRemExcHandler(struct PrivatePPCBase* PowerPCBase, APTR xlock)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    if (!(xlock))
    {
        return;
    }
    while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

    myAddHeadPPC(PowerPCBase,(struct List*)&PowerPCBase->pp_RemovedExc, (struct Node*)xlock);

    FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

    CauseDECInterrupt(PowerPCBase);

    struct ExcInfo* myInfo = xlock;

    while (myInfo->ei_ExcData.ed_Flags & EXCF_ACTIVE);

    FreeAllExcMem(PowerPCBase, myInfo);

    FreeVec68K(PowerPCBase, xlock);

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
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    ULONG status = HW_AVAILABLE;

    if (flags == HW_TRACEON) //case will result in a need for SDA_BASE
    {
        ULONG key = mySuper(PowerPCBase);
        setMSR(getMSR() | PSL_SE);
        myUser(PowerPCBase, key);
    }
    else if (flags == HW_TRACEOFF)
    {
        ULONG key = mySuper(PowerPCBase);
        setMSR(getMSR() & ~PSL_SE);
        myUser(PowerPCBase, key);
    }
    else if (flags == HW_BRANCHTRACEON)
    {
        ULONG key = mySuper(PowerPCBase);
        setMSR(getMSR() | PSL_BE);
        myUser(PowerPCBase, key);
    }
    else if (flags == HW_BRANCHTRACEOFF)
    {
        ULONG key = mySuper(PowerPCBase);
        setMSR(getMSR() & ~PSL_SE);
        myUser(PowerPCBase, key);
    }
    else if (flags == HW_FPEXCON)
    {
        ULONG key = mySuper(PowerPCBase);
        SetFPExc();
        setMSR(getMSR() | PSL_FE0 | PSL_FE1);
        myUser(PowerPCBase, key);
    }
    else if (flags == HW_FPEXCOFF)
    {
        ULONG key = mySuper(PowerPCBase);
        setMSR(getMSR() & ~(PSL_FE0 | PSL_FE1));
        myUser(PowerPCBase, key);
    }
    else if (flags == HW_SETIBREAK)
    {
        ULONG address = (ULONG)param;
        address = (address & -4) | 3;
        ULONG key = mySuper(PowerPCBase);
        setIABR(address);
        myUser(PowerPCBase, key);
    }
    else if (flags == HW_CLEARIBREAK)
    {
        ULONG key = mySuper(PowerPCBase);
        setIABR(0);
        myUser(PowerPCBase, key);
    }
    else if (flags == HW_SETDBREAK)
    {
        ULONG address = (ULONG)param;
        address = (address & -8) | 7;
        ULONG key = mySuper(PowerPCBase);
        setDABR(address);
        myUser(PowerPCBase, key);
    }
    else if (flags == HW_CLEARDBREAK)
    {
        ULONG key = mySuper(PowerPCBase);
        setDABR(0);
        myUser(PowerPCBase, key);
    }
    else if (flags == HW_CPUTYPE) //private
    {
        status = PowerPCBase->pp_CPUInfo;
    }
    else if (flags == HW_SETDEBUGMODE) //private
    {
        ULONG level = (ULONG)param;
        PowerPCBase->pp_DebugLevel = level;
    }
    else if (flags == HW_PPCSTATE) //private
    {
        status = PPCSTATEF_POWERSAVE;
        if (PowerPCBase->pp_WaitingTasks.mlh_Head->mln_Succ)
        {
            status = PPCSTATEF_APPACTIVE;
        }
        if (PowerPCBase->pp_ReadyTasks.mlh_Head->mln_Succ)
        {
            status = PPCSTATEF_APPACTIVE;
        }
    }
    else
    {
        status = HW_NOTAVAILABLE;
    }

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return status;
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
    struct UInt64 myInt64;
    struct WaitTime myWait;
    ULONG value1, value2, counter, tbu, tbl, recvdsig;
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemWaitList);

    PowerPCBase->pp_FlagWait = -1;

    value2 = 60000000;

    mulInt64((struct UInt64*)&myInt64, PowerPCBase->pp_BusClock, value2);
    value1 = Calculator(myInt64.ui_High, myInt64.ui_Low, 4000000);            //Magic

    counter = time / value2;
    value2 = counter * value2;
    value2 = time - value2;

    mulInt64((struct UInt64*)&myInt64, PowerPCBase->pp_BusClock, value2);
    value2 = Calculator(myInt64.ui_High, myInt64.ui_Low, 4000000);            //Magic

    while (1)
    {
        tbu = getTBU();
        tbl = getTBL();
        if (tbu == getTBU())
        {
            break;
        }
    }

    FinalCalc(counter, tbu, tbl, value1, value2, (struct WaitTime*)&myWait);  //Magic

    myWait.wt_Task = PowerPCBase->pp_ThisPPCProc;
    myWait.wt_Node.ln_Pri = 0;
    myWait.wt_Node.ln_Type = NT_UNKNOWN;
    myWait.wt_Node.ln_Name = PowerPCBase->pp_ThisPPCProc->tp_Task.tc_Node.ln_Name;

    myAddHeadPPC(PowerPCBase, (struct List*)&PowerPCBase->pp_WaitTime, (struct Node*)&myWait);

    PowerPCBase->pp_FlagWait = 0;

    myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemWaitList);

    recvdsig = myWaitPPC(PowerPCBase, signals | SIGF_WAIT);

    PowerPCBase->pp_FlagWait = -1;

    while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

    if (!((PowerPCBase->pp_ThisPPCProc->tp_Task.tc_SigRecvd | recvdsig) & SIGF_WAIT))
    {
        myRemovePPC(PowerPCBase, (struct Node*)&myWait);
    }
    else
    {
        PowerPCBase->pp_ThisPPCProc->tp_Task.tc_SigRecvd &= ~SIGF_WAIT;
    }

    FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

    PowerPCBase->pp_FlagWait = 0;

    recvdsig &= ~SIGF_WAIT;

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return recvdsig;
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
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return; //stub

    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

    switch (mode)
    {
        case CHMMU_STANDARD:
        {
            myTask->tp_Flags &= ~MMUF_BAT;
            GetBATs(PowerPCBase);
            break;
        }
        case CHMMU_BAT:
        {
            myTask->tp_Flags |= MMUF_BAT;
            StoreBATs(PowerPCBase);
            break;
        }
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myGetInfo(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	struct TagItem* myTagItem = NULL;

    struct TagItemPtr tagPtr;
    tagPtr.tip_TagItem = taglist;

	while (myTagItem = myNextTagItemPPC(PowerPCBase, (struct TagItem**)&tagPtr))
    {
        if ((myTagItem->ti_Tag & 0xffffe000) == GETINFO_TAGS)
		{
            switch (myTagItem->ti_Tag)
            {
                case GETINFO_CPU:
                {
                    if(PowerPCBase->pp_DeviceID == DEVICE_MPC8343E)
                    {
                        myTagItem->ti_Data = CPUF_603E;
                    }
                    else
                    {
                        myTagItem->ti_Data = 0;
                    }
                    break;
                }
                case GETINFO_PVR:
                {
                    myTagItem->ti_Data = PowerPCBase->pp_CPUInfo;
                    break;
                }
                case GETINFO_ICACHE:
                case GETINFO_DCACHE:
                {
                    ULONG shift;
                    if (myTagItem->ti_Tag == GETINFO_DCACHE)
                    {
                        shift = 12;
                    }
                    else
                    {
                        shift = 13;
                    }
                    ULONG state = (PowerPCBase->pp_CPUHID0 >> shift) & 0x7;
                    switch (state)
                    {
                       case 4:
                       {
                           myTagItem->ti_Data = CACHEF_ON_UNLOCKED;
                           break;
                       }
                       case 5:
                       {
                           myTagItem->ti_Data = CACHEF_ON_LOCKED;
                           break;
                       }
                       case 0:
                       {
                           myTagItem->ti_Data = CACHEF_OFF_UNLOCKED;
                           break;
                       }
                       default:
                       {
                           myTagItem->ti_Data = CACHEF_OFF_LOCKED;
                           break;
                       }
                    }
                    break;
                }
                case GETINFO_PAGETABLE:
                {
                    myTagItem->ti_Data = PowerPCBase->pp_CPUSDR1 & 0xffff0000;
                    break;
                }
                case GETINFO_TABLESIZE:
                {
                    myTagItem->ti_Data = PowerPCBase->pp_PageTableSize;
                    break;
                }
                case GETINFO_BUSCLOCK:
                {
                    myTagItem->ti_Data = PowerPCBase->pp_BusClock;
                    break;
                }
                case GETINFO_CPUCLOCK:
                {
                    myTagItem->ti_Data = PowerPCBase->pp_CPUSpeed;
                    break;
                }
                case GETINFO_CPULOAD:
                {
                    myTagItem->ti_Data = PowerPCBase->pp_CPULoad;
                    break;
                }
                case GETINFO_SYSTEMLOAD:
                {
                    myTagItem->ti_Data = PowerPCBase->pp_SystemLoad;
                    break;
                }
                case GETINFO_L2STATE:
                {
                    myTagItem->ti_Data = PowerPCBase->pp_L2State;
                    break;
                }
                case GETINFO_L2SIZE:
                {
                    myTagItem->ti_Data = PowerPCBase->pp_L2Size;
                    break;
                }
            }
		}
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct MsgPortPPC* myCreateMsgPortPPC(struct PrivatePPCBase* PowerPCBase)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return myPort;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myDeleteMsgPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	myNewListPPC(PowerPCBase, (struct List*)&port->mp_Port.mp_MsgList);

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

	myEnqueuePPC(PowerPCBase, (struct List*)&PowerPCBase->pp_Ports, (struct Node*)port);

	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myRemPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

	myRemovePPC(PowerPCBase, (struct Node*)port);

	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct MsgPortPPC* myFindPortPPC(struct PrivatePPCBase* PowerPCBase, STRPTR name)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

	struct MsgPortPPC* myPort = (struct MsgPortPPC*)myFindNamePPC(PowerPCBase, (struct List*)&PowerPCBase->pp_Ports, name);

	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return myPort;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct Message* myWaitPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&port->mp_Semaphore);

    volatile struct Message* topMsg = (struct Message*)port->mp_Port.mp_MsgList.lh_Head;

    while (!(topMsg->mn_Node.ln_Succ))
    {
        ULONG waitSig = 1 << (port->mp_Port.mp_SigBit);

        myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&port->mp_Semaphore);

        myWaitPPC(PowerPCBase, waitSig);

        myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&port->mp_Semaphore);

        topMsg = (struct Message*)port->mp_Port.mp_MsgList.lh_Head;
    }

    myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&port->mp_Semaphore);

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return (struct Message*)topMsg;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myPutMsgPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port, struct Message* message)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    message->mn_Node.ln_Type = NT_MESSAGE;

    myAddTailPPC(PowerPCBase, (struct List*)&port->mp_Port.mp_MsgList, (struct Node*)message);

    if ((port->mp_Port.mp_SigTask) && (!(port->mp_Port.mp_Flags & PF_ACTION)))
    {
         mySignalPPC(PowerPCBase, port->mp_Port.mp_SigTask, (1 << port->mp_Port.mp_SigBit));
    }

    myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&port->mp_Semaphore);

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct Message* myGetMsgPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&port->mp_Semaphore);

    struct Message* myMsg = (struct Message*)myRemHeadPPC(PowerPCBase, (struct List*)&port->mp_Port.mp_MsgList);

    myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&port->mp_Semaphore);

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return myMsg;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myReplyMsgPPC(struct PrivatePPCBase* PowerPCBase, struct Message* message)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    if (!(message->mn_ReplyPort))
    {
        message->mn_Node.ln_Type = NT_FREEMSG;
        return;
    }

    if (message->mn_Node.ln_Type = NT_XMSG68K)
    {
        message->mn_Node.ln_Type = NT_REPLYMSG;
        struct MsgFrame* myFrame = CreateMsgFramePPC(PowerPCBase);
        myFrame->mf_Arg[0] = (ULONG)message;
        myFrame->mf_Identifier = ID_RX68;
        SendMsgFramePPC(PowerPCBase, myFrame);
        return;
    }
    else
    {
        struct MsgPortPPC* myPort = (struct MsgPortPPC*)message->mn_ReplyPort;
        myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&myPort->mp_Semaphore);
        myAddTailPPC(PowerPCBase, (struct List*)&myPort->mp_Port.mp_MsgList, (struct Node*)message);

        if ((myPort->mp_Port.mp_SigTask) && (!(myPort->mp_Port.mp_Flags & PF_ACTION)))
        {
             mySignalPPC(PowerPCBase, myPort->mp_Port.mp_SigTask, (1 << myPort->mp_Port.mp_SigBit));
        }

        myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&myPort->mp_Semaphore);
    }
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
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    CopyMemQuickPPC(PowerPCBase, source, dest, size);

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct Message* myAllocXMsgPPC(struct PrivatePPCBase* PowerPCBase, ULONG length, struct MsgPortPPC* port)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	length = (length + sizeof(struct Message) + 31) & -32;

	struct Message* myXMessage;

	if (myXMessage = myAllocVecPPC(PowerPCBase, length, MEMF_PUBLIC | MEMF_CLEAR | MEMF_PPC, 32L))
	{
		myXMessage->mn_ReplyPort = (struct MsgPort*)port;
		myXMessage->mn_Length = length;
	}


    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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

PPCFUNCTION VOID  myGetSysTimePPC(struct PrivatePPCBase* PowerPCBase, struct timeval* timeval)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);
    struct UInt64 myInt64;
    ULONG tbu, tbl, secs, micros, temp;

    while (1)
    {
        tbu = getTBU();
        tbl = getTBL();
        if (tbu == getTBU())
        {
            break;
        }
    }
    ULONG busClockMod = PowerPCBase->pp_BusClock >> 2;

    secs = Calculator(tbu, tbl, busClockMod);
    timeval->tv_secs = secs;

    temp = secs * busClockMod;
    temp = tbl - temp;

    mulInt64((struct UInt64*)&myInt64, 1000000, temp);

    micros = Calculator(myInt64.ui_High, myInt64.ui_Low, busClockMod);

    timeval->tv_micro = micros;

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myAddTimePPC(struct PrivatePPCBase* PowerPCBase, struct timeval* dest, struct timeval* source)
{
    ULONG timeSecs = 0;
    LONG timeMicro = dest->tv_micro + source->tv_micro;
    if (1000000 <= timeMicro)
    {
        timeSecs = 1;
        timeMicro -= 1000000;
    }
    dest->tv_secs = dest->tv_secs + source->tv_secs + timeSecs;
    dest->tv_micro = timeMicro;

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySubTimePPC(struct PrivatePPCBase* PowerPCBase, struct timeval* dest, struct timeval* source)
{
    ULONG timeSecs = 0;
    LONG timeMicro = dest->tv_micro + source->tv_micro;
    if (timeMicro < 0)
    {
        timeSecs = 1;
        timeMicro += 1000000;
    }
    dest->tv_secs = dest->tv_secs - source->tv_secs - timeSecs;
    dest->tv_micro = timeMicro;

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myCmpTimePPC(struct PrivatePPCBase* PowerPCBase, struct timeval* dest, struct timeval* source)
{
    if (dest->tv_secs < source->tv_secs)
    {
        return CMP_DESTLESS;
    }
    else if (dest->tv_secs > source->tv_secs)
    {
        return CMP_DESTGREATER;
    }
    else
    {
        if (dest->tv_micro < source->tv_micro)
        {
            return CMP_DESTLESS;
        }
        else if (dest->tv_micro > source->tv_micro)
        {
            return CMP_DESTGREATER;
        }
        else
        {
            return CMP_EQUAL;
        }
    }
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct MsgPortPPC* mySetReplyPortPPC(struct PrivatePPCBase* PowerPCBase, struct Message* message, struct MsgPortPPC* port)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	struct MsgPortPPC* oldPort = (struct MsgPortPPC*)message->mn_ReplyPort;
	message->mn_ReplyPort = (struct MsgPort*)port;

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return oldPort;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG mySnoopTask(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    ULONG value;

    struct SnoopData* mySnoop = AllocVec68K(PowerPCBase, sizeof(struct SnoopData), MEMF_PUBLIC | MEMF_CLEAR);

    if (mySnoop)
    {
        if (value = myGetTagDataPPC(PowerPCBase, SNOOP_CODE, 0, taglist))
        {
            mySnoop->sd_Code = (APTR)value;
            mySnoop->sd_Data = myGetTagDataPPC(PowerPCBase, SNOOP_DATA, 0, taglist);
            if(!(mySnoop->sd_Type = myGetTagDataPPC(PowerPCBase, SNOOP_TYPE, 0, taglist)))
            {
                FreeVec68K(PowerPCBase, mySnoop);
                printDebug(PowerPCBase, (struct DebugArgs*)&args);
                return 0;
            }
            myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);

            myAddHeadPPC(PowerPCBase, (struct List*)&PowerPCBase->pp_Snoop, (struct Node*)mySnoop);

            myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);

            printDebug(PowerPCBase, (struct DebugArgs*)&args);

            return (ULONG)mySnoop;

        }
        else
        {
            FreeVec68K(PowerPCBase, mySnoop);
        }
    }
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return 0;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myEndSnoopTask(struct PrivatePPCBase* PowerPCBase, ULONG id)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    if (id)
    {
        myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);

        myRemovePPC(PowerPCBase, (struct Node*)id);

        myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);

        FreeVec68K(PowerPCBase, (APTR)id);
    }

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myGetHALInfo(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    struct TagItem* myTagItem;

    if (myTagItem = myFindTagItemPPC(PowerPCBase,HINFO_ALEXC_HIGH, taglist))
    {
        myTagItem->ti_Data = PowerPCBase->pp_AlignmentExcHigh;
    }
    if (myTagItem = myFindTagItemPPC(PowerPCBase,HINFO_ALEXC_LOW, taglist))
    {
        myTagItem->ti_Data = PowerPCBase->pp_AlignmentExcLow;
    }
    if (myTagItem = myFindTagItemPPC(PowerPCBase,HINFO_DSEXC_HIGH, taglist))
    {
        myTagItem->ti_Data = PowerPCBase->pp_DataExcHigh;
    }
    if (myTagItem = myFindTagItemPPC(PowerPCBase,HINFO_DSEXC_LOW, taglist))
    {
        myTagItem->ti_Data = PowerPCBase->pp_DataExcLow;
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySetScheduling(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    struct TagItem* myTagItem;
    ULONG result;

    if (myTagItem = myFindTagItemPPC(PowerPCBase, SCHED_REACTION, taglist))
    {
        result = myTagItem->ti_Data;
        if (result < 1)
        {
            result = 1;
        }
        else if (result > 20)
        {
            result = 20;
        }
        PowerPCBase->pp_LowActivityPri = (result * 1000);
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct TaskPPC* myFindTaskByID(struct PrivatePPCBase* PowerPCBase, LONG id)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return foundTask;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG mySetNiceValue(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* task, LONG nice)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return OldSignals;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myObtainSemaphoreSharedPPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);
    struct SemWait myWait;

    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

    while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

    if (!(SemaphorePPC->ssppc_SS.ss_QueueCount += 1))
    {
        FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
        SemaphorePPC->ssppc_SS.ss_NestCount += 1;
    }
    else
    {
        if ((!(SemaphorePPC->ssppc_SS.ss_Owner)) || (SemaphorePPC->ssppc_SS.ss_Owner == (struct Task*)myTask))
        {
            FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
            SemaphorePPC->ssppc_SS.ss_NestCount += 1;
        }
        else
        {
            myWait.sw_Task = (struct TaskPPC*)((ULONG)myTask | SM_SHARED);
            myTask->tp_Task.tc_SigRecvd &= ~(SIGF_SINGLE);
            myAddTailPPC(PowerPCBase, (struct List*)&SemaphorePPC->ssppc_SS.ss_WaitQueue, (struct Node*)&myWait);
            FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
            myWaitPPC(PowerPCBase, SIGF_SINGLE);
        }
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAttemptSemaphoreSharedPPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);
    LONG result;

    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

    while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

    if (!(SemaphorePPC->ssppc_SS.ss_QueueCount + 1) || !(SemaphorePPC->ssppc_SS.ss_Owner) || (SemaphorePPC->ssppc_SS.ss_Owner == (struct Task*)myTask))
    {
        SemaphorePPC->ssppc_SS.ss_QueueCount += 1;
        SemaphorePPC->ssppc_SS.ss_NestCount += 1;
        result = ATTEMPT_SUCCESS;
    }
    else
    {
        result = ATTEMPT_FAILURE;
    }

    FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myProcurePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, struct SemaphoreMessage* SemaphoreMessage)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    SemaphoreMessage->ssm_Semaphore = (struct SignalSemaphore*)PowerPCBase->pp_ThisPPCProc;

    struct TaskPPC* owner = PowerPCBase->pp_ThisPPCProc;

    ULONG myStore = (ULONG)&SemaphoreMessage->ssm_Message.mn_Node.ln_Type;

    if (*((ULONG*)(myStore)) = (ULONG)SemaphoreMessage->ssm_Message.mn_Node.ln_Name)
    {
        owner = NULL;
    }

    while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

    if ((SemaphorePPC->ssppc_SS.ss_QueueCount += 1) && (SemaphorePPC->ssppc_SS.ss_Owner))
    {
        myAddTailPPC(PowerPCBase, (struct List*)&SemaphorePPC->ssppc_SS.ss_WaitQueue, (struct Node*)SemaphoreMessage);
        FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
    }
    else
    {
        if (!(SemaphorePPC->ssppc_SS.ss_QueueCount += 1))
        {
            SemaphorePPC->ssppc_SS.ss_Owner = (struct Task*)PowerPCBase->pp_ThisPPCProc;
        }
        SemaphoreMessage->ssm_Semaphore = (struct SignalSemaphore*)SemaphorePPC;
        SemaphoreMessage->ssm_Message.mn_Node.ln_Type = 0;
        SemaphorePPC->ssppc_SS.ss_NestCount += 1;
        FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
        myReplyMsgPPC(PowerPCBase, (struct Message*)SemaphoreMessage);
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myVacatePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, struct SemaphoreMessage* SemaphoreMessage)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    ULONG myStore = (ULONG)&SemaphoreMessage->ssm_Message.mn_Node.ln_Type;
    myStore = 0;
    SemaphoreMessage->ssm_Semaphore = 0;

    while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

    struct Node* currMsg = (struct Node*)SemaphorePPC->ssppc_SS.ss_WaitQueue.mlh_Head;

    while (1)
    {
        if (currMsg == (struct Node*)SemaphoreMessage)
        {
            SemaphorePPC->ssppc_SS.ss_QueueCount -= 1;
            myRemovePPC(PowerPCBase, currMsg);
            FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
            myReplyMsgPPC(PowerPCBase, (struct Message*)SemaphoreMessage);
            break;
        }
        else
        {
            if(!(currMsg = currMsg->ln_Succ))
            {
                FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
                myReleaseSemaphorePPC(PowerPCBase, SemaphorePPC);
                break;
            }
        }
    }
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
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	struct poolHeader* myPoolHeader = 0;

	if (puddle_size >= thres_size)
	{
		if (myPoolHeader = AllocVec68K(PowerPCBase, sizeof(struct poolHeader), flags))
		{
			puddle_size = (puddle_size + 31) & -32;
			myNewListPPC(PowerPCBase, (struct List*)&myPoolHeader->ph_PuddleList);
			myNewListPPC(PowerPCBase, (struct List*)&myPoolHeader->ph_BlockList);

			myPoolHeader->ph_Requirements = flags;
			myPoolHeader->ph_PuddleSize = puddle_size;
			myPoolHeader->ph_ThresholdSize = thres_size;

			myAddHeadPPC(PowerPCBase, &PowerPCBase->pp_ThisPPCProc->tp_TaskPools, (struct Node*)&myPoolHeader->ph_Node);
		}
	}
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return myPoolHeader;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myDeletePoolPPC(struct PrivatePPCBase* PowerPCBase, APTR poolheader)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION APTR myAllocPooledPPC(struct PrivatePPCBase* PowerPCBase, APTR poolheader, ULONG size)
{
    struct DebugArgs args;
    struct Node* memBlock;
    struct MemHeader* puddle;
    APTR mem = NULL;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    struct poolHeader* myHeader = (struct poolHeader*)poolheader;

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemMemory);

    if (myHeader->ph_ThresholdSize < size)
    {
        if (memBlock = AllocVec68K(PowerPCBase, size+32, myHeader->ph_Requirements))
        {
            memBlock->ln_Type = NT_MEMORY;
            myAddHeadPPC(PowerPCBase, (struct List*)&myHeader->ph_BlockList, memBlock);
            mem = (APTR)((ULONG)memBlock + 32);
            (*((ULONG*)((ULONG)mem - 4))) = size + 32;
        }
    }
    else
    {
        if (!(puddle = (struct MemHeader*)myRemHeadPPC(PowerPCBase, (struct List*)&myHeader->ph_PuddleList)))
        {
            if (puddle = AllocVec68K(PowerPCBase, myHeader->ph_PuddleSize + sizeof(struct MemHeader), myHeader->ph_Requirements))
            {
                struct MemChunk* startPuddle = (struct MemChunk*)((ULONG)puddle + sizeof(struct MemHeader));
                puddle->mh_First = startPuddle;
                puddle->mh_Lower = startPuddle;
                puddle->mh_Node.ln_Type = NT_MEMORY;
                puddle->mh_Attributes = myHeader->ph_Requirements;
                startPuddle->mc_Next = NULL;
                startPuddle->mc_Bytes = myHeader->ph_PuddleSize;
                puddle->mh_Free = myHeader->ph_PuddleSize;
                puddle->mh_Upper = (APTR)((ULONG)startPuddle + myHeader->ph_PuddleSize);
            }
        }
        if (puddle)
        {
            if (!(mem = AllocatePPC(PowerPCBase, puddle, size + 32)))
            {
                myAddHeadPPC(PowerPCBase, (struct List*)&myHeader->ph_PuddleList, (struct Node*)puddle);

                if (puddle = AllocVec68K(PowerPCBase, myHeader->ph_PuddleSize + sizeof(struct MemHeader), myHeader->ph_Requirements))
                {
                    struct MemChunk* startPuddle = (struct MemChunk*)((ULONG)puddle + sizeof(struct MemHeader));
                    puddle->mh_First = startPuddle;
                    puddle->mh_Lower = startPuddle;
                    puddle->mh_Node.ln_Type = NT_MEMORY;
                    puddle->mh_Attributes = myHeader->ph_Requirements;
                    startPuddle->mc_Next = NULL;
                    startPuddle->mc_Bytes = myHeader->ph_PuddleSize;
                    puddle->mh_Free = myHeader->ph_PuddleSize;
                    puddle->mh_Upper = (APTR)((ULONG)startPuddle + myHeader->ph_PuddleSize);
                    mem = AllocatePPC(PowerPCBase, puddle, size + 32);
                 }
            }
            if (puddle)
            {
                ULONG divider = (myHeader->ph_PuddleSize >> 8) & 0xff;
                ULONG granularity = (puddle->mh_Free / divider) + 0x80;
                puddle->mh_Node.ln_Pri = (UBYTE)granularity;
                myEnqueuePPC(PowerPCBase, (struct List*)&myHeader->ph_PuddleList, (struct Node*)puddle);
            }
            if (mem)
            {
                (*((ULONG*)((ULONG)mem - 4))) = size + 32;
            }
        }
    }

    myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemMemory);

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return mem;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myFreePooledPPC(struct PrivatePPCBase* PowerPCBase, APTR poolheader, APTR ptr, ULONG size)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);
    ULONG realsize;
    struct poolHeader* myHeader = poolheader;

    if (!(ptr))
    {
        return;
    }
    if (!(size))
    {
        realsize = (*((ULONG*)((ULONG)ptr - 4)));
        size = realsize - 32;
    }

    ULONG mem = (ULONG)ptr - 32;

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemMemory);

    if (myHeader->ph_PuddleSize < size)
    {
        struct Node* memBlock = (struct Node*)(mem);
        myRemovePPC(PowerPCBase, memBlock);
        FreeVec68K(PowerPCBase, (APTR)memBlock);
    }
    else
    {
        struct MemHeader* currPuddle = (struct MemHeader*)myHeader->ph_PuddleList.mlh_Head;
        struct MemHeader* nextPuddle;

        while (nextPuddle = (struct MemHeader*)currPuddle->mh_Node.ln_Succ)
        {
            if ((mem >= (ULONG)currPuddle->mh_Lower) && (mem < (ULONG)currPuddle->mh_Upper))
            {
                DeallocatePPC(PowerPCBase, currPuddle, (APTR)mem, realsize);

                myRemovePPC(PowerPCBase, (struct Node*)currPuddle);

                if (currPuddle->mh_Free == myHeader->ph_PuddleSize)
                {
                    FreeVec68K(PowerPCBase, currPuddle);
                }
                else
                {
                    ULONG divider = (myHeader->ph_PuddleSize >> 8) & 0xff;
                    ULONG granularity = (currPuddle->mh_Free / divider) + 0x80;
                    currPuddle->mh_Node.ln_Pri = (UBYTE)granularity;
                    myEnqueuePPC(PowerPCBase, (struct List*)&myHeader->ph_PuddleList, (struct Node*)currPuddle);
                }
                break;
            }
            currPuddle = nextPuddle;
        }
    }
    myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemMemory);
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
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    LONG status = PUBMSG_SUCCESS;

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

    struct MsgPortPPC* myPort = myFindPortPPC(PowerPCBase, portname);

    if (myPort)
    {
        myPutMsgPPC(PowerPCBase, myPort, message);
    }
    else
    {
        status = PUBMSG_NOPORT;
    }

    myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return status;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAddUniquePortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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
		
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAddUniqueSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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
	
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

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
    BOOL result;
    APTR newStack;
    ULONG src, dst, siz, sup;
    struct MemList* stackMem;
    struct DebugArgs args;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    if (NewStackSize < PowerPCBase->pp_ThisPPCProc->tp_StackSize)
    {
        return FALSE;
    }

    if (!(newStack = AllocVec68K(PowerPCBase, NewStackSize, MEMF_PUBLIC|MEMF_CLEAR|MEMF_PPC)))
    {
        return FALSE;
    }

    if (stackMem = AllocVec68K(PowerPCBase, sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR|MEMF_PPC))
    {
        struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;
        stackMem->ml_NumEntries = 1;
        stackMem->ml_ME[0].me_Un.meu_Addr = newStack;
        stackMem->ml_ME[0].me_Length = NewStackSize;

        myAddHeadPPC(PowerPCBase, (struct List*)&myTask->tp_Task.tc_MemEntry, (struct Node*)stackMem);

        src = (ULONG)myTask->tp_Task.tc_SPLower;
        sup = (ULONG)myTask->tp_Task.tc_SPUpper;
        siz = sup - src;
        dst = (ULONG)newStack + NewStackSize - siz;

        myCopyMemPPC(PowerPCBase, (APTR)src, (APTR)dst, siz);

        myTask->tp_Task.tc_SPLower = (APTR)dst;
        myTask->tp_Task.tc_SPUpper = (APTR)((ULONG)newStack + NewStackSize);
        myTask->tp_StackSize = NewStackSize;

        SwapStack(sup, ((ULONG)newStack + NewStackSize));

        result = TRUE;
    }
    else
    {
        FreeVec68K(PowerPCBase, stackMem);
        result = FALSE;
    }

    printDebug(PowerPCBase, (struct DebugArgs*)&args);
    return result;
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

