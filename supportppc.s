## Copyright (c) 2019, 2020 Dennis van der Boon
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.

#********************************************************************************************

.global     _LockMutexPPC, _FreeMutexPPC, _GetName, _CopyMemQuickPPC, _SwapStack, _SetFPExc
.global     _RunCPP

#********************************************************************************************

.section "functions","acrx"

#********************************************************************************************
#
#    Function to try and lock a hardware mutex.
#
#********************************************************************************************

_LockMutexPPC:
        lwzx	r0,r0,r3
        cmpwi	r0,0
        bne-	.AtomicOn

        lwarx	r0,r0,r3
        cmpwi	r0,0
        bne-	.AtomicOn

        li      r0,-1
        stwcx.	r0,r0,r3
        bne-	.AtomicOn

        li      r3,-1
        b       .AtomicOff

.AtomicOn:	
        li      r3,0

.AtomicOff:	
        isync
        blr

#********************************************************************************************
#
#    Funtion to free a hardware mutex.
#
#********************************************************************************************

_FreeMutexPPC:
        sync
        li      r0,0
        stw	    r0,0(r3)
        blr

#********************************************************************************************
#
#    Entry point for dispatching tasks. Jumped to from kernel using rfi.
#
#********************************************************************************************

_RunCPP:
        blr

#********************************************************************************************
#
#    Getting a name for our clean-up task without the need for SDA/relocation.
#
#********************************************************************************************

_GetName:
       mflr     r4
       bl       .skipName

       .byte  "Kryten\0\0"

.skipName:
      mflr      r3
      mtlr      r4
      blr

#********************************************************************************************
#
#
#
#********************************************************************************************

_CopyMemQuickPPC:
      mfctr     r8

      andi.     r3,r4,7
      bne-      .NoSAlign8

      andi.     r7,r5,7
      beq-      .Align8

.NoSAlign8:	
      andi.     r3,r4,3
      bne-      .NoSAlign4

      andi.     r7,r5,3
      beq-      .Align4

.NoSAlign4:	
      andi.     r3,r4,1
      bne-      .NoSAlign2

      andi.     r7,r5,1
      beq-      .Align2

.NoSAlign2:	
      mr.       r6,r6
      beq-      .ExitCopy

      mtctr     r6
      subi      r4,r4,1
      subi      r5,r5,1
.LoopAl1:	
      lbzu      r0,1(r4)
      stbu      r0,1(r5)
      bdnz+     .LoopAl1

      b         .ExitCopy

.Align2:	
      rlwinm    r7,r6,31,1,31
      mtctr     r7
      subi      r4,r4,2
      subi      r5,r5,2
      mr.       r7,r7
      beq-      .ExitAl2

.LoopAl2:	
      lhzu      r0,2(r4)
      sthu      r0,2(r5)
      bdnz+     .LoopAl2

.ExitAl2:	
      andi.     r6,r6,1
      beq-      .ExitCopy

      lbzu      r0,2(r4)
      stbu      r0,2(r5)
      b         .ExitCopy

.Align4:
      rlwinm    r7,r6,30,2,31
      mtctr     r7
      subi      r4,r4,4
      subi      r5,r5,4
      mr.       r7,r7
      beq-      .SmallSize4

.LoopAl4:
      lwzu      r0,4(r4)
      stwu      r0,4(r5)
      bdnz+     .LoopAl4

.SmallSize4:
      andi.     r6,r6,3
      beq-      .ExitCopy

      mtctr     r6
      addi      r4,r4,3
      addi      r5,r5,3

.SmallLoop4:	
      lbzu      r0,1(r4)
      stbu      r0,1(r5)
      bdnz+     .SmallLoop4

      b         .ExitCopy

.Align8:	
      rlwinm    r7,r6,29,3,31
      mtctr     r7
      subi      r4,r4,8
      subi      r5,r5,8
      mr.       r7,r7
      beq-      .SmallSize8

.LoopAl8:	
      lfdu      f0,8(r4)
      stfdu     f0,8(r5)
      bdnz+     .LoopAl8

.SmallSize8:
      andi.     r6,r6,7
      beq-      .ExitCopy

      mtctr     r6
      addi      r4,r4,7
      addi      r5,r5,7

.SmallLoop8:	
      lbzu      r0,1(r4)
      stbu      r0,1(r5)
      bdnz+     .SmallLoop8

.ExitCopy:	
      mtctr     r8
      blr

#********************************************************************************************
#
#
#
#********************************************************************************************

_SwapStack:

        sub     r5,r3,r1
        sub     r1,r4,r5
        sub     r4,r4,r3
        mr      r6,r1

.StckPntrLoop:
        lwz     r3,0(r6)
        mr.     r3,r3
        beq-    .DoneSwap

        add     r3,r3,r4
        stw     r3,0(r6)
        mr      r6,r3
        b      .StckPntrLoop

.DoneSwap:
        blr

#********************************************************************************************
#
#
#
#********************************************************************************************

_SetFPExc:
        mtfsfi	1,0
        mtfsfi	2,0
        mtfsb0	3
        mtfsb0	12
        mtfsb0	21
        mtfsb0	22
        mtfsb0	23
        blr

#********************************************************************************************


