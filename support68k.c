// Copyright (c) 2015-2019 Dennis van der Boon
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
#include <powerpc/powerpc.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/doshunks.h>

#include "constants.h"
#include "Internals68k.h"
#include "libstructs.h"

extern APTR OldLoadSeg, OldNewLoadSeg, OldAllocMem, OldAddTask, OldRemTask;
extern struct PPCBase* myPPCBase;
APTR   RemSysTask;
UBYTE powerlib[] = "powerpc.library\0";
UBYTE ampname[]  = "AmigaAMP\0";

/********************************************************************************************
*
*	Task patch functions. To remove PPC tasks and free memory when the 68K part is ended
*
*********************************************************************************************/

void commonRemTask(__reg("a1") struct Task* myTask, __reg("a6") struct ExecBase* SysBase)
{
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

    if (!(testAttr) || (testAttr & MEMF_FAST))
    {
        testAttr = 0; //dummy
    }
    else 
    {
        if (!(testAttr & MEMF_PUBLIC) || (testAttr & MEMF_CHIP))
        {
            return ((*AllocMem_ptr)(byteSize, attributes, SysBase));
        }
    }

    return ((*AllocMem_ptr)(byteSize, attributes, SysBase));
}

/********************************************************************************************
*
*	Executable loading patches to load PPC (related) code to PPC memory
*
*********************************************************************************************/

LONG ReadFunc(__reg("d1") BPTR readhandle, __reg("a0") APTR buffer, __reg("d0") LONG length, __reg("a6") struct DosLibrary* DOSBase)
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

APTR AllocFunc(__reg("d0") ULONG size, __reg("d1") ULONG flags, __reg("a6") struct ExecBase* SysBase)
{
    APTR memBlock;

    if (!(myPPCBase->PPC_Flags & 0x4))
    {
        if ((flags == MEMF_PUBLIC | MEMF_FAST) || (flags == MEMF_PUBLIC | MEMF_CHIP))
        {
            SysBase->ThisTask->tc_Flags &= ~TF_PPC;
            memBlock = AllocMem(size, flags);
            SysBase->ThisTask->tc_Flags |= TF_PPC;
            return memBlock;
        }
    }
    SysBase->ThisTask->tc_Flags |= TF_PPC;
    flags = (flags & MEMF_CLEAR) | MEMF_PUBLIC | MEMF_PPC;
    memBlock = AllocMem(size, flags);
    return memBlock;
}

void FreeFunc(__reg("a1") APTR memory, __reg("d0") ULONG size, __reg("a6") struct ExecBase* SysBase)
{
    FreeMem(memory, size);

    return;
}


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
    if (myLock = Open(name, MODE_OLDFILE))
    {
        if (myFIB = (struct FileInfoBlock*)AllocDosObject(DOS_FIB, NULL))
        {
            if (ExamineFH(myLock, myFIB))
            {
                myProt = myFIB->fib_DiskKey;
                FreeDosObject(DOS_FIB, (APTR)myFIB);
                if (myProt & 0x30000 == 0x20000)
                {
                    SysBase->ThisTask->tc_Flags &= ~TF_PPC;
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
                        SysBase->ThisTask->tc_Flags &= ~TF_PPC;
                        SetProtection(name, myProt | 0x20000);
                        return -1;
                     }
                    else if (!(mySeglist))
                    {
                        return NULL;
                    }
                    else
                    {
                        if (myProt & 0x30000 == 0x10000)
                        {
                            return mySeglist;
                        }
                        BPTR testSeglist = mySeglist;
                        while (testSeglist)
                        {
                            UBYTE* mySegData = (UBYTE*)(*((ULONG*)((testSeglist << 2) + 4)));
                            LONG mySegSize  = (*((ULONG*)(testSeglist - 4)));
                            testSeglist = (*((ULONG*)(testSeglist)));
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
                        SysBase->ThisTask->tc_Flags &= ~TF_PPC;
                        SetProtection(name, myProt | 0x20000);
                    }
                }
            }
        }
    }
    return -1;
}

PATCH68K BPTR patchLoadSeg(__reg("d1") STRPTR name, __reg("a6") struct DosLibrary* DOSBase)
{
    BPTR (*LoadSeg_ptr)(__reg("d1") STRPTR, __reg("a6") struct DosLibrary*) = OldLoadSeg;
#if 0                                           //Patch disabled for now
    BPTR mySegList;

    mySegList = myLoader(name);

    if (mySegList == -1)
    {
        return ((*LoadSeg_ptr)(name, DOSBase));
    }
    return mySegList;
#else
    return ((*LoadSeg_ptr)(name, DOSBase));
#endif
}

PATCH68K BPTR patchNewLoadSeg(__reg("d1") STRPTR file, __reg("d2") struct TagItem* tags,
                     __reg("a6") struct DosLibrary* DOSBase)
{
    BPTR (*NewLoadSeg_ptr)(__reg("d1") STRPTR, __reg("d2") struct TagItem*, __reg("a6") struct DosLibrary*) = OldNewLoadSeg;
#if 0                                           //Patch disabled for now
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
#else
    return ((*NewLoadSeg_ptr)(file, tags, DOSBase));
#endif
}

/********************************************************************************************
*
*	Process that handles messages from the PPC.
*
*********************************************************************************************/


FUNC68K void MasterControl(void)
{
    struct ExecBase* SysBase = myPPCBase->PPC_SysLib;
    ULONG mySignal = 0;

    while (!(mySignal & SIGBREAKF_CTRL_F))
    {
        mySignal = Wait(SIGBREAKF_CTRL_F);
    }

    struct Task* myTask = SysBase->ThisTask;

    struct InternalConsts *myData = (struct InternalConsts*)myTask->tc_UserData;

    return;
}

/********************************************************************************************
*
*	Interrupt that gets called by the PPC card.
*
*********************************************************************************************/

FUNC68K ULONG sonInt(__reg("a1") APTR data, __reg("a5") APTR code)
{
    return 0;
}

/********************************************************************************************
*
*	VBLANK Interrupt that supports unreliable (for now) messaging from K1/M1
*
*********************************************************************************************/

FUNC68K ULONG zenInt(__reg("a1") APTR data, __reg("a5") APTR code)
{
    return 0;
}

//make the next ones each for every bridge type or use ifs?

/********************************************************************************************
*
*	Functions that creates a message for the PPC
*
*********************************************************************************************/

struct MsgFrame* CreateMsgFrame(struct PPCBase* PowerPCBase)
{
    struct PrivatePPCBase* myBase = (struct PrivatePPCBase*)PowerPCBase;
    struct ExecBase* SysBase = PowerPCBase->PPC_SysLib;

    ULONG msgFrame = NULL;

    Disable();

    switch (myBase->pp_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            break;
        }

        case DEVICE_MPC8343E:
        {
            struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(myBase->pp_PPCMemBase + FIFO_OFFSET));
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

void SendMsgFrame(struct PPCBase* PowerPCBase, struct MsgFrame* msgFrame)
{
    struct PrivatePPCBase* myBase = (struct PrivatePPCBase*)PowerPCBase;
    struct ExecBase* SysBase = PowerPCBase->PPC_SysLib;

    Disable();

    switch (myBase->pp_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            break;
        }

        case DEVICE_MPC8343E:
        {
            struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(myBase->pp_PPCMemBase + FIFO_OFFSET));

            *((ULONG*)(myFIFO->kf_MIIPH)) = (ULONG)msgFrame;
            myFIFO->kf_MIIPH = (myFIFO->kf_MIIPH + 4) & 0xffff3fff;
            writememLong(myBase->pp_BridgeConfig, IMMR_IMR0, (ULONG)msgFrame);
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

void FreeMsgFrame(struct PPCBase* PowerPCBase, struct MsgFrame*  msgFrame)
{
    struct PrivatePPCBase* myBase = (struct PrivatePPCBase*)PowerPCBase;
    struct ExecBase* SysBase = PowerPCBase->PPC_SysLib;

    Disable();

    msgFrame->mf_Identifier = ID_FREE;

    switch (myBase->pp_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            break;
        }

        case DEVICE_MPC8343E:
        {
            struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(myBase->pp_PPCMemBase + FIFO_OFFSET));

            *((ULONG*)(myFIFO->kf_MIOFH)) = (ULONG)msgFrame;
            myFIFO->kf_MIOFH = (myFIFO->kf_MIOFH + 4) & 0xffff3fff;
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

struct MsgFrame* GetMsgFrame(struct PPCBase* PowerPCBase)
{
    struct PrivatePPCBase* myBase = (struct PrivatePPCBase*)PowerPCBase;
    struct ExecBase* SysBase = PowerPCBase->PPC_SysLib;

    ULONG msgFrame = -1;

    Disable();

    switch (myBase->pp_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            break;
        }

        case DEVICE_MPC8343E:
        {
            struct killFIFO* myFIFO = (struct killFIFO*)((ULONG)(myBase->pp_PPCMemBase + FIFO_OFFSET));

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
