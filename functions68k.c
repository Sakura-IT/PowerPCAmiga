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

#include <proto/exec.h>
#include <proto/powerpc.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <powerpc/powerpc.h>
#include <powerpc/tasksPPC.h>

#include "constants.h"
#include "libstructs.h"
#include "internals68k.h"

/********************************************************************************************
*
*	struct Library* Open (void)
*
*********************************************************************************************/

LIBFUNC68K struct Library* myOpen(__reg("a6") struct PPCBase* PowerPCBase)
{
    struct Task* myTask;

    if (PowerPCBase)
    {
        struct ExecBase* SysBase = PowerPCBase->PPC_SysLib;
        myTask = SysBase->ThisTask;
        myTask->tc_Flags |= TF_PPC;
        PowerPCBase->PPC_LibNode.lib_OpenCnt += 1;
        PowerPCBase->PPC_LibNode.lib_Flags &= (~LIBF_DELEXP);
    }

    return ((struct Library*)PowerPCBase);
}

LIBFUNC68K struct Library* warpOpen(__reg("a6") struct Library* WarpBase)
{
    if (WarpBase)
    {
        WarpBase->lib_OpenCnt += 1;
        WarpBase->lib_Flags &= (~LIBF_DELEXP);
    }

    return (WarpBase);
}

/********************************************************************************************
*
*	void Close (void)
*
*********************************************************************************************/

LIBFUNC68K BPTR myClose(__reg("a6") struct PPCBase* PowerPCBase)
{
    BPTR mySeglist = 0;

    if (PowerPCBase->PPC_LibNode.lib_OpenCnt -=1)
    {
        return 0;
    }

    if (PowerPCBase->PPC_LibNode.lib_Flags &= (~LIBF_DELEXP))
    {
        mySeglist = myExpunge(PowerPCBase);
        return mySeglist;
    }
    return 0;
}

LIBFUNC68K BPTR warpClose(__reg("a6") struct Library* WarpBase)
{
    BPTR mySeglist = 0;

    if (WarpBase->lib_OpenCnt -=1)
    {
        return 0;
    }

    if (WarpBase->lib_Flags &= (~LIBF_DELEXP))
    {
        //mySeglist = warpExpunge(WarpBase);    If implementing Expunge, pay attention to this.
        return mySeglist;
    }
    return 0;
}

/********************************************************************************************
*
*	void Expunge (void)
*
*********************************************************************************************/

LIBFUNC68K BPTR myExpunge(__reg("a6") struct PPCBase* PowerPCBase)
{
    return NULL;
#if 0
    if (!(PowerPCBase->PPC_LibNode.lib_OpenCnt))
    {
        PowerPCBase->PPC_LibNode.lib_Flags |= LIBF_DELEXP;
        return NULL;
    }

    struct ExecBase *SysBase = PowerPCBase->PPC_SysLib;
    BPTR mySeglist = (BPTR)PowerPCBase->PPC_SegList;

    Disable();
    Remove((struct Node*)PowerPCBase);
    FreeMem((APTR)PowerPCBase,
            (ULONG)(PowerPCBase->PPC_LibNode.lib_NegSize+PowerPCBase->PPC_LibNode.lib_PosSize));
    //FreeVec((APTR) ppc code functions
    Enable();
    return mySeglist;
#endif
}

/********************************************************************************************
*
*	void Reserved (void)
*
*********************************************************************************************/

LIBFUNC68K ULONG myReserved(void)
{
    return 0;
}

/********************************************************************************************
*
*	LONG RunPPC(struct PowerPC* message)
*
********************************************************************************************/

LIBFUNC68K LONG myRunPPC(__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") struct PPCArgs* PPStruct)
{
    struct MinList myList;
    struct MirrorTask* myMirror;
    ULONG  stackMem;
    ULONG  taskName;
    UWORD  cmndSize;
    struct TaskPPC* newPPCTask;
    struct CommandLineInterface* comLineInt;
    STRPTR cmndName;

    struct MirrorTask* thisMirrorNode   = NULL;
    struct ExecBase* SysBase      = PowerPCBase->PPC_SysLib;
    struct Task* thisTask         = SysBase->ThisTask;
    struct Process* thisProc      = (struct Process*)thisTask;

    struct PrivatePPCBase* myBase = (struct PrivatePPCBase*)PowerPCBase;

    if (thisTask->tc_Node.ln_Type != NT_PROCESS)
    {
        return (PPERR_MISCERR);
    }

    myList = myBase->pp_MirrorList;
    myMirror = (struct MirrorTask*)myList.mlh_Head;

    while (myMirror->mt_Node.mln_Succ)
    {
        if (thisTask == myMirror->mt_MirrorTask)
        {
           if (myMirror->mt_Flags)
           {
               return (PPERR_ASYNCERR);
           }
           thisMirrorNode = myMirror;
           break;
        }

    myMirror = (struct MirrorTask*)myMirror->mt_Node.mln_Succ;

    }

    if (!(thisMirrorNode))
    {
        thisTask->tc_Flags &= ~TF_PPC;

        thisMirrorNode = (struct MirrorTask*)AllocVec(sizeof(struct MirrorTask), (MEMF_PUBLIC | MEMF_CLEAR));

        if(!(thisMirrorNode))
        {
            return (PPERR_MISCERR);
        }

        thisMirrorNode->mt_MirrorPort = CreateMsgPort();

        if(!(thisMirrorNode->mt_MirrorPort))
        {
            return (PPERR_MISCERR);
        }

        thisTask->tc_Flags |= TF_PPC;

        thisMirrorNode->mt_MirrorTask = thisTask;

        Disable();

        AddHead((struct List*)&myList, (struct Node*)thisMirrorNode);

        Enable();

        stackMem = (ULONG)AllocVec32((((ULONG)(thisTask->tc_SPUpper) - (ULONG)(thisTask->tc_SPLower)) | 0x80000) + 2048,
                              MEMF_PUBLIC | MEMF_PPC | MEMF_REVERSE);

        if(!(stackMem))
        {
           return (PPERR_MISCERR);
        }

        newPPCTask = (struct TaskPPC*)stackMem;

        for (int n=0; n<512; n++)
        {
            *((ULONG*)(stackMem)) = 0;
            stackMem +=4;
        }

        newPPCTask->tp_TaskPools.lh_Head     = (struct Node*)&newPPCTask->tp_TaskPools.lh_TailPred;
        newPPCTask->tp_TaskPools.lh_TailPred = (struct Node*)&newPPCTask->tp_TaskPools.lh_Tail;

        if (comLineInt = (struct CommandLineInterface*)((ULONG)(thisProc->pr_CLI <<2)))
        {
            if (!(cmndName = (STRPTR)((ULONG)(comLineInt->cli_CommandName <<2))))
            {
                cmndName = (STRPTR)thisTask->tc_Node.ln_Name;
                cmndSize = 255;
            }
            else
            {
                cmndSize = *((UBYTE*)(cmndName));
                cmndName += 1;
            }

        }
        else
        {
            cmndName = (STRPTR)thisTask->tc_Node.ln_Name;
            cmndSize = 255;
        }

    char* destName = (char*)((ULONG)newPPCTask + 1720);

    ULONG pointer = 0;

    while ((cmndName[pointer]) && (pointer < (cmndSize + 1)))
    {
        destName[pointer] = cmndName[pointer];
        pointer += 1;
    }
    ULONG appendix = (ULONG)&destName[pointer];
    *((ULONG*)(appendix)) = 0x5F505043;             //_PPC

    }

    // build the message and send. Then fall-through to WaitForPPC
    return 0;
}

/********************************************************************************************
*
*	LONG WaitForPPC(struct PowerPC* message)
*
********************************************************************************************/

LIBFUNC68K LONG myWaitForPPC(__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") struct PPCArgs* PPStruct)
{
    return 0;
}

/********************************************************************************************
*
*	ULONG CPUType = GetCPU(void)
*
*********************************************************************************************/

LIBFUNC68K ULONG myGetCPU(__reg("a6") struct PPCBase* PowerPCBase)
{
    struct PPCArgs pa;
    LONG status;
    UWORD cpuType;

	pa.PP_Code = PowerPCBase;
	pa.PP_Offset = _LVOSetHardware;
	pa.PP_Flags = 0;
	pa.PP_Stack = 0;
	pa.PP_StackSize = 0;
	pa.PP_Regs[0] = (ULONG) PowerPCBase;
    pa.PP_Regs[1] = HW_CPUTYPE;

	//pa.PP_Regs[12] = (ULONG) &LinkerDB;

	if (!(status = RunPPC(&pa)))
    {
        cpuType = (pa.PP_Regs[0] >> 16);

	    switch (cpuType)
        {
            case 0x7000:
            {
                return (CPUF_G3);

            }
            case 0x8000:
            {
                return (CPUF_G4);
            }
            case 0x8083:
            {
                return (CPUF_603E);
            }
        }

        switch (cpuType & 0xfff)
        {
            case 8:
            {
                return (CPUF_G3);
            }
            case 0xc:
            {
                return (CPUF_G4);
            }
        }
    }
    return 0;
}

/********************************************************************************************
*
*	ULONG PPCState = GetPPCState(void)
*
********************************************************************************************/

LIBFUNC68K ULONG myGetPPCState(__reg("a6") struct PPCBase* PowerPCBase)
{
    struct PPCArgs pa;
    LONG status;
    ULONG result = 0;

	pa.PP_Code = PowerPCBase;
	pa.PP_Offset = _LVOSetHardware;
	pa.PP_Flags = 0;
	pa.PP_Stack = 0;
	pa.PP_StackSize = 0;
	pa.PP_Regs[0] = (ULONG) PowerPCBase;
    pa.PP_Regs[1] = HW_PPCSTATE;

	if (!(status = RunPPC(&pa)))
    {
        result = pa.PP_Regs[0];
    }

    return result;
}

/********************************************************************************************
*
*	struct TaskPPC* CreatePPCTask(struct TagItem *TagItems)
*
*********************************************************************************************/


LIBFUNC68K struct TaskPPC* myCreatePPCTask(__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") struct TagItem* TagItems)
{
    struct PPCArgs pa;

	pa.PP_Code = PowerPCBase;
	pa.PP_Offset = _LVOCreateTaskPPC;
	pa.PP_Flags = 0;
	pa.PP_Stack = 0;
	pa.PP_StackSize = 0;
	pa.PP_Regs[0] = (ULONG) PowerPCBase;
    pa.PP_Regs[1] = (ULONG) TagItems;

	RunPPC(&pa);

    return ((struct TaskPPC*)pa.PP_Regs[0]);
}

/********************************************************************************************
*
*	APTR memblock = AllocVec32(ULONG memsize, ULONG attributes)
*
********************************************************************************************/

LIBFUNC68K APTR myAllocVec32(__reg("a6") struct PPCBase* PowerPCBase, __reg("d0") ULONG memsize, __reg("d1") ULONG attributes)
{

    ULONG oldmemblock;
    struct Task *myTask;
    struct ExecBase *SysBase;

    ULONG memblock = 0;

    if (PowerPCBase)
    {
        SysBase = PowerPCBase->PPC_SysLib;
    }
    else
    {
        SysBase = *((struct ExecBase **)4UL);
    }

    attributes &= MEMF_CLEAR;
    attributes |= (MEMF_PUBLIC | MEMF_PPC);
    memsize += 0x38;

    if (memblock = (ULONG)AllocVec(memsize, attributes))
    {
        myTask = SysBase->ThisTask;
        myTask->tc_Flags |= TF_PPC;

        oldmemblock = memblock;
        memblock += 0x27;
        memblock &= -32L;
        *((ULONG*)(memblock - 4)) = oldmemblock;
    }
    return (APTR)memblock;
}

/********************************************************************************************
*
*	void FreeVec32(APTR memblock)
*
********************************************************************************************/

LIBFUNC68K void myFreeVec32(__reg("a6") struct PPCBase* PowerPCBase, __reg("a1") APTR memblock)
{
    ULONG oldmemblock;
    struct ExecBase *SysBase = PowerPCBase->PPC_SysLib;

    if (memblock)
    {
        oldmemblock = *((ULONG*)((ULONG)memblock - 4));
        FreeVec((APTR)oldmemblock);
    }
    return;
}

/********************************************************************************************
*
*	struct Message* AllocXMsg(ULONG bodysize, struct MsgPort* replyport)
*
********************************************************************************************/

LIBFUNC68K struct Message* myAllocXMsg(__reg("a6") struct PPCBase* PowerPCBase,
                            __reg("d0") ULONG bodysize, __reg("a0") struct MsgPort* replyport)
{
    struct Message* myXMsg;

    bodysize += 31;
    bodysize &= -32;

    myXMsg = (struct Message*)AllocVec32(bodysize, MEMF_PUBLIC|MEMF_PPC|MEMF_CLEAR);

    if (myXMsg)
    {
        myXMsg->mn_ReplyPort = replyport;
        myXMsg->mn_Length = bodysize;
        return myXMsg;
    }

    return NULL;
}


/********************************************************************************************
*
*	void FreeXMsg(struct Message* myXMsg) // a0
*
********************************************************************************************/

LIBFUNC68K void myFreeXMsg(__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") struct Message* myXMsg)
{
    FreeVec32((APTR)myXMsg);
    return;
}

/********************************************************************************************
*
*	void SetCache68K(cacheflags, start, length) // d0,a0,d1
*
********************************************************************************************/

LIBFUNC68K void mySetCache68K(__reg("a6") struct PPCBase* PowerPCBase, __reg("d0") ULONG cacheflags,
                   __reg("a0") APTR start, __reg("d1") ULONG length)
{
    struct ExecBase* SysBase = PowerPCBase->PPC_SysLib;

    switch (cacheflags)
    {
        case CACHE_DCACHEOFF:
        {
            CacheControl(0,CACRF_EnableD);
            break;
        }
        case CACHE_DCACHEON:
        {
            CacheControl(CACRF_EnableD, CACRF_EnableD);
            break;
        }
        case CACHE_ICACHEOFF:
        {
            CacheControl(0, CACRF_EnableI);
            break;
        }
        case CACHE_ICACHEON:
        {
            CacheControl(CACRF_EnableI, CACRF_EnableI);
            break;
        }
        case CACHE_DCACHEFLUSH:
        case CACHE_DCACHEINV:
        {
            if(start && length)
            {
                CacheClearE(start, length, CACRF_ClearD);
            }
            else
            {
                CacheClearU();
            }
            break;

        }
        case CACHE_ICACHEINV:
        {
            if(start && length)
            {
                CacheClearE(start, length, CACRF_ClearI);
            }
            else
            {
                CacheClearU();
            }
            break;
        }
    }
    return;
}



/********************************************************************************************
*
*	void PowerDebugMode(ULONG debuglevel)
*
********************************************************************************************/

LIBFUNC68K void myPowerDebugMode(__reg("a6") struct PPCBase* PowerPCBase, __reg("d0") ULONG debuglevel)
{
    struct PPCArgs pa;

    if (debuglevel > 3)
    {
        return;
    }

	pa.PP_Code = PowerPCBase;
	pa.PP_Offset = _LVOSetHardware;
	pa.PP_Flags = 0;
	pa.PP_Stack = 0;
	pa.PP_StackSize = 0;
	pa.PP_Regs[0] = (ULONG) PowerPCBase;
    pa.PP_Regs[1] = HW_SETDEBUGMODE;
    pa.PP_Regs[8] = debuglevel;

	RunPPC(&pa);

    return;
}


/********************************************************************************************
*
*	void SPrintF68K(STRPTR Formatstring, APTR values) // a0,a1
*
********************************************************************************************/

APTR __RawPutChar(__reg("a6") void *, __reg("d0") UBYTE MyChar)="\tjsr\t-516(a6)";

#define RawPutChar(MyChar) __RawPutChar(SysBase, (MyChar))

void PutChProc(__reg("a6") struct ExecBase* SysBase, __reg("d0") UBYTE mychar,
               __reg("a3") APTR PutChData)
{
    RawPutChar(mychar);
    return;
}

LIBFUNC68K void mySPrintF68K(__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") STRPTR Formatstring,
                  __reg("a1") APTR values)
{
    struct ExecBase *SysBase = PowerPCBase->PPC_SysLib;

    RawDoFmt(Formatstring, values, &PutChProc, NULL);
    return;
}


/********************************************************************************************
*
*	void PutXMsg(struct MsgPort* MsgPortPPC, struct Message* message)
*
********************************************************************************************/

LIBFUNC68K void myPutXMsg(__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") struct MsgPort* MsgPortPPC,
               __reg("a1") struct Message* message)
{
    return;
}


/********************************************************************************************
*
*	void CausePPCInterrupt(void)
*
********************************************************************************************/

LIBFUNC68K void myCausePPCInterrupt(__reg("a6") struct PPCBase* PowerPCBase)
{
    return;
}

