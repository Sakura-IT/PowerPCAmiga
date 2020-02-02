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

#include <exec/exec.h>
#include <powerpc/tasksPPC.h>
#include <powerpc/powerpc.h>
#include <exec/memory.h>
#include "constants.h"
#include "libstructs.h"
#include "Internalsppc.h"

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

    if (realPri < defaultPri)
    {
        myTask->tp_Prioffset = defaultPri - myTask->tp_Priority;
    }

    struct Node* nextNode;
    struct Node* myNode = list->lh_Head;
    while (nextNode = myNode->ln_Succ)
    {
        struct TaskPPC* chkTask = (struct TaskPPC*)myNode;

        if (chkTask->tp_Flags & TASKPPCF_THROW)
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

	if (size == (ULONG)memBlock)
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
			struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_OFFSET));
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
			struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_OFFSET));
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
			struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_OFFSET));
			*((ULONG*)(myFIFO->kf_MIOPH)) = (ULONG)msgFrame;
			myFIFO->kf_MIOPH = (myFIFO->kf_MIOPH + 4) & 0xffff3fff;
			writememLongPPC(PowerPCBase->pp_BridgeConfig, IMMR_OMR0, (ULONG)msgFrame);
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
	
    msgFrame->mf_Identifier = ID_FREE;

	switch (PowerPCBase->pp_DeviceID)
	{
		case DEVICE_HARRIER:
		{
			break;
		}

		case DEVICE_MPC8343E:
		{
			struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_OFFSET));
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

		if (0x40 < s1 < 0x5b)
		{
			s1 |= 0x20;
		}
		else if (s1 > 0x5a)
		{
			if (0xc0 < s1 < 0xe0)
			{
				s1 |= 0x20;
			}
		}
		if (0x40 < s2 < 0x5b)
		{
			s2 |= 0x20;
		}
		else if (s2 > 0x5a)
		{
			if (0xc0 < s2 < 0xe0)
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

	while (!(LockMutexPPC((ULONG)&PowerPCBase->pp_Mutex)));

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
*
*
*********************************************************************************************/

PPCFUNCTION VOID EndTaskPPC(VOID)
{
    return;
}

/********************************************************************************************
*
*        APTR = AllocatePPC(struct Library*, struct MemHeader*, ULONG byteSize)
*
*********************************************************************************************/

PPCFUNCTION APTR AllocatePPC(struct PrivatePPCBase* PowerPCBase, struct MemHeader* memHeader, ULONG byteSize)
{
    if (!(byteSize))
    {
        return NULL;
    }

    byteSize = (byteSize + 31) & -32;

    if (byteSize > memHeader->mh_Free)
    {
        return NULL;
    }

    struct MemChunk* memChunk;
    struct MemChunk* currChunk;
    struct MemChunk* newChunk;

    if (!(memChunk = (struct MemChunk*)&memHeader->mh_First))
    {
        return NULL;
    }

    while (currChunk = memChunk->mc_Next)
    {
        if (currChunk->mc_Bytes == byteSize)
        {
            memChunk->mc_Next = currChunk->mc_Next;
            break;
        }
        if (currChunk->mc_Bytes > byteSize)
        {
            newChunk = currChunk + byteSize;
            newChunk->mc_Next = currChunk->mc_Next;
            newChunk->mc_Bytes = currChunk->mc_Bytes - byteSize;
            memChunk->mc_Next = newChunk;
            break;
        }

        memChunk = currChunk;
    }

    if (!(currChunk))
    {
        return NULL;
    }

    memHeader->mh_Free -= byteSize;         //clear alloc?

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
	if (!(byteSize))
	{
		return;
	}

	ULONG testSize = (((ULONG)memoryBlock) - ((ULONG)(memoryBlock) & -32));
	struct MemChunk* newChunk = (struct MemChunk*)((ULONG)(memoryBlock) & -32);
	ULONG freeSize = testSize + byteSize + 31;

	if (!(freeSize &= -32))
	{
		return;
	}

	struct MemChunk* currChunk = (struct MemChunk*)&memHeader->mh_First;

	ULONG flag = 0;

	while (currChunk->mc_Next)
	{
		if (currChunk->mc_Next > newChunk)
		{
			if (currChunk == (struct MemChunk*)&memHeader->mh_First)
			{
				break;
			}
			if (newChunk > currChunk->mc_Bytes + currChunk)
			{
				break;
			}
			else if (newChunk < currChunk->mc_Bytes + currChunk)
			{
			    storeR0(0x454d454d);    //EMEM
                HaltTask();
			}
			flag = 1;
			break;
		}
		else if (currChunk->mc_Next == newChunk)
		{
			storeR0(0x454d454d);    //EMEM
            HaltTask();
		}
	currChunk = currChunk->mc_Next;
	}

	if (flag)
	{
		currChunk->mc_Bytes += freeSize;
		newChunk = currChunk;
	}
	else
	{
		newChunk->mc_Next = currChunk->mc_Next;
		currChunk->mc_Next = newChunk;
		newChunk->mc_Bytes = freeSize;
	}

	struct MemChunk* nextChunk = newChunk->mc_Next;
	if (nextChunk)
	{
		if ((ULONG)nextChunk < (ULONG)(newChunk) + newChunk->mc_Bytes)
		{
			storeR0(0x454d454d);    //EMEM
            HaltTask();
		}
		else if ((ULONG)nextChunk > (ULONG)(newChunk) + newChunk->mc_Bytes)
		{
			newChunk->mc_Next = nextChunk->mc_Next;
			newChunk->mc_Bytes = nextChunk->mc_Bytes + newChunk->mc_Bytes;
		}
	}

	memHeader->mh_Free += freeSize;

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

PPCFUNCTION VOID GetBATs(struct PrivatePPCBase* PowerPCBase)
{
    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

    ULONG key = mySuper(PowerPCBase);

    MoveFromBAT(CHMMU_BAT0, (struct BATArray*)(((ULONG)myTask->tp_BATStorage) + 0));
    MoveFromBAT(CHMMU_BAT1, (struct BATArray*)(((ULONG)myTask->tp_BATStorage) + 16));
    MoveFromBAT(CHMMU_BAT2, (struct BATArray*)(((ULONG)myTask->tp_BATStorage) + 32));
    MoveFromBAT(CHMMU_BAT3, (struct BATArray*)(((ULONG)myTask->tp_BATStorage) + 48));

    MoveToBAT(CHMMU_BAT0, (struct BATArray*)&PowerPCBase->pp_StoredBATs);
    MoveToBAT(CHMMU_BAT1, (struct BATArray*)&PowerPCBase->pp_StoredBATs);
    MoveToBAT(CHMMU_BAT2, (struct BATArray*)&PowerPCBase->pp_StoredBATs);
    MoveToBAT(CHMMU_BAT3, (struct BATArray*)&PowerPCBase->pp_StoredBATs);

    myUser(PowerPCBase, key);

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION VOID StoreBATs(struct PrivatePPCBase* PowerPCBase)
{
    struct TaskPPC* myTask = PowerPCBase->pp_ThisPPCProc;

    ULONG key = mySuper(PowerPCBase);

    MoveToBAT(CHMMU_BAT0, (struct BATArray*)&PowerPCBase->pp_StoredBATs[0]);
    MoveToBAT(CHMMU_BAT1, (struct BATArray*)&PowerPCBase->pp_StoredBATs[1]);
    MoveToBAT(CHMMU_BAT2, (struct BATArray*)&PowerPCBase->pp_StoredBATs[2]);
    MoveToBAT(CHMMU_BAT3, (struct BATArray*)&PowerPCBase->pp_StoredBATs[3]);

    MoveFromBAT(CHMMU_BAT0, (struct BATArray*)(((ULONG)myTask->tp_BATStorage) + 0));
    MoveFromBAT(CHMMU_BAT1, (struct BATArray*)(((ULONG)myTask->tp_BATStorage) + 16));
    MoveFromBAT(CHMMU_BAT2, (struct BATArray*)(((ULONG)myTask->tp_BATStorage) + 32));
    MoveFromBAT(CHMMU_BAT3, (struct BATArray*)(((ULONG)myTask->tp_BATStorage) + 64));

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
    PowerPCBase->pp_ThisPPCProc->tp_Task.tc_Node.ln_Name = GetName();
    struct TaskPPC* myTask;
    APTR myPool;
    struct MemList* myMem;

    while (1)
    {
        myWaitTime(PowerPCBase, 0, 0x4c0000); // Around 5 seconds.
        while (myTask = (struct TaskPPC*)myRemHeadPPC(PowerPCBase, (struct List*)&PowerPCBase->pp_RemovedTasks))
        {
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

PPCFUNCTION VOID StartTask(struct PrivatePPCBase* PowerPCBase, struct MsgFrame* myFrame)
{
    // go asm start
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
                while (!(LockMutexPPC((ULONG)&PowerPCBase->pp_Mutex)));

                myTask->tp_Task.tc_SigRecvd |= mask;

                FreeMutexPPC((ULONG)&PowerPCBase->pp_Mutex);
            }
            while (newFrame = (struct MsgFrame*)myGetMsgPPC(PowerPCBase, myTask->tp_Msgport))
            {
                switch (newFrame->mf_Identifier)
                {
                    case ID_TPPC:
                    {
                               //go asm start
                        break;
                    }
                    case ID_END:
                    {
                               //go kill ppc
                        break;
                    }
                    default:
                    {
                        break;
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

/********************************************************************************************/


