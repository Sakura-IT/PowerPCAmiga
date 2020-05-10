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

#include <proto/exec.h>
#include <proto/powerpc.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/ports.h>
#include <powerpc/powerpc.h>
#include <powerpc/tasksPPC.h>

#include "constants.h"
#include "libstructs.h"
#include "internals68k.h"

extern APTR OldRemTask;

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

LIBFUNC68K LONG myCRunPPC(__reg("a6") struct PrivatePPCBase* PowerPCBase, __reg("a0") struct PPCArgs* PPStruct)
{
    ULONG  stackMem, stackSize, taskName;
    APTR   stackPP;
    UWORD  cmndSize;
    struct NewTask* newPPCTask;
    struct CommandLineInterface* comLineInt;
    STRPTR cmndName;

    struct ExecBase* SysBase      = PowerPCBase->pp_PowerPCBase.PPC_SysLib;
    struct Task* thisTask         = SysBase->ThisTask;
    struct Process* thisProc      = (struct Process*)thisTask;

    if (thisTask->tc_Node.ln_Type != NT_PROCESS)
    {
        return (PPERR_MISCERR);
    }

    struct MirrorTask* myMirror = (struct MirrorTask*)PowerPCBase->pp_MirrorList.mlh_Head;
    struct MirrorTask* nxtMirror;

    while (nxtMirror = (struct MirrorTask*)myMirror->mt_Node.mln_Succ)
    {
        if (thisTask == myMirror->mt_Task)
        {
            if (myMirror->mt_Flags & PPF_ASYNC)
            {
                return (PPERR_ASYNCERR);
            }
            break;
        }
        myMirror = nxtMirror;
    }

    if (!(nxtMirror))
    {
        thisTask->tc_Flags &= ~TF_PPC;

        if (!(myMirror = (struct MirrorTask*)AllocVec(sizeof(struct MirrorTask), (MEMF_PUBLIC | MEMF_CLEAR))))
        {
            return (PPERR_MISCERR);
        }

        if(!(myMirror->mt_Port = CreateMsgPort()))
        {
            return (PPERR_MISCERR);
        }

        thisTask->tc_Flags |= TF_PPC;

        myMirror->mt_Task    = thisTask;
        myMirror->mt_PPCTask = NULL;

        Disable();

        AddHead((struct List*)&PowerPCBase->pp_MirrorList, (struct Node*)myMirror);

        Enable();

        stackSize = ((((ULONG)(thisTask->tc_SPUpper) - (ULONG)(thisTask->tc_SPLower)) | 0x80000) & -128);

        if (!(stackMem = (ULONG)AllocVec32((stackSize + 2048), MEMF_PUBLIC | MEMF_PPC | MEMF_REVERSE)))
        {
           return (PPERR_MISCERR);
        }

        newPPCTask = (struct NewTask*)stackMem;

        for (int n=0; n<(sizeof(struct NewTask)/4); n++)
        {
            *((ULONG*)(stackMem)) = 0;
            stackMem +=4;
        }

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

        char* destName = (char*)((ULONG)&newPPCTask->nt_Name);

        ULONG pointer = 0;

        while ((cmndName[pointer]) && (pointer < (cmndSize + 1)))
        {
            destName[pointer] = cmndName[pointer];
            pointer += 1;
        }
        ULONG appendix = (ULONG)&destName[pointer];
        *((ULONG*)(appendix)) = ID_PPC;
    }

    struct MsgFrame* myFrame = CreateMsgFrame(PowerPCBase);

    myFrame->mf_Message.mn_Length       = MSGLEN;
    myFrame->mf_Identifier              = ID_TPPC;
    myFrame->mf_Message.mn_Node.ln_Type = NT_MESSAGE;
    myFrame->mf_Message.mn_Node.ln_Name = (APTR)stackSize;
    myFrame->mf_Message.mn_ReplyPort    = myMirror->mt_Port;
    myFrame->mf_MirrorPort              = myMirror->mt_Port;
    myFrame->mf_PPCTask                 = myMirror->mt_PPCTask;
    myFrame->mf_Signals                 = (thisTask->tc_SigRecvd & 0xfffff000) & ~(1 << (ULONG)myMirror->mt_Port->mp_SigBit);
    myFrame->mf_Arg[0]                  = (ULONG)newPPCTask;
    myFrame->mf_Arg[1]                  = thisTask->tc_SigAlloc;
    myFrame->mf_Arg[2]                  = (ULONG)thisTask;

    if ((PPStruct->PP_Stack) && (PPStruct->PP_StackSize))
    {
        if (PPStruct->PP_Flags == PPF_ASYNC)
        {
            return (PPERR_MISCERR);
        }
        if (!(stackPP = AllocVec32(PPStruct->PP_StackSize, MEMF_PUBLIC|MEMF_PPC|MEMF_REVERSE)))
        {
            return (PPERR_MISCERR);
        }
        CopyMem((const APTR)PPStruct->PP_Stack, stackPP, PPStruct->PP_StackSize);
        PPStruct->PP_Stack = stackPP;
    }
    else
    {
        PPStruct->PP_Stack = NULL;
    }

//    MyCopy((APTR)PPStruct, &myFrame->mf_PPCArgs, (sizeof(struct PPCArgs)/4)-1);

    CopyMemQuick((APTR)PPStruct, (APTR)(&myFrame->mf_PPCArgs), sizeof(struct PPCArgs));

    SendMsgFrame(PowerPCBase, myFrame);

    if (PPStruct->PP_Flags == PPF_ASYNC)
    {
        myMirror->mt_Flags = PPStruct->PP_Flags;
        return (PPERR_SUCCESS);
    }
    myMirror->mt_Flags = ((PPStruct->PP_Flags) | PPF_ASYNC);

    return myWaitForPPC(PowerPCBase, PPStruct);
}

/********************************************************************************************
*
*	LONG WaitForPPC(struct PowerPC* message)
*
********************************************************************************************/

LIBFUNC68K LONG myWaitForPPC(__reg("a6") struct PrivatePPCBase* PowerPCBase, __reg("a0") struct PPCArgs* PPStruct)
{
    struct MsgFrame* myFrame;
    struct ExecBase* SysBase            = PowerPCBase->pp_PowerPCBase.PPC_SysLib;
    struct Task* thisTask               = SysBase->ThisTask;

    ULONG Signals;

    struct MirrorTask* myMirror = (struct MirrorTask*)PowerPCBase->pp_MirrorList.mlh_Head;
    struct MirrorTask* nxtMirror;

    while (nxtMirror = (struct MirrorTask*)myMirror->mt_Node.mln_Succ)
    {
        if (thisTask == myMirror->mt_Task)
        {
            if ((myMirror->mt_Flags) & PPF_ASYNC)
            {
                ((myMirror->mt_Flags) ^= PPF_ASYNC);
            }
            else
            {
                return (PPERR_ASYNCERR);
            }

            ULONG andTemp = ~(0xfff + (1 << (ULONG)myMirror->mt_Port->mp_SigBit));

            while (1)
            {
                while (1)
                {
                    if (thisTask->tc_Node.ln_Pri < 0)
                    {
                        thisTask->tc_Node.ln_Pri = 0;
                    }

                    Signals = Wait((ULONG)thisTask->tc_SigAlloc & 0xfffff000);

                    if (Signals & (1 << (ULONG)myMirror->mt_Port->mp_SigBit))
                    {
                        break;
                    }
                    else if (Signals & andTemp)
                    {
                        struct MsgFrame* crossFrame = CreateMsgFrame(PowerPCBase);
                        crossFrame->mf_Identifier   = ID_LLPP;
                        crossFrame->mf_Signals      = (Signals & andTemp);
                        crossFrame->mf_Arg[0]       = (ULONG)thisTask;
                        SendMsgFrame(PowerPCBase, crossFrame);
                    }
                }
                thisTask->tc_SigRecvd |= (Signals & andTemp);
                while (myFrame = (struct MsgFrame*)GetMsg(myMirror->mt_Port))
                {
                    if (myFrame->mf_Identifier == ID_FPPC)
                    {
                        thisTask->tc_SigRecvd |= myFrame->mf_Signals;
                        myMirror->mt_PPCTask = myFrame->mf_PPCTask;

                        if (PPStruct->PP_Stack)
                        {
                            FreeVec32(PPStruct->PP_Stack);
                        }

                        //MyCopy(&myFrame->mf_PPCArgs, (APTR)PPStruct, (sizeof(struct PPCArgs)/4)-1);

                        CopyMemQuick((APTR)(&myFrame->mf_PPCArgs), (APTR)PPStruct, sizeof(struct PPCArgs));
                        FreeMsgFrame(PowerPCBase, myFrame);
                        return (PPERR_SUCCESS);
                    }
                    else if (myFrame->mf_Identifier == ID_T68K)
                    {
                        thisTask->tc_SigRecvd |= myFrame->mf_Signals;
                        myMirror->mt_PPCTask = myFrame->mf_PPCTask;

                        Run68KCode(SysBase, &myFrame->mf_PPCArgs);

                        struct MsgFrame* doneFrame = CreateMsgFrame(PowerPCBase);

                        CopyMemQuick((APTR)myFrame, (APTR)doneFrame, sizeof(struct MsgFrame));

                        doneFrame->mf_Identifier = ID_DONE;
                        doneFrame->mf_Signals    = thisTask->tc_SigRecvd & andTemp;
                        doneFrame->mf_Arg[0]     = (ULONG)thisTask;
                        doneFrame->mf_Arg[1]     = thisTask->tc_SigAlloc;

                        thisTask->tc_SigRecvd ^= doneFrame->mf_Signals;

                        SendMsgFrame(PowerPCBase, doneFrame);
                        FreeMsgFrame(PowerPCBase, myFrame);
                    }
                    else if (myFrame->mf_Identifier == ID_END)
                    {
                        FreeMsgFrame(PowerPCBase, myFrame);
                        commonRemTask(NULL, 1, SysBase);
                        void (*RemTask_ptr)(__reg("a1") struct Task*, __reg("a6") struct ExecBase*) = OldRemTask;
                        RemTask_ptr(NULL, SysBase);
                        break;
                    }
                    else
                    {
                        FreeMsgFrame(PowerPCBase, myFrame);
                        PrintError(SysBase, "68K mirror task received illegal command packet");
                    }
                }
            }

        }
        myMirror = nxtMirror;
    }
    return (PPERR_ASYNCERR);
}

/********************************************************************************************
*
*	ULONG CPUType = GetCPU(void)
*
*********************************************************************************************/

LIBFUNC68K ULONG myGetCPU(__reg("a6") struct PrivatePPCBase* PowerPCBase)
{
    UWORD cpuType = PowerPCBase->pp_CPUInfo >> 16;

    switch (cpuType)
    {
        case 0x7000:
        {
            return (CPUF_G3);
        }
        case 0x8000:
        {
            return (CPUF_G3);  //debugdebug should be g4
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
            return (CPUF_G3);  //debugdebug should be g4 (to prevent Altivec for now)
        }
    }
    return 0;
}

/********************************************************************************************
*
*	ULONG PPCState = GetPPCState(void)
*
********************************************************************************************/

LIBFUNC68K ULONG myGetPPCState(__reg("a6") struct PrivatePPCBase* PowerPCBase)
{
    struct PPCArgs pa;
    LONG status;
    ULONG result = 0;

	pa.PP_Code      = PowerPCBase;
	pa.PP_Offset    = _LVOSetHardware;
	pa.PP_Flags     = 0;
	pa.PP_Stack     = 0;
	pa.PP_StackSize = 0;
	pa.PP_Regs[0]   = (ULONG) PowerPCBase;
    pa.PP_Regs[1]   = HW_PPCSTATE;

	if (!(status = myRunPPC(PowerPCBase, (APTR)&pa)))
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


LIBFUNC68K struct TaskPPC* myCreatePPCTask(__reg("a6") struct PrivatePPCBase* PowerPCBase, __reg("a0") struct TagItem* TagItems)
{
    struct PPCArgs pa;

	pa.PP_Code      = PowerPCBase;
	pa.PP_Offset    = _LVOCreateTaskPPC;
	pa.PP_Flags     = 0;
	pa.PP_Stack     = 0;
	pa.PP_StackSize = 0;
	pa.PP_Regs[0]   = (ULONG) PowerPCBase;
    pa.PP_Regs[1]   = (ULONG) TagItems;

	myRunPPC(PowerPCBase, (APTR)&pa);

    return ((struct TaskPPC*)pa.PP_Regs[0]);
}

/********************************************************************************************
*
*	APTR memblock = AllocVec32(ULONG memsize, ULONG attributes)
*
********************************************************************************************/

LIBFUNC68K APTR myAllocVec32(__reg("a6") struct PPCBase* PowerPCBase, __reg("d0") ULONG memsize, __reg("d1") ULONG attributes)
{

    ULONG oldmemblock, memblock;
    struct Task *myTask;
    struct ExecBase *SysBase;

    if (PowerPCBase)
    {
        SysBase = PowerPCBase->PPC_SysLib;
    }
    else
    {
        SysBase = *((struct ExecBase **)4UL);
    }

    attributes &= (MEMF_CLEAR | MEMF_REVERSE);
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

LIBFUNC68K void myPowerDebugMode(__reg("a6") struct PrivatePPCBase* PowerPCBase, __reg("d0") ULONG debuglevel)
{
    struct PPCArgs pa;

    if (debuglevel > 3)
    {
        return;
    }

	pa.PP_Code      = PowerPCBase;
	pa.PP_Offset    = _LVOSetHardware;
	pa.PP_Flags     = 0;
	pa.PP_Stack     = 0;
	pa.PP_StackSize = 0;
	pa.PP_Regs[0]   = (ULONG) PowerPCBase;
    pa.PP_Regs[1]   = HW_SETDEBUGMODE;
    pa.PP_Regs[8]   = debuglevel;

	myRunPPC(PowerPCBase, (APTR)&pa);

    return;
}


/********************************************************************************************
*
*	void SPrintF68K(STRPTR Formatstring, APTR values) // a0,a1
*
********************************************************************************************/

APTR __RawPutChar(__reg("a6") void *, __reg("d0") UBYTE MyChar)="\tjsr\t-516(a6)";

#define RawPutChar(MyChar) __RawPutChar(SysBase, (MyChar))

void PutChProc(__reg("d0") UBYTE mychar, __reg("a3") APTR PutChData)
{
    struct ExecBase* SysBase = (struct ExecBase*)PutChData;
    RawPutChar(mychar);
    return;
}

LIBFUNC68K void mySPrintF68K(__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") STRPTR Formatstring,
                  __reg("a1") APTR values)
{
    struct ExecBase *SysBase = PowerPCBase->PPC_SysLib;

    RawDoFmt(Formatstring, values, &PutChProc, (APTR)SysBase);
    return;
}


/********************************************************************************************
*
*	void PutXMsg(struct MsgPort* MsgPortPPC, struct Message* message)
*
********************************************************************************************/

LIBFUNC68K void myPutXMsg(__reg("a6") struct PrivatePPCBase* PowerPCBase, __reg("a0") struct MsgPort* MsgPortPPC,
               __reg("a1") struct Message* message)
{
    message->mn_Node.ln_Type = NT_XMSG68K;
    struct MsgFrame* myFrame = CreateMsgFrame(PowerPCBase);

    myFrame->mf_Message.mn_Node.ln_Type = NT_MESSAGE;
    myFrame->mf_Message.mn_Length = MSGLEN;
    myFrame->mf_Identifier = ID_XPPC;
    myFrame->mf_Arg[0] = (ULONG)MsgPortPPC;
    myFrame->mf_Arg[1] = (ULONG)message;

    SendMsgFrame(PowerPCBase, myFrame);
    return;
}


/********************************************************************************************
*
*	void CausePPCInterrupt(void)
*
********************************************************************************************/

LIBFUNC68K void myCausePPCInterrupt(__reg("a6") struct PrivatePPCBase* PowerPCBase)
{
    switch (PowerPCBase->pp_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            writememLong(PowerPCBase->pp_BridgeMsgs, PMEP_MGIM0, -1);
            break;
        }
        case DEVICE_MPC8343E:
        {
            writememLong(PowerPCBase->pp_BridgeConfig, IMMR_IDR, IMMR_IDR_IDR0);
            break;
        }
        case DEVICE_MPC107:
        {
            break;
        }
    }
}

