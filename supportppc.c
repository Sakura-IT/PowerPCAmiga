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
#include <powerpc/tasksPPC.h>
#include <powerpc/powerpc.h>
#include "libstructs.h"
#include "Internalsppc.h"

PPCFUNCTION VOID InsertOnPri(struct PrivatePPCBase* PowerPCBase, struct List* list, struct TaskPPC* myTask)
{
    return;
}

PPCFUNCTION APTR AllocVec68K(struct PrivatePPCBase* PowerPCBase, ULONG size, ULONG flags)
{
    return NULL;
}

PPCFUNCTION LONG FreeVec68K(struct PrivatePPCBase* PowerPCBase, APTR memBlock)
{
    return 0;
}

PPCFUNCTION VOID FlushDCache(struct PrivatePPCBase* PowerPCBase)
{
    return;
}

PPCFUNCTION LONG AtomicTest(struct PrivatePPCBase* PowerPCBase, APTR testLocation)
{
    return NULL;
}

PPCFUNCTION VOID AtomicDone(APTR testLocation)
{
    return;
}

PPCFUNCTION VOID DisablePPC(VOID)
{
    return;
}

PPCFUNCTION VOID EnablePPC(VOID)
{
    return;
}

//unique for every bridge or use ifs?

PPCFUNCTION ULONG CreateMsgFramePPC(struct PrivatePPCBase* PowerPCBase)
{
    return 0;
}

PPCFUNCTION VOID SendMsgFramePPC(struct PrivatePPCBase* PowerPCBase, ULONG msgFrame)
{
    return;
}

PPCFUNCTION VOID FreeMsgFramePPC(struct PrivatePPCBase* PowerPCBase, ULONG msgFrame)
{
    return;
}

#if 0
ULONG GetMsgFramePPC(struct PrivatePPCBase* PowerPCBase)
{
    return 0;
}
#endif

PPCFUNCTION LONG StricmpPPC(STRPTR string1, STRPTR string2)
{
    return 0;
}

PPCFUNCTION ULONG GetLen(STRPTR string)
{
    return 0;
}

PPCFUNCTION STRPTR CopyStr(APTR source, APTR dest)
{
    return NULL;
}

PPCFUNCTION VOID CauseDECInterrupt(struct PrivatePPCBase* PowerPCBase)
{
    return;
}

PPCFUNCTION ULONG CheckExcSignal(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* myTask, ULONG signal)
{
    return 0;
}

PPCFUNCTION VOID EndTaskPPC(VOID)
{
    return;
}

PPCFUNCTION APTR AllocatePPC(struct PrivatePPCBase* PowerPCBase, struct MemHeader* memHeader, ULONG byteSize)
{
    return NULL;
}

PPCFUNCTION VOID DeallocatePPC(struct PrivatePPCBase* PowerPCBase, struct MemHeader* memHeader,
                   APTR memoryBlock, ULONG byteSize)
{
    return;
}

PPCFUNCTION VOID printDebugEntry(struct PrivatePPCBase* PowerPCBase, ULONG function, ULONG r4, ULONG r5,
                   ULONG r6, ULONG r7)
{
    return;
}

PPCFUNCTION VOID printDebugExit(struct PrivatePPCBase* PowerPCBase, ULONG function, ULONG result)
{
    return;
}

//BAT stuff not in yet.
