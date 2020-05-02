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

#pragma pack(push,2)
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
#pragma pack(pop)

/********************************************************************************************
*
*        LONG = Run68K(struct Library* PowerPCBase, struct PPCArgs* PPStruct)
*
*********************************************************************************************/

PPCFUNCTION LONG myRun68K(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct PPCArgs* PPStruct)
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
    args.db_Function = 0 | (3<<8) | (1<<16) | (1<<17);
    args.db_Arg[0] = (ULONG)PPStruct;
    args.db_Arg[1] = (ULONG)PPStruct->PP_Code;            //Extra info
    args.db_Arg[2] = PPStruct->PP_Offset;                 //Extra info
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	if (!(PPStruct->PP_Code))
	{
		HaltError(ERR_R68K);
	}
	if (PPStruct->PP_Flags)
	{
		HaltError(ERR_ESNC);
	}

    PowerPCBase->pp_NumRun68k += 1;

	struct MsgFrame* myFrame = CreateMsgFramePPC(PowerPCBase);

    myCopyMemPPC(PowerPCBase, (APTR)PPStruct, (APTR)&myFrame->mf_PPCArgs, sizeof(struct PPCArgs));

	struct PrivateTask* privTask = (struct PrivateTask*)PowerPCBase->pp_ThisPPCProc;
    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

	myFrame->mf_Identifier = ID_T68K;
	myFrame->mf_PPCTask = (struct TaskPPC*)myTask;
	myFrame->mf_MirrorPort = privTask->pt_MirrorPort;
	myFrame->mf_Arg[0] = myTask->tp_Task.tc_SigAlloc;
    myFrame->mf_Arg[1] = 0;
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


PPCFUNCTION LONG myWaitFor68K(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct PPCArgs* PPStruct)
{
    struct DebugArgs args;
    args.db_Function = 1 | (1<<8) | (1<<16) | (1<<17);
    args.db_Arg[0] = (ULONG)PPStruct;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    ULONG recvdSigs, signals;
    ULONG sigMask = SIGF_DOS;

    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

    while (1)
    {
        recvdSigs = myWaitPPC(PowerPCBase, myTask->tp_Task.tc_SigAlloc & 0xfffff100);
        if (recvdSigs & sigMask)
        {
            if (signals = recvdSigs & ~sigMask)
            {
                while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));
                myTask->tp_Task.tc_SigRecvd |= signals;
                FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
            }
            struct MsgFrame* myFrame;
            while (myFrame = (struct MsgFrame*)myGetMsgPPC(PowerPCBase, myTask->tp_Msgport))
            {
                switch (myFrame->mf_Identifier)
                {
                    case ID_DNLL:
                    {
                        ULONG value = myFrame->mf_PPCArgs.PP_Regs[0];
                        FreeMsgFramePPC(PowerPCBase, myFrame);
                        return (LONG)value;
                    }
                    case ID_END:
                    {
                        KillTask(PowerPCBase, myFrame);
                        break;
                    }
                    case ID_TPPC:
                    {
                        SetupRunPPC(PowerPCBase, myFrame);
                        break;
                    }
                    case ID_DONE:
                    {
                        struct NewTask* myNewTask = (struct NewTask*)myTask;
                        if (!(myNewTask->nt_Task.pt_MirrorPort))
                        {
                            myNewTask->nt_Task.pt_MirrorPort = myFrame->mf_MirrorPort;
                            myNewTask->nt_Task.pt_Mirror68K = (struct Task*)myFrame->mf_Arg[0];
                        }

                        while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));
                        myTask->tp_Task.tc_SigRecvd |= myFrame->mf_Signals;
                        FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

                        myCopyMemPPC(PowerPCBase, (APTR)&myFrame->mf_PPCArgs, (APTR)PPStruct, sizeof(struct PPCArgs));
                        FreeMsgFramePPC(PowerPCBase, myFrame);

                        return PPERR_SUCCESS;
                    }
                    default:
                    {
                        HaltError(ERR_EFIF);
                        break;
                    }
                }
            }

        }
        else
        {
            struct NewTask* myNewTask = (struct NewTask*)myTask;
            struct MsgPort* mirrorPort;
            if ((signals = recvdSigs & ~sigMask) && (mirrorPort = myNewTask->nt_Task.pt_MirrorPort))
            {
                mySignal68K(PowerPCBase, mirrorPort->mp_SigTask, signals);
            }
        }
    }
    return 0;
}

/********************************************************************************************
*
*        VOID mySPrintF (struct Library*, STRPTR, APTR)
*
*********************************************************************************************/

PPCFUNCTION VOID mySPrintF(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") STRPTR Formatstring, __reg("r5") APTR Values)
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

PPCFUNCTION APTR myAllocVecPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG size, __reg("r5") ULONG flags, __reg("r6") ULONG align)
{
    struct DebugArgs args;
    args.db_Function = 2 | (3<<8) | (1<<16) | (2<<17);
    args.db_Arg[0] = size;
    args.db_Arg[1] = flags;
    args.db_Arg[2] = align;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);
    APTR mem = 0;

    flags = (flags & ~MEMF_CHIP) | MEMF_PPC;

    struct poolHeader* currPool = (struct poolHeader*)PowerPCBase->pp_ThisPPCProc->tp_TaskPools.lh_Head;
    struct poolHeader* nextPool;

    while (nextPool = (struct poolHeader*)currPool->ph_Node.mln_Succ)
    {
        if ((currPool->ph_ThresholdSize == 0x40000) && (currPool->ph_Requirements == flags))
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
    if (!(nextPool))
    {
        currPool = myCreatePoolPPC(PowerPCBase, flags, 0x80000, 0x40000);
    }
    if (currPool)
    {
        mem = myAllocPooledPPC(PowerPCBase, currPool, size + align + 32);
    }
    if (mem)
    {
        ULONG origmem = (ULONG)mem;
        mem = (APTR)(((ULONG)mem + 31 + align) & (-align));
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
    args.db_Arg[0] = (ULONG)mem;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return mem;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myFreeVecPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") APTR memblock)
{
    struct DebugArgs args;
    args.db_Function = 3 | (1<<8) | (1<<16) | (2<<17);
    args.db_Arg[0] = (ULONG)memblock;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	if (memblock)
	{
		ULONG* myMemblock = memblock;
		myFreePooledPPC(PowerPCBase, (APTR)myMemblock[-2], (APTR)myMemblock[-3], myMemblock[-1]);
	}
	
    return MEMERR_SUCCESS;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct TaskPPC* myCreateTaskPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct TagItem* taglist)
{
    struct DebugArgs args;
    args.db_Function = 4 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)taglist;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    ULONG value, memStack, myCode;
    LONG longvalue;
    APTR memBATStorage, memName;

    struct TaskPPC* newTask;
    struct MemList* memBATml;
    struct MemList* memNameml;
    struct MemList* memStackml;
    struct MemList* memFrameml;

    struct iframe* memFrame;
    struct MsgPortPPC* memPort;
    struct TaskPtr* taskPtr;

    struct TaskPPC* currTask = PowerPCBase->pp_ThisPPCProc;
    currTask->tp_Flags |= TASKPPCF_CHOWN;

    if (myCode = myGetTagDataPPC(PowerPCBase, TASKATTR_CODE, 0, taglist))
    {
        if (newTask = AllocVec68K(PowerPCBase, sizeof(struct PrivateTask), MEMF_PUBLIC | MEMF_CLEAR))
        {
            newTask->tp_Link.tl_Task = newTask;
            newTask->tp_Link.tl_Sig = 0xfff;
            myNewListPPC(PowerPCBase, (struct List*)&newTask->tp_Task.tc_MemEntry);
            newTask->tp_Task.tc_Node.ln_Type = NT_PPCTASK;
            newTask->tp_Task.tc_Flags = TF_PROCTIME;
            newTask->tp_PowerPCBase = PowerPCBase;
            if (memBATStorage = AllocVec68K(PowerPCBase, sizeof(struct BATArray) * 4, MEMF_PUBLIC | MEMF_CLEAR))
            {
                newTask->tp_BATStorage = memBATStorage;
                if (memBATml = AllocVec68K(PowerPCBase, sizeof(struct MemList), MEMF_PUBLIC | MEMF_CLEAR))
                {
                    memBATml->ml_NumEntries = 1;
                    memBATml->ml_ME[0].me_Un.meu_Addr = memBATStorage;
                    memBATml->ml_ME[0].me_Length = sizeof(struct BATArray) * 4;
                    myAddHeadPPC(PowerPCBase, (struct List*)&newTask->tp_Task.tc_MemEntry, (struct Node*)memBATml);

                    if (value = myGetTagDataPPC(PowerPCBase, TASKATTR_NAME, 0, taglist))
                    {
                        ULONG nameSize = GetLen((STRPTR)value) + 1;
                        if (memName = AllocVec68K(PowerPCBase, nameSize, MEMF_PUBLIC | MEMF_CLEAR))
                        {
                            if (memNameml = AllocVec68K(PowerPCBase, sizeof(struct MemList), MEMF_PUBLIC | MEMF_CLEAR))
                            {
                                memNameml->ml_NumEntries = 1;
                                memNameml->ml_ME[0].me_Un.meu_Addr = memName;
                                memNameml->ml_ME[0].me_Length = nameSize;
                                myAddHeadPPC(PowerPCBase, (struct List*)&newTask->tp_Task.tc_MemEntry, (struct Node*)memNameml);
                                CopyStr((APTR)value, (APTR)memName);
                                newTask->tp_Task.tc_Node.ln_Name = memName;

                                if (value = myGetTagDataPPC(PowerPCBase, TASKATTR_SYSTEM, 0, taglist))
                                {
                                    newTask->tp_Flags = TASKPPCF_SYSTEM;
                                }

                                if (value = myGetTagDataPPC(PowerPCBase, TASKATTR_ATOMIC, 0, taglist))
                                {
                                    newTask->tp_Flags |= TASKPPCF_ATOMIC;
                                }

                                myNewListPPC(PowerPCBase, (struct List*)&newTask->tp_TaskPools);

                                newTask->tp_Task.tc_Node.ln_Pri = myGetTagDataPPC(PowerPCBase, TASKATTR_PRI, 0, taglist);

                                longvalue = (LONG)myGetTagDataPPC(PowerPCBase, TASKATTR_NICE, 0, taglist);

                                if (longvalue < -20)
                                {
                                    longvalue = -20;
                                }
                                else if (longvalue > 20)
                                {
                                    longvalue = 20;
                                }

                                newTask->tp_Nice = longvalue;

                                if (value = myGetTagDataPPC(PowerPCBase, TASKATTR_MOTHERPRI, 0, taglist))
                                {
                                    newTask->tp_Task.tc_Node.ln_Pri = currTask->tp_Task.tc_Node.ln_Pri;
                                    newTask->tp_Nice = currTask->tp_Nice;
                                }

                                value = myGetTagDataPPC(PowerPCBase, TASKATTR_STACKSIZE, 0x10000, taglist);

                                if (value < 0x10000)
                                {
                                    value = 0x10000;
                                }

                                newTask->tp_StackSize = value;

                                if (memStack = (ULONG)AllocVec68K(PowerPCBase, value, MEMF_PUBLIC | MEMF_CLEAR))
                                {
                                    ULONG SPointer = memStack + value;
                                    newTask->tp_Task.tc_SPLower = (APTR)memStack;
                                    newTask->tp_Task.tc_SPUpper = (APTR)SPointer;
                                    SPointer = (SPointer - 56) & -32;
                                    newTask->tp_Task.tc_SPReg = (APTR)SPointer;
                                    newTask->tp_StackMem = (APTR)memStack;

                                    if (memStackml = AllocVec68K(PowerPCBase, sizeof(struct MemList), MEMF_PUBLIC | MEMF_CLEAR))
                                    {
                                        memStackml->ml_NumEntries = 1;
                                        memStackml->ml_ME[0].me_Un.meu_Addr = (APTR)memStack;
                                        memStackml->ml_ME[0].me_Length = value;
                                        myAddHeadPPC(PowerPCBase, (struct List*)&newTask->tp_Task.tc_MemEntry, (struct Node*)memStackml);

                                        if (memFrame = AllocVec68K(PowerPCBase, sizeof(struct iframe), MEMF_PUBLIC | MEMF_CLEAR))
                                        {
                                            newTask->tp_ContextMem = memFrame;
                                            memFrame->if_Context.ec_GPR[1] = SPointer;
                                            if (memFrameml = AllocVec68K(PowerPCBase, sizeof(struct MemList), MEMF_PUBLIC | MEMF_CLEAR))
                                            {
                                                memFrameml->ml_NumEntries = 1;
                                                memFrameml->ml_ME[0].me_Un.meu_Addr = (APTR)memFrame;
                                                memFrameml->ml_ME[0].me_Length = sizeof(struct iframe);
                                                myAddHeadPPC(PowerPCBase, (struct List*)&newTask->tp_Task.tc_MemEntry, (struct Node*)memFrameml);

                                                memFrame->if_Context.ec_UPC.ec_SRR0 = myCode;
                                                memFrame->if_Context.ec_SRR1 = MACHINESTATE_DEFAULT;
                                                memFrame->if_Context.ec_GPR[2] = getR2();

                                                if (value = myGetTagDataPPC(PowerPCBase, TASKATTR_BAT, 0, taglist))
                                                {
                                                    myCopyMemPPC(PowerPCBase, &PowerPCBase->pp_StoredBATs, &memFrame->if_BATs, sizeof(struct BATArray) * 4);  //Not working
                                                    myCopyMemPPC(PowerPCBase, &PowerPCBase->pp_StoredBATs, newTask->tp_BATStorage, sizeof(struct BATArray) * 4);
                                                }
                                                else
                                                {
                                                    myCopyMemPPC(PowerPCBase, &PowerPCBase->pp_SystemBATs, &memFrame->if_BATs, sizeof(struct BATArray) * 4);
                                                    myCopyMemPPC(PowerPCBase, &PowerPCBase->pp_SystemBATs, newTask->tp_BATStorage, sizeof(struct BATArray) * 4);
                                                }

                                                for (int i = 0; i < 16; i++)
                                                {
                                                    memFrame->if_Segments[i] = PowerPCBase->pp_SystemSegs[i];
                                                }

                                                value = *((ULONG*)((ULONG)PowerPCBase + _LVOEndTask + 2));

                                                memFrame->if_Context.ec_LR = myGetTagDataPPC(PowerPCBase, TASKATTR_EXITCODE, value, taglist);

                                                if (value = myGetTagDataPPC(PowerPCBase, TASKATTR_PRIVATE, 0, taglist))
                                                {
                                                    memFrame->if_Context.ec_GPR[13] = value | 1;
                                                }
                                                else
                                                {
                                                    memFrame->if_Context.ec_GPR[13] = (ULONG)PowerPCBase->pp_ThisPPCProc;
                                                }

                                                if (value = myGetTagDataPPC(PowerPCBase, TASKATTR_INHERITR2, 0, taglist))
                                                {
                                                    memFrame->if_Context.ec_GPR[2] = getR2();
                                                }
                                                else
                                                {
                                                    memFrame->if_Context.ec_GPR[2] = myGetTagDataPPC(PowerPCBase, TASKATTR_R2, 0, taglist);
                                                }

                                                memFrame->if_Context.ec_GPR[3] = myGetTagDataPPC(PowerPCBase, TASKATTR_R3, 0, taglist);
                                                memFrame->if_Context.ec_GPR[4] = myGetTagDataPPC(PowerPCBase, TASKATTR_R4, 0, taglist);
                                                memFrame->if_Context.ec_GPR[5] = myGetTagDataPPC(PowerPCBase, TASKATTR_R5, 0, taglist);
                                                memFrame->if_Context.ec_GPR[6] = myGetTagDataPPC(PowerPCBase, TASKATTR_R6, 0, taglist);
                                                memFrame->if_Context.ec_GPR[7] = myGetTagDataPPC(PowerPCBase, TASKATTR_R7, 0, taglist);
                                                memFrame->if_Context.ec_GPR[8] = myGetTagDataPPC(PowerPCBase, TASKATTR_R8, 0, taglist);
                                                memFrame->if_Context.ec_GPR[9] = myGetTagDataPPC(PowerPCBase, TASKATTR_R9, 0, taglist);
                                                memFrame->if_Context.ec_GPR[10] = myGetTagDataPPC(PowerPCBase, TASKATTR_R10, 0, taglist);

                                                if (memPort = AllocVec68K(PowerPCBase, sizeof(struct MsgPortPPC), MEMF_PUBLIC | MEMF_CLEAR))
                                                {
                                                    myNewListPPC(PowerPCBase, (struct List*)&memPort->mp_IntMsg);
                                                    myNewListPPC(PowerPCBase, (struct List*)&memPort->mp_Port.mp_MsgList);

                                                    newTask->tp_Task.tc_SigAlloc = SYS_SIGALLOC;

                                                    memPort->mp_Port.mp_SigBit = SIGB_DOS;

                                                    if (myInitSemaphorePPC(PowerPCBase, &memPort->mp_Semaphore) == -1)
                                                    {
                                                        memPort->mp_Port.mp_SigTask = newTask;
                                                        memPort->mp_Port.mp_Flags = PA_SIGNAL;
                                                        memPort->mp_Port.mp_Node.ln_Type = NT_MSGPORTPPC;
                                                        newTask->tp_Msgport = memPort;

                                                        newTask->tp_PoolMem = myGetTagDataPPC(PowerPCBase, TASKATTR_NOTIFYMSG, 0, taglist);

                                                        if (newTask->tp_MessageRIP = AllocVec68K(PowerPCBase, 928, MEMF_PUBLIC | MEMF_CLEAR))
                                                        {
                                                            if (taskPtr = AllocVec68K(PowerPCBase, sizeof(struct TaskPtr), MEMF_PUBLIC | MEMF_CLEAR))
                                                            {
                                                                taskPtr->tptr_Task = newTask;
                                                                newTask->tp_TaskPtr = taskPtr;
                                                                taskPtr->tptr_Node.ln_Name = newTask->tp_Task.tc_Node.ln_Name;

                                                                myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemTaskList);
                                                                myAddTailPPC(PowerPCBase, (struct List*)&PowerPCBase->pp_AllTasks, (struct Node*)taskPtr);
                                                                PowerPCBase->pp_NumAllTasks += 1;
                                                                myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemTaskList);

                                                                while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

                                                                if (newTask->tp_Flags & TASKPPCF_SYSTEM)
                                                                {
                                                                    PowerPCBase->pp_IdSysTasks += 1;
                                                                    newTask->tp_Id = PowerPCBase->pp_IdSysTasks;
                                                                }
                                                                else
                                                                {
                                                                    PowerPCBase->pp_IdUsrTasks += 1;
                                                                    newTask->tp_Id = PowerPCBase->pp_IdUsrTasks;
                                                                }

                                                                newTask->tp_Quantum = PowerPCBase->pp_StdQuantum;
                                                                newTask->tp_Task.tc_State = TS_READY;
                                                                newTask->tp_Desired = 0; //from NICE table TBI
                                                                newTask->tp_Flags |= TASKPPCF_CREATORPPC;

                                                                InsertOnPri(PowerPCBase, (struct List*)&PowerPCBase->pp_ReadyTasks, newTask);

                                                                PowerPCBase->pp_FlagReschedule = -1;

                                                                FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

                                                                myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);

                                                                struct SnoopData* currSnoop = (struct SnoopData*)PowerPCBase->pp_Snoop.mlh_Head;
                                                                struct SnoopData* nextSnoop;

                                                                while (nextSnoop = (struct SnoopData*)currSnoop->sd_Node.ln_Succ)
                                                                {
                                                                    if (currSnoop->sd_Type == SNOOP_START)
                                                                    {
                                                                        ULONG (*runSnoop)(__reg("r2") ULONG, __reg("r3") struct TaskPPC*,
                                                                                          __reg("r4") ULONG, __reg("r5") struct TaskPPC*,
                                                                                          __reg("r6") ULONG) = currSnoop->sd_Code;
                                                                        ULONG tempR2 = getR2();
                                                                        runSnoop(currSnoop->sd_Data, newTask, myCode, currTask, CREATOR_PPC);
                                                                        storeR2(tempR2);
                                                                    }
                                                                    currSnoop = nextSnoop;
                                                                }

                                                                myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);

                                                                CauseDECInterrupt(PowerPCBase);

                                                                currTask->tp_Flags &= ~TASKPPCF_CHOWN;
                                                                args.db_Arg[0] = (ULONG)newTask;
                                                                printDebug(PowerPCBase, (struct DebugArgs*)&args);

                                                                return newTask;
                                                            }
                                                            FreeVec68K(PowerPCBase, newTask->tp_MessageRIP);
                                                        }
                                                        FreeVec68K(PowerPCBase, memPort->mp_Semaphore.ssppc_reserved);
                                                    }
                                                    FreeVec68K(PowerPCBase, memPort);
                                                }
                                                FreeVec68K(PowerPCBase, memFrameml);
                                            }
                                            FreeVec68K(PowerPCBase, memFrame);
                                        }
                                        FreeVec68K(PowerPCBase, memStackml);
                                    }
                                    FreeVec68K(PowerPCBase, (APTR)memStack);
                                }
                                FreeVec68K(PowerPCBase, memNameml);
                            }
                            FreeVec68K(PowerPCBase, memName);
                        }
                    }
                    FreeVec68K(PowerPCBase, memBATml);
                }
                FreeVec68K(PowerPCBase, memBATStorage);
            }
            FreeVec68K(PowerPCBase, newTask);
        }
    }
    args.db_Arg[0] = NULL;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);
    return NULL;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myDeleteTaskPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct TaskPPC* PPCtask)
{
    struct DebugArgs args;
    args.db_Function = 5 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)PPCtask;
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

    if ((PPCtask->tp_Flags & TASKPPCF_CREATORPPC) && (PPCtask->tp_Msgport))
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

    if (mirrorPort = PPCNewtask->nt_Task.pt_MirrorPort)
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
        TaskHalt();
    }
    else if (!(PPCtask->tp_Flags & (TASKPPCF_CRASHED)))
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

PPCFUNCTION struct TaskPPC* myFindTaskPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") STRPTR name)
{
    struct DebugArgs args;
    args.db_Function = 6 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)name;
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

    args.db_Arg[0] = (ULONG)fndTask;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return fndTask;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myInitSemaphorePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    args.db_Function = 7 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)SemaphorePPC;
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

    args.db_Arg[0] = result;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myFreeSemaphorePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    args.db_Function = 8 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)SemaphorePPC;
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

PPCFUNCTION LONG myAddSemaphorePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    args.db_Function = 9 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)SemaphorePPC;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);
	
    LONG result = 0;

	if (result = myInitSemaphorePPC(PowerPCBase, SemaphorePPC))
	{
		myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

		myEnqueuePPC(PowerPCBase, (struct List*)&PowerPCBase->pp_Semaphores, (struct Node*)SemaphorePPC);

		myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

	}

    args.db_Arg[0] = result;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myRemSemaphorePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    args.db_Function = 10 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)SemaphorePPC;
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

PPCFUNCTION VOID myObtainSemaphorePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    args.db_Function = 11 | (1<<8) | (1<<16) | (2<<17);
    args.db_Arg[0] = (ULONG)SemaphorePPC;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

	SemaphorePPC->ssppc_SS.ss_QueueCount += 1;

	struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

	if ((SemaphorePPC->ssppc_SS.ss_QueueCount) && ((SemaphorePPC->ssppc_SS.ss_Owner != (struct Task*)myTask)))
	{
        struct SemWait myWait;
		myWait.sw_Task = myTask;
		myTask->tp_Task.tc_SigRecvd &= ~SIGF_SINGLE;

		myAddTailPPC(PowerPCBase, (struct List*)&SemaphorePPC->ssppc_SS.ss_WaitQueue, (struct Node*)&myWait);

		FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

		myWaitPPC(PowerPCBase, SIGF_SINGLE);
	}
	else
	{
		SemaphorePPC->ssppc_SS.ss_Owner = (struct Task*)myTask;
        FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
        SemaphorePPC->ssppc_SS.ss_NestCount += 1;
	}

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAttemptSemaphorePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    args.db_Function = 12 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)SemaphorePPC;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	LONG result = 0;

	while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

	struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

	if ((SemaphorePPC->ssppc_SS.ss_QueueCount + 1) && (!(SemaphorePPC->ssppc_SS.ss_Owner == (struct Task*)myTask)))
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

    args.db_Arg[0] = result;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myReleaseSemaphorePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    args.db_Function = 13 | (1<<8) | (1<<16) | (2<<17);
    args.db_Arg[0] = (ULONG)SemaphorePPC;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

	SemaphorePPC->ssppc_SS.ss_NestCount -= 1;

	if (SemaphorePPC->ssppc_SS.ss_NestCount < 0)
	{
        FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
        writeTest(0x6f000000, (ULONG)SemaphorePPC);
        HaltError(ERR_ESEM);
	}

	if (SemaphorePPC->ssppc_SS.ss_NestCount > 0)
	{
		SemaphorePPC->ssppc_SS.ss_QueueCount -= 1;
		FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
	}
	else
	{
		SemaphorePPC->ssppc_SS.ss_Owner = 0;
		SemaphorePPC->ssppc_SS.ss_QueueCount -= 1;
        FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
		
        if (SemaphorePPC->ssppc_SS.ss_QueueCount != -1)
		{
			SemaphorePPC->ssppc_lock = 1;
			struct SemWait* myWait = (struct SemWait*)myRemHeadPPC(PowerPCBase, (struct List*)&SemaphorePPC->ssppc_SS.ss_WaitQueue);

            if (myWait)
			{
				struct TaskPPC* currTask = myWait->sw_Task;
                struct TaskPPC* sigTask = (struct TaskPPC*)((ULONG)currTask & ~SM_SHARED);
				if (!(SM_SHARED & (ULONG)currTask)) //not shared
				{
					if (sigTask)
					{
                        SemaphorePPC->ssppc_SS.ss_Owner = (struct Task*)sigTask;
						SemaphorePPC->ssppc_SS.ss_NestCount += 1;
						mySignalPPC(PowerPCBase, sigTask, SIGF_SINGLE);
					}
					else
					{
                        SemaphorePPC->ssppc_SS.ss_Owner = (struct Task*)myWait->sw_Semaphore;
						myWait->sw_Semaphore = SemaphorePPC;
						SemaphorePPC->ssppc_SS.ss_NestCount += 1;
						myReplyMsgPPC(PowerPCBase, (struct Message*)myWait);

                        myWait = (struct SemWait*)SemaphorePPC->ssppc_SS.ss_WaitQueue.mlh_Head;
                        struct SemWait* nxtWait;

                        while (nxtWait = (struct SemWait*)myWait->sw_Node.mln_Succ)
                        {
                            if (!(myWait->sw_Task))
                            {
                                if (!(myWait->sw_Semaphore))
                                {
                                    struct MinNode* predWait = myWait->sw_Node.mln_Pred;
                                    predWait->mln_Succ = (struct MinNode*)nxtWait;
                                    nxtWait->sw_Node.mln_Pred = predWait;

                                    SemaphorePPC->ssppc_SS.ss_NestCount += 2;
                                    myWait->sw_Semaphore = SemaphorePPC;

                                    myReplyMsgPPC(PowerPCBase, (struct Message*)myWait);
                                }
                            }
                            else if (myWait->sw_Task == (struct TaskPPC*)SemaphorePPC->ssppc_SS.ss_Owner)
                            {
                                struct MinNode* predWait = myWait->sw_Node.mln_Pred;
                                predWait->mln_Succ = (struct MinNode*)nxtWait;
                                nxtWait->sw_Node.mln_Pred = predWait;

                                SemaphorePPC->ssppc_SS.ss_NestCount += 1;
						        mySignalPPC(PowerPCBase, myWait->sw_Task, SIGF_SINGLE);

                                break;
                            }
                            myWait = nxtWait;
                        }
					}
				}
				else //shared
				{
                    struct SemWait* nxtWait = NULL;

                    while (1)
                    {
                        while (nxtWait)
                        {
                            myWait = nxtWait;
                            nxtWait = (struct SemWait*)myWait->sw_Node.mln_Succ;

                            if ((ULONG)myWait->sw_Task & SM_SHARED)
                            {
                                sigTask = (struct TaskPPC*)((ULONG)myWait->sw_Task & ~SM_SHARED);
                                myRemovePPC(PowerPCBase, (struct Node*)myWait);
                                break;
                            }
                            myWait = 0;
                        }
                        if (!(myWait))
                        {
                            break;
                        }

                        SemaphorePPC->ssppc_SS.ss_NestCount += 1;
                        if (sigTask)
                        {
                            mySignalPPC(PowerPCBase, sigTask, SIGF_SINGLE);
                        }
                        else
                        {
                            myWait->sw_Semaphore = SemaphorePPC;
                            myWait->sw_Task = 0;
                            myReplyMsgPPC(PowerPCBase, (struct Message*)myWait);
                        }

                        if (!(nxtWait = (struct SemWait*)myWait->sw_Node.mln_Succ))
                        {
                            break;
                        }
                    }
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

PPCFUNCTION struct SignalSemaphorePPC* myFindSemaphorePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") STRPTR name)
{
    struct DebugArgs args;
    args.db_Function = 14 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)name;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

	struct SignalSemaphorePPC* mySem = (struct SignalSemaphorePPC*)myFindNamePPC(PowerPCBase, (struct List*)&PowerPCBase->pp_SemSemList, name);

	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);

    args.db_Arg[0] = (ULONG)mySem;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return mySem;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myInsertPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct List* list, __reg("r5") struct Node* node, __reg("r6") struct Node* pred)
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

PPCFUNCTION VOID myAddHeadPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct List* list, __reg("r5") struct Node* node)
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

PPCFUNCTION VOID myAddTailPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct List* list, __reg("r5") struct Node* node)
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

PPCFUNCTION VOID myRemovePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct Node* node)
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

PPCFUNCTION struct Node* myRemHeadPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct List* list)
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

PPCFUNCTION struct Node* myRemTailPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct List* list)
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

PPCFUNCTION VOID myEnqueuePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct List* list, __reg("r5") struct Node* node)
{
    LONG myPrio = (LONG)node->ln_Pri;
	struct Node* nextNode = list->lh_Head;

	while(nextNode->ln_Succ)
	{
		LONG cmpPrio = (LONG)nextNode->ln_Pri;

		if (myPrio > cmpPrio)
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

PPCFUNCTION struct Node* myFindNamePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct List* list, __reg("r5") STRPTR name)
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
            offset++;
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

PPCFUNCTION struct TagItem* myFindTagItemPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG tagvalue, __reg("r5") struct TagItem* taglist)
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

PPCFUNCTION ULONG myGetTagDataPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG tagvalue, __reg("r5") ULONG tagdefault, __reg("r6") struct TagItem* taglist)
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

PPCFUNCTION struct TagItem* myNextTagItemPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct TagItem** tagitem)
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

PPCFUNCTION LONG myAllocSignalPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") LONG signum)
{
    struct DebugArgs args;
    args.db_Function = 15 | (1<<8) | (1<<16);
    args.db_Arg[0] = signum;
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

    if (resAlloc > -1)
    {
        PowerPCBase->pp_ThisPPCProc->tp_Task.tc_SigAlloc |= 1 << resAlloc;

        while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));
        PowerPCBase->pp_ThisPPCProc->tp_Task.tc_SigRecvd &= ~(1 << resAlloc);
        PowerPCBase->pp_ThisPPCProc->tp_Task.tc_SigWait &= ~(1 << resAlloc);
        FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
    }

    args.db_Arg[0] = resAlloc;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return resAlloc;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myFreeSignalPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") LONG signum)
{
    struct DebugArgs args;
    args.db_Function = 16 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)signum;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    BYTE testSig = (BYTE)signum;
	if (!(testSig == -1))
	{
		PowerPCBase->pp_ThisPPCProc->tp_Task.tc_SigAlloc &= ~(1<<signum);
		struct TaskLink* taskLink = &PowerPCBase->pp_ThisPPCProc->tp_Link;
		taskLink->tl_Sig &= ~(1<<signum);
	}
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG mySetSignalPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG signals, __reg("r5") ULONG mask)
{
    struct DebugArgs args;
    args.db_Function = 17 | (2<<8) | (1<<16);
    args.db_Arg[0] = signals;
    args.db_Arg[1] = mask;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;
	while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

	ULONG oldSig = myTask->tp_Task.tc_SigRecvd;
	myTask->tp_Task.tc_SigRecvd = (signals &= mask) | (oldSig &= ~mask);

	FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

	CheckExcSignal(PowerPCBase, myTask, 0);

    args.db_Arg[0] = oldSig;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return oldSig;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySignalPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct TaskPPC* task, __reg("r5") ULONG signals)
{
    struct DebugArgs args;
    args.db_Function = 18 | (2<<8) | (1<<16) | (2<<17);
    args.db_Arg[0] = signals;
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
            signals = CheckExcSignal(PowerPCBase, task, signals);
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

PPCFUNCTION ULONG myWaitPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG signals)
{
    struct DebugArgs args;
    args.db_Function = 19 | (1<<8) | (1<<16) | (2<<17);
    args.db_Arg[0] = signals;
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

		while((volatile)myTask->tp_Task.tc_State != TS_RUN);

		while(!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

		signals = myTask->tp_Task.tc_SigWait;
	}

	myTask->tp_Task.tc_SigRecvd ^= maskSig;

	FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

    args.db_Arg[0] = maskSig;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return maskSig;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG mySetTaskPriPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct TaskPPC* task, __reg("r5") LONG pri)
{
    struct DebugArgs args;
    args.db_Function = 20 | (2<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)task;
    args.db_Arg[1] = pri;
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

    args.db_Arg[0] = oldPri;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return oldPri;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySignal68K(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct Task* task, __reg("r5") ULONG signals)
{
    struct DebugArgs args;
    args.db_Function = 21 | (2<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)task;
    args.db_Arg[1] = signals;
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

PPCFUNCTION VOID mySetCache(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG flags, __reg("r5") APTR start, __reg("r6") ULONG length)
{
    struct DebugArgs args;
    args.db_Function = 22 | (3<<8) | (1<<16);
    args.db_Arg[0] = flags;
    args.db_Arg[1] = (ULONG)start;
    args.db_Arg[2] = length;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    ULONG value, key;

    switch (flags)
    {
        case CACHE_DCACHEOFF:
        {
            if (!(PowerPCBase->pp_CacheDState))
            {
                FlushDCache(PowerPCBase);

                key = mySuper(PowerPCBase);
                value = getHID0();
                value &= ~HID0_DCE;
                setHID0(value);
                PowerPCBase->pp_CacheDState = -1;
                myUser(PowerPCBase, key);
            }
            break;
        }
        case CACHE_DCACHEON:
        {
            PowerPCBase->pp_CacheDState = 0;
            key = mySuper(PowerPCBase);
            value = getHID0();
            value |= HID0_DCE;
            setHID0(value);
            myUser(PowerPCBase, key);

            break;
        }
        case CACHE_DCACHELOCK:
        {
            if ((start) && (length) && !(PowerPCBase->pp_CacheDLockState))
            {
                FlushDCache(PowerPCBase);
                ULONG iterations = (ULONG)start + length;
                ULONG mask = -32;
                start = (APTR)((ULONG)start & mask);
                ULONG mem = (ULONG)start;
                iterations = (((iterations + 31) & mask) - (ULONG)start) >> 5;

                for (int i = 0; i < iterations; i++)
                {
                    loadWord(mem);
                    mem += CACHELINE_SIZE;
                }

                key = mySuper(PowerPCBase);
                value = getHID0();
                value |= HID0_DLOCK;
                setHID0(value);
                PowerPCBase->pp_CacheDLockState = -1;
                myUser(PowerPCBase, key);
            }
            break;
        }
        case CACHE_DCACHEUNLOCK:
        {
            PowerPCBase->pp_CacheDLockState = 0;
            key = mySuper(PowerPCBase);
            value = getHID0();
            value &= ~HID0_DLOCK;
            setHID0(value);
            myUser(PowerPCBase, key);
            break;
        }
        case CACHE_DCACHEFLUSH:
        {
            if (!(PowerPCBase->pp_CacheDState) && !(PowerPCBase->pp_CacheDLockState))
            {
                if ((start) && (length))
                {
                    key = mySuper(PowerPCBase);
                    ULONG iterations = (ULONG)start + length;
                    ULONG mask = -32;
                    start = (APTR)((ULONG)start & mask);
                    ULONG mem = (ULONG)start;
                    iterations = (((iterations + 31) & mask) - (ULONG)start) >> 5;

                    for (int i = 0; i < iterations; i++)
                    {
                        dFlush(mem);
                        mem += CACHELINE_SIZE;
                    }
                    sync();
                    myUser(PowerPCBase, key);
                }
                else                
                {
                    FlushDCache(PowerPCBase);
                }
            }
            break;
        }
        case CACHE_ICACHEOFF:
        {
            key = mySuper(PowerPCBase);
            value = getHID0();
            value &= ~HID0_ICE;
            setHID0(value);
            myUser(PowerPCBase, key);
            break;
        }
        case CACHE_ICACHEON:
        {
            key = mySuper(PowerPCBase);
            value = getHID0();
            value |= HID0_ICE;
            setHID0(value);
            myUser(PowerPCBase, key);
            break;
        }
        case CACHE_ICACHELOCK:
        {
            key = mySuper(PowerPCBase);
            value = getHID0();
            value |= HID0_ILOCK;
            setHID0(value);
            myUser(PowerPCBase, key);
            break;
        }
        case CACHE_ICACHEUNLOCK:
        {
            key = mySuper(PowerPCBase);
            value = getHID0();
            value &= ~HID0_ILOCK;
            setHID0(value);
            myUser(PowerPCBase, key);
            break;
        }
        case CACHE_ICACHEINV:
        {
            key = mySuper(PowerPCBase);
            if ((start) && (length))
            {
                ULONG iterations = (ULONG)start + length;
                ULONG mask = -32;
                start = (APTR)((ULONG)start & mask);
                ULONG mem = (ULONG)start;
                iterations = (((iterations + 31) & mask) - (ULONG)start) >> 5;

                for (int i = 0; i < iterations; i++)
                {
                    iInval(mem);
                    mem += CACHELINE_SIZE;
                }
                isync();
            }
            else
            {
                FlushICache();
            }
            myUser(PowerPCBase, key);
            break;
        }
    }
    switch (flags) //To prevent SDA_BASE
    {
        case CACHE_DCACHEINV:
        {
            if ((start) && (length))
            {
                key = mySuper(PowerPCBase);
                ULONG iterations = (ULONG)start + length;
                ULONG mask = -32;
                start = (APTR)((ULONG)start & mask);
                ULONG mem = (ULONG)start;
                iterations = (((iterations + 31) & mask) - (ULONG)start) >> 5;

                for (int i = 0; i < iterations; i++)
                {
                    dInval(mem);
                    mem += CACHELINE_SIZE;
                }
                sync();
                myUser(PowerPCBase, key);
            }
            break;
        }
        case  CACHE_L2CACHEON:
        {
            if (PowerPCBase->pp_DeviceID != DEVICE_MPC8343E)
            {
                key = mySuper(PowerPCBase);
                value = getL2State();
                value |= L2CR_L2E;
                setL2State(value);
                myUser(PowerPCBase, key);
                PowerPCBase->pp_CurrentL2Size = PowerPCBase->pp_L2Size;
            }
            break;
        }
        case CACHE_L2CACHEOFF:
        {
            if (PowerPCBase->pp_DeviceID != DEVICE_MPC8343E)
            {
                FlushDCache(PowerPCBase);
                key = mySuper(PowerPCBase);
                value = getL2State();
                value &= ~L2CR_L2E;
                setL2State(value);
                myUser(PowerPCBase, key);
                PowerPCBase->pp_CurrentL2Size = 0;
            }
            break;
        }
        case CACHE_L2WTON:
        {
            if (PowerPCBase->pp_DeviceID != DEVICE_MPC8343E)
            {
                key = mySuper(PowerPCBase);
                value = getL2State();
                value |= L2CR_L2WT;
                setL2State(value);
                myUser(PowerPCBase, key);
            }
            break;
        }
        case CACHE_L2WTOFF:
        {
            if (PowerPCBase->pp_DeviceID != DEVICE_MPC8343E)
            {
                key = mySuper(PowerPCBase);
                value = getL2State();
                value &= ~L2CR_L2WT;
                setL2State(value);
                myUser(PowerPCBase, key);
            }
            break;
        }
        case CACHE_TOGGLEDFLUSH:
        {
            if (PowerPCBase->pp_DeviceID != DEVICE_MPC8343E)
            {
                PowerPCBase->pp_CacheDisDFlushAll = !PowerPCBase->pp_CacheDisDFlushAll;
            }
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

PPCFUNCTION APTR mySetExcHandler(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct TagItem* taglist)
{
    struct DebugArgs args;
    args.db_Function = 23 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)taglist;
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

    if (!(value & (EXCF_SMALLCONTEXT | EXCF_LARGECONTEXT)) || !(value & (EXCF_GLOBAL | EXCF_LOCAL)))
    {
        FreeVec68K(PowerPCBase, excInfo);
        printDebug(PowerPCBase, (struct DebugArgs*)&args);
        return NULL;
    }           

    struct TaskPPC* value2 = (struct TaskPPC*)myGetTagDataPPC(PowerPCBase, EXCATTR_TASK, 0, taglist);

    if ((value & EXCF_GLOBAL) || ((value & EXCF_LOCAL) && (value2)))
    {
        excInfo->ei_ExcData.ed_Task = value2;
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
            AddExcList(PowerPCBase, excInfo, newData, (ULONG*)&excInfo->ei_MachineCheck, EXCF_MCHECK);
        }
        if (value & EXCF_DACCESS)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (ULONG*)&excInfo->ei_DataAccess, EXCF_DACCESS);
        }
        if (value & EXCF_IACCESS)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (ULONG*)&excInfo->ei_InstructionAccess, EXCF_IACCESS);
        }
        if (value & EXCF_INTERRUPT)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (ULONG*)&excInfo->ei_Interrupt, EXCF_INTERRUPT);
        }
        if (value & EXCF_ALIGN)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (ULONG*)&excInfo->ei_Alignment, EXCF_ALIGN);
        }
        if (value & EXCF_PROGRAM)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (ULONG*)&excInfo->ei_Program, EXCF_PROGRAM);
        }
        if (value & EXCF_FPUN)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (ULONG*)&excInfo->ei_FPUnavailable, EXCF_FPUN);
        }
        if (value & EXCF_DECREMENTER)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (ULONG*)&excInfo->ei_Decrementer, EXCF_DECREMENTER);
        }
        if (value & EXCF_SC)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (ULONG*)&excInfo->ei_SystemCall, EXCF_SC);
        }
        if (value & EXCF_TRACE)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (ULONG*)&excInfo->ei_Trace, EXCF_TRACE);
        }
        if (value & EXCF_PERFMON)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (ULONG*)&excInfo->ei_PerfMon, EXCF_PERFMON);
        }
        if (value & EXCF_IABR)
        {
            if (!(newData = AllocVec68K(PowerPCBase, sizeof(struct ExcData), MEMF_PUBLIC)))
            {
                flag = 1;
                break;
            }
            AddExcList(PowerPCBase, excInfo, newData, (ULONG*)&excInfo->ei_IABR, EXCF_IABR);
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

    while (!((volatile)excInfo->ei_ExcData.ed_LastExc->ed_Flags & EXCF_ACTIVE));

    args.db_Arg[0] = (ULONG)excInfo;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return excInfo;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myRemExcHandler(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") APTR xlock)
{
    struct DebugArgs args;
    args.db_Function = 24 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)xlock;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    if (xlock)
    {
        while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

        myAddHeadPPC(PowerPCBase,(struct List*)&PowerPCBase->pp_RemovedExc, (struct Node*)xlock);

        FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

        CauseDECInterrupt(PowerPCBase);

        struct ExcInfo* myInfo = xlock;

        while ((volatile)myInfo->ei_ExcData.ed_LastExc->ed_Flags & EXCF_ACTIVE);

        FreeAllExcMem(PowerPCBase, myInfo);

        FreeVec68K(PowerPCBase, xlock);
    }

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG mySuper(__reg("r3") struct PrivatePPCBase* PowerPCBase)
{
    ULONG result = -1;

    result = getCPUState(result, SUPERKEY);

    return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myUser(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG key)
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

PPCFUNCTION ULONG mySetHardware(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG flags, __reg("r5") APTR param)
{
    struct DebugArgs args;
    args.db_Function = 25 | (2<<8) | (1<<16);
    args.db_Arg[0] = flags;
    args.db_Arg[1] = (ULONG)param;
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

    args.db_Arg[0] = status;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return status;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myModifyFPExc(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG fpflags)
{
    struct DebugArgs args;
    args.db_Function = 26 | (1<<8) | (1<<16);
    args.db_Arg[0] = fpflags;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    if (fpflags & FPF_EN_OVERFLOW)
    {
        FPE_Enable(1);
    }
    if (fpflags & FPF_DIS_OVERFLOW)
    {
        FPE_Disable(1);
    }
    if (fpflags & FPF_EN_UNDERFLOW)
    {
        FPE_Enable(2);
    }
    if (fpflags & FPF_DIS_UNDERFLOW)
    {
        FPE_Disable(2);
    }
    if (fpflags & FPF_EN_ZERODIVIDE)
    {
        FPE_Enable(4);
    }
    if (fpflags & FPF_DIS_ZERODIVIDE)
    {
        FPE_Disable(4);
    }
    if (fpflags & FPF_EN_INEXACT)
    {
        FPE_Enable(8);
    }
    if (fpflags & FPF_DIS_INEXACT)
    {
        FPE_Disable(8);
    }
    if (fpflags & FPF_EN_INVALID)
    {
        FPE_Enable(16);
    }
    if (fpflags & FPF_DIS_INVALID)
    {
        FPE_Disable(16);
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG myWaitTime(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG signals, __reg("r5") ULONG time)
{
    struct DebugArgs args;
    args.db_Function = 27 | (2<<8) | (1<<16) | (1<<17);
    args.db_Arg[0] = signals;
    args.db_Arg[1] = time;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    struct UInt64 myInt64;
    struct WaitTime myWait;
    ULONG value1, value2, counter, tbu, tbl, recvdsig;

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

    args.db_Arg[0] = recvdsig;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return recvdsig;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct TaskPtr* myLockTaskList(__reg("r3") struct PrivatePPCBase* PowerPCBase)
{
	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemTaskList);
	return ((struct TaskPtr*)PowerPCBase->pp_AllTasks.mlh_Head);
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myUnLockTaskList(__reg("r3") struct PrivatePPCBase* PowerPCBase)
{
	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemTaskList);
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID mySetExcMMU(__reg("r3") struct PrivatePPCBase* PowerPCBase)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myClearExcMMU(__reg("r3") struct PrivatePPCBase* PowerPCBase)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION BOOL myChangeStack(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG NewStackSize)
{
    struct DebugArgs args;
    args.db_Function = 28 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)NewStackSize;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    BOOL result;
    APTR newStack;
    ULONG src, dst, siz, sup;
    struct MemList* stackMem;

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

    args.db_Arg[0] = result;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);
    return result;
}
/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myChangeMMU(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG mode)
{
    struct DebugArgs args;
    args.db_Function = 29 | (1<<8) | (1<<16);
    args.db_Arg[0] = mode;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return; //stub

    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

    switch (mode)
    {
        case CHMMU_STANDARD:
        {
            myTask->tp_Flags &= ~MMUF_BAT;
            GetBATs(PowerPCBase, myTask);
            break;
        }
        case CHMMU_BAT:
        {
            myTask->tp_Flags |= MMUF_BAT;
            StoreBATs(PowerPCBase, myTask);
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

PPCFUNCTION VOID myGetInfo(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct TagItem* taglist)
{
    struct DebugArgs args;
    args.db_Function = 30 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)taglist;
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
                    ULONG state = (PowerPCBase->pp_CPUHID0 >> shift) & 0x5;
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
                    myTagItem->ti_Data = PowerPCBase->pp_PageTableSize>>10;
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

PPCFUNCTION struct MsgPortPPC* myCreateMsgPortPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase)
{
    struct DebugArgs args;
    args.db_Function = 31 | (1<<16);
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	struct MsgPortPPC* myPort;
	LONG signal, result;

	if (myPort = AllocVec68K(PowerPCBase, sizeof(struct MsgPortPPC), MEMF_PUBLIC | MEMF_CLEAR | MEMF_PPC))
	{
        signal = myAllocSignalPPC(PowerPCBase, -1);
        if (signal != -1)
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

    args.db_Arg[0] = (ULONG)myPort;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return myPort;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myDeleteMsgPortPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct MsgPortPPC* port)
{
    struct DebugArgs args;
    args.db_Function = 32 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)port;
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

PPCFUNCTION VOID myAddPortPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct MsgPortPPC* port)
{
    struct DebugArgs args;
    args.db_Function = 33 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)port;
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

PPCFUNCTION VOID myRemPortPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct MsgPortPPC* port)
{
    struct DebugArgs args;
    args.db_Function = 34 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)port;
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

PPCFUNCTION struct MsgPortPPC* myFindPortPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") STRPTR name)
{
    struct DebugArgs args;
    args.db_Function = 35 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)name;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

	struct MsgPortPPC* myPort = (struct MsgPortPPC*)myFindNamePPC(PowerPCBase, (struct List*)&PowerPCBase->pp_Ports, name);

	myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);

    args.db_Arg[0] = (ULONG)myPort;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return myPort;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct Message* myWaitPortPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct MsgPortPPC* port)
{
    struct DebugArgs args;
    args.db_Function = 36 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)port;
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

    args.db_Arg[0] = (ULONG)topMsg;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return (struct Message*)topMsg;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myPutMsgPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct MsgPortPPC* port, __reg("r5") struct Message* message)
{
    struct DebugArgs args;
    args.db_Function = 37 | (2<<8) | (1<<16) | (2<<17);
    args.db_Arg[0] = (ULONG)port;
    args.db_Arg[1] = (ULONG)message;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&port->mp_Semaphore);

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

PPCFUNCTION struct Message* myGetMsgPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct MsgPortPPC* port)
{
    struct DebugArgs args;
    args.db_Function = 38 | (1<<8) | (1<<16) | (2<<17);
    args.db_Arg[0] = (ULONG)port;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&port->mp_Semaphore);

    struct Message* myMsg = (struct Message*)myRemHeadPPC(PowerPCBase, (struct List*)&port->mp_Port.mp_MsgList);

    myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&port->mp_Semaphore);

    args.db_Arg[0] = (ULONG)myMsg;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return myMsg;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myReplyMsgPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct Message* message)
{
    struct DebugArgs args;
    args.db_Function = 39 | (1<<8) | (1<<16) | (2<<17);
    args.db_Arg[0] = (ULONG)message;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    if (!(message->mn_ReplyPort))
    {
        message->mn_Node.ln_Type = NT_FREEMSG;
        return;
    }

    if (message->mn_Node.ln_Type == NT_XMSG68K)
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

PPCFUNCTION VOID myCopyMemPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") APTR source, __reg("r5") APTR dest, __reg("r6") ULONG size)
{
    struct DebugArgs args;
    args.db_Function = 40 | (3<<8) | (1<<16) | (2<<17);
    args.db_Arg[0] = (ULONG)source;
    args.db_Arg[1] = (ULONG)dest;
    args.db_Arg[2] = size;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    CopyMemQuickPPC(PowerPCBase, source, dest, size);

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct Message* myAllocXMsgPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG length, __reg("r5") struct MsgPortPPC* port)
{
    struct DebugArgs args;
    args.db_Function = 41 | (2<<8) | (1<<16);
    args.db_Arg[0] = length;
    args.db_Arg[1] = (ULONG)port;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	length = (length + sizeof(struct Message) + 31) & -32;

	struct Message* myXMessage;

	if (myXMessage = myAllocVecPPC(PowerPCBase, length, MEMF_PUBLIC | MEMF_CLEAR | MEMF_PPC, 32L))
	{
		myXMessage->mn_ReplyPort = (struct MsgPort*)port;
		myXMessage->mn_Length = length;
	}

    args.db_Arg[0] = (ULONG)myXMessage;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return myXMessage;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myFreeXMsgPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct Message* message)
{
	struct DebugArgs args;
    args.db_Function = 42 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)message;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    myFreeVecPPC(PowerPCBase, message);

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myPutXMsgPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct MsgPort* port, __reg("r5") struct Message* message)
{
    struct DebugArgs args;
    args.db_Function = 43 | (2<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)port;
    args.db_Arg[1] = (ULONG)message;
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

PPCFUNCTION VOID myGetSysTimePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct timeval* timeval)
{
    struct DebugArgs args;
    args.db_Function = 44 | (1<<8) | (1<<16) | (1<<17);
    args.db_Arg[0] = (ULONG)timeval;
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

PPCFUNCTION VOID myAddTimePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct timeval* dest, __reg("r5") struct timeval* source)
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

PPCFUNCTION VOID mySubTimePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct timeval* dest, __reg("r5") struct timeval* source)
{
    ULONG timeSecs = 0;
    LONG timeMicro = dest->tv_micro - source->tv_micro;
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

PPCFUNCTION LONG myCmpTimePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct timeval* dest, __reg("r5") struct timeval* source)
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

PPCFUNCTION struct MsgPortPPC* mySetReplyPortPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct Message* message, __reg("r5") struct MsgPortPPC* port)
{
    struct DebugArgs args;
    args.db_Function = 45 | (2<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)message;
    args.db_Arg[1] = (ULONG)port;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	struct MsgPortPPC* oldPort = (struct MsgPortPPC*)message->mn_ReplyPort;
	message->mn_ReplyPort = (struct MsgPort*)port;

    args.db_Arg[0] = (ULONG)oldPort;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return oldPort;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG mySnoopTask(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct TagItem* taglist)
{
    struct DebugArgs args;
    args.db_Function = 46 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)taglist;
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
                mySnoop = 0;
            }
            else
            {
                myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);

                myAddHeadPPC(PowerPCBase, (struct List*)&PowerPCBase->pp_Snoop, (struct Node*)mySnoop);

                myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);
            }
        }
        else
        {
            FreeVec68K(PowerPCBase, mySnoop);
            mySnoop = 0;
        }
    }

    args.db_Arg[0] = (ULONG)mySnoop;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return (ULONG)mySnoop;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myEndSnoopTask(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG id)
{
    struct DebugArgs args;
    args.db_Function = 47 | (1<<8) | (1<<16);
    args.db_Arg[0] = id;
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

PPCFUNCTION VOID myGetHALInfo(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct TagItem* taglist)
{
    struct DebugArgs args;
    args.db_Function = 48 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)taglist;
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

PPCFUNCTION VOID mySetScheduling(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct TagItem* taglist)
{
    struct DebugArgs args;
    args.db_Function = 49 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)taglist;
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

PPCFUNCTION struct TaskPPC* myFindTaskByID(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") LONG id)
{
    struct DebugArgs args;
    args.db_Function = 50 | (1<<8) | (1<<16);
    args.db_Arg[0] = id;
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

    args.db_Arg[0] = (ULONG)foundTask;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return foundTask;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG mySetNiceValue(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct TaskPPC* task, __reg("r5") LONG nice)
{
    struct DebugArgs args;
    args.db_Function = 51 | (2<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)task;
    args.db_Arg[1] = nice;
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

    args.db_Arg[0] = oldNice;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return oldNice;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myTrySemaphorePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC, __reg("r5") ULONG timeout)
{
    struct DebugArgs args;
    args.db_Function = 52 | (2<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)SemaphorePPC;
    args.db_Arg[1] = timeout;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    LONG status;

    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

    while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

    while (!(LockMutexPPC((volatile ULONG)SemaphorePPC->ssppc_reserved)));

    if (!(SemaphorePPC->ssppc_SS.ss_QueueCount += 1))
    {
        SemaphorePPC->ssppc_SS.ss_Owner = (struct Task*)myTask;
        FreeMutexPPC((ULONG)SemaphorePPC->ssppc_reserved);
        FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
        status = ATTEMPT_SUCCESS;
    }
    else
    {
        if (SemaphorePPC->ssppc_SS.ss_Owner == (struct Task*)myTask)
        {
            FreeMutexPPC((ULONG)SemaphorePPC->ssppc_reserved);
            FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
            status = ATTEMPT_SUCCESS;
        }
        else
        {
            struct SemWait semWait;
            semWait.sw_Task = myTask;
            myTask->tp_Task.tc_SigRecvd &= ~SIGF_SINGLE;
            myAddTailPPC(PowerPCBase, (struct List*)&SemaphorePPC->ssppc_SS.ss_WaitQueue, (struct Node*)&semWait);

            FreeMutexPPC((ULONG)SemaphorePPC->ssppc_reserved);
            FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

            ULONG signal = myWaitTime(PowerPCBase, SIGF_SINGLE, timeout);

            while (1)
            {
                while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));
                if (!((volatile)SemaphorePPC->ssppc_lock))
                {
                    break;
                }
                FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
            }

            ULONG sigMask = signal | myTask->tp_Task.tc_SigRecvd;
            sigMask &= ~SIGF_SINGLE;
            myTask->tp_Task.tc_SigRecvd = sigMask;

            if (!(signal & SIGF_SINGLE))
            {
                myRemovePPC(PowerPCBase, (struct Node*)&semWait);
            }

            FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

            if (signal & SIGF_SINGLE)
            {
                status = ATTEMPT_SUCCESS;
            }
            else
            {
                SemaphorePPC->ssppc_SS.ss_NestCount += 1;
                status = ATTEMPT_FAILURE;
            }
        }
    }
    return status;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG mySetExceptPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG signals, __reg("r5") ULONG mask, __reg("r6") ULONG flag)
{
    struct DebugArgs args;
    args.db_Function = 53 | (3<<8) | (1<<16);
    args.db_Arg[0] = signals;
    args.db_Arg[1] = mask;
    args.db_Arg[2] = flag;
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

    args.db_Arg[0] = OldSignals;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return OldSignals;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myObtainSemaphoreSharedPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    args.db_Function = 54 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)SemaphorePPC;
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

PPCFUNCTION LONG myAttemptSemaphoreSharedPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    args.db_Function = 55 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)SemaphorePPC;
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

    args.db_Arg[0] = result;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myProcurePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC, __reg("r5") struct SemaphoreMessage* SemaphoreMessage)
{
    struct DebugArgs args;
    args.db_Function = 56 | (2<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)SemaphorePPC;
    args.db_Arg[1] = (ULONG)SemaphoreMessage;
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

PPCFUNCTION VOID myVacatePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC, __reg("r5") struct SemaphoreMessage* SemaphoreMessage)
{
    struct DebugArgs args;
    args.db_Function = 57 | (2<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)SemaphorePPC;
    args.db_Arg[1] = (ULONG)SemaphoreMessage;
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

PPCFUNCTION APTR myCreatePoolPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG flags, __reg("r5") ULONG puddle_size, __reg("r6") ULONG thres_size)
{
    struct DebugArgs args;
    args.db_Function = 58 | (3<<8) | (1<<16);
    args.db_Arg[0] = flags;
    args.db_Arg[1] = puddle_size;
    args.db_Arg[2] = thres_size;
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

    args.db_Arg[0] = (ULONG)myPoolHeader;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return myPoolHeader;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myDeletePoolPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") APTR poolheader)
{
    struct DebugArgs args;
    args.db_Function = 59 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)poolheader;
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

PPCFUNCTION APTR myAllocPooledPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") APTR poolheader, __reg("r5") ULONG size)
{
    struct DebugArgs args;
    args.db_Function = 60 | (2<<8) | (1<<16) | (2<<17);
    args.db_Arg[0] = (ULONG)poolheader;
    args.db_Arg[1] = size;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    struct Node* memBlock;
    struct MemHeader* puddle;
    APTR mem = NULL;
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
                ULONG divider = (myHeader->ph_PuddleSize >> 8) & 0xffffff;
                ULONG granularity = (puddle->mh_Free / divider) + 0x80;
                puddle->mh_Node.ln_Pri = (UBYTE)granularity;
                myEnqueuePPC(PowerPCBase, (struct List*)&myHeader->ph_PuddleList, (struct Node*)puddle);
            }
            if (mem)
            {
                mem = (APTR)((ULONG)mem + 32);
                (*((ULONG*)((ULONG)mem - 4))) = size + 32;
            }
        }
    }

    myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemMemory);

    args.db_Arg[0] = (ULONG)mem;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return mem;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myFreePooledPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") APTR poolheader, __reg("r5") APTR ptr, __reg("r6") ULONG size)
{
    struct DebugArgs args;
    args.db_Function = 61 | (3<<8) | (1<<16) | (2<<17);
    args.db_Arg[0] = (ULONG)poolheader;
    args.db_Arg[1] = (ULONG)ptr;
    args.db_Arg[2] = size;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    struct poolHeader* myHeader = poolheader;

    if (!(ptr))
    {
        return;
    }
    if (!(size))
    {
        size = (*((ULONG*)((ULONG)ptr - 4))) - 32;
    }

    ULONG mem = (ULONG)ptr - 32;

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemMemory);

    if (myHeader->ph_ThresholdSize < size)
    {
        myRemovePPC(PowerPCBase, (struct Node*)mem);
        FreeVec68K(PowerPCBase, (APTR)mem);
    }
    else
    {
        struct MemHeader* currPuddle = (struct MemHeader*)myHeader->ph_PuddleList.mlh_Head;
        struct MemHeader* nextPuddle;

        while (nextPuddle = (struct MemHeader*)currPuddle->mh_Node.ln_Succ)
        {
            if ((mem >= (ULONG)currPuddle->mh_Lower) && (mem < (ULONG)currPuddle->mh_Upper))
            {
                DeallocatePPC(PowerPCBase, currPuddle, (APTR)mem, size + 32);

                myRemovePPC(PowerPCBase, (struct Node*)currPuddle);

                if (currPuddle->mh_Free == myHeader->ph_PuddleSize)
                {
                    FreeVec68K(PowerPCBase, currPuddle);
                }
                else
                {
                    ULONG divider = (myHeader->ph_PuddleSize >> 8) & 0xffffff;
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

PPCFUNCTION APTR myRawDoFmtPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") STRPTR formatstring, __reg("r5") APTR datastream, __reg("r6") APTR (*putchproc)(), __reg("r7") APTR putchdata)
{
    struct DebugArgs args;
    args.db_Function = 62 | (4<<8) | (1<<16) | (1<<17);
    args.db_Arg[0] = (ULONG)formatstring;
    args.db_Arg[1] = (ULONG)datastream;
    args.db_Arg[2] = (ULONG)putchproc;
    args.db_Arg[3] = (ULONG)putchdata;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    UBYTE sLength;
    STRPTR stringPtr;
    struct RDFData rdfData;

    if (putchproc == (APTR)1)
    {
        return PowerPCBase;   //emulate real WarpOS where r3 is not changed (and so is PowerPCBase)
    }

    rdfData.rd_DataStream = datastream;
    rdfData.rd_PutChData = putchdata;
    rdfData.rd_FormatString = formatstring;

    UBYTE currChar;

    while (currChar = rdfData.rd_FormatString[0])
    {
        rdfData.rd_FormatString += 1;
        if (currChar == '%')
        {
            rdfData.rd_BufPointer = (STRPTR)&rdfData.rd_Buffer;
            currChar = rdfData.rd_FormatString[0];
            ULONG flag = 0;
            if (currChar == '-')        //flag->left align
            {
                flag |= RDFF_JUSTIFY;
                rdfData.rd_FormatString += 1;
            }
            currChar = rdfData.rd_FormatString[0];
            if (currChar == '0')        //flag->prepends zeros
            {
                flag |= RDFF_PREPEND;
            }
            LONG prependNum = getNum(&rdfData);
            rdfData.rd_TruncateNum = 0;
            currChar = rdfData.rd_FormatString[0];
            if (currChar == '.')        //precision
            {
                rdfData.rd_FormatString += 1;
                rdfData.rd_TruncateNum = getNum(&rdfData);
            }
            currChar = rdfData.rd_FormatString[0];
            if (currChar == 'l')        //length->long
            {
                flag |= RDFF_LONG;
                rdfData.rd_FormatString += 1;
            }
            currChar = rdfData.rd_FormatString[0];
            rdfData.rd_FormatString += 1;
            switch (currChar)
            {
                case 'd':    //type->signed decimal number
                case 'D':
                {
                    MakeDecimal(&rdfData, TRUE, AdjustParam(&rdfData, flag));
                    goto FmtOutput;
                }
                case 'x':    //type->hexidecimal number
                case 'X':
                {
                    MakeHex(&rdfData, flag, AdjustParam(&rdfData, flag));
                    goto FmtOutput;
                }
                case 's':    //type->null-terminated string
                {
                    if (!(stringPtr = (STRPTR)AdjustParamInt(&rdfData)))
                    {
                        break;
                    }
                    rdfData.rd_BufPointer = stringPtr;
                    goto StrOutput;
                }
                case 'b':    //type->bcpl string
                {
                    if (!(stringPtr = (STRPTR)AdjustParamInt(&rdfData)))
                    {
                        break;
                    }
                    stringPtr = (STRPTR)((ULONG)stringPtr << 2);

                    sLength = stringPtr[0];

                    if (!(sLength))
                    {
                        break;
                    }

                    stringPtr += 1;
                    sLength -= 1;
                    if (stringPtr[sLength])
                    {
                        sLength += 1;
                    }
                    rdfData.rd_BufPointer = stringPtr;
                    goto BStrOutput;
                }
                case 'u':    //type->unsigned decimal number
                case 'U':
                {
                    MakeDecimal(&rdfData, FALSE, AdjustParam(&rdfData, flag));
                    goto FmtOutput;
                }
                case 'c':    //type->char
                {
                    rdfData.rd_BufPointer[0] = AdjustParam(&rdfData, flag);
                    rdfData.rd_BufPointer += 1;

          FmtOutput:
                    rdfData.rd_BufPointer[0] = 0;
                    rdfData.rd_BufPointer = (STRPTR)&rdfData.rd_Buffer;

          StrOutput:
                    sLength = -1;
                    STRPTR filler = rdfData.rd_BufPointer;
                    while (filler[0])
                    {
                        sLength -= 1;
                        filler += 1;
                    }
                    sLength = ~sLength;

         BStrOutput:
                    if ((!(rdfData.rd_TruncateNum)) || ((rdfData.rd_TruncateNum) && (sLength > rdfData.rd_TruncateNum)))
                    {
                        rdfData.rd_TruncateNum = sLength;
                    }

                    prependNum = prependNum - rdfData.rd_TruncateNum;
                    if (prependNum < 0)
                    {
                        prependNum = 0;
                    }

                    if (!(flag & RDFF_JUSTIFY))
                    {
                        PerformPad(&rdfData, flag, putchproc, prependNum);
                    }

                    if (rdfData.rd_TruncateNum)
                    {
                        for (int i = 0; i < rdfData.rd_TruncateNum; i++)
                        {
                            UBYTE currChar = rdfData.rd_BufPointer[0];
                            rdfData.rd_BufPointer += 1;

                            if (putchproc)
                            {
                                rdfData.rd_PutChData  = putchproc(rdfData.rd_PutChData, currChar);
                            }
                            else
                            {
                                STRPTR myData = (STRPTR)rdfData.rd_PutChData;
                                myData[0] = currChar;
                                rdfData.rd_PutChData = (APTR)((ULONG)rdfData.rd_PutChData + 1);
                            }
                        }
                    }

                    if (flag & RDFF_JUSTIFY)
                    {
                        PerformPad(&rdfData, flag, putchproc, prependNum);
                    }
                    break;
                }
                default:
                {
                    if (!(putchproc))
                    {
                        STRPTR myData = (STRPTR)rdfData.rd_PutChData;
                        myData[0] = currChar;
                        rdfData.rd_PutChData = (APTR)((ULONG)rdfData.rd_PutChData + 1);
                    }
                    else
                    {
                        rdfData.rd_PutChData = putchproc(rdfData.rd_PutChData, currChar);
                    }
                }
            }
        }
        else
        {
            if (!(putchproc))
            {
                STRPTR myData = (STRPTR)rdfData.rd_PutChData;
                myData[0] = currChar;
                rdfData.rd_PutChData = (APTR)((ULONG)rdfData.rd_PutChData + 1);
            }
            else
            {
                rdfData.rd_PutChData = putchproc(rdfData.rd_PutChData , currChar);
            }
        }
    }

    if (!(putchproc))
    {
        STRPTR myData = (STRPTR)rdfData.rd_PutChData;
        myData[0] = currChar;
        putchdata = (APTR)((ULONG)rdfData.rd_PutChData + 1);
    }
    else
    {
        rdfData.rd_PutChData = putchproc(rdfData.rd_PutChData , currChar);
    }

    return rdfData.rd_DataStream;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myPutPublicMsgPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") STRPTR portname, __reg("r5") struct Message* message)
{
    struct DebugArgs args;
    args.db_Function = 63 | (2<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)portname;
    args.db_Arg[1] = (ULONG)message;
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

    args.db_Arg[0] = status;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

    return status;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAddUniquePortPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct MsgPortPPC* port)
{
    struct DebugArgs args;
    args.db_Function = 64 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)port;
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
		
    args.db_Arg[0] = result;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG myAddUniqueSemaphorePPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC)
{
    struct DebugArgs args;
    args.db_Function = 65 | (1<<8) | (1<<16);
    args.db_Arg[0] = (ULONG)SemaphorePPC;
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
	
    args.db_Arg[0] = result;
    printDebug(PowerPCBase, (struct DebugArgs*)&args);

	return result;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION BOOL myIsExceptionMode(__reg("r3") struct PrivatePPCBase* PowerPCBase)
{
    BOOL status = PowerPCBase->pp_ExceptionMode;
    return status;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID myNewListPPC(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct List* list)
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

PPCFUNCTION VOID myCauseInterrupt(__reg("r3") struct PrivatePPCBase* PowerPCBase)
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

PPCFUNCTION VOID myFreeAllMem(__reg("r3") struct PrivatePPCBase* PowerPCBase)
{
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG myRun68KLowLevel(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") ULONG Code, __reg("r5") LONG Offset, __reg("r6") ULONG a0,
                                 __reg("r7") ULONG a1, __reg("r8") ULONG d0, __reg("r9") ULONG d1)
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

