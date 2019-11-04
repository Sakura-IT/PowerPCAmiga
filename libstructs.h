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

#include <powerpc/powerpc.h>
#include <powerpc/portsPPC.h>
#include <powerpc/semaphoresPPC.h>
#include <powerpc/tasksPPC.h>
#include <exec/exec.h>
#include <dos/dos.h>
#include <powerpc/portsPPC.h>
#include <powerpc/tasksPPC.h>
#include <powerpc/powerpc.h>

struct MsgFrame {
struct Message      mf_Message;
ULONG               mf_Identifier;
struct MsgPort*     mf_MirrorPort;
struct MsgPortPPC*  mf_PPCPort;
struct PPCArgs      mf_PPCArgs;
struct MsgPort*     mf_MCPort;          //Needed?
ULONG               mf_Signals;
ULONG               mf_Arg[3];
};

struct MirrorTask {
struct MinNode      mt_Node;
struct Task*        mt_MirrorTask;
struct MsgPort*     mt_MirrorPort;
ULONG               mt_Flags;
};

struct PPCZeroPage {
ULONG               zp_PPCMemBase;
struct ExecBase*    zp_SysBase;
struct MemHeader*   zp_PPCMemHeader;
struct DosLibrary*  zp_DOSBase;
struct MsgPort*     zp_MCPort;
volatile ULONG      zp_Status;
struct PPCBase*     zp_PowerPCBase;
ULONG               zp_PageTableSize;
struct UtilityBase* zp_UtilityBase;
ULONG               zp_CacheGap[7];
ULONG               zp_MemSize;
};

struct InternalConsts {
struct ExecBase*        ic_SysBase;
BPTR                    ic_SegList;
struct DosLibrary*      ic_DOSBase;
struct PciBase*         ic_PciBase;
struct PPCBase*         ic_PowerPCBase;
struct ExpansionBase*   ic_ExpansionBase;
ULONG                   ic_MemBase;
ULONG                   ic_gfxMem;
ULONG                   ic_gfxSize;
ULONG                   ic_gfxConfig;
ULONG                   ic_startBAT;
ULONG                   ic_sizeBAT;
ULONG                   ic_env1;
ULONG                   ic_env2;
ULONG                   ic_env3;
UWORD                   ic_gfxType;
UWORD                   ic_gfxSubType;
};

struct InitData {
ULONG               id_Reserved;
ULONG               id_Status;
ULONG               id_MemBase;
ULONG               id_MemSize;
ULONG               id_GfxMemBase;
ULONG               id_GfxMemSize;
UWORD               id_GfxType;
UWORD               id_GfxSubType;
ULONG               id_GfxConfigBase;
ULONG               id_Environment1;
ULONG               id_Environment2;
ULONG               id_Environment3;
ULONG               id_XPMIBase;
ULONG               id_StartBat;
ULONG               id_SizeBat;
};

struct BATs {
ULONG               bt_ibat0u;
ULONG               bt_ibat0l;
ULONG               bt_ibat1u;
ULONG               bt_ibat1l;
ULONG               bt_ibat2u;
ULONG               bt_ibat2l;
ULONG               bt_ibat3u;
ULONG               bt_ibat3l;
ULONG               bt_dbat0u;
ULONG               bt_dbat0l;
ULONG               bt_dbat1u;
ULONG               bt_dbat1l;
ULONG               bt_dbat2u;
ULONG               bt_dbat2l;
ULONG               bt_dbat3u;
ULONG               bt_dbat3l;
};

struct PrivatePPCBase {
struct PPCBase              pp_PowerPCBase;
struct MinList              pp_MirrorList;
ULONG                       pp_DeviceID;
UBYTE                       pp_DebugLevel;
UBYTE                       pp_EnAlignExc;
UBYTE                       pp_EnDAccessExc;
UBYTE                       pp_Pad;
ULONG                       pp_TaskExitCode;
ULONG                       pp_PPCMemBase;
ULONG                       pp_Atomic;
ULONG                       pp_TaskExcept;
ULONG                       pp_PPCMemSize;
ULONG                       pp_MCPort;
ULONG                       pp_L2Size;
ULONG                       pp_CurrentL2Size;
ULONG                       pp_L2State;
ULONG                       pp_CPUSDR1;
ULONG                       pp_CPUHID0;
ULONG                       pp_CPUSpeed;
ULONG                       pp_CPUInfo;
struct MinList              pp_WaitingTasks;
struct MinList              pp_AllTasks;
struct MinList              pp_Snoop;
struct MinList              pp_Semaphores;
struct MinList              pp_RemovedExc;
struct MinList              pp_ReadyExc;
struct MinList              pp_InstalledExc;
struct MinList              pp_ExcInterrupt;
struct MinList              pp_ExcIABR;
struct MinList              pp_ExcPerfMon;
struct MinList              pp_ExcTrace;
struct MinList              pp_ExcSystemCall;
struct MinList              pp_ExcDecrementer;
struct MinList              pp_ExcFPUn;
struct MinList              pp_ExcProgram;
struct MinList              pp_ExcAlign;
struct MinList              pp_ExcIAccess;
struct MinList              pp_ExcDAccess;
struct MinList              pp_ExcMCheck;
struct MinList              pp_ExcSysMan;
struct MinList              pp_ExcTherMan;
struct MinList              pp_WaitTime;
struct MinList              pp_Ports;
struct MinList              pp_NewTasks;
struct MinList              pp_ReadyTasks;
struct MinList              pp_RemovedTasks;
struct MinList              pp_MsgQueue;
struct SignalSemaphorePPC   pp_SemWaitList;
struct SignalSemaphorePPC   pp_SemTaskList;
struct SignalSemaphorePPC   pp_SemSemList;
struct SignalSemaphorePPC   pp_SemPortList;
struct SignalSemaphorePPC   pp_SemSnoopList;
struct SignalSemaphorePPC   pp_SemMemory;
ULONG                       pp_AlignmentExcHigh;
ULONG                       pp_AlignmentExcLow;
ULONG                       pp_DataExcHigh;
ULONG                       pp_DataExcLow;
struct MsgPortPPC*          pp_CurrentPort;
UBYTE                       pp_ExternalInt;
UBYTE                       pp_EnAltivec;
UBYTE                       pp_ExceptionMode;
UBYTE                       pp_CacheDoDFlushAll;
UBYTE                       pp_CacheDState;
UBYTE                       pp_CacheDLockState;
UBYTE                       pp_FlagReschedule;
UBYTE                       pp_FlagWait;
UBYTE                       pp_FlagPortInUse;
UBYTE                       pp_FlagBusyCounter;
UBYTE                       pp_Pad2[2];
ULONG                       pp_NumAllTasks;
struct TaskPPC*             pp_ThisPPCProc;
ULONG                       pp_StartTBL;
ULONG                       pp_CurrentTBL;
ULONG                       pp_SystemLoad;
ULONG                       pp_NICETable;
ULONG                       pp_LowActivityPrio;
ULONG                       pp_IdSysTasks;
ULONG                       pp_IdUsrTasks;
ULONG                       pp_LowActPrioOffset;
ULONG                       pp_ErrorStrings;
ULONG                       pp_BusClock;
ULONG                       pp_Quantum;
ULONG                       pp_NumRun68k;
struct BATs                 pp_ExceptionBATs;
struct BATs                 pp_StoredBATs;
struct BATs                 pp_SystemBATs;
};

struct killFIFO {
ULONG                       kf_MIOFH;
ULONG                       kf_MIOPT;
ULONG                       kf_MIIFT;
ULONG                       kf_MIIPH;
ULONG                       kf_Previous;
ULONG                       kf_CacheGap[3];
ULONG                       kf_MIOFT;
ULONG                       kf_MIOPH;
ULONG                       kf_MIIFH;
ULONG                       kf_MIIPT;
};

