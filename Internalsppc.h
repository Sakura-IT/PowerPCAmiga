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


#include <exec/types.h>

void illegal(void) = "\t.long\t0\n";            //debug function

#define PPCFUNCTION __section ("functions","acrx") __entry
#define PPCKERNEL __section ("kernel","acrx") __entry

#define _LVOAllocVec32      -54
#define _LVOFreeVec32       -60
#define _LVOSprintF68K      -66
#define _LVOAllocVec        -684
#define _LVOAllocMem        -198
#define _LVOStricmp         -162


struct MsgFrame* __CreateMsgFramePPC(void *)="\tlwz\tr0,-868(r3)\n\tmtlr\tr0\n\tblrl";
#define libCreateMsgFramePPC() __CreateMsgFramePPC(PowerPCBase)
struct MsgFrame* __GetMsgFramePPC(void *)="\tlwz\tr0,-874(r3)\n\tmtlr\tr0\n\tblrl";
#define libGetMsgFramePPC() __GetMsgFramePPC(PowerPCBase)
VOID __SendMsgFramePPC(void *, struct MsgFrame* msgframe)="\tlwz\tr0,-880(r3)\n\tmtlr\tr0\n\tblrl";
#define libSendMsgFramePPC(msgframe) __SendMsgFramePPC(PowerPCBase, (msgframe))
VOID __FreeMsgFramePPC(void *, struct MsgFrame* msgframe)="\tlwz\tr0,-886(r3)\n\tmtlr\tr0\n\tblrl";
#define libFreeMsgFramePPC(msgframe) __FreeMsgFramePPC(PowerPCBase, (msgframe))

#define _LVOSystemStart     -892
#define _LVOStartTask       -898

void        Reset  (void);
ULONG   GetExcTable(void);
ULONG   GetVecEntry(void);

ULONG getLeadZ(ULONG value)               = "\tcntlzw\tr3,r3\n";
ULONG getPVR(void)                        = "\tmfpvr\tr3\n";
ULONG getSDR1(void)                       = "\tmfsdr1\tr3\n";
void  setSDR1(ULONG value)                = "\tmtsdr1\tr3\n";
void  setIABR(ULONG value)                = "\tmtspr\t1010,r3\n";
void  setDABR(ULONG value)                = "\tmtspr\t1013,r3\n";
void  setSRR0(ULONG value)                = "\tmtsrr0\tr3\n";
void  setSRR1(ULONG value)                = "\tmtsrr1\tr3\n";
void  setSRIn(ULONG keyVal, ULONG segVal) = "\tmtsrin\tr3,r4\n";
ULONG getSRIn(ULONG address)              = "\tmfsrin\tr3,r3\n";
void  setMSR(ULONG value)                 = "\tsync\n\tmtmsr\tr3\n\tisync\n\tsync\n";
ULONG getMSR(void)                        = "\tmfmsr\tr3\n";
ULONG getHID0(void)                       = "\tmfspr\tr3,1008\n";
ULONG getHID1(void)                       = "\tmfspr\tr3,1009\n";
void  setHID0(ULONG)                      = "\tsync\n\tmtspr\t1008,r3\n\tsync\n\tisync\n";
void  setDEC(LONG value)                  = "\tmtdec\tr3\n";
ULONG getDEC(void)                        = "\tmfdec\tr3\n";
ULONG getTBU(void)                        = "\tmftbu\tr3\n";
ULONG getTBL(void)                        = "\tmftbl\tr3\n";
void  tlbSync(void)                       = "\ttlbsync\n";
void  tlbIe(ULONG value)                  = "\ttlbie\tr3\n";
void  storeR0(ULONG value)                = "\tmr\tr0,r3\n";
ULONG getR0(void)                         = "\tmr\tr3,r0\n";
ULONG getR2(void)                         = "\tmr\tr3,r2\n";
void  storeR2(ULONG value)                = "\tmr\tr2,r3\n";
void  HaltTask(void)                      = "\t.long\t0\n";
void  dFlush(ULONG address)               = "\tdcbf\tr0,r3\n";
void  iInval(ULONG address)               = "\ticbi\tr0,r3\n";
void  dInval(ULONG address)               = "\tdcbi\tr0,r3\n";
ULONG getL2State(void)                    = "\tmfl2cr\tr3\n";
void  setL2State(ULONG value)             = "\tmtl2cr\tr3\n\tsync\n";
void  sync(void)                          = "\tsync\n";
void  isync(void)                         = "\tisync\n";
void  loadWord(ULONG)                     = "\tlwz\tr3,0(r3)\n"; //to bypass optimizations
ULONG getCPUState(ULONG res, ULONG key)   = "\tmfpvr\tr4\n";
ULONG roll(ULONG value, ULONG shift)      = "\trlwinm\tr3,r3,r4,0,31\n";

ULONG loadPCI(ULONG base, ULONG offset)                    = "\tlwbrx\tr3,r3,r4\n";
void  storePCI(ULONG base, ULONG offset, LONG value)       = "\tstwbrx\tr5,r3,r4\n";

void setBAT0(ULONG ibatl, ULONG ibatu, ULONG dbatl, ULONG dbatu)
        ="\tmtibatl\t0,r3\n\tmtibatu\t0,r4\n\tmtdbatl\t0,r5\n\tmtdbatu\t0,r6\n";
void setBAT1(ULONG ibatl, ULONG ibatu, ULONG dbatl, ULONG dbatu)
        ="\tmtibatl\t1,r3\n\tmtibatu\t1,r4\n\tmtdbatl\t1,r5\n\tmtdbatu\t1,r6\n";
void setBAT2(ULONG ibatl, ULONG ibatu, ULONG dbatl, ULONG dbatu)
        ="\tmtibatl\t2,r3\n\tmtibatu\t2,r4\n\tmtdbatl\t2,r5\n\tmtdbatu\t2,r6\n";
void setBAT3(ULONG ibatl, ULONG ibatu, ULONG dbatl, ULONG dbatu)
        ="\tmtibatl\t3,r3\n\tmtibatu\t3,r4\n\tmtdbatl\t3,r5\n\tmtdbatu\t3,r6\n";

void mSync(void)                          = "\tsync\n\tisync\n";

void mulInt64(struct UInt64* myint64, ULONG value1, ULONG value2)
        ="\tmullw\tr6,r4,r5\n\
          \tstw\tr6,4(r3)\n\
          \tmulhw\tr6,r4,r5\n\
          \tstw\tr6,0(r3)\n";

void mvfrBAT0(struct BATArray*)
        ="\tmfibatu\tr4,0\n\
          \tstw\tr4,0(r3)\n\
          \tmfibatl\tr4,0\n\
          \tstw\tr4,4(r3)\n\
          \tmfdbatu\tr4,0\n\
          \tstw\tr4,8(r3)\n\
          \tmfdbatl\tr4,0\n\
          \tstw\tr4,12(r3)\n";

void mvfrBAT0(struct BATArray*)
        ="\tmfibatu\tr4,0\n\
          \tstw\tr4,0(r3)\n\
          \tmfibatl\tr4,0\n\
          \tstw\tr4,4(r3)\n\
          \tmfdbatu\tr4,0\n\
          \tstw\tr4,8(r3)\n\
          \tmfdbatl\tr4,0\n\
          \tstw\tr4,12(r3)\n";

void mvfrBAT1(struct BATArray*)
        ="\tmfibatu\tr4,1\n\
          \tstw\tr4,0(r3)\n\
          \tmfibatl\tr4,1\n\
          \tstw\tr4,4(r3)\n\
          \tmfdbatu\tr4,r1\n\
          \tstw\tr4,8(r3)\n\
          \tmfdbatl\tr4,1\n\
          \tstw\tr4,12(r3)\n";

void mvfrBAT2(struct BATArray*)
        ="\tmfibatu\tr4,2\n\
          \tstw\tr4,0(r3)\n\
          \tmfibatl\tr4,2\n\
          \tstw\tr4,4(r3)\n\
          \tmfdbatu\tr4,2\n\
          \tstw\tr4,8(r3)\n\
          \tmfdbatl\tr4,2\n\
          \tstw\tr4,12(r3)\n";

void mvfrBAT3(struct BATArray*)
        ="\tmfibatu\tr4,3\n\
          \tstw\tr4,0(r3)\n\
          \tmfibatl\tr4,3\n\
          \tstw\tr4,4(r3)\n\
          \tmfdbatu\tr4,3\n\
          \tstw\tr4,8(r3)\n\
          \tmfdbatl\tr4,3\n\
          \tstw\tr4,12(r3)\n";

void mvtoBAT0(struct BATArray*)
        ="\tlwz\tr4,0(r3)\n\
          \tmtibatu\t0,r4\n\
          \tlwz\tr4,4(r3)\n\
          \tmtibatu\t0,r4\n\
          \tlwz\tr4,8(r3)\n\
          \tmtibatu\t0,r4\n\
          \tlwz\tr4,12(r3)\n\
          \tmtibatu\t0,r4\n";

void mvtoBAT1(struct BATArray*)
        ="\tlwz\tr4,0(r3)\n\
          \tmtibatu\t1,r4\n\
          \tlwz\tr4,4(r3)\n\
          \tmtibatu\t1,r4\n\
          \tlwz\tr4,8(r3)\n\
          \tmtibatu\t1,r4\n\
          \tlwz\tr4,12(r3)\n\
          \tmtibatu\t1,r4\n";

void mvtoBAT2(struct BATArray*)
        ="\tlwz\tr4,0(r3)\n\
          \tmtibatu\t2,r4\n\
          \tlwz\tr4,4(r3)\n\
          \tmtibatu\t2,r4\n\
          \tlwz\tr4,8(r3)\n\
          \tmtibatu\t2,r4\n\
          \tlwz\tr4,12(r3)\n\
          \tmtibatu\t2,r4\n";

void mvtoBAT3(struct BATArray*)
        ="\tlwz\tr4,0(r3)\n\
          \tmtibatu\t3,r4\n\
          \tlwz\tr4,4(r3)\n\
          \tmtibatu\t3,r4\n\
          \tlwz\tr4,8(r3)\n\
          \tmtibatu\t3,r4\n\
          \tlwz\tr4,12(r3)\n\
          \tmtibatu\t3,r4\n";

LONG myRun68K(struct PrivatePPCBase* PowerPCBase, struct PPCArgs* PPStruct);
LONG myWaitFor68K(struct PrivatePPCBase* PowerPCBase, struct PPCArgs* PPStruct);
VOID mySPrintF(struct PrivatePPCBase* PowerPCBase, STRPTR Formatstring, APTR Values);
APTR myAllocVecPPC(struct PrivatePPCBase* PowerPCBase, ULONG size, ULONG flags, ULONG align);
LONG myFreeVecPPC(struct PrivatePPCBase* PowerPCBase, APTR memblock);
struct TaskPPC* myCreateTaskPPC(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist);
VOID myDeleteTaskPPC(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* PPCtask);
struct TaskPPC* myFindTaskPPC(struct PrivatePPCBase* PowerPCBase, STRPTR name);
LONG myInitSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
VOID myFreeSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
LONG myAddSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
VOID myRemSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
VOID myObtainSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
LONG myAttemptSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
VOID myReleaseSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
struct SignalSemaphorePPC* myFindSemaphorePPC(struct PrivatePPCBase* PowerPCBase, STRPTR name);
VOID myInsertPPC(struct PrivatePPCBase* PowerPCBase, struct List* list, struct Node* node, struct Node* pred);
VOID myAddHeadPPC(struct PrivatePPCBase* PowerPCBase, struct List* list, struct Node* node);
VOID myAddTailPPC(struct PrivatePPCBase* PowerPCBase, struct List* list, struct Node* node);
VOID myRemovePPC(struct PrivatePPCBase* PowerPCBase, struct Node* node);
struct Node* myRemHeadPPC(struct PrivatePPCBase* PowerPCBase, struct List* list);
struct Node* myRemTailPPC(struct PrivatePPCBase* PowerPCBase, struct List* list);
VOID myEnqueuePPC(struct PrivatePPCBase* PowerPCBase, struct List* list, struct Node* node);
struct Node* myFindNamePPC(struct PrivatePPCBase* PowerPCBase, struct List* list, STRPTR name);
struct TagItem* myFindTagItemPPC(struct PrivatePPCBase* PowerPCBase, ULONG value, struct TagItem* taglist);
ULONG myGetTagDataPPC(struct PrivatePPCBase* PowerPCBase, ULONG tagvalue, ULONG tagdefault, struct TagItem* taglist);
struct TagItem* myNextTagItemPPC(struct PrivatePPCBase* PowerPCBase, struct TagItem** tagitem);
LONG myAllocSignalPPC(struct PrivatePPCBase* PowerPCBase, LONG signum);
VOID myFreeSignalPPC(struct PrivatePPCBase* PowerPCBase, LONG signum);
ULONG mySetSignalPPC(struct PrivatePPCBase* PowerPCBase, ULONG signals, ULONG mask);
VOID mySignalPPC(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* task, ULONG signals);
ULONG myWaitPPC(struct PrivatePPCBase* PowerPCBase, ULONG signals);
LONG mySetTaskPriPPC(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* task, LONG pri);
VOID mySignal68K(struct PrivatePPCBase* PowerPCBase, struct Task* task, ULONG signals);
VOID mySetCache(struct PrivatePPCBase* PowerPCBase, ULONG flags, APTR start, ULONG length);
APTR mySetExcHandler(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist);
VOID myRemExcHandler(struct PrivatePPCBase* PowerPCBase, APTR xlock);
ULONG mySuper(struct PrivatePPCBase* PowerPCBase);
VOID myUser(struct PrivatePPCBase* PowerPCBase, ULONG key);
ULONG mySetHardware(struct PrivatePPCBase* PowerPCBase, ULONG flags, APTR param);
VOID myModifyFPExc(struct PrivatePPCBase* PowerPCBase, ULONG fpflags);
ULONG myWaitTime(struct PrivatePPCBase* PowerPCBase, ULONG signals, ULONG time);
struct TaskPtr* myLockTaskList(struct PrivatePPCBase* PowerPCBase);
VOID myUnLockTaskList(struct PrivatePPCBase* PowerPCBase);
VOID mySetExcMMU(struct PrivatePPCBase* PowerPCBase);
VOID myClearExcMMU(struct PrivatePPCBase* PowerPCBase);
VOID myChangeMMU(struct PrivatePPCBase* PowerPCBase, ULONG mode);
VOID myGetInfo(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist);
struct MsgPortPPC* myCreateMsgPortPPC(struct PrivatePPCBase* PowerPCBase);
VOID myDeleteMsgPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port);
VOID myAddPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port);
VOID myRemPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port);
struct MsgPortPPC* myFindPortPPC(struct PrivatePPCBase* PowerPCBase, STRPTR port);
struct Message* myWaitPortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port);
VOID myPutMsgPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port, struct Message* message);
struct Message* myGetMsgPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port);
VOID myReplyMsgPPC(struct PrivatePPCBase* PowerPCBase, struct Message* message);
VOID myFreeAllMem(struct PrivatePPCBase* PowerPCBase);
VOID myCopyMemPPC(struct PrivatePPCBase* PowerPCBase, APTR source, APTR dest, ULONG size);
VOID CopyMemQuickPPC(struct PrivatePPCBase* PowerPCBase, APTR source, APTR dest, ULONG size);
struct Message* myAllocXMsgPPC(struct PrivatePPCBase* PowerPCBase, ULONG length, struct MsgPortPPC* port);
VOID myFreeXMsgPPC(struct PrivatePPCBase* PowerPCBase, struct Message* message);
VOID myPutXMsgPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPort* port, struct Message* message);
VOID myGetSysTimePPC(struct PrivatePPCBase* PowerPCBase, struct timeval* timeval);
VOID myAddTimePPC(struct PrivatePPCBase* PowerPCBase, struct timeval* dest, struct timeval* source);
VOID mySubTimePPC(struct PrivatePPCBase* PowerPCBase, struct timeval* dest, struct timeval* source);
LONG myCmpTimePPC(struct PrivatePPCBase* PowerPCBase, struct timeval* dest, struct timeval* source);
struct MsgPortPPC* mySetReplyPortPPC(struct PrivatePPCBase* PowerPCBase, struct Message* message, struct MsgPortPPC* port);
ULONG mySnoopTask(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist);
VOID myEndSnoopTask(struct PrivatePPCBase* PowerPCBase, ULONG id);
VOID myGetHALInfo(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist);
VOID mySetScheduling(struct PrivatePPCBase* PowerPCBase, struct TagItem* taglist);
struct TaskPPC* myFindTaskByID(struct PrivatePPCBase* PowerPCBase, LONG id);
LONG mySetNiceValue(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* task, LONG nice);
LONG myTrySemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, ULONG timeout);
VOID myNewListPPC(struct PrivatePPCBase* PowerPCBase, struct List* list);
ULONG mySetExceptPPC(struct PrivatePPCBase* PowerPCBase, ULONG signals, ULONG mask, ULONG flag);
VOID myObtainSemaphoreSharedPPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
LONG myAttemptSemaphoreSharedPPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
VOID myProcurePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, struct SemaphoreMessage* SemaphoreMessage);
VOID myVacatePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC, struct SemaphoreMessage* SemaphoreMessage);
VOID myCauseInterrupt(struct PrivatePPCBase* PowerPCBase);
APTR myCreatePoolPPC(struct PrivatePPCBase* PowerPCBase, ULONG flags, ULONG puddle_size, ULONG trehs_size);
VOID myDeletePoolPPC(struct PrivatePPCBase* PowerPCBase, APTR poolheader);
APTR myAllocPooledPPC(struct PrivatePPCBase* PowerPCBase, APTR poolheader, ULONG size);
VOID myFreePooledPPC(struct PrivatePPCBase* PowerPCBase, APTR poolheader, APTR ptr, ULONG size);
APTR myRawDoFmtPPC(struct PrivatePPCBase* PowerPCBase, STRPTR formatstring, APTR datastream, APTR (*putchproc)(), APTR putchdata);
LONG myPutPublicMsgPPC(struct PrivatePPCBase* PowerPCBase, STRPTR portname, struct Message* message);
LONG myAddUniquePortPPC(struct PrivatePPCBase* PowerPCBase, struct MsgPortPPC* port);
LONG myAddUniqueSemaphorePPC(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC);
BOOL myIsExceptionMode(struct PrivatePPCBase* PowerPCBase);
VOID myAllocPrivateMem(void);
VOID myFreePrivateMem(void);
VOID myResetPPC(void);
BOOL myChangeStack(struct PrivatePPCBase* PowerPCBase, ULONG NewStackSize);
ULONG myRun68KLowLevel(struct PrivatePPCBase* PowerPCBase, ULONG Code, LONG Offset, ULONG a0, ULONG a1, ULONG d0, ULONG d1);
VOID printDebug(struct PrivatePPCBase* PowerPCBase, struct DebugArgs* args);
APTR AllocVec68K(struct PrivatePPCBase* PowerPCBase, ULONG size, ULONG flags);
VOID FreeVec68K(struct PrivatePPCBase* PowerPCBase, APTR memBlock);
LONG LockMutexPPC(ULONG mutex);
VOID FreeMutexPPC(ULONG mutex);
ULONG CheckExcSignal(struct PrivatePPCBase* PowerPCBase, struct TaskPPC* myTask, ULONG signal);
VOID GetBATs(struct PrivatePPCBase* PowerPCBase);
VOID StoreBATs(struct PrivatePPCBase* PowerPCBase);
VOID MoveToBAT(ULONG BATnumber, struct BATArray* batArray);
VOID MoveFromBAT(ULONG BATnumber, struct BATArray* batArray);
struct MsgFrame* CreateMsgFramePPC(struct PrivatePPCBase* PowerPCBase);
struct MsgFrame* GetMsgFramePPC(struct PrivatePPCBase* PowerPCBase);
VOID SendMsgFramePPC(struct PrivatePPCBase* PowerPCBase, struct MsgFrame* msgFrame);
VOID FreeMsgFramePPC(struct PrivatePPCBase* PowerPCBase, struct MsgFrame* msgFrame);
LONG StricmpPPC(STRPTR string1, STRPTR string2);
VOID CauseDECInterrupt(struct PrivatePPCBase* PowerPCBase);
VOID InsertOnPri(struct PrivatePPCBase* PowerPCBase, struct List* list, struct TaskPPC* myTask);
VOID HandleMsgs(struct PrivatePPCBase* PowerPCBase);
VOID SwitchPPC(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe);
VOID SystemStart(struct PrivatePPCBase* PowerPCBase);
void DispatchPPC(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe, struct MsgFrame* newtask);
void StartTask(struct PrivatePPCBase* PowerPCBase, struct MsgFrame* myFrame);
void KillTask(struct PrivatePPCBase* PowerPCBase, struct MsgFrame* myFrame);
void CommonExcHandler(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe, struct List* excList);
void CommonExcError(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe);
VOID FlushDCache(struct PrivatePPCBase* PowerPCBase);
STRPTR GetName(void);
ULONG SmallExcHandler(struct ExcData* data, struct iframe* iframe);
ULONG DoAlign(struct iframe* iframe, ULONG SRR0);
ULONG DoDataStore(struct iframe* iframe, ULONG SRR0, struct DataMsg* data);
ULONG FinDataStore(ULONG value, struct iframe* iframe, ULONG SRR0, struct DataMsg* data);
VOID SwapStack(ULONG oldspupper, ULONG newspupper);
APTR AllocatePPC(struct PrivatePPCBase* PowerPCBase, struct MemHeader* memHeader, ULONG byteSize);
VOID DeallocatePPC(struct PrivatePPCBase* PowerPCBase, struct MemHeader* memHeader, APTR memoryBlock, ULONG byteSize);
VOID SetFPExc(VOID);
VOID FinalCalc(ULONG counter, ULONG tbu, ULONG tbl, ULONG value1, ULONG value2, struct WaitTime* myWait);
ULONG Calculator(ULONG value1, ULONG value2, ULONG value3);
VOID FreeAllExcMem(struct PrivatePPCBase* PowerPCBase, struct ExcInfo* excInfo);
VOID AddExcList(struct PrivatePPCBase* PowerPCBase, struct ExcInfo* excInfo, struct ExcData* newData, struct Node* currExc, ULONG flag);
VOID FPE_Enable(ULONG value);
VOID FPE_Disable(ULONG value);
VOID EnablePPC(VOID);
VOID DisablePPC(VOID);
VOID RunCPP(VOID);
VOID FlushICache(VOID);
ULONG* GetDecTable(VOID);

