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
#include <exec/exec.h>
#include <powerpc/tasksPPC.h>
#include <powerpc/powerpc.h>
#include <exec/memory.h>
#include "constants.h"
#include "libstructs.h"
#include "Internalsppc.h"
#pragma pack(pop)

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID writememLongPPC(ULONG Base, ULONG offset, ULONG value)
{
	*((ULONG*)(Base + offset)) = value;
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID InsertOnPri(struct PrivatePPCBase* PowerPCBase, struct List* list, struct TaskPPC* myTask)
{
    LONG realPri    = myTask->tp_Priority + myTask->tp_Prioffset;
    LONG defaultPri = PowerPCBase->pp_LowActivityPri + PowerPCBase->pp_LowActivityPriOffset;

    if (realPri >= defaultPri)
    {
        realPri = defaultPri;
        myTask->tp_Prioffset = defaultPri - myTask->tp_Priority;
    }

    struct Node* nextNode;
    struct Node* myNode = list->lh_Head;
    while (nextNode = myNode->ln_Succ)
    {
        struct TaskPPC* chkTask = (struct TaskPPC*)myNode;

        if (chkTask->tp_Flags & TASKPPCF_ATOMIC)
        {
            myNode = nextNode;
            break;
        }

        LONG cmpPri = chkTask->tp_Priority + chkTask->tp_Prioffset;

        if (realPri > cmpPri)
        {
            break;
        }

        myNode = nextNode;
    }

    struct Node* pred = myNode->ln_Pred;
	myNode->ln_Pred = (struct Node*)myTask;
	myTask->tp_Task.tc_Node.ln_Succ = myNode;
	myTask->tp_Task.tc_Node.ln_Pred = pred;
	pred->ln_Succ = (struct Node*)myTask;
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION APTR AllocVec68K(struct PrivatePPCBase* PowerPCBase, ULONG size, ULONG flags)
{
	APTR memBlock = NULL;

	if (size)
	{
		flags = (flags & ~MEMF_CHIP) | MEMF_PPC;
		memBlock = (APTR)myRun68KLowLevel(PowerPCBase, (ULONG)PowerPCBase, _LVOAllocVec32, 0, 0, size, flags);
		if (memBlock)
		{
			mySetCache(PowerPCBase, CACHE_DCACHEINV, memBlock, size);
		}
	}
	return memBlock;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID FreeVec68K(struct PrivatePPCBase* PowerPCBase, APTR memBlock)
{
    myRun68KLowLevel(PowerPCBase, (ULONG)PowerPCBase, _LVOFreeVec32, 0, (ULONG)memBlock, 0, 0);

	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID FlushDCache(struct PrivatePPCBase* PowerPCBase)
{
    ULONG key = mySuper(PowerPCBase);
    ULONG cacheSize;

    DisablePPC();

    if (PowerPCBase->pp_CacheDisDFlushAll)
    {
        cacheSize = 0;
    }
    else
    {
        cacheSize = PowerPCBase->pp_CurrentL2Size;
    }

    cacheSize = (cacheSize >> 5) + CACHE_L1SIZE;
    ULONG mem = ((PowerPCBase->pp_PPCMemSize - 0x400000) + PowerPCBase->pp_PPCMemBase);

    ULONG mem2 = mem;

    for (int i = 0; i < cacheSize; i++)
    {
        loadWord(mem);
        mem += CACHELINE_SIZE;
    }

    for (int i = 0; i < cacheSize; i++)
    {
        dFlush(mem2);
        mem2 += CACHELINE_SIZE;
    }

    setDEC(PowerPCBase->pp_Quantum);

    EnablePPC();

    myUser(PowerPCBase, key);

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID DisablePPC(VOID)
{
    ULONG msrbits = getMSR();
    msrbits &= ~PSL_EE;
    setMSR(msrbits);

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID EnablePPC(VOID)
{
    ULONG msrbits = getMSR();
    msrbits |= PSL_EE;
    setMSR(msrbits);

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID ForbidPPC(struct PrivatePPCBase* PowerPCBase)
{
    PowerPCBase->pp_FlagForbid = 1;

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID PermitPPC(struct PrivatePPCBase* PowerPCBase)
{
    PowerPCBase->pp_FlagForbid = 0;

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct MsgFrame* CreateMsgFramePPC(struct PrivatePPCBase* PowerPCBase)
{
	ULONG key;
    ULONG msgFrame = 0;

	if (!(PowerPCBase->pp_ExceptionMode))
    {
        key = mySuper(PowerPCBase);
	    DisablePPC();
    }

	switch (PowerPCBase->pp_DeviceID)
	{
		case DEVICE_HARRIER:
		{
			break;
		}

		case DEVICE_MPC8343E:
		{
			struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));
			msgFrame = *((ULONG*)(myFIFO->kf_MIOFT));
			myFIFO->kf_MIOFT = (myFIFO->kf_MIOFT + 4) & 0xffff3fff;
			break;
		}

		case DEVICE_MPC107:
		{
			break;
		}

		default:
		{
			break;
		}
	}
    if (!(PowerPCBase->pp_ExceptionMode))
    {
	    EnablePPC();
	    myUser(PowerPCBase, key);
    }
//#if 0
    if (msgFrame)
    {
        ULONG clearFrame = msgFrame;
        for (int i=0; i<48; i++)
        {
            *((ULONG*)(clearFrame)) = 0;
            clearFrame += 4;
        }
    }
//#endif
	return (struct MsgFrame*)msgFrame;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION struct MsgFrame* GetMsgFramePPC(struct PrivatePPCBase* PowerPCBase)
{
	ULONG key;
    ULONG msgFrame = 0;

	if (!(PowerPCBase->pp_ExceptionMode))
    {
	    key = mySuper(PowerPCBase);
	    DisablePPC();
    }
	
    switch (PowerPCBase->pp_DeviceID)
	{
		case DEVICE_HARRIER:
		{
			break;
		}

		case DEVICE_MPC8343E:
		{
			struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));
			if (myFIFO->kf_MIIPH != myFIFO->kf_MIIPT)
            {
                msgFrame = *((ULONG*)(myFIFO->kf_MIIPT));
			    myFIFO->kf_MIIPT = (myFIFO->kf_MIIPT + 4) & 0xffff3fff;
            }
			break;
		}

		case DEVICE_MPC107:
		{
			break;
		}

		default:
		{
			break;
		}
	}

	if (!(PowerPCBase->pp_ExceptionMode))
    {
	    EnablePPC();
	    myUser(PowerPCBase, key);
    }
	return (struct MsgFrame*)msgFrame;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID SendMsgFramePPC(struct PrivatePPCBase* PowerPCBase, struct MsgFrame* msgFrame)
{
	ULONG key;

    if (!(PowerPCBase->pp_ExceptionMode))
	{
        key = mySuper(PowerPCBase);
	    DisablePPC();
    }

	switch (PowerPCBase->pp_DeviceID)
	{
		case DEVICE_HARRIER:
		{
			break;
		}

		case DEVICE_MPC8343E:
		{
            struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));
			*((ULONG*)(myFIFO->kf_MIOPH)) = (ULONG)msgFrame;
			myFIFO->kf_MIOPH = (myFIFO->kf_MIOPH + 4) & 0xffff3fff;
			storePCI(IMMR_ADDR_DEFAULT, IMMR_OMR0, (ULONG)msgFrame);
            break;
		}

		case DEVICE_MPC107:
		{
			break;
		}

		default:
		{
			break;
		}
	}

	if (!(PowerPCBase->pp_ExceptionMode))
    {
	    EnablePPC();
	    myUser(PowerPCBase, key);
    }
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID FreeMsgFramePPC(struct PrivatePPCBase* PowerPCBase, struct MsgFrame* msgFrame)
{
	ULONG key;

    if (!(PowerPCBase->pp_ExceptionMode))
	{
        key = mySuper(PowerPCBase);
	    DisablePPC();
    }
	
    //msgFrame->mf_Identifier = ID_FREE;

	switch (PowerPCBase->pp_DeviceID)
	{
		case DEVICE_HARRIER:
		{
			break;
		}

		case DEVICE_MPC8343E:
		{
			struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));
			*((ULONG*)(myFIFO->kf_MIIFH)) = (ULONG)msgFrame;
			myFIFO->kf_MIIFH = (myFIFO->kf_MIIFH + 4) & 0xffff3fff;
			break;
		}

		case DEVICE_MPC107:
		{
			break;
		}

		default:
		{
			break;
		}
	}

	if (!(PowerPCBase->pp_ExceptionMode))
    {
	    EnablePPC();
	    myUser(PowerPCBase, key);
    }
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION LONG StricmpPPC(STRPTR string1, STRPTR string2)
{
    LONG result = 0;
	ULONG offset = 0;
	UBYTE s1,s2;

	do
	{
		s1 = string1[offset];
		s2 = string2[offset];

		if ((0x40 < s1) && (s1< 0x5b))
		{
			s1 |= 0x20;
		}
		else if (s1 > 0x5a)
		{
			if ((0xc0 < s1) && (s1 < 0xe0))
			{
				s1 |= 0x20;
			}
		}
		if ((0x40 < s2) && (s2 < 0x5b))
		{
			s2 |= 0x20;
		}
		else if (s2 > 0x5a)
		{
			if ((0xc0 < s2) && (s2 < 0xe0))
			{
				s2 |= 0x20;
			}
		}
		if (s1 != s2)
		{
			if (s1 < s2)
			{
				result = 1;
			}
			else
			{
				result = -1;
			}
			break;
		}
		offset ++;
	} while (s1);

	return result;
}
/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG GetLen(STRPTR string)
{
	ULONG offset = 0;

	while (string[offset])
	{
	 	offset++;
	}
	return offset;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION STRPTR CopyStr(APTR source, APTR dest)
{
	ULONG offset = -1;

    UBYTE* mySource = (UBYTE*)source;
    UBYTE* myDest   = (UBYTE*)dest;

	do
	{
		offset ++;
		myDest[offset] = mySource[offset];
	} while (mySource[offset]);

    offset ++;

    return &myDest[offset];
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID CauseDECInterrupt(struct PrivatePPCBase* PowerPCBase)
{
	ULONG key;

	if (!(PowerPCBase->pp_ExceptionMode))
	{
		key = mySuper(PowerPCBase);
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

PPCFUNCTION ULONG CheckExcSignal(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* myTask, ULONG signal)
{
	ULONG sigmask;

	while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

	sigmask = myTask->tp_Task.tc_SigExcept & (myTask->tp_Task.tc_SigRecvd | signal);

	if (!(sigmask))
	{
		FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
		return signal;
	}
	myTask->tp_Task.tc_SigRecvd |= sigmask;
	signal = signal & ~sigmask;
	PowerPCBase->pp_TaskExcept = myTask;
	FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
	CauseDECInterrupt(PowerPCBase);

    volatile ULONG test;

	do
	{
		test = (ULONG)PowerPCBase->pp_TaskExcept;
	} while (test);

	return signal;

}

/********************************************************************************************
*
*        APTR = AllocatePPC(struct Library*, struct MemHeader*, ULONG byteSize)
*
*********************************************************************************************/

PPCFUNCTION APTR AllocatePPC(struct PrivatePPCBase* PowerPCBase, struct MemHeader* memHeader, ULONG byteSize)
{
    struct MemChunk* currChunk = NULL;

    if (byteSize)
    {
        byteSize = (byteSize + 31) & -32;

        if (byteSize <= memHeader->mh_Free)
        {
            struct MemChunk* newChunk;
            struct MemChunk* prevChunk = (struct MemChunk*)&memHeader->mh_First;

            while (currChunk = prevChunk->mc_Next)
            {
                if (currChunk->mc_Bytes == byteSize)
                {
                    prevChunk->mc_Next = currChunk->mc_Next;
                    break;
                }
                if (currChunk->mc_Bytes > byteSize)
                {
                    newChunk = (struct MemChunk*)((ULONG)currChunk + byteSize);
                    newChunk->mc_Next = currChunk->mc_Next;
                    newChunk->mc_Bytes = currChunk->mc_Bytes - byteSize;
                    prevChunk->mc_Next = newChunk;
                    break;
                }
                prevChunk = currChunk;
            }

            if (currChunk)
            {
                memHeader->mh_Free -= byteSize;         //clear alloc?
            }
        }
    }
    return (APTR)currChunk;
}

/********************************************************************************************
*
*        VOID DeallocatePPC(struct Library*, struct MemHeader*, APTR, ULONG)
*
*********************************************************************************************/

PPCFUNCTION VOID DeallocatePPC(struct PrivatePPCBase* PowerPCBase, struct MemHeader* memHeader,
                   APTR memoryBlock, ULONG byteSize)
{
	if (byteSize)
    {
	    ULONG testSize = (((ULONG)memoryBlock) - ((ULONG)(memoryBlock) & -32));
	    struct MemChunk* testChunk = (struct MemChunk*)((ULONG)(memoryBlock) & -32);
	    ULONG freeSize = testSize + byteSize + 31;

	    if (freeSize &= -32)
        {
	        struct MemChunk* currChunk = (struct MemChunk*)&memHeader->mh_First;

	        ULONG flag = 0;

	        while (currChunk->mc_Next)
	        {
		        if (currChunk->mc_Next > testChunk)
		        {
			        if ((currChunk == (struct MemChunk*)&memHeader->mh_First) || ((ULONG)testChunk > currChunk->mc_Bytes + (ULONG)currChunk))
			        {
                        break;
			        }
			        else if ((ULONG)testChunk < currChunk->mc_Bytes + (ULONG)currChunk)
			        {
                        HaltError(ERR_EMEM);
			        }
			        flag = 1;
			        break;
		        }
		        else if (currChunk->mc_Next == testChunk)
		        {
                    HaltError(ERR_EMEM);
		        }
	            currChunk = currChunk->mc_Next;
	        }
	
            if (flag)
	        {
		        currChunk->mc_Bytes += freeSize;
		        testChunk = currChunk;
	        }
	        else
            {
		        testChunk->mc_Next = currChunk->mc_Next;
		        currChunk->mc_Next = testChunk;
		        testChunk->mc_Bytes = freeSize;
	        }

	        struct MemChunk* nextChunk = testChunk->mc_Next;
	
            if (nextChunk)
	        {
		        if ((ULONG)nextChunk < (ULONG)(testChunk) + testChunk->mc_Bytes)
		        {
                    HaltError(ERR_EMEM);
		        }
		        else if ((ULONG)nextChunk > (ULONG)(testChunk) + testChunk->mc_Bytes)
		        {
			        testChunk->mc_Next = nextChunk->mc_Next;
			        testChunk->mc_Bytes = nextChunk->mc_Bytes + testChunk->mc_Bytes;
		        }
	        }
	        memHeader->mh_Free += freeSize;
        }
    }
	return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID printDebug(struct PrivatePPCBase* PowerPCBase, struct DebugArgs* args)
{
    if (PowerPCBase->pp_DebugLevel)
    {
        HaltError(PowerPCBase->pp_DebugLevel);
        struct MsgFrame* myFrame = CreateMsgFramePPC(PowerPCBase);
        if (args->db_Process == (APTR)ID_DBGS)
        {
            myFrame->mf_Identifier = ID_DBGE;
        }
        else
        {
            myFrame->mf_Identifier = ID_DBGS;
        }
        args->db_Process = PowerPCBase->pp_ThisPPCProc;
        myCopyMemPPC(PowerPCBase, (APTR)args, (APTR)&myFrame->mf_PPCArgs, sizeof(struct DebugArgs));
        SendMsgFramePPC(PowerPCBase, myFrame);
        args->db_Process = (APTR)ID_DBGS;
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID GetBATs(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* task)
{
    ULONG key = mySuper(PowerPCBase);

    MoveFromBAT(CHMMU_BAT0, (struct BATArray*)(((ULONG)task->tp_BATStorage) + 0));
    MoveFromBAT(CHMMU_BAT1, (struct BATArray*)(((ULONG)task->tp_BATStorage) + 16));
    MoveFromBAT(CHMMU_BAT2, (struct BATArray*)(((ULONG)task->tp_BATStorage) + 32));
    MoveFromBAT(CHMMU_BAT3, (struct BATArray*)(((ULONG)task->tp_BATStorage) + 48));

    MoveToBAT(CHMMU_BAT0, (struct BATArray*)&PowerPCBase->pp_StoredBATs[0]);
    MoveToBAT(CHMMU_BAT1, (struct BATArray*)&PowerPCBase->pp_StoredBATs[1]);
    MoveToBAT(CHMMU_BAT2, (struct BATArray*)&PowerPCBase->pp_StoredBATs[2]);
    MoveToBAT(CHMMU_BAT3, (struct BATArray*)&PowerPCBase->pp_StoredBATs[3]);

    myUser(PowerPCBase, key);

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID StoreBATs(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* task)
{
    ULONG key = mySuper(PowerPCBase);

    MoveToBAT(CHMMU_BAT0, (struct BATArray*)&PowerPCBase->pp_StoredBATs[0]);
    MoveToBAT(CHMMU_BAT1, (struct BATArray*)&PowerPCBase->pp_StoredBATs[1]);
    MoveToBAT(CHMMU_BAT2, (struct BATArray*)&PowerPCBase->pp_StoredBATs[2]);
    MoveToBAT(CHMMU_BAT3, (struct BATArray*)&PowerPCBase->pp_StoredBATs[3]);

    MoveFromBAT(CHMMU_BAT0, (struct BATArray*)(((ULONG)task->tp_BATStorage) + 0));
    MoveFromBAT(CHMMU_BAT1, (struct BATArray*)(((ULONG)task->tp_BATStorage) + 16));
    MoveFromBAT(CHMMU_BAT2, (struct BATArray*)(((ULONG)task->tp_BATStorage) + 32));
    MoveFromBAT(CHMMU_BAT3, (struct BATArray*)(((ULONG)task->tp_BATStorage) + 64));

    myUser(PowerPCBase, key);
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID MoveToBAT(ULONG BATnumber, struct BATArray* batArray)
{
    switch (BATnumber)
    {
       case CHMMU_BAT0:
       {
           mvtoBAT0(batArray);
           break;
       }
       case CHMMU_BAT1:
       {
           mvtoBAT1(batArray);
           break;
       }
       case CHMMU_BAT2:
       {
           mvtoBAT2(batArray);
           break;
       }
       case CHMMU_BAT3:
       {
           mvtoBAT3(batArray);
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

PPCFUNCTION VOID MoveFromBAT(ULONG BATnumber, struct BATArray* batArray)
{
    switch (BATnumber)
    {
       case CHMMU_BAT0:
       {
           mvfrBAT0(batArray);
           break;
       }
       case CHMMU_BAT1:
       {
           mvfrBAT1(batArray);
           break;
       }
       case CHMMU_BAT2:
       {
           mvfrBAT2(batArray);
           break;
       }
       case CHMMU_BAT3:
       {
           mvfrBAT3(batArray);
           break;
       }
    }
    return;
}

/********************************************************************************************
*
*    Function to set up the system processes.
*
*********************************************************************************************/

PPCFUNCTION VOID SystemStart(struct PrivatePPCBase* PowerPCBase)
{
    struct TaskPPC* myTask;
    APTR myPool;
    struct MemList* myMem;

    while (1)
    {
        myWaitTime(PowerPCBase, 0, 0x4c0000); // Around 5 seconds.
        while (myTask = (struct TaskPPC*)myRemHeadPPC(PowerPCBase, (struct List*)&PowerPCBase->pp_RemovedTasks))
        {
            if (myTask->tp_Flags & (TASKPPCF_CRASHED | TASKPPCF_CREATORPPC))
            {
                myDeleteTaskPPC(PowerPCBase, myTask);
            }
            while (myPool = (APTR)myRemHeadPPC(PowerPCBase, (struct List*)&myTask->tp_TaskPools))
            {
                myDeletePoolPPC(PowerPCBase, myPool);
            }
            while (myMem = (struct MemList*)myRemHeadPPC(PowerPCBase, (struct List*)&myTask->tp_Task.tc_MemEntry))
            {
                FreeVec68K(PowerPCBase, myMem->ml_ME[0].me_Un.meu_Addr);
                FreeVec68K(PowerPCBase, myMem);
            }
            FreeVec68K(PowerPCBase, myTask);
         }
    }
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID FreeAllExcMem(struct PrivatePPCBase* PowerPCBase, struct ExcInfo* myInfo)
{
    APTR myData;

    if (myData = myInfo->ei_MachineCheck)
    {
        FreeVec68K(PowerPCBase,myData);
    }
    if (myData = myInfo->ei_DataAccess)
    {
        FreeVec68K(PowerPCBase,myData);
    }
    if (myData = myInfo->ei_InstructionAccess)
    {
        FreeVec68K(PowerPCBase,myData);
    }
    if (myData = myInfo->ei_Alignment)
    {
        FreeVec68K(PowerPCBase,myData);
    }
    if (myData = myInfo->ei_Program)
    {
        FreeVec68K(PowerPCBase,myData);
    }
    if (myData = myInfo->ei_FPUnavailable)
    {
        FreeVec68K(PowerPCBase,myData);
    }
    if (myData = myInfo->ei_Decrementer)
    {
        FreeVec68K(PowerPCBase,myData);
    }
    if (myData = myInfo->ei_SystemCall)
    {
        FreeVec68K(PowerPCBase,myData);
    }
    if (myData = myInfo->ei_Trace)
    {
        FreeVec68K(PowerPCBase,myData);
    }
    if (myData = myInfo->ei_PerfMon)
    {
        FreeVec68K(PowerPCBase,myData);
    }
    if (myData = myInfo->ei_IABR)
    {
        FreeVec68K(PowerPCBase,myData);
    }
    if (myData = myInfo->ei_Interrupt)
    {
        FreeVec68K(PowerPCBase,myData);
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID AddExcList(struct PrivatePPCBase* PowerPCBase, struct ExcInfo* excInfo, struct ExcData* newData, ULONG* currExc, ULONG flag)
{
    myCopyMemPPC(PowerPCBase, (APTR)excInfo, (APTR)newData, sizeof(struct ExcData));
    currExc[0] = (ULONG)newData;
    newData->ed_ExcID = flag;
    excInfo->ei_ExcData.ed_LastExc = newData;
    while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));
    myAddHeadPPC(PowerPCBase, (struct List*)&PowerPCBase->pp_ReadyExc, (struct Node*)newData);
    FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID SetupRunPPC(struct PrivatePPCBase* PowerPCBase, struct MsgFrame* myFrame)
{

    mySetCache(PowerPCBase, CACHE_ICACHEINV, 0, 0);

    while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

    myTask->tp_Task.tc_SigRecvd |= myFrame->mf_Signals;
    FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);

    ULONG myCode = (ULONG)myFrame->mf_PPCArgs.PP_Code + myFrame->mf_PPCArgs.PP_Offset;
    if (myFrame->mf_PPCArgs.PP_Offset)
    {
        myCode = *((ULONG*)(myCode + 2));
    }

    struct SnoopData* currSnoop = (struct SnoopData*)PowerPCBase->pp_Snoop.mlh_Head;
    struct SnoopData* nextSnoop;

    while (nextSnoop = (struct SnoopData*)currSnoop->sd_Node.ln_Succ)
    {
        if (currSnoop->sd_Type == SNOOP_START)
        {
            ULONG (*runSnoop)(__reg("r2") ULONG, __reg("r3") struct TaskPPC*,
            __reg("r4") ULONG, __reg("r5") struct Task*,
            __reg("r6") ULONG) = currSnoop->sd_Code;
            ULONG tempR2 = getR2();
            runSnoop(currSnoop->sd_Data, myTask, myCode, (struct Task*)myFrame->mf_Arg[2], CREATOR_68K);
            storeR2(tempR2);
        }
        currSnoop = nextSnoop;
    }

    myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);

    if ((myFrame->mf_PPCArgs.PP_Stack) && (myFrame->mf_PPCArgs.PP_StackSize))
    {
        mySetCache(PowerPCBase, CACHE_DCACHEINV, myFrame->mf_PPCArgs.PP_Stack, myFrame->mf_PPCArgs.PP_StackSize);
    }

    myTask->tp_Task.tc_SigAlloc = myFrame->mf_Arg[1];

    struct iframe storeFrame;

    RunCPP((struct iframe*)&storeFrame, myCode, &myFrame->mf_PPCArgs);

    struct MsgFrame* newFrame = CreateMsgFramePPC(PowerPCBase);

    newFrame->mf_Identifier = ID_FPPC;
    newFrame->mf_Message.mn_Length = MSGLEN;
    newFrame->mf_Message.mn_Node.ln_Type = NT_MESSAGE;

    while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

    newFrame->mf_Arg[0]  = myTask->tp_Task.tc_SigAlloc;
    newFrame->mf_Signals = myTask->tp_Task.tc_SigRecvd & 0xfffff000;
    myTask->tp_Task.tc_SigRecvd &= 0xfff;

    FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);

    newFrame->mf_Message.mn_ReplyPort = myFrame->mf_Message.mn_ReplyPort;
    newFrame->mf_PPCTask = myTask;

    myCopyMemPPC(PowerPCBase, &myFrame->mf_PPCArgs.PP_Regs, &newFrame->mf_PPCArgs.PP_Regs, (15*4) + (8*8));

    SendMsgFramePPC(PowerPCBase, newFrame);
    FreeMsgFramePPC(PowerPCBase, myFrame);

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID StartTask(struct PrivatePPCBase* PowerPCBase, struct MsgFrame* myFrame)
{
    SetupRunPPC(PowerPCBase, myFrame);

    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;
    struct PrivateTask* myPTask = (struct PrivateTask*)myTask;
    struct MsgFrame* newFrame;
    struct MsgPort* myPort;
    ULONG signals;
    ULONG mask;

    while(1)
    {
        signals = myWaitPPC(PowerPCBase, myTask->tp_Task.tc_SigAlloc & 0xfffff100);

        if (signals & SIGF_DOS)
        {
            if (mask = signals & ~SIGF_DOS)
            {
                while (!(LockMutexPPC((volatile ULONG)&PowerPCBase->pp_Mutex)));

                myTask->tp_Task.tc_SigRecvd |= mask;

                FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
            }
            while (newFrame = (struct MsgFrame*)myGetMsgPPC(PowerPCBase, myTask->tp_Msgport))
            {
                switch (newFrame->mf_Identifier)
                {
                    case ID_TPPC:
                    {
                        SetupRunPPC(PowerPCBase, newFrame);
                        break;
                    }
                    case ID_END:
                    {
                        KillTask(PowerPCBase, newFrame);
                        break;
                    }
                    default:
                    {
                        HaltError(0xBAD0BAD0);
                        break; //error
                    }
                }
            }
        }
        else if (mask = signals & ~SIGF_DOS)
        {
            if (myPort = myPTask->pt_MirrorPort)
            {
                mySignal68K(PowerPCBase, myPort->mp_SigTask, mask);
            }
        }
    }
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID EndTask(VOID)
{
    struct PPCZeroPage *myZP = 0;
    ULONG key = mySuper(NULL);             //Super does not use PowerPCBase
    struct PrivatePPCBase* PowerPCBase = (struct PrivatePPCBase*)myZP->zp_PowerPCBase;
    myUser(PowerPCBase, key);
    myDeleteTaskPPC(PowerPCBase, NULL);

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID KillTask(struct PrivatePPCBase* PowerPCBase, struct MsgFrame* myFrame)
{
    FreeMsgFramePPC(PowerPCBase, myFrame);

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);

    struct SnoopData* currSnoop = (struct SnoopData*)PowerPCBase->pp_Snoop.mlh_Head;
    struct SnoopData* nextSnoop;

    struct TaskPPC* PPCtask = PowerPCBase->pp_ThisPPCProc;

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

    myObtainSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemTaskList);

    if (PPCtask->tp_TaskPtr)
    {
        myRemovePPC(PowerPCBase, (struct Node*)PPCtask->tp_TaskPtr);
    }

    PowerPCBase->pp_NumAllTasks -= 1;

    myReleaseSemaphorePPC(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemTaskList);

    PPCtask->tp_Task.tc_State = TS_REMOVED;
    PowerPCBase->pp_FlagReschedule = -1;
    CauseDECInterrupt(PowerPCBase);
    TaskHalt();
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION ULONG getNum(struct RDFData* rdfData)
{
    ULONG result = 0;

    while (1)
    {
        UBYTE myChar = rdfData->rd_FormatString[0];
        rdfData->rd_FormatString += 1;
        if ((myChar >= '0') && (myChar <= '9'))
        {
            result = result * 10;
            myChar -= '0';
            result = result + myChar;
        }
        else
        {
            rdfData->rd_FormatString -= 1;
            break;
        }
    }
    return result;
}

/********************************************************************************************/

PPCFUNCTION LONG AdjustParamInt(struct RDFData* rdfData)
{
    ULONG* mem = (ULONG*)rdfData->rd_DataStream;
    LONG value = mem[0];
    mem = (ULONG*)((ULONG)mem + 4);
    rdfData->rd_DataStream = (APTR)mem;
    return value;
}

/********************************************************************************************/

PPCFUNCTION LONG AdjustParam(struct RDFData* rdfData, ULONG flag)
{
    WORD value;
    if (flag & RDFF_LONG)
    {
        AdjustParamInt(rdfData);
    }
    else
    {
        UWORD* mem = (UWORD*)rdfData->rd_DataStream;
        value = mem[0];
        mem = (UWORD*)((ULONG)mem + 2);
        rdfData->rd_DataStream = (APTR)mem;
    }
    return (LONG)value;
}

/********************************************************************************************/

PPCFUNCTION VOID MakeDecimal(struct RDFData* rdfData, BOOL sign, LONG value)
{
    if ((sign) && (value < 0))
    {
        rdfData->rd_BufPointer[0] = '-';
        rdfData->rd_BufPointer += 1;
        value = -value;
    }
    ULONG* myTable = GetDecTable();
    ULONG tableValue;
    ULONG cmpnumber = '0';
    ULONG number = cmpnumber;
    {
        while (tableValue = myTable[0])
        {
            while (tableValue < value)
            {
               value -= tableValue;
               number += 1;
            }
            if (number != cmpnumber)
            {
               cmpnumber = 0;
               rdfData->rd_BufPointer[0] = (UBYTE)number;
               rdfData->rd_BufPointer += 1;
            }
            myTable += 1;
        }
    }
    rdfData->rd_BufPointer[0] = (UBYTE)(value + '0');
    rdfData->rd_BufPointer += 1;
    return;
}

/********************************************************************************************/

PPCFUNCTION VOID MakeHex(struct RDFData* rdfData, ULONG flag, LONG value)
{
    ULONG iterations, currNibble;
    ULONG mySwitch = 0;

    if (value)
    {
        if (flag & RDFF_LONG)
        {
            iterations = 8;
        }
        else
        {
            iterations = 4;
            value = roll(value, 16);
        }
        for (int i = 0; i < iterations; i++)
        {
            currNibble = roll(value, 4) & 0xf;
            if ((currNibble) || (mySwitch))
            {
                mySwitch = -1;
                if (currNibble > 9)
                {
                    currNibble += 55;
                }
                else
                {
                    currNibble += 48;
                }

                rdfData->rd_BufPointer[0] = currNibble;
                rdfData->rd_BufPointer += 1;
            }
        }
        return;
    }
    rdfData->rd_BufPointer[0] = (UBYTE)(value + '0');
    rdfData->rd_BufPointer += 1;
    return;
}

/********************************************************************************************/

PPCFUNCTION VOID PerformPad(struct RDFData* rdfData, ULONG flag, APTR (*putchproc)(), LONG prependNum)
{
    UBYTE currChar;
    UBYTE useChar;

    if (flag & RDFF_PREPEND)
    {
        currChar = rdfData->rd_BufPointer[0];
        if (currChar == '-')
        {
            rdfData->rd_BufPointer += 1;
            rdfData->rd_TruncateNum -= 1;
            if (putchproc)
            {
                rdfData->rd_PutChData = putchproc(rdfData->rd_PutChData, currChar);
            }
            else
            {
                STRPTR myData = (STRPTR)rdfData->rd_PutChData;
                myData[0] = currChar;
                rdfData->rd_PutChData = (APTR)((ULONG)rdfData->rd_PutChData + 1);
            }
         }
        useChar = '0';
    }
    else
    {
        useChar = ' ';
    }
    if (prependNum)
    {
        for (int i = 0; i < prependNum; i++)
        {
            if (putchproc)
            {
                rdfData->rd_PutChData  = putchproc(rdfData->rd_PutChData, useChar);
            }
            else
            {
                STRPTR myData = (STRPTR)rdfData->rd_PutChData;
                myData[0] = useChar;
                rdfData->rd_PutChData = (APTR)((ULONG)rdfData->rd_PutChData + 1);
            }
        }
    }
    return;
}

/********************************************************************************************/
