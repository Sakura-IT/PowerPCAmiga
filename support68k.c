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
#include <powerpc/powerpc.h>

#include <proto/exec.h>

#include "constants.h"
#include "Internals68k.h"
#include "libstructs.h"

FUNC68K void patchRemTask(__reg("a1") struct Task* myTask, __reg("a6") struct ExecBase* SysBase)
{
    return;
}

FUNC68K APTR patchAddTask(__reg("a1") struct Task* myTask, __reg("a2") APTR initialPC,
                  __reg("a3") APTR finalPC, __reg("a6") struct ExecBase* SysBase)
{
    return NULL;
}

FUNC68K APTR patchAllocMem(__reg("d0") ULONG byteSize, __reg("d1") ULONG attributes,
                   __reg("a6") struct ExecBase* SysBase)
{
    return NULL;
}

FUNC68K BPTR patchLoadSeg(__reg("d1") STRPTR name, __reg("a6") struct DosLibrary* DOSBase)
{
    return NULL;
}

FUNC68K BPTR patchNewLoadSeg(__reg("d1") STRPTR file, __reg("d2") struct TagItem* tags,
                     __reg("a6") struct DosLibrary* DOSBase)
{
    return NULL;
}


FUNC68K void MasterControl(void)
{
    struct ExecBase* SysBase = *((struct ExecBase **)4UL);
    ULONG mySignal = 0;

    while (!(mySignal & SIGBREAKF_CTRL_F))
    {
        mySignal = Wait(SIGBREAKF_CTRL_F);
    }

    struct Task* myTask = SysBase->ThisTask;

    struct InternalConsts *myData = (struct InternalConsts*)myTask->tc_UserData;

    return;
}

FUNC68K ULONG sonInt(__reg("a1") APTR data, __reg("a5") APTR code)
{
    return 0;
}

FUNC68K ULONG zenInt(__reg("a1") APTR data, __reg("a5") APTR code)
{
    return 0;
}

//make the next ones each for every bridge type or use ifs?

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

    return (struct MsgFrame*)msgFrame;
}

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

void FreeMsgFrame(struct PPCBase* PowerPCBase, struct MsgFrame*  msgFrame)
{
    struct PrivatePPCBase* myBase = (struct PrivatePPCBase*)PowerPCBase;
    struct ExecBase* SysBase = PowerPCBase->PPC_SysLib;

    Disable();

    msgFrame->mf_Identifier = 0x46524545;        //FREE

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
