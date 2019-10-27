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

void   illegal(void) = "\tillegal\n";           //debug function


#define _LVOSetHardware         -540
#define _LVOCreateTaskPPC       -336

BPTR   myExpunge(__reg("a6") struct PPCBase* PowerPCBase);

void   writememLong(ULONG Base, ULONG offset, ULONG value);
ULONG  readmemLong (ULONG Base, ULONG offset);

void   getENVs    (struct InternalConsts* myConsts);
void   PrintError (struct InternalConsts* myConsts, UBYTE* errortext);
void   PrintCrtErr(struct InternalConsts* myConsts, UBYTE* crterrtext);

struct InitData *SetupKiller (struct InternalConsts* myConsts, ULONG devfuncnum, struct PciDevice* ppcdevice, ULONG initPointer);
struct InitData *SetupHarrier(struct InternalConsts* myConsts, ULONG devfuncnum, struct PciDevice* ppcdevice, ULONG initPointer);
struct InitData *SetupMPC107 (struct InternalConsts* myConsts, ULONG devfuncnum, struct PciDevice* ppcdevice, ULONG initPointer);
