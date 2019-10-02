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

void InsertOnPri(struct PPCBase* PowerPCBase, struct List* list, struct TaskPPC* myTask)
{
    return;
}

APTR AllocVec68K(struct PPCBase* PowerPCBase, ULONG size)
{
    return NULL;
}

LONG FreeVec68K(struct PPCBase* PowerPCBase, APTR memBlock)
{
    return 0;
}

void FlushDCache(struct PPCBase* PowerPCBase)
{
    return;
}

LONG AtomicTest(struct PPCBase* PowerPCBase, APTR testLocation)
{
    return NULL;
}

void AtomicDone(APTR testLocation)
{
    return;
}

void DisablePPC(void)
{
    return;
}

void EnablePPC(void)
{
    return;
}

//unique for every bridge or use ifs?

ULONG CreateMsgFramePPC(struct PPCBase* PowerPCBase)
{
    return 0;
}

void SendMsgFramePPC(struct PPCBase* PowerPCBase, ULONG msgFrame)
{
    return;
}

void FreeMsgFramePPC(struct PPCBase* PowerPCBase, ULONG msgFrame)
{
    return;
}

#if 0
ULONG GetMsgFramePPC(struct PPCBase* PowerPCBase)
{
    return 0;
}
#endif

LONG StricmpPPC(STRPTR string1, STRPTR string2)
{
    return 0;
}

ULONG GetLen(STRPTR string)
{
    return 0;
}

STRPTR CopyStr(APTR source, APTR dest)
{
    return NULL;
}

void CauseDECInterrupt(struct PPCBase* PowerPCBase)
{
    return;
}

ULONG CheckExcSignal(struct PPCBase* PowerPCBase, struct TaskPPC* myTask, ULONG signal)
{
    return 0;
}

void EndTaskPPC(void)
{
    return;
}

APTR AllocatePPC(struct PPCBase* PowerPCBase, struct MemHeader* memHeader, ULONG byteSize)
{
    return NULL;
}

void DeallocatePPC(struct PPCBase* PowerPCBase, struct MemHeader* memHeader,
                   APTR memoryBlock, ULONG byteSize)
{
    return;
}

//BAT stuff not in yet.
