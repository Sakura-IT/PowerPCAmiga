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
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <utility/tagitem.h>
#include <dos/dostags.h>
#include <powerpc/powerpc.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/prometheus.h>
#include <dos/doshunks.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>

#include "constants.h"
#include "libstructs.h"
#include "Internals68k.h"

extern APTR OldLoadSeg, OldNewLoadSeg, OldAllocMem, OldAddTask, OldRemTask;
extern struct ExecBase* mySysBase;
extern struct PPCBase* myPPCBase;

APTR  RemSysTask;
UBYTE powerlib[] = "powerpc.library\0";
UBYTE ampname[]  = "AmigaAMP\0";
UBYTE testlongs[] = "2005_68K_PPCsk_0sk_1\0\0\0\0";
UBYTE CrashMessage[] =	 
            "Task name: '%s'  Task address: %08lx\n"
			"Exception: %s\n\n"
			"SRR0: %08lx    SRR1:  %08lx     MSR:   %08lx    HID0: %08lx\n"
			"PVR:  %08lx    DAR:   %08lx     DSISR: %08lx    SDR1: %08lx\n"
			"DEC:  %08lx    TBU:   %08lx     TBL:   %08lx    XER:  %08lx\n"
			"CR:   %08lx    FPSCR: %08lx     LR:    %08lx    CTR:  %08lx\n\n"
			"R0-R3:   %08lx %08lx %08lx %08lx   IBAT0: %08lx %08lx\n"
			"R4-R7:   %08lx %08lx %08lx %08lx   IBAT1: %08lx %08lx\n"
			"R8-R11:  %08lx %08lx %08lx %08lx   IBAT2: %08lx %08lx\n"
			"R12-R15: %08lx %08lx %08lx %08lx   IBAT3: %08lx %08lx\n"
			"R16-R19: %08lx %08lx %08lx %08lx   DBAT0: %08lx %08lx\n"
			"R20-R23: %08lx %08lx %08lx %08lx   DBAT1: %08lx %08lx\n"
			"R24-R27: %08lx %08lx %08lx %08lx   DBAT2: %08lx %08lx\n"
			"R28-R31: %08lx %08lx %08lx %08lx   DBAT3: %08lx %08lx\n\n\0";

UBYTE msgPanic[] = "Kernel Panic!\0";

UBYTE excSysReset[]    = "System Reset\0";
UBYTE excMCheck[]      = "Machine Check\0";
UBYTE excDAccess[]     = "Data Storage\0";
UBYTE excIAccess[]     = "Instruction Storage\0";
UBYTE excInterrupt[]   = "External\0";
UBYTE excAlign[]       = "Alignment\0";
UBYTE excProgram[]     = "Program\0";
UBYTE excFPUn[]        = "FPU Unavailable\0";
UBYTE excDecrementer[] = "Decrementer\0";
UBYTE excAltivecUnav[] = "AltiVec Unavailable\0";
UBYTE excSC[]          = "System Call\0";
UBYTE excTrace[]       = "Trace\0";
UBYTE excFPAssist[]    = "Floating Point Assist\0";
UBYTE excPerfMon[]     = "Performance Monitor\0";
UBYTE excIABR[]        = "Instruction Breakpoint\0";
UBYTE excSysMan[]      = "System Management\0";
UBYTE excAVAssist[]    = "AltiVec Assist\0";
UBYTE excTherMan[]     = "Thermal Management\0";
UBYTE excUnsupported[] = "Unsupported\0";

ULONG ExcStrings[24]   = {(ULONG)&excUnsupported, (ULONG)&excUnsupported, (ULONG)&excMCheck,
                          (ULONG)&excDAccess, (ULONG)&excIAccess, (ULONG)&excInterrupt,
                          (ULONG)&excAlign, (ULONG)&excProgram, (ULONG)&excFPUn, (ULONG)&excDecrementer,
                          (ULONG)&excAltivecUnav, (ULONG)&excUnsupported, (ULONG)&excSC,
                          (ULONG)&excTrace, (ULONG)&excFPAssist, (ULONG)&excPerfMon,
                          (ULONG)&excUnsupported, (ULONG)&excUnsupported, (ULONG)&excUnsupported,
                          (ULONG)&excIABR, (ULONG)&excSysMan, (ULONG)&excUnsupported,
                          (ULONG)&excAVAssist, (ULONG)&excTherMan};


UBYTE FRun68K[]                      = "Run68K\0";
UBYTE FWaitFor68K[]                  = "WaitFor68K\0";
UBYTE FAllocVecPPC[]                 = "AllocVecPPC\0";
UBYTE FFreeVecPPC[]                  = "FreeVecPPC\0";
UBYTE FCreateTaskPPC[]               = "CreateTaskPPC\0";
UBYTE FDeleteTaskPPC[]               = "DeleteTaskPPC\0";
UBYTE FFindTaskPPC[]                 = "FindTaskPPC\0";
UBYTE FInitSemaphorePPC[]            = "InitSemaphorePPC\0";
UBYTE FFreeSemaphorePPC[]            = "FreeSemaphorePPC\0";
UBYTE FAddSemaphorePPC[]             = "AddSemaphorePPC\0";
UBYTE FRemSemaphorePPC[]             = "RemSemaphorePPC\0";
UBYTE FObtainSemaphorePPC[]          = "ObtainSemaphorePPC\0";
UBYTE FAttemptSemaphorePPC[]         = "AttemptSemaphorePPC\0";
UBYTE FReleaseSemaphorePPC[]         = "ReleaseSemaphorePPC\0";
UBYTE FFindSemaphorePPC[]            = "FindSemaphorePPC\0";
UBYTE FAllocSignalPPC[]              = "AllocSignalPPC\0";
UBYTE FFreeSignalPPC[]               = "FreeSignalPPC\0";
UBYTE FSetSignalPPC[]                = "SetSignalPPC\0";
UBYTE FSignalPPC[]                   = "SignalPPC\0";
UBYTE FWaitPPC[]                     = "WaitPPC\0";
UBYTE FSetTaskPriPPC[]               = "SetTaskPriPPC\0";
UBYTE FSignal68K[]                   = "Signal68K\0";
UBYTE FSetCache[]                    = "SetCache\0";
UBYTE FSetExcHandler[]               = "SetExcHandler\0";
UBYTE FRemExcHandler[]               = "RemExcHandler\0";
UBYTE FSetHardware[]                 = "SetHardware\0";
UBYTE FModifyFPExc[]                 = "ModifyFPExc\0";
UBYTE FWaitTime[]                    = "WaitTime\0";
UBYTE FChangeStack[]                 = "ChangeStack\0";
UBYTE FChangeMMU[]                   = "ChangeMMU\0";
UBYTE FGetInfo[]                     = "GetInfo\0";
UBYTE FCreateMsgPortPPC[]            = "CreateMsgPortPPC\0";
UBYTE FDeleteMsgPortPPC[]            = "DeleteMsgPortPPC\0";
UBYTE FAddPortPPC[]                  = "AddPortPPC\0";
UBYTE FRemPortPPC[]                  = "RemPortPPC\0";
UBYTE FFindPortPPC[]                 = "FindPortPPC\0";
UBYTE FWaitPortPPC[]                 = "WaitPortPPC\0";
UBYTE FPutMsgPPC[]                   = "PutMsgPPC\0";
UBYTE FGetMsgPPC[]                   = "GetMsgPPC\0";
UBYTE FReplyMsgPPC[]                 = "ReplyMsgPPC\0";
UBYTE FCopyMemPPC[]                  = "CopyMemPPC\0";
UBYTE FAllocXMsgPPC[]                = "AllocXMsgPPC\0";
UBYTE FFreeXMsgPPC[]                 = "FreeXMsgPPC\0";
UBYTE FPutXMsgPPC[]                  = "PutXMsgPPC\0";
UBYTE FGetSysTimePPC[]               = "GetSysTimePPC\0";
UBYTE FSetReplyPortPPC[]             = "SetReplyPortPPC\0";
UBYTE FSnoopTask[]                   = "SnoopTask\0";
UBYTE FEndSnoopTask[]                = "EndSnoopTask\0";
UBYTE FGetHALInfo[]                  = "GetHALInfo\0";
UBYTE FSetScheduling[]               = "SetScheduling\0";
UBYTE FFindTaskByID[]                = "FindTaskByID\0";
UBYTE FSetNiceValue[]                = "SetNiceValue\0";
UBYTE FTrySemaphorePPC[]             = "TrySemaphorePPC\0";
UBYTE FSetExceptPPC[]                = "SetExceptPPC\0";
UBYTE FObtainSemaphoreSharedPPC[]    = "ObtainSemaphoreSharedPPC\0";
UBYTE FAttemptSemaphoreSharedPPC[]   = "AttemptSempahoreSharedPPC\0";
UBYTE FProcurePPC[]                  = "ProcurePPC\0";
UBYTE FVacatePPC[]                   = "VacatePPC\0";
UBYTE FCreatePoolPPC[]               = "CreatePoolPPC\0";
UBYTE FDeletePoolPPC[]               = "DeletePoolPPC\0";
UBYTE FAllocPooledPPC[]              = "AllocPooledPPC\0";
UBYTE FFreePooledPPC[]               = "FreePooledPPC\0";
UBYTE FRawDoFmtPPC[]                 = "RawDoFmtPPC\0";
UBYTE FPutPublicMsgPPC[]             = "PutPublicMsgPPC\0";
UBYTE FAddUniquePortPPC[]            = "AddUniquePortPPC\0";
UBYTE FAddUniqueSemaphorePPC[]       = "AddUniqueSemaphorePPC\0";

UBYTE FAllocatePPC[]                 = "AllocatePPC\0";
UBYTE FDeallocatePPC[]               = "DeallocatePPC\0";

#if 0
UBYTE FSPrintF[]                     = "SPrintF\0";
UBYTE FInsertPPC[]                   = "InsertPPC\0";
UBYTE FAddHeadPPC[]                  = "AddHeadPPC\0";
UBYTE FAddTailPPC[]                  = "AddtailPPC\0";
UBYTE FRemovePPC[]                   = "RemovePPC\0";
UBYTE FRemHeadPPC[]                  = "RemHeadPPC\0";
UBYTE FRemTailPPC[]                  = "RemTailPPC\0";
UBYTE FEnqueuePPC[]                  = "EnqueuePPC\0";
UBYTE FFindNamePPC[]                 = "FindNamePPC\0";
UBYTE FFindTagItemPPC[]              = "FindTagItemPPC\0";
UBYTE FGetTagDataPPC[]               = "GetTagItemPPC\0";
UBYTE FNextTagItemPPC[]              = "NextTagItemPPC\0";
UBYTE FSuper[]                       = "Super\0";
UBYTE FUser[]                        = "User\0";
UBYTE FLockTaskList[]                = "LockTaskList\0";
UBYTE FUnLockTaskList[]              = "UnlockTaskList\0";
UBYTE FSetExcMMU[]                   = "SetExcMMU\0";
UBYTE FClearExcMMU[]                 = "ClearExcMMU\0";
UBYTE FFreeAllMem[]                  = "FreeAllMem\0";
UBYTE FAddTimePPC[]                  = "AddTimePPC\0";
UBYTE FSubTimePPC[]                  = "SubTimePPC\0";
UBYTE FCmpTimePPC[]                  = "CmpTimePPC\0";
UBYTE FNewListPPC[]                  = "NewListPPC\0";
UBYTE FCauseInterrupt[]              = "CauseInterrupt\0";
UBYTE FAllocPrivateMem[]             = "AllocPrivateMem\0";
UBYTE FFreePrivateMem[]              = "FreePrivateMem\0";
UBYTE FResetPPC[]                    = "ResetPPC\0";
UBYTE FIsExceptionMode[]             = "IsExceptionMode\0";
UBYTE FRun68KLowLevel[]              = "Run68KLowLevel\0";
#endif

ULONG FuncStrings[68]   = {(ULONG)&FRun68K, (ULONG)&FWaitFor68K, (ULONG)&FAllocVecPPC,
                          (ULONG)&FFreeVecPPC, (ULONG)&FCreateTaskPPC, (ULONG)&FDeleteTaskPPC,
                          (ULONG)&FFindTaskPPC, (ULONG)&FInitSemaphorePPC, (ULONG)&FFreeSemaphorePPC,
                          (ULONG)&FAddSemaphorePPC, (ULONG)&FRemSemaphorePPC, (ULONG)&FObtainSemaphorePPC,
                          (ULONG)&FAttemptSemaphorePPC, (ULONG)&FReleaseSemaphorePPC, (ULONG)&FFindSemaphorePPC,
                          (ULONG)&FAllocSignalPPC, (ULONG)&FFreeSignalPPC, (ULONG)&FSetSignalPPC,
                          (ULONG)&FSignalPPC, (ULONG)&FWaitPPC, (ULONG)&FSetTaskPriPPC,
                          (ULONG)&FSignal68K, (ULONG)&FSetCache, (ULONG)&FSetExcHandler,
                          (ULONG)&FRemExcHandler, (ULONG)&FSetHardware, (ULONG)&FModifyFPExc,
                          (ULONG)&FWaitTime, (ULONG)&FChangeStack, (ULONG)&FChangeMMU,
                          (ULONG)&FGetInfo, (ULONG)&FCreateMsgPortPPC, (ULONG)&FDeleteMsgPortPPC,
                          (ULONG)&FAddPortPPC, (ULONG)&FRemPortPPC, (ULONG)&FFindPortPPC,
                          (ULONG)&FWaitPortPPC, (ULONG)&FPutMsgPPC, (ULONG)&FGetMsgPPC,
                          (ULONG)&FReplyMsgPPC, (ULONG)&FCopyMemPPC, (ULONG)&FAllocXMsgPPC,
                          (ULONG)&FFreeXMsgPPC, (ULONG)&FPutXMsgPPC, (ULONG)&FGetSysTimePPC,
                          (ULONG)&FSetReplyPortPPC, (ULONG)&FSnoopTask, (ULONG)&FEndSnoopTask,
                          (ULONG)&FGetHALInfo, (ULONG)&FSetScheduling, (ULONG)&FFindTaskByID,
                          (ULONG)&FSetNiceValue, (ULONG)&FTrySemaphorePPC, (ULONG)&FSetExceptPPC,
                          (ULONG)&FObtainSemaphoreSharedPPC, (ULONG)&FAttemptSemaphoreSharedPPC, (ULONG)&FProcurePPC,
                          (ULONG)&FVacatePPC, (ULONG)&FCreatePoolPPC, (ULONG)&FDeletePoolPPC,
                          (ULONG)&FAllocPooledPPC, (ULONG)&FFreePooledPPC, (ULONG)&FRawDoFmtPPC,
                          (ULONG)&FPutPublicMsgPPC, (ULONG)&FAddUniquePortPPC, (ULONG)&FAddUniqueSemaphorePPC,
                          (ULONG)&FAllocatePPC,(ULONG)FDeallocatePPC};

#ifdef DEBUG

ULONG dsi_Count = 0;

struct dsiInfo
{
    ULONG dsi_Lower;
    ULONG dsi_Upper;
    ULONG dsi_Total;
};

struct dsiInfo di[10];


ULONG countDSI(__reg("d0") ULONG dsi_Address)
{
    ULONG init = 0;
    ULONG result = 1;
    struct PPCBase* PowerPCBase = myPPCBase;

    if (!(dsi_Count))
    {
        struct ExecBase* SysBase = mySysBase;
        struct MemHeader* currMem = (struct MemHeader*)SysBase->MemList.lh_Head;
        struct MemHeader* nextMem;

        while (nextMem = (struct MemHeader*)currMem->mh_Node.ln_Succ)
        {
            di[init].dsi_Lower = (ULONG)(currMem->mh_Lower);
            di[init].dsi_Upper = (ULONG)(currMem->mh_Upper);
            di[init].dsi_Total = 0;

            if (init++ == 9)
            {
                break;
            }
            currMem = nextMem;
        }

        while (init < 10)
        {
            di[init].dsi_Lower = 0;
            di[init].dsi_Upper = 0;
            di[init].dsi_Total = 0;
            init++;
        }
    }

    init = 0;

    while (di[init].dsi_Lower)
    {
        if ((di[init].dsi_Lower <= dsi_Address) && (di[init].dsi_Upper >= dsi_Address))
        {
            di[init].dsi_Total += 1;
            break;
        }
        init++;
    }

    if (!(di[init].dsi_Lower))
    {
        if (dsi_Address != 4)
        {
            di[9].dsi_Total += 1;
            result = 0;
            mySPrintF68K((struct PPCBase*)PowerPCBase, "Detected DSI hit outside of memory bounds at %08lx\n\0", (APTR)&dsi_Address);
        }
    }

    dsi_Count += 1;

    if (!(dsi_Count & 0x3ff))
    {
        mySPrintF68K((struct PPCBase*)PowerPCBase, "Overview of previous 1024 DSI hits\n\0",0);

        init = 0;
        while (di[init].dsi_Lower)
        {
             mySPrintF68K((struct PPCBase*)PowerPCBase, "DSI Count %08lx - %08lx: %ld\n\0", (APTR)&di[init]);
             init += 1;
        }

        if (di[9].dsi_Total)
        {
             mySPrintF68K((struct PPCBase*)PowerPCBase, "DSI Count unknown: %ld\n\0", (APTR)&di[9].dsi_Total);
        }

        mySPrintF68K((struct PPCBase*)PowerPCBase, "End of report\n\0",0);

        dsi_Count = 0;
    }
    return result;
}

#endif


/********************************************************************************************
*
*	Task patch functions. To remove PPC tasks and free memory when the 68K part is ended
*
*********************************************************************************************/

void commonRemTask(__reg("a1") struct Task* myTask, __reg("d0") ULONG flag, __reg("a6") struct ExecBase* SysBase)
{
    if (!(myTask))
    {
        myTask = SysBase->ThisTask;
    }

    if (myTask->tc_Node.ln_Type == NT_PROCESS)
    {
        struct PrivatePPCBase* PowerPCBase = (struct PrivatePPCBase*)myPPCBase;
        struct MirrorTask* currMir = (struct MirrorTask*)PowerPCBase->pp_MirrorList.mlh_Head;
        struct MirrorTask* nxtMir;
        while (nxtMir = (struct MirrorTask*)currMir->mt_Node.mln_Succ)
        {
            if (currMir->mt_Task == myTask)
            {
                if (!(flag))
                {
                    struct MsgFrame* myFrame = CreateMsgFrame(PowerPCBase);
                    myFrame->mf_Identifier = ID_END;
                    myFrame->mf_PPCTask = currMir->mt_PPCTask;
                    SendMsgFrame(PowerPCBase, myFrame);
                }
                Disable();
                Remove((struct Node*)currMir);
                Enable();
                DeleteMsgPort(currMir->mt_Port);
                FreeVec(currMir);

                break;
            }
            currMir = nxtMir;
        }
    }
    return;
}

/********************************************************************************************/

PATCH68K void SysExitCode(void)
{
    struct Task* myTask;
    void (*RemSysTask_ptr)() = RemSysTask;

    struct ExecBase* SysBase = myPPCBase->PPC_SysLib;
    myTask = SysBase->ThisTask;

    commonRemTask(myTask, 0, SysBase);

    RemSysTask_ptr();

    return;
}

/********************************************************************************************/

PATCH68K void patchRemTask(__reg("a1") struct Task* myTask, __reg("a6") struct ExecBase* SysBase)
{
    void (*RemTask_ptr)(__reg("a1") struct Task*, __reg("a6") struct ExecBase*) = OldRemTask;

    commonRemTask (myTask, 0, SysBase);

    RemTask_ptr(myTask, SysBase);

    return;
}

/********************************************************************************************/

PATCH68K APTR patchAddTask(__reg("a1") struct Task* myTask, __reg("a2") APTR initialPC,
                  __reg("a3") APTR finalPC, __reg("a6") struct ExecBase* SysBase)
{
    APTR (*AddTask_ptr)(__reg("a1") struct Task*, __reg("a2") APTR,
    __reg("a3") APTR, __reg("a6") struct ExecBase*) = OldAddTask;

    if (myTask->tc_Node.ln_Type == NT_PROCESS)
    {
        if (!(finalPC))
        {
            finalPC = (APTR)&patchRemTask;
        }
        else
        {
            if (!((ULONG)finalPC & 0xff000000))
            {
                RemSysTask = finalPC;
                finalPC = (APTR)&SysExitCode;
            }
        }
    }
    return ((*AddTask_ptr)(myTask, initialPC, finalPC, SysBase));
}

/********************************************************************************************
*
*	Allocate memory patch to load PPC (related) code to PPC memory
*
*********************************************************************************************/

PATCH68K APTR patchAllocMem(__reg("d0") ULONG byteSize, __reg("d1") ULONG attributes,
                   __reg("a6") struct ExecBase* SysBase)
{
    APTR (*AllocMem_ptr)(__reg("d0") ULONG, __reg("d1") ULONG, __reg("a6") struct ExecBase*) = OldAllocMem;

                    //AmigaAMP

    USHORT testAttr = (USHORT)attributes;

    if (testAttr)
    {
        if ( (testAttr & MEMF_CHIP) || !(testAttr & MEMF_PUBLIC))
        {
            return ((*AllocMem_ptr)(byteSize, attributes, SysBase));
        }
    }               //TB_WARN (A1200) not yet implemented

    struct Task* myTask = SysBase->ThisTask;
    char * myName = NULL;

    ULONG highTaskByte = (ULONG)(myTask) & 0xf0000000;

    if (highTaskByte)
    {
        highTaskByte &= 0xe0000000;
        struct PrivatePPCBase *privBase = (struct PrivatePPCBase*) myPPCBase;
        ULONG highBaseByte = (ULONG)(privBase->pp_PPCMemBase) & 0xe0000000;
        if (!(highTaskByte ^= highBaseByte))
        {
            myTask->tc_Flags |= TF_PPC;
        }
    }

    struct Process* myProcess = (struct Process*)myTask;

    if ((myTask->tc_Node.ln_Type != NT_PROCESS) || ((myTask->tc_Node.ln_Type == NT_PROCESS) && ((myProcess->pr_CLI == NULL))))
    {
        myName = myTask->tc_Node.ln_Name;
        while (myName[0] != 0)
        {
            myName ++;
        }
        myName -= 5;
    }
    else
    {
        struct CommandLineInterface* myCLI;

        if (myCLI = (struct CommandLineInterface *)((ULONG)(myProcess->pr_CLI << 2)))
        {
            if (myName = (char *)(ULONG)(myCLI->cli_CommandName << 2))
            {
                UBYTE offset = myName[0] - 3;
                if (!(offset & 0x80))
                {
                    myName += offset;
                }
            }
        }
    }

    ULONG testValue;
    ULONG longnum = 0;

    if (myName)
    {
        while (testValue = *((ULONG*)testlongs + longnum))
        {
            if (testValue == *((ULONG*)myName))
            {
                attributes |= MEMF_PPC;
                return ((*AllocMem_ptr)(byteSize, attributes, SysBase));
            }
        longnum += 1;
        }
        if ((*((LONG*)myName) == 0x70656564) || ((*((ULONG*)myName) == 0x2e657865) && (*((ULONG*)myName - 1) == 0x70616365)))
        {
            return ((*AllocMem_ptr)(byteSize, attributes, SysBase));
        }

    }

    if (myTask->tc_Flags & TF_PPC)
    {
        attributes |= MEMF_PPC;
    }
    return ((*AllocMem_ptr)(byteSize, attributes, SysBase));
}

/********************************************************************************************
*
*	Executable loading patches to load PPC (related) code to PPC memory
*   (AUTODOCS ARE WRONG, called with d1, d2, d3 and not d1, a0, d0)
*
*********************************************************************************************/

LONG ReadFunc(__reg("d1") BPTR readhandle, __reg("d2") APTR buffer, __reg("d3") LONG length, __reg("a6") struct DosLibrary* DOSBase)
{
    LONG result;

    ULONG* myBuffer = (ULONG*)buffer;

    result = Read(readhandle, buffer, length);

    if (!(myPPCBase->PPC_Flags & 0x4))
    {
        if ((myBuffer[0] == HUNK_HEADER) && (myBuffer[1] == NULL) && (myBuffer[3] == NULL) && (myBuffer[2] - myBuffer[4] - 1 == NULL))
        {
            if (((myBuffer[5] == 0x71E) && (myBuffer[6] == 0x710)) ||  //Cybermand
                ((myBuffer[5] == 0x84E) && (myBuffer[6] == 0xEE)) ||  //CyberPi
                ((myBuffer[5] == 0x9468) && (myBuffer[6] == 0x4F3))) //Radeon
            {
                return result;
            }
            else if (!(myBuffer[5] & (1<<HUNKB_CHIP)))
            {
                myBuffer[5] |= (1<<HUNKB_FAST);
            }
        }
    }
    return result;
}

/********************************************************************************************/

APTR AllocFunc(__reg("d0") ULONG size, __reg("d1") ULONG flags, __reg("a6") struct ExecBase* SysBase)
{
    APTR memBlock;

    if (!(myPPCBase->PPC_Flags & 0x4))
    {
        if ((flags == (MEMF_PUBLIC | MEMF_FAST)) || (flags == (MEMF_PUBLIC | MEMF_CHIP)))
        {
            SysBase->ThisTask->tc_Flags &= ~TF_PPC;
            memBlock = AllocMem(size, flags);
            SysBase->ThisTask->tc_Flags |= TF_PPC;
            return memBlock;
        }
    }
    SysBase->ThisTask->tc_Flags |= TF_PPC;
    flags = (flags & (MEMF_REVERSE | MEMF_CLEAR)) | MEMF_PUBLIC | MEMF_PPC;
    memBlock = AllocMem(size, flags);
    return memBlock;
}

void FreeFunc(__reg("a1") APTR memory, __reg("d0") ULONG size, __reg("a6") struct ExecBase* SysBase)
{
    FreeMem(memory, size);

    return;
}

/********************************************************************************************/

BPTR myLoader(STRPTR name) // -1 = try normal 0 = fail
{
    struct FuncTable
    {
        APTR ReadFunc;
        APTR AllocFunc;
        APTR FreeFunc;
        ULONG Stack;
    };

    struct FuncTable myFuncTable;
    struct ExecBase* SysBase   = myPPCBase->PPC_SysLib;
    struct DosLibrary* DOSBase = myPPCBase->PPC_DosLib;
    BPTR myLock, mySeglist;
    struct FileInfoBlock* myFIB;
    LONG myProt;

    if (*((ULONG*)SysBase->ThisTask->tc_Node.ln_Name) == 0x44656649)    //DefI(cons)
    {
        return -1;
    }

    SysBase->ThisTask->tc_Flags &= ~TF_PPC;

    if (myLock = Open(name, MODE_OLDFILE))
    {
        
        if (myFIB = (struct FileInfoBlock*)AllocDosObject(DOS_FIB, NULL))
        {
            if (ExamineFH(myLock, myFIB))
            {
                myProt = myFIB->fib_Protection;
                FreeDosObject(DOS_FIB, (APTR)myFIB);
                if (myProt & 0x20000)
                {                    
                    Close(myLock);
                    return -1;
                }
                else
                {
                    SysBase->ThisTask->tc_Flags |= TF_PPC;
                    myFuncTable.ReadFunc  = (APTR)&ReadFunc;
                    myFuncTable.AllocFunc = (APTR)&AllocFunc;
                    myFuncTable.FreeFunc  = (APTR)&FreeFunc;
                    myFuncTable.Stack     = 0;
                    mySeglist = InternalLoadSeg(myLock, NULL, (LONG*)&myFuncTable, (LONG*)&myFuncTable.Stack);
                    Close(myLock);
                    if (mySeglist < 0)
                    {
                        UnLoadSeg(!mySeglist);
                        SetProtection(name, myProt | 0x20000);
                        SysBase->ThisTask->tc_Flags &= ~TF_PPC;
                        return -1;
                     }
                    else if (!(mySeglist))
                    {
                        return NULL;
                    }
                    else
                    {
                        if (myProt & 0x10000)
                        {
                            return mySeglist;
                        }
                        BPTR testSeglist = mySeglist;
                        while (testSeglist)
                        {
                            ULONG curPointer = (ULONG)testSeglist << 2;
                            UBYTE* mySegData = (UBYTE*)(curPointer + 4);
                            LONG mySegSize   = (*((ULONG*)(curPointer - 4)));
                            testSeglist = (*((ULONG*)(curPointer)));

                            for (int i=mySegSize; i >= 0; i--)
                            {
                                ULONG* myLongData = (ULONG*)mySegData;
                                if (myLongData[0] == 0x52414345)            // "RACE"
                                {
                                    SetProtection(name, myProt | 0x10000);
                                    return mySeglist;
                                }
                                for (int i=0; i<20; i++)
                                {
                                    if (mySegData[i] == powerlib[i])
                                    {
                                        if (powerlib[i+1] == 0)
                                        {
                                            SetProtection(name, myProt | 0x10000);
                                            return mySeglist;
                                        }
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                for (int i=0; i<20; i++)
                                {
                                    if (mySegData[i] == ampname[i])
                                    {
                                        if (ampname[+1] == 0)
                                        {
                                            SetProtection(name, myProt | 0x10000);
                                            return mySeglist;
                                        }
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                mySegData += 1;
                            }
                        }
                        UnLoadSeg(mySeglist);
                        SetProtection(name, myProt | 0x20000);
                        SysBase->ThisTask->tc_Flags &= ~TF_PPC;
                    }
                }
            }
        }
    }
    return -1;
}

/********************************************************************************************/

PATCH68K BPTR patchLoadSeg(__reg("d1") STRPTR name, __reg("a6") struct DosLibrary* DOSBase)
{
    BPTR (*LoadSeg_ptr)(__reg("d1") STRPTR, __reg("a6") struct DosLibrary*) = OldLoadSeg;

    BPTR mySegList;

    mySegList = myLoader(name);

    if (mySegList == -1)
    {
        return ((*LoadSeg_ptr)(name, DOSBase));
    }
    return mySegList;
}

/********************************************************************************************/

PATCH68K BPTR patchNewLoadSeg(__reg("d1") STRPTR file, __reg("d2") struct TagItem* tags,
                     __reg("a6") struct DosLibrary* DOSBase)
{
    BPTR (*NewLoadSeg_ptr)(__reg("d1") STRPTR, __reg("d2") struct TagItem*, __reg("a6") struct DosLibrary*) = OldNewLoadSeg;

    BPTR mySegList;
    if (tags)
    {
        return ((*NewLoadSeg_ptr)(file, tags, DOSBase));
    }

    mySegList = myLoader(file);

    if (mySegList == -1)
    {
        return ((*NewLoadSeg_ptr)(file, tags, DOSBase));
    }
    return mySegList;
}

/********************************************************************************************
*
*	Process that handles messages from the PPC.
*
*********************************************************************************************/

FUNC68K void MirrorTask(void)
{
	struct PrivatePPCBase* PowerPCBase = (struct PrivatePPCBase*)myPPCBase;
	struct ExecBase* SysBase = PowerPCBase->pp_PowerPCBase.PPC_SysLib;
	struct MsgFrame* myFrame;
    struct MsgPort* mirrorPort;

    struct Task* myTask = SysBase->ThisTask;

    myTask->tc_Flags &= ~TF_PPC;

    if (!(mirrorPort = CreateMsgPort()))
    {
        PrintError(SysBase, "General memory allocation error");
        return;
    }

    myTask->tc_Flags |= TF_PPC;

    ULONG mySignal = Wait(SIGBREAKF_CTRL_F);

    if (myFrame = (struct MsgFrame*)myTask->tc_UserData)
    {
        PutMsg(mirrorPort, (struct Message*)myFrame);
    }

    ULONG andTemp = ~(0xfff + (1 << (ULONG)mirrorPort->mp_SigBit));

    while (1)
    {
        mySignal = Wait((ULONG)myTask->tc_SigAlloc & 0xfffff000);

        if (mySignal & (1 << (ULONG)mirrorPort->mp_SigBit))
        {
            myTask->tc_SigRecvd |= (mySignal & andTemp);
            while (myFrame = (struct MsgFrame*)GetMsg(mirrorPort))
            {
            if (myFrame->mf_Identifier == ID_T68K)
            {
                myTask->tc_SigRecvd |= myFrame->mf_Signals;
                myFrame->mf_MirrorPort = mirrorPort;

                Run68KCode(SysBase, &myFrame->mf_PPCArgs);

                struct MsgFrame* doneFrame = CreateMsgFrame(PowerPCBase);

                CopyMemQuick((APTR)myFrame, (APTR)doneFrame, sizeof(struct MsgFrame));

                doneFrame->mf_Identifier = ID_DONE;
                doneFrame->mf_Signals    = myTask->tc_SigRecvd & andTemp;
                doneFrame->mf_Arg[0]     = (ULONG)myTask;
                doneFrame->mf_Arg[1]     = myTask->tc_SigAlloc;

                myTask->tc_SigRecvd ^= doneFrame->mf_Signals;

                SendMsgFrame(PowerPCBase, doneFrame);
                FreeMsgFrame(PowerPCBase, myFrame);
            }
            else if (myFrame->mf_Identifier == ID_END)
            {
                FreeMsgFrame(PowerPCBase, myFrame);
                return;
            }
            else
            {
                FreeMsgFrame(PowerPCBase, myFrame);
                PrintError(SysBase, "68K mirror task received illegal command packet");
            }
            }
        }
        else if (mySignal & andTemp)
        {
            struct MsgFrame* crossFrame = CreateMsgFrame(PowerPCBase);
            crossFrame->mf_Identifier   = ID_LLPP;
            crossFrame->mf_Signals      = (mySignal & andTemp);
            crossFrame->mf_Arg[0]       = (ULONG)myTask;
            SendMsgFrame(PowerPCBase, crossFrame);
        }
    }
}

/********************************************************************************************/

FUNC68K void MasterControl(void)
{
	struct PrivatePPCBase* PowerPCBase = (struct PrivatePPCBase*)myPPCBase;
	struct ExecBase* SysBase = PowerPCBase->pp_PowerPCBase.PPC_SysLib;
    struct DosLibrary* DOSBase = PowerPCBase->pp_PowerPCBase.PPC_DosLib;
	struct MsgFrame* myFrame;
    struct MsgPort* mcPort;
	struct PPCZeroPage* myZP = (struct PPCZeroPage*)PowerPCBase->pp_PPCMemBase;

	if (!(mcPort = CreateMsgPort()))
    {
        PrintError(SysBase, "General memory allocation error");
        return;
    }

	PowerPCBase->pp_MasterControl = mcPort;

    ULONG mySignal = 0;

	while (!(mySignal & SIGBREAKF_CTRL_F))
	{
		mySignal = Wait(SIGBREAKF_CTRL_F);
	}

	struct Task* myTask = SysBase->ThisTask;

	//struct InternalConsts *myData = (struct InternalConsts*)myTask->tc_UserData;

    myZP->zp_Status = STATUS_INIT;

	while (1)
	{
		WaitPort(mcPort);

		while (myFrame = (struct MsgFrame*)GetMsg(mcPort))
		{
			myTask->tc_Flags |= TF_PPC;
			if (myFrame->mf_Message.mn_Node.ln_Type == NT_REPLYMSG)
			{
				if(myFrame->mf_Message.mn_Node.ln_Name)
				{
					struct MsgFrame* newFrame = CreateMsgFrame(PowerPCBase);
					newFrame->mf_Identifier = ID_XMSG;
					newFrame->mf_Arg[0] = (ULONG)myFrame;
					newFrame->mf_Message.mn_ReplyPort = (struct MsgPort*)myFrame->mf_Message.mn_Node.ln_Name;
					myFrame->mf_Message.mn_ReplyPort = (struct MsgPort*)myFrame->mf_Message.mn_Node.ln_Name;
					SendMsgFrame(PowerPCBase, newFrame);
				}
				else
				{
					PrintError(SysBase, "MasterControl received illegal reply message");
				}
			}
			else
			{
				switch (myFrame->mf_Identifier)
				{
					case ID_T68K:
					{
                        char *ppcName = myFrame->mf_PPCTask->tp_Task.tc_Node.ln_Name;
						char Name68k[255];
						char nameAppend[] = "_68K\0";

						UBYTE offset = 0;
						while (Name68k[offset] = ppcName[offset])
						{
							offset++;
						}

						CopyMem(&nameAppend, &Name68k[offset], 5);

						struct Process* myProc;
						myTask->tc_Flags &= ~TF_PPC;
						myProc = CreateNewProcTags(
						NP_Entry, (ULONG)&MirrorTask,
						NP_Name, &Name68k,
						NP_Priority, 0,
						NP_StackSize, 0x20000,
						TAG_DONE);
						myTask->tc_Flags |= TF_PPC;

						if(!(myProc))
						{
							PrintError(SysBase, "Could not start 68K mirror process");
							break;
						}
						myProc->pr_Task.tc_UserData = (APTR)myFrame;
						myProc->pr_Task.tc_SigAlloc = myFrame->mf_Arg[0];
						Signal((struct Task*)myProc, SIGBREAKF_CTRL_F);
						break;
					}
					case ID_LL68:
					{
                        ULONG (*DoLL68K_ptr)(__reg("d0") ULONG, __reg("d1") ULONG, __reg("a0") ULONG,
							 __reg("a1") ULONG, __reg("a6") ULONG) =
							 (APTR)(myFrame->mf_PPCArgs.PP_Regs[0] + myFrame->mf_PPCArgs.PP_Regs[1]);
						ULONG result = (*DoLL68K_ptr)(myFrame->mf_PPCArgs.PP_Regs[4], myFrame->mf_PPCArgs.PP_Regs[5],
								myFrame->mf_PPCArgs.PP_Regs[2], myFrame->mf_PPCArgs.PP_Regs[3],
								myFrame->mf_PPCArgs.PP_Regs[0]);
						struct MsgFrame* newFrame = CreateMsgFrame(PowerPCBase);
						newFrame->mf_PPCArgs.PP_Regs[0] = result;
						newFrame->mf_Identifier = ID_DNLL;
                        newFrame->mf_PPCTask = myFrame->mf_PPCTask;

                        SendMsgFrame(PowerPCBase, newFrame);
                        FreeMsgFrame(PowerPCBase, myFrame);

						break;
					}
#if 0
					case ID_____:
					{
						
                        void (*FreeMem_ptr)(__reg("d0") ULONG, __reg("a1") ULONG, __reg("a6") struct ExecBase*) =
							 (APTR)(myFrame->mf_PPCArgs.PP_Regs[0] + myFrame->mf_PPCArgs.PP_Regs[1]);
						(*FreeMem_ptr)(myFrame->mf_PPCArgs.PP_Regs[4], myFrame->mf_PPCArgs.PP_Regs[3], SysBase);
						FreeMsgFrame(PowerPCBase, myFrame);
						break;
					}
#endif
					case ID_DBGS:
					{
                        struct DebugArgs* dbargs = (struct DebugArgs*)&myFrame->mf_PPCArgs;
                        ULONG numargs = dbargs->db_Function >> 8;
                        dbargs->db_Function = FuncStrings[(dbargs->db_Function & 0xff)];
                        switch (numargs)
                        {
                            case 1:
                            {
                                mySPrintF68K((struct PPCBase*)PowerPCBase, "%s -> %s(%08lx)\n\0", (APTR)dbargs);
                                break;
                            }
                            case 2:
                            {
                                mySPrintF68K((struct PPCBase*)PowerPCBase, "%s -> %s(%08lx, %08lx)\n\0", (APTR)dbargs);
                                break;
                            }
                            case 3:
                            {
                                mySPrintF68K((struct PPCBase*)PowerPCBase, "%s -> %s(%08lx, %08lx, %08lx)\n\0", (APTR)dbargs);
                                break;
                            }
                            case 4:
                            {
                                mySPrintF68K((struct PPCBase*)PowerPCBase, "%s -> %s(%08lx, %08lx, %08lx, %08lx)\n\0", (APTR)dbargs);
						        break;
                            }
                            default:
                            {
                                mySPrintF68K((struct PPCBase*)PowerPCBase, "%s -> %s()\n\0", (APTR)dbargs);
                               break;
                            }

                        }
                        FreeMsgFrame(PowerPCBase, myFrame);
						break;
					}
					case ID_DBGE:
					{
                        struct DebugArgs* dbargs = (struct DebugArgs*)&myFrame->mf_PPCArgs;
                        dbargs->db_Function = FuncStrings[(dbargs->db_Function & 0xff)];
                        mySPrintF68K((struct PPCBase*)PowerPCBase, "%s -> %s() = %08lx\n\0", (APTR)dbargs);
						FreeMsgFrame(PowerPCBase, myFrame);
						break;
					}
					case ID_CRSH:
					{
                        struct DosLibrary* DOSBase = PowerPCBase->pp_PowerPCBase.PPC_DosLib;
						FreeMsgFrame(PowerPCBase, myFrame);
						BPTR excWindow;
						if (!(excWindow = Open("CON:0/20/680/250/PowerPC Exception/AUTO/CLOSE/WAIT/INACTIVE", MODE_NEWFILE)))
						{
							PrintError(SysBase, "PPC crashed but could not output crash window");
							break;
						}
						if(!(*((ULONG*)(PowerPCBase->pp_PPCMemBase + (ULONG)(FIFO_END + 0x100)))))
						{
							*((ULONG*)(PowerPCBase->pp_PPCMemBase + (ULONG)(FIFO_END + 0x100))) = (ULONG)&msgPanic;
						}
                        ULONG* errorData = (ULONG*)(PowerPCBase->pp_PPCMemBase + (ULONG)(FIFO_END + 0x100));
                        errorData[2] = ExcStrings[(errorData[2])];

						VFPrintf(excWindow, (STRPTR)&CrashMessage, (LONG*)(PowerPCBase->pp_PPCMemBase + (ULONG)(FIFO_END + 0x100)));
						switch (*((ULONG*)(PowerPCBase->pp_PPCMemBase + (ULONG)(FIFO_END + 0x14c))))
						{
							case ERR_ESEM:
							{
								PrintError(SysBase, "PPC Semaphore in illegal state");
								break;
							}
							case ERR_EFIF:
							{
								PrintError(SysBase, "PPC received an illegal command packet");
								break;
							}
							case ERR_ESNC:
							{
								PrintError(SysBase, "Async Run68K function not supported");
								break;
							}
							case ERR_EMEM:
							{
								PrintError(SysBase, "PPC CPU ran out of memory");
								break;
							}
							case ERR_ECOR:
							{
								PrintError(SysBase, "PPC Memory corruption detected during freeing");
								break;
							}
							case ERR_ETIM:
							{
								PrintError(SysBase, "PPC timed out while waiting on 68K");
								break;
							}
							case ERR_R68K:
							{
								PrintError(SysBase, "Illegal Run68K function execution detected");
								break;
							}
						}
						break;
					}
					default:
					{
                        PrintError(SysBase, "68K received an illegal command packet");
						break;
					}
				}
			}
		}
	}
}

/********************************************************************************************
*
*	Interrupt that gets called by the PPC card.
*
*********************************************************************************************/

FUNC68K ULONG GortInt(__reg("a1") APTR data, __reg("a5") APTR code)
{
    struct PrivatePPCBase* PowerPCBase = (struct PrivatePPCBase*)myPPCBase;
    struct ExecBase* SysBase = PowerPCBase->pp_PowerPCBase.PPC_SysLib;
	struct MsgFrame* myFrame;
	ULONG flag = 0;

    switch (PowerPCBase->pp_DeviceID)
	{
		case DEVICE_HARRIER:
		{
			if (readmemL(PowerPCBase->pp_BridgeMsgs, PMEP_MIST))
            {
                flag = 1;
            }
            break;
		}
		case DEVICE_MPC107:
		{
			if ((readmemL(PowerPCBase->pp_BridgeMsgs, MPC107_OMISR)) & MPC107_OPQI)
            {
                flag = 1;
            }
            break;
		}
		case DEVICE_MPC8343E:
        case DEVICE_MPC8314E:
		{
			if ((data == (APTR)SUPERKEY) || (readmemL(PowerPCBase->pp_BridgeConfig, IMMR_OMISR) & IMMR_OMISR_OM0I))
			{
                writememL(PowerPCBase->pp_BridgeConfig, IMMR_OMISR, IMMR_OMISR_OM0I);
                flag = 1;
			}
            break;
		}
		default:
		{
			break; //error
		}
	}
    if (flag)
    {
		while ((myFrame = GetMsgFrame(PowerPCBase)) != (APTR)-1)
		{
            switch (myFrame->mf_Identifier)
			{
				case ID_T68K:
				case ID_END:
				{
                    struct MsgPort* mirrorPort;
                    if (mirrorPort = myFrame->mf_MirrorPort)
					{
						if (myFrame->mf_Identifier != ID_END)
                        {
                            struct Task* sigTask = mirrorPort->mp_SigTask;
						    sigTask->tc_SigAlloc = myFrame->mf_Arg[0];
						}
                        PutMsg(mirrorPort, &myFrame->mf_Message);
                    }
					else
					{
                        PutMsg(PowerPCBase->pp_MasterControl, &myFrame->mf_Message);
					}
					break;
				}
				case ID_FPPC:
				{
                    struct MsgPort* replyPort = myFrame->mf_Message.mn_ReplyPort;
					struct Task* sigTask = replyPort->mp_SigTask;
					sigTask->tc_SigAlloc = myFrame->mf_Arg[0];
					ReplyMsg(&myFrame->mf_Message);
					break;
				}
				case ID_XMSG:
				{
					struct MsgPort* xPort = (struct MsgPort*)myFrame->mf_Arg[0];
					struct Message* xMessage = (struct Message*)myFrame->mf_Arg[1];
					xMessage->mn_Node.ln_Name = (APTR)xMessage->mn_ReplyPort;
					xMessage->mn_ReplyPort = PowerPCBase->pp_MasterControl;
					FreeMsgFrame(PowerPCBase, myFrame);
					PutMsg(xPort, xMessage);
					break;
				}
				case ID_SIG:
				{
					Signal((struct Task*)myFrame->mf_Arg[0], myFrame->mf_Signals);
					FreeMsgFrame(PowerPCBase, myFrame);
					break;
				}
				case ID_RX68:
				{
					struct Message* xMessage = (struct Message*)myFrame->mf_Arg[0];
					struct MsgPort* xPort = xMessage->mn_ReplyPort;
					FreeMsgFrame(PowerPCBase, myFrame);
					PutMsg(xPort, xMessage);
					break;
				}
				case ID_GETV:
				{
                    myFrame->mf_Arg[0] = *((ULONG*)(myFrame->mf_Arg[1]));
#ifdef DEBUG
                    if (!(countDSI(myFrame->mf_Arg[1])))
                    {
                        myFrame->mf_Arg[0] = ERR_EMEM;
                    }
#endif
					myFrame->mf_Identifier = ID_DONE;
					FreeMsgFrame(PowerPCBase, myFrame);
					break;
				}
				case ID_PUTW:
				{
                    *((ULONG*)(myFrame->mf_Arg[1])) = myFrame->mf_Arg[0];
					myFrame->mf_Identifier = ID_DONE;
#ifdef DEBUG
                    countDSI(myFrame->mf_Arg[1]);
#endif

					FreeMsgFrame(PowerPCBase, myFrame);
					break;
				}
				case ID_PUTH:
				{
                    *((USHORT*)(myFrame->mf_Arg[1])) = (USHORT)myFrame->mf_Arg[0];
					myFrame->mf_Identifier = ID_DONE;
#ifdef DEBUG
                    countDSI(myFrame->mf_Arg[1]);
#endif
					FreeMsgFrame(PowerPCBase, myFrame);
					break;
				}
				case ID_PUTB:
				{
                    *((UBYTE*)(myFrame->mf_Arg[1])) = (UBYTE)myFrame->mf_Arg[0];
					myFrame->mf_Identifier = ID_DONE;
#ifdef DEBUG
                    countDSI(myFrame->mf_Arg[1]);
#endif
					FreeMsgFrame(PowerPCBase, myFrame);
					break;
				}
				default:
				{
					PutMsg(PowerPCBase->pp_MasterControl, (struct Message*)myFrame);
                    break;
				}
			}
		}
    }
	return flag; //debugdebug should be flag or 0
}

/********************************************************************************************
*
*	VBLANK Interrupt that supports unreliable (for now) messaging from K1/M1
*
*********************************************************************************************/

FUNC68K ULONG ZenInt(__reg("a1") APTR data, __reg("a5") APTR code)
{

    struct PrivatePPCBase* PowerPCBase = (struct PrivatePPCBase*)myPPCBase;
    struct Custom* custom = (struct Custom*)CUSTOMBASE;

	ULONG configBase = PowerPCBase->pp_BridgeConfig;

	if (!(readmemL(PowerPCBase->pp_BridgeConfig, IMMR_OMISR) & IMMR_OMISR_OM0I))
	{
		struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));
		if (myFIFO->kf_MIOPT != myFIFO->kf_MIOPH)
		{
            custom->intena = INTF_PORTS;
            data = (APTR)SUPERKEY;
			GortInt(data, code);
			custom->intena = INTF_SETCLR|INTF_INTEN|INTF_PORTS;
		}
	}
	return 0;
}

/********************************************************************************************
*
*	Functions that creates a message for the PPC
*
*********************************************************************************************/

struct MsgFrame* CreateMsgFrame(struct PrivatePPCBase* PowerPCBase)
{
    struct ExecBase* SysBase = PowerPCBase->pp_PowerPCBase.PPC_SysLib;
    struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));
    ULONG msgFrame = NULL;

    Disable();

    while (1)
    {
        switch (PowerPCBase->pp_DeviceID)
        {
            case DEVICE_HARRIER:
            {
                msgFrame = readmemL(PowerPCBase->pp_BridgeMsgs, PMEP_MIIQ);
                break;
            }
            case DEVICE_MPC8343E:
            case DEVICE_MPC8314E:
            {
                msgFrame = *((ULONG*)(myFIFO->kf_MIIFT));
                myFIFO->kf_MIIFT = (myFIFO->kf_MIIFT + 4) & 0xffff3fff;
                break;
            }
            case DEVICE_MPC107:
            {
                msgFrame = readmemL(PowerPCBase->pp_BridgeMsgs, MPC107_IFQPR);
                break;
            }
        }
        if (msgFrame != myFIFO->kf_CreatePrevious)
        {
            myFIFO->kf_CreatePrevious = msgFrame;
            break;
        }
    }
    Enable();

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
*	Function that sends a message to the PPC and subsequently interrupts the PPC
*
*********************************************************************************************/

void SendMsgFrame(struct PrivatePPCBase* PowerPCBase, struct MsgFrame* msgFrame)
{
    struct ExecBase* SysBase = PowerPCBase->pp_PowerPCBase.PPC_SysLib;

    Disable();

    switch (PowerPCBase->pp_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            writememL(PowerPCBase->pp_BridgeMsgs, PMEP_MIIQ, (ULONG)msgFrame);
            break;
        }

        case DEVICE_MPC8343E:
        case DEVICE_MPC8314E:
        {
            struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));

            *((ULONG*)(myFIFO->kf_MIIPH)) = (ULONG)msgFrame;
            myFIFO->kf_MIIPH = (myFIFO->kf_MIIPH + 4) & 0xffff3fff;
            writememL(PowerPCBase->pp_BridgeConfig, IMMR_IMR0, (ULONG)msgFrame);
            break;
        }

        case DEVICE_MPC107:
        {
            writememL(PowerPCBase->pp_BridgeMsgs, MPC107_IFQPR, (ULONG)msgFrame);
            break;
        }

        default:
        {
            break;
        }
    }

    Enable();

    return;
}

/********************************************************************************************
*
*	Function that frees a message send to the 68K processor by the PPC
*
*********************************************************************************************/

void FreeMsgFrame(struct PrivatePPCBase* PowerPCBase, struct MsgFrame* msgFrame)
{
    struct ExecBase* SysBase = PowerPCBase->pp_PowerPCBase.PPC_SysLib;

    Disable();

    msgFrame->mf_Identifier = ID_FREE;

    switch (PowerPCBase->pp_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            ULONG msgOffset = readmemL(PowerPCBase->pp_BridgeConfig, XCSR_MIOFH);
            writememL(PowerPCBase->pp_BridgeConfig, XCSR_MIOFH, (msgOffset + 4) & 0xffff3fff);
            writememL(PowerPCBase->pp_PPCMemBase, msgOffset, (ULONG)msgFrame);
            break;
        }
        case DEVICE_MPC8343E:
        case DEVICE_MPC8314E:
        {
            struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));

            *((ULONG*)(myFIFO->kf_MIOFH)) = (ULONG)msgFrame;
            myFIFO->kf_MIOFH = (myFIFO->kf_MIOFH + 4) & 0xffff3fff;
            break;
        }
        case DEVICE_MPC107:
        {
            writememL(PowerPCBase->pp_BridgeMsgs, MPC107_OFQPR, (ULONG)msgFrame);
            break;
        }
    }

    Enable();

    return;
}

/********************************************************************************************
*
*	Function that receives a message from the FIFO which was send to the 68K by the PPC
*
*********************************************************************************************/

struct MsgFrame* GetMsgFrame(struct PrivatePPCBase* PowerPCBase)
{
    struct ExecBase* SysBase = PowerPCBase->pp_PowerPCBase.PPC_SysLib;
    struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));
    ULONG msgFrame = -1;

    Disable();

    while (1)
    {
        switch (PowerPCBase->pp_DeviceID)
        {
            case DEVICE_HARRIER:
            {
                if (readmemL(PowerPCBase->pp_BridgeConfig, XCSR_MIOPT) ==
                    readmemL(PowerPCBase->pp_BridgeConfig, XCSR_MIOPH))
                {
                    break;
                }
                else
                {
                    ULONG msgOffset = readmemL(PowerPCBase->pp_BridgeConfig, XCSR_MIOPT);
                    msgFrame = readmemL(PowerPCBase->pp_PPCMemBase, msgOffset);
                    writememL(PowerPCBase->pp_BridgeConfig, XCSR_MIOPT, (msgOffset + 4) & 0xffff3fff);
                }
                break;
            }
            case DEVICE_MPC8343E:
            case DEVICE_MPC8314E:
            {
                if (myFIFO->kf_MIOPT == myFIFO->kf_MIOPH)
                {
                    break;
                }
                else
                {
                    msgFrame = *((ULONG*)(myFIFO->kf_MIOPT));
                    myFIFO->kf_MIOPT = (myFIFO->kf_MIOPT + 4) & 0xffff3fff;
                }
                break;
            }
            case DEVICE_MPC107:
            {
                msgFrame = readmemL(PowerPCBase->pp_BridgeMsgs, MPC107_OFQPR);
                break;
            }
        }
        if (msgFrame == -1)
        {
            break;
        }
        else if (msgFrame != myFIFO->kf_GetPrevious)
        {
            myFIFO->kf_GetPrevious = msgFrame;
            break;
        }
    }

    Enable();

    return (struct MsgFrame*)msgFrame;
}


//CreateProc, CreateNewProc, RunCommand, SystemTagList
