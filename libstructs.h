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
#include <exec/exec.h>
#include <dos/dos.h>

struct PPCZeroPage {
ULONG               zp_PPCMemBase;
struct ExecBase*    zp_SysBase;
struct MemHeader*   zp_PPCMemHeader;
struct DosLibrary*  zp_DOSBase;
struct MsgPort*     zp_MCPort;
ULONG               zp_Status;
struct PPCBase*     zp_PowerPCBase;
ULONG               zp_PageTableSize;
};

struct InternalConsts {
struct ExecBase*        ic_SysBase;
BPTR                    ic_SegList;
struct DosLibrary*      ic_DOSBase;
struct PciBase*         ic_PciBase;
struct PPCBase*         ic_PowerPCBase;
struct ExpansionBase*   ic_ExpansionBase;
ULONG                   ic_gfxMem;
ULONG                   ic_gfxSize;
ULONG                   ic_gfxConfig;
ULONG                   ic_startBAT;
ULONG                   ic_sizeBAT;
ULONG                   ic_env1;
ULONG                   ic_env2;
ULONG                   ic_env3;
UWORD                   ic_gfxType;
};

struct InitData {
ULONG               id_Reserved;
ULONG               id_Status;
ULONG               id_MemBase;
ULONG               id_MemSize;
ULONG               id_GfxMemBase;
ULONG               id_GfxMemSize;
ULONG               id_GfxType;
ULONG               id_GfxConfigMemBase;
ULONG               id_Environment1;
ULONG               id_Environment2;
ULONG               id_Environment3;
ULONG               id_XPMIBase;
ULONG               id_StartBat;
ULONG               id_SizeBat;
};

