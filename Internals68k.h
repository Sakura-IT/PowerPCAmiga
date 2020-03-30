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

void illegal(void) = "\tillegal\n";           //debug function

static const APTR LibVectors[TOTAL_FUNCS+1];
static const APTR WarpVectors[5];

#define _LVOSetHardware         -540
#define _LVOCreateTaskPPC       -336
#define _LVOLoadSeg             -150
#define _LVONewLoadSeg          -768
#define _LVOAllocMem            -198
#define _LVOAddTask             -282
#define _LVORemTask             -288

#define LIBFUNC68K __entry
#define FUNC68K __entry
#define PATCH68K __saveds

PATCH68K BPTR   patchLoadSeg   (__reg("d1") STRPTR name, __reg("a6") struct DosLibrary* DOSBase);
PATCH68K BPTR   patchNewLoadSeg(__reg("d1") STRPTR file, __reg("d2") struct TagItem* tags,
                       __reg("a6") struct DosLibrary* DOSBase);
PATCH68K APTR   patchAllocMem  (__reg("d0") ULONG byteSize, __reg("d1") ULONG attributes,
                       __reg("a6") struct ExecBase* SysBase);
PATCH68K APTR   patchAddTask   (__reg("a1") struct Task* myTask, __reg("a2") APTR initialPC,
                       __reg("a3") APTR finalPC, __reg("a6") struct ExecBase* SysBase);
PATCH68K void   patchRemTask   (__reg("a1") struct Task* myTask, __reg("a6") struct ExecBase* SysBase);

struct PPCBase* LibInit(__reg("d0") struct PPCBase *ppcbase,
              __reg("a0") BPTR seglist, __reg("a6") struct ExecBase* __sys);

BPTR   myExpunge(__reg("a6") struct PPCBase* PowerPCBase);

void   writememLong(ULONG Base, ULONG offset, ULONG value);
ULONG  readmemLong (ULONG Base, ULONG offset);

void   getENVs      (struct InternalConsts* myConsts);
void   PrintCrtErr  (struct InternalConsts* myConsts, UBYTE* crterrtext);
void   PrintError   (struct ExecBase* SysBase, UBYTE* errortext);
void   MasterControl(void);
void   Run68KCode   (__reg("a6") struct ExecBase* SysBase, __reg("a0") struct PPCArgs* PPStruct);
void   commonRemTask(__reg("a1") struct Task* myTask, __reg("d0") ULONG flag, __reg("a6") struct ExecBase* SysBase);


ULONG  GortInt      (__reg("a1") APTR data, __reg("a5") APTR code);

static inline struct MsgFrame*  CreateMsgFrame(struct PrivatePPCBase* PowerPCBase);
void              SendMsgFrame  (struct PrivatePPCBase* PowerPCBase, struct MsgFrame* msgFrame);
void              FreeMsgFrame  (struct PrivatePPCBase* PowerPCBase, struct MsgFrame* msgFrame);
struct MsgFrame*  GetMsgFrame   (struct PrivatePPCBase* PowerPCBase);

struct InitData *SetupKiller (struct InternalConsts* myConsts, ULONG devfuncnum, struct PciDevice* ppcdevice, ULONG initPointer);
struct InitData *SetupHarrier(struct InternalConsts* myConsts, ULONG devfuncnum, struct PciDevice* ppcdevice, ULONG initPointer);
struct InitData *SetupMPC107 (struct InternalConsts* myConsts, ULONG devfuncnum, struct PciDevice* ppcdevice, ULONG initPointer);

struct Library* warpOpen         (__reg("a6") struct Library* WarpBase);
BPTR            warpClose        (__reg("a6") struct Library* WarpBase);

struct Library* myOpen         (__reg("a6") struct PPCBase* PowerPCBase);
BPTR            myClose        (__reg("a6") struct PPCBase* PowerPCBase);
BPTR            myExpunge      (__reg("a6") struct PPCBase* PowerPCBase);
ULONG           myReserved     (void);
LONG            myRunPPC       (__reg("a6") struct PrivatePPCBase* PowerPCBase, __reg("a0") struct PPCArgs* PPStruct);
LONG            myWaitForPPC   (__reg("a6") struct PrivatePPCBase* PowerPCBase, __reg("a0") struct PPCArgs* PPStruct);
ULONG           myGetCPU       (__reg("a6") struct PrivatePPCBase* PowerPCBase);
ULONG           myGetPPCState  (__reg("a6") struct PrivatePPCBase* PowerPCBase);
struct TaskPPC* myCreatePPCTask(__reg("a6") struct PrivatePPCBase* PowerPCBase, __reg("a0") struct TagItem* TagItems);

APTR            myAllocVec32 (__reg("a6") struct PPCBase* PowerPCBase, __reg("d0") ULONG memsize, __reg("d1") ULONG attributes);
void            myFreeVec32  (__reg("a6") struct PPCBase* PowerPCBase, __reg("a1") APTR memblock);
struct Message* myAllocXMsg  (__reg("a6") struct PPCBase* PowerPCBase,
                              __reg("d0") ULONG bodysize, __reg("a0") struct MsgPort* replyport);
void            myFreeXMsg   (__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") struct Message* myXMsg);
void            mySetCache68K(__reg("a6") struct PPCBase* PowerPCBase, __reg("d0") ULONG cacheflags,
                              __reg("a0") APTR start, __reg("d1") ULONG length);
void         myPowerDebugMode(__reg("a6") struct PrivatePPCBase* PowerPCBase, __reg("d0") ULONG debuglevel);
void         PutChProc       (__reg("a6") struct ExecBase* SysBase, __reg("d0") UBYTE mychar,
                              __reg("a3") APTR PutChData);
void         mySPrintF68K    (__reg("a6") struct PPCBase* PowerPCBase, __reg("a0") STRPTR Formatstring,
                              __reg("a1") APTR values);
void         myPutXMsg       (__reg("a6") struct PrivatePPCBase* PowerPCBase, __reg("a0") struct MsgPort* MsgPortPPC,
                              __reg("a1") struct Message* message);
void      myCausePPCInterrupt(__reg("a6") struct PrivatePPCBase* PowerPCBase);

ULONG ZenInt(__reg("a1") APTR data, __reg("a5") APTR code);
ULONG GortInt(__reg("a1") APTR data, __reg("a5") APTR code);

VOID  MyCopy(__reg("a0") APTR source, __reg("a1") APTR destination, __reg("d0") ULONG size);
VOID  GetClock(__reg("a0") ULONG memory);
