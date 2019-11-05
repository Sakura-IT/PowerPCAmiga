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

void illegal(void) = "\tillegal\n";           //debug function

static const APTR LibVectors[TOTAL_FUNCS+1];
static const APTR WarpVectors[5];

#define _LVOSetHardware         -540
#define _LVOCreateTaskPPC       -336

#define LIBFUNC68K __entry
#define FUNC68K __entry

BPTR   myExpunge(__reg("a6") struct PPCBase* PowerPCBase);

void   writememLong(ULONG Base, ULONG offset, ULONG value);
ULONG  readmemLong (ULONG Base, ULONG offset);

void   getENVs    (struct InternalConsts* myConsts);
void   PrintError (struct InternalConsts* myConsts, UBYTE* errortext);
void   PrintCrtErr(struct InternalConsts* myConsts, UBYTE* crterrtext);
void   MasterControl(void);

struct MsgFrame*  CreateMsgFrame(struct PPCBase* PowerPCBase);
void              SendMsgFrame  (struct PPCBase* PowerPCBase, struct MsgFrame* msgFrame);
void              FreeMsgFrame  (struct PPCBase* PowerPCBase, struct MsgFrame* msgFrame);
struct MsgFrame*  GetMsgFrame   (struct PPCBase* PowerPCBase);

struct InitData *SetupKiller (struct InternalConsts* myConsts, ULONG devfuncnum, struct PciDevice* ppcdevice, ULONG initPointer);
struct InitData *SetupHarrier(struct InternalConsts* myConsts, ULONG devfuncnum, struct PciDevice* ppcdevice, ULONG initPointer);
struct InitData *SetupMPC107 (struct InternalConsts* myConsts, ULONG devfuncnum, struct PciDevice* ppcdevice, ULONG initPointer);

struct Library* warpOpen         (__reg("a6") struct Library* WarpBase);
BPTR            warpClose        (__reg("a6") struct Library* WarpBase);

struct Library* myOpen         (__reg("a6") struct PPCBase* PowerPCBase);
BPTR            myClose        (__reg("a6") struct PPCBase* PowerPCBase);
BPTR            myExpunge      (__reg("a6") struct PPCBase* PowerPCBase);
ULONG           myReserved     (void);
LONG            myRunPPC       (__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") struct PPCArgs* PPStruct);
LONG            myWaitForPPC   (__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") struct PPCArgs* PPStruct);
ULONG           myGetCPU       (__reg("a6") struct PPCBase* PowerPCBase);
ULONG           myGetPPCState  (__reg("a6") struct PPCBase* PowerPCBase);
struct TaskPPC* myCreatePPCTask(__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") struct TagItem* TagItems);

APTR            myAllocVec32 (__reg("a6") struct PPCBase* PowerPCBase, __reg("d0") ULONG memsize, __reg("d1") ULONG attributes);
void            myFreeVec32  (__reg("a6") struct PPCBase* PowerPCBase, __reg("a1") APTR memblock);
struct Message* myAllocXMsg  (__reg("a6") struct PPCBase* PowerPCBase,
                              __reg("d0") ULONG bodysize, __reg("a0") struct MsgPort* replyport);
void            myFreeXMsg   (__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") struct Message* myXMsg);
void            mySetCache68K(__reg("a6") struct PPCBase* PowerPCBase, __reg("d0") ULONG cacheflags,
                              __reg("a0") APTR start, __reg("d1") ULONG length);
void         myPowerDebugMode(__reg("a6") struct PPCBase* PowerPCBase, __reg("d0") ULONG debuglevel);
void         PutChProc       (__reg("a6") struct ExecBase* SysBase, __reg("d0") UBYTE mychar,
                              __reg("a3") APTR PutChData);
void         mySPrintF68K    (__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") STRPTR Formatstring,
                              __reg("a1") APTR values);
void         myPutXMsg       (__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") struct MsgPort* MsgPortPPC,
                              __reg("a1") struct Message* message);
void      myCausePPCInterrupt(__reg("a6") struct PPCBase* PowerPCBase);

