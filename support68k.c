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
#include <dos/doshunks.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>

#include "constants.h"
#include "libstructs.h"
#include "Internals68k.h"

extern APTR OldLoadSeg, OldNewLoadSeg, OldAllocMem, OldAddTask, OldRemTask;
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

/********************************************************************************************
*
*	Task patch functions. To remove PPC tasks and free memory when the 68K part is ended
*
*********************************************************************************************/

void commonRemTask(__reg("a1") struct Task* myTask, __reg("a6") struct ExecBase* SysBase)
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
                struct MsgFrame* myFrame = CreateMsgFrame(PowerPCBase);
                myFrame->mf_Identifier = ID_END;
                SendMsgFrame(PowerPCBase, myFrame);
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

PATCH68K void SysExitCode(void)
{
    struct Task* myTask;
    void (*RemSysTask_ptr)() = RemSysTask;

    struct ExecBase* SysBase = myPPCBase->PPC_SysLib;
    myTask = SysBase->ThisTask;

    commonRemTask(myTask, SysBase);

    RemSysTask_ptr();

    return;
}

PATCH68K void patchRemTask(__reg("a1") struct Task* myTask, __reg("a6") struct ExecBase* SysBase)
{
    void (*RemTask_ptr)(__reg("a1") struct Task*, __reg("a6") struct ExecBase*) = OldRemTask;

    commonRemTask (myTask, SysBase);

    RemTask_ptr(myTask, SysBase);

    return;
}

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
            if (((myBuffer[5] == 0x71E) && (myBuffer[6] == 0x710)) || ((myBuffer[5] == 0x84E) && (myBuffer[6] == 0xEE)))
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

	ULONG mySignal = 0;

	while (!(mySignal & SIGBREAKF_CTRL_F))
	{
		mySignal = Wait(SIGBREAKF_CTRL_F);
	}


	myFrame = (struct MsgFrame*)myTask->tc_UserData;

    ULONG andTemp = ~(0xfff + (1 << (ULONG)mirrorPort->mp_SigBit));

    while (1)
    {
        while (myFrame)
        {
            if (myFrame->mf_Identifier == ID_END)
            {
                FreeMsgFrame(PowerPCBase, myFrame);
                return;
            }
            else if (myFrame->mf_Identifier == ID_T68K)
            {
                myTask->tc_SigRecvd |= myFrame->mf_Arg[2];
                myFrame->mf_MirrorPort = mirrorPort;

                Run68KCode(SysBase, &myFrame->mf_PPCArgs);

                struct MsgFrame* doneFrame = CreateMsgFrame(PowerPCBase);

                CopyMem((const APTR) &myFrame, (APTR)&doneFrame, sizeof(struct MsgFrame));

                doneFrame->mf_Identifier = ID_DONE;
                doneFrame->mf_Arg[0]     = myTask->tc_SigRecvd & andTemp;
                doneFrame->mf_Arg[1]     = myTask->tc_SigAlloc;
                doneFrame->mf_Arg[2]     = (ULONG)myTask;

                SendMsgFrame(PowerPCBase, doneFrame);
                FreeMsgFrame(PowerPCBase, myFrame);
            }
            else
            {
                FreeMsgFrame(PowerPCBase, myFrame);
                PrintError(SysBase, "68K mirror task received illegal command packet");
            }
            myFrame = (struct MsgFrame*)GetMsg(mirrorPort);
        }
        mySignal = Wait((ULONG)myTask->tc_SigAlloc & 0xfffff000);

        if (mySignal & (1 << (ULONG)mirrorPort->mp_SigBit))
        {
            myFrame = (struct MsgFrame*)GetMsg(mirrorPort);
        }
        else if (mySignal & andTemp)
        {
            struct MsgFrame* crossFrame = CreateMsgFrame(PowerPCBase);
            crossFrame->mf_Identifier   = ID_LLPP;
            crossFrame->mf_Arg[0]       = (mySignal & andTemp);
            crossFrame->mf_Arg[1]       = (ULONG)myTask;
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
			if (myTask->tc_Node.ln_Type == NT_REPLYMSG)
			{
				if(myTask->tc_Node.ln_Name)
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
						myTask->tc_SigAlloc = myFrame->mf_Arg[0];
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
						newFrame->mf_Identifier = ID_DNLL; //original code had more

                        SendMsgFrame(PowerPCBase, newFrame);
                        FreeMsgFrame(PowerPCBase, myFrame);

						break;
					}
					case ID_FREE:
					{
						void (*FreeMem_ptr)(__reg("d0") ULONG, __reg("a1") ULONG, __reg("a6") struct ExecBase*) =
							 (APTR)(myFrame->mf_PPCArgs.PP_Regs[0] + myFrame->mf_PPCArgs.PP_Regs[1]);
						(*FreeMem_ptr)(myFrame->mf_PPCArgs.PP_Regs[4], myFrame->mf_PPCArgs.PP_Regs[3], SysBase);
						FreeMsgFrame(PowerPCBase, myFrame);
						break;
					}
					case ID_DBGS:
					{
						mySPrintF68K((struct PPCBase*)PowerPCBase, "Process: %s Function: %s r4,r5,r6,r7 = "
								       "%08lx,%08lx,%08lx,%08lx\n\0", (APTR)&myFrame->mf_PPCArgs);
						FreeMsgFrame(PowerPCBase, myFrame);
						break;
					}
					case ID_DBGE:
					{
						mySPrintF68K((struct PPCBase*)PowerPCBase, "Process: %s Function: %s r3 = %08lx\n\0",
                                       (APTR)&myFrame->mf_PPCArgs);
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
							case ERR_ETIM:
							{
								PrintError(SysBase, "PPC timed out while waiting on 68K");
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
			break;
		}
		case DEVICE_MPC107:
		{
			break;
		}
		case DEVICE_MPC8343E:
		{
			if(readmemLong(PowerPCBase->pp_BridgeConfig, IMMR_OMISR) & IMMR_OMISR_OM0I)
			{
				flag = 1;
			}
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
			switch(myFrame->mf_Identifier)
			{
				case ID_T68K:
				case ID_END:
				{
					struct MsgPort* mirrorPort;
                    if ((mirrorPort = myFrame->mf_MirrorPort) && (myFrame->mf_Identifier != ID_END))
					{
						struct Task* sigTask = mirrorPort->mp_SigTask;
						sigTask->tc_SigAlloc = myFrame->mf_Arg[0];
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
					myFrame->mf_Identifier = ID_DONE;
					FreeMsgFrame(PowerPCBase, myFrame);
					break;
				}
				case ID_PUTW:
				{
                    *((ULONG*)(myFrame->mf_Arg[1])) = myFrame->mf_Arg[0];
					myFrame->mf_Identifier = ID_DONE;
					FreeMsgFrame(PowerPCBase, myFrame);
					break;
				}
				case ID_PUTH:
				{
                    *((USHORT*)(myFrame->mf_Arg[1])) = (USHORT)myFrame->mf_Arg[0];
					myFrame->mf_Identifier = ID_DONE;
					FreeMsgFrame(PowerPCBase, myFrame);
					break;
				}
				case ID_PUTB:
				{
                    *((UBYTE*)(myFrame->mf_Arg[1])) = (UBYTE)myFrame->mf_Arg[0];
					myFrame->mf_Identifier = ID_DONE;
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

        switch (PowerPCBase->pp_DeviceID)
	    {
		    case DEVICE_HARRIER:
		    {
			    break;
		    }
		    case DEVICE_MPC107:
		    {
			    break;
		    }
		    case DEVICE_MPC8343E:
		    {
				writememLong(PowerPCBase->pp_BridgeConfig, IMMR_OMISR, IMMR_OMISR_OM0I);
                break;
			}
		}
	}
	return flag;
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

	if (!(readmemLong(PowerPCBase->pp_BridgeConfig, IMMR_OMISR) & IMMR_OMISR_OM0I))
	{
		struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));
		if (myFIFO->kf_MIOPT != myFIFO->kf_MIOPH)
		{
            custom->intena = INTF_PORTS;
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

    ULONG msgFrame = NULL;

    Disable();

    switch (PowerPCBase->pp_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            break;
        }

        case DEVICE_MPC8343E:
        {
            struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));
            while (1)
            {                
                msgFrame = *((ULONG*)(myFIFO->kf_MIIFT));
                myFIFO->kf_MIIFT = (myFIFO->kf_MIIFT + 4) & 0xffff3fff;
                if (msgFrame != myFIFO->kf_CreatePrevious)
                {
                    myFIFO->kf_CreatePrevious = msgFrame;
                    break;
                }                
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

    Enable();

    if (msgFrame)
    {
        ULONG clearFrame = msgFrame;
        for (int i=0; i<48; i++)
        {
            *((ULONG*)(clearFrame)) = 0;
            clearFrame += 4;
        }
    }

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
            break;
        }

        case DEVICE_MPC8343E:
        {
            struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));

            *((ULONG*)(myFIFO->kf_MIIPH)) = (ULONG)msgFrame;
            myFIFO->kf_MIIPH = (myFIFO->kf_MIIPH + 4) & 0xffff3fff;
            writememLong(PowerPCBase->pp_BridgeConfig, IMMR_IMR0, (ULONG)msgFrame);

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
            break;
        }

        case DEVICE_MPC8343E:
        {
            struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));

            *((ULONG*)(myFIFO->kf_MIOFH)) = (ULONG)msgFrame;
            myFIFO->kf_MIOFH = (myFIFO->kf_MIOFH + 4) & 0xffff3fff;

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

    ULONG msgFrame = -1;

    Disable();

    switch (PowerPCBase->pp_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            break;
        }

        case DEVICE_MPC8343E:
        {
            struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(PowerPCBase->pp_PPCMemBase + FIFO_END));

            if (myFIFO->kf_MIOPT == myFIFO->kf_MIOPH)
            {
                break;
            }
            while (1)
            {
                msgFrame = *((ULONG*)(myFIFO->kf_MIOPT));
                myFIFO->kf_MIOPT = (myFIFO->kf_MIOPT + 4) & 0xffff3fff;
                if (msgFrame != myFIFO->kf_GetPrevious)
                {
                    myFIFO->kf_GetPrevious = msgFrame;
                    break;
                }
            }
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

    Enable();

    return (struct MsgFrame*)msgFrame;
}


//CreateProc, CreateNewProc, RunCommand, SystemTagList
