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

#define PPCFUNCTION __section ("functions","acrx") __entry

PPCFUNCTION void InsertOnPri(struct PPCBase* PowerPCBase, struct List* list, struct TaskPPC* myTask)
{
    return;
}

PPCFUNCTION APTR AllocVec68K(struct PPCBase* PowerPCBase, ULONG size)
{
    return NULL;
}

PPCFUNCTION LONG FreeVec68K(struct PPCBase* PowerPCBase, APTR memBlock)
{
    return 0;
}

PPCFUNCTION void FlushDCache(struct PPCBase* PowerPCBase)
{
    return;
}

PPCFUNCTION LONG AtomicTest(struct PPCBase* PowerPCBase, APTR testLocation)
{
    return NULL;
}

PPCFUNCTION void AtomicDone(APTR testLocation)
{
    return;
}

PPCFUNCTION void DisablePPC(void)
{
    return;
}

PPCFUNCTION void EnablePPC(void)
{
    return;
}

//unique for every bridge or use ifs?

PPCFUNCTION ULONG CreateMsgFramePPC(struct PPCBase* PowerPCBase)
{
    return 0;
}

PPCFUNCTION void SendMsgFramePPC(struct PPCBase* PowerPCBase, ULONG msgFrame)
{
    return;
}

PPCFUNCTION void FreeMsgFramePPC(struct PPCBase* PowerPCBase, ULONG msgFrame)
{
    return;
}

#if 0
ULONG GetMsgFramePPC(struct PPCBase* PowerPCBase)
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

PPCFUNCTION void CauseDECInterrupt(struct PPCBase* PowerPCBase)
{
    return;
}

PPCFUNCTION ULONG CheckExcSignal(struct PPCBase* PowerPCBase, struct TaskPPC* myTask, ULONG signal)
{
    return 0;
}

PPCFUNCTION void EndTaskPPC(void)
{
    return;
}

PPCFUNCTION APTR AllocatePPC(struct PPCBase* PowerPCBase, struct MemHeader* memHeader, ULONG byteSize)
{
    return NULL;
}

PPCFUNCTION void DeallocatePPC(struct PPCBase* PowerPCBase, struct MemHeader* memHeader,
                   APTR memoryBlock, ULONG byteSize)
{
    return;
}

//BAT stuff not in yet.
