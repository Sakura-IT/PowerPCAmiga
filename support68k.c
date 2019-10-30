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

ULONG CreateMsgFrame(void)
{
    return 0;
}

void SendMsgFrame(ULONG msgFrame)
{
    return;
}

void FreeMsgFrame(ULONG msgFrame)
{
    return;
}

ULONG GetMsgFrame(void)
{
    return 0;
}


//CreateProc, CreateNewProc, RunCommand, SystemTagList
