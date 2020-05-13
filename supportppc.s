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

.include    constantsppc.i
.include    macros.i

#********************************************************************************************

.global     _LockMutexPPC, _FreeMutexPPC, _CopyMemQuickPPC, _SwapStack, _SetFPExc
.global     _FinalCalc, _Calculator, _FPE_Enable, _FPE_Disable, _RunCPP, _GetDecTable
.global     _xAllocatePPC, _xDeallocatePPC

#********************************************************************************************

.section "functions","acrx"

#********************************************************************************************
#
#	support: memory = AllocatePPC(Memheader, byteSize) // r3=r4,r5
#
#********************************************************************************************

_xAllocatePPC:
		prolog 228,'TOC'

		stwu	r31,-4(r13)
		stwu	r30,-4(r13)
		stwu	r29,-4(r13)
		stwu	r28,-4(r13)

		mr.	r3,r5
		beq-	.ExitAlloc

		mr	r31,r5
		mr	r30,r4
		addi	r31,r31,31			#Check: Original was 7
		loadreg	r29,0xffffffe0
		and	r31,r31,r29

		lwz	r29,MH_FREE(r30)
		cmplw	r31,29
		ble	.EnoughRoom

		li	r3,0

		b	.ExitAlloc

.EnoughRoom:	la	r4,MH_FIRST(r30)

.NextChunk:	lwz	r5,MC_NEXT(r4)
		mr.	r3,r5

		beq-	.ExitAlloc

		lwz	r29,MC_BYTES(r5)
		cmplw	r31,r29
		bgt	.TooBeaucoup
		bne	.NotPerfect

		lwz	r29,MC_NEXT(r5)
		stw	r29,MC_NEXT(r4)
		mr	r3,r5

		b	.SetFree

.NotPerfect:	add	r28,r5,r31
		lwz	r29,MC_NEXT(r5)
		stw	r29,MC_NEXT(r28)
		lwz	r29,MC_BYTES(r5)
		sub	r29,r29,r31
		stw	r29,MC_BYTES(r28)
		stw	r28,MC_NEXT(r4)
		mr	r3,r5

		b	.SetFree

.TooBeaucoup:	lwz	r4,MC_NEXT(r4)
		mr.	r4,r4

		bne	.NextChunk

		li	r3,0

		b	.ExitAlloc

.SetFree:	lwz	r29,MH_FREE(r30)
		sub	r29,r29,r31
		stw	r29,MH_FREE(r30)

		subi	r28,r3,1
		mfctr	r29
		mtctr	r31
		li	r30,0
.ClearAlloc:	stbu	r30,1(r28)
		bdnz	.ClearAlloc
		mtctr	r29

.ExitAlloc:	lwz	r28,0(r13)
		lwz	r29,4(r13)
		lwz	r30,8(r13)
		lwz	r31,12(r13)
		addi	r13,r13,16

		epilog 'TOC'

#********************************************************************************************
#
#	support: void DeallocatePPC(Memheader, memoryBlock, byteSize) // r4,r5,r6(a0,a1,d0)
#
#********************************************************************************************

_xDeallocatePPC:
		prolog 228,'TOC'

		stwu	r31,-4(r13)
		stwu	r30,-4(r13)
		stwu	r29,-4(r13)
		stwu	r28,-4(r13)
		stwu	r27,-4(r13)
		stwu	r26,-4(r13)

		mr.	r31,r6
		beq 	.ExitDealloc

		mr	r29,r4
		mr  r30,r5

#        loadreg r28,-32
#		 and r30,r5,r28
#		 sub r5,r5,r30
#		 add r6,r5,r31
#		 addi	 r6,r6,31
#		 and.	 r6,r6,r28

#		 beq .ExitDealloc

		la	r28,MH_FIRST(r29)
		lwz	r27,MC_NEXT(r28)
		mr.	r27,r27

		beq	.LinkNewMC

.NextMemChunk:	cmplw	r27,r30

		bgt	.CorrectMC
		beq	.GuruTime

		mr	r28,r27
		lwz	r27,MC_NEXT(r28)
		mr.	r27,r27

		bne	.NextMemChunk

.CorrectMC:	la	r26,MH_FIRST(r29)
		cmplw	r28,r26

		beq	.LinkNewMC

		lwz	r27,MC_BYTES(r28)
		add	r27,r27,r28
		cmplw	r30,r27

		beq	.JoinThem
		blt	.GuruTime

.LinkNewMC:	lwz	r27,MC_NEXT(r28)
		stw	r27,MC_NEXT(r30)
		stw	r30,MC_NEXT(r28)
		stw	r6,MC_BYTES(r30)

		b	.DoNextMC

.JoinThem:	lwz	r27,MC_BYTES(r28)
		add	r27,r27,r6
		stw	r27,MC_BYTES(r28)
		mr	r30,r28

.DoNextMC:	lwz	r26,MC_NEXT(r30)
		mr.	r26,r26

		beq	.UpdateFree

		lwz	r27,MC_BYTES(r30)
		add	r27,r27,r30
		cmplw	r26,r27

		blt	.GuruTime
		bne	.UpdateFree

		mr	r28,r26
		lwz	r27,MC_NEXT(r28)
		stw	r27,MC_NEXT(r30)
		lwz	r27,MC_BYTES(r28)
		lwz	r26,MC_BYTES(r30)
		add	r27,r27,r26
		stw	r27,MC_BYTES(r30)

.UpdateFree:	lwz	r27,MH_FREE(r29)
		add	r27,r27,r6
		stw	r27,MH_FREE(r29)

.ExitDealloc:	lwz	r26,0(r13)
		lwz	r27,4(r13)
		lwz	r28,8(r13)
		lwz	r29,12(r13)
		lwz	r30,16(r13)
		lwz	r31,20(r13)
		addi	r13,r13,24

		epilog 'TOC'

#********************************************************************************************

.GuruTime:	loadreg	r0,'EMEM'

		illegal

.GTHalt:	b	.GTHalt			#STUB
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
#    Entry point for dispatching tasks. r3 = iframe, r4 = code, r5 = args
#
#********************************************************************************************

_RunCPP:
        stwu    r1,-256(r1)
        stw     r3,200(r1)

        stw     r3,IF_CONTEXT_GPR+12(r3)
        la      r3,IF_CONTEXT_GPR(r3)
        stw     r0,0(r3)
        mflr    r0
        stwu    r0,4(r3)        #using spot r1 for lr
        stwu    r2,4(r3)
        stwu    r4,8(r3)
        stwu    r5,4(r3)
        stwu    r6,4(r3)
        stwu    r7,4(r3)
        stwu    r8,4(r3)
        stwu    r9,4(r3)
        stwu    r10,4(r3)
        stwu    r11,4(r3)
        stwu    r12,4(r3)
        stwu    r13,4(r3)
        stwu    r14,4(r3)
        stwu    r15,4(r3)
        stwu    r16,4(r3)
        stwu    r17,4(r3)
        stwu    r18,4(r3)
        stwu    r19,4(r3)
        stwu    r20,4(r3)
        stwu    r21,4(r3)
        stwu    r22,4(r3)
        stwu    r23,4(r3)
        stwu    r24,4(r3)
        stwu    r25,4(r3)
        stwu    r26,4(r3)
        stwu    r27,4(r3)
        stwu    r28,4(r3)
        stwu    r29,4(r3)
        stwu    r30,4(r3)
        stwu    r31,4(r3)

        stfdu   f0,4(r3)
        stfdu   f1,8(r3)
        stfdu   f2,8(r3)
        stfdu   f3,8(r3)
        stfdu   f4,8(r3)
        stfdu   f5,8(r3)
        stfdu   f6,8(r3)
        stfdu   f7,8(r3)
        stfdu   f8,8(r3)
        stfdu   f9,8(r3)
        stfdu   f10,8(r3)
        stfdu   f11,8(r3)
        stfdu   f12,8(r3)
        stfdu   f13,8(r3)
        stfdu   f14,8(r3)
        stfdu   f15,8(r3)
        stfdu   f16,8(r3)
        stfdu   f17,8(r3)
        stfdu   f18,8(r3)
        stfdu   f19,8(r3)
        stfdu   f20,8(r3)
        stfdu   f21,8(r3)
        stfdu   f22,8(r3)
        stfdu   f23,8(r3)
        stfdu   f24,8(r3)
        stfdu   f25,8(r3)
        stfdu   f26,8(r3)
        stfdu   f27,8(r3)
        stfdu   f28,8(r3)
        stfdu   f29,8(r3)
        stfdu   f30,8(r3)
        stfdu   f31,8(r3)

        lwz     r7,PP_STACK(r5)
        lwz     r8,PP_STACKSIZE(r5)

        mr.     r7,r7
        beq     .noStack

        mr.     r8,r8
        beq    .noStack

        mr      r9,r1
        sub     r1,r1,r8
        subi    r10,r1,1
        subi    r11,r7,1

.copyStack:
        lbzu    r12,1(r11)
        stbu    r12,1(r10)
        subi    r8,r8,1
        mr.     r8,r8
        bne     .copyStack

        subi    r1,r1,24
        stw     r9,0(r1)
        b       .doneStack

.noStack:
        stwu    r1,-68(r1)
.doneStack:
        mr      r14,r5
        mtlr    r4

        lwz	    r2,PP_REGS+12*4(r14)
        lwz	    r3,PP_REGS+0*4(r14)
        lwz	    r4,PP_REGS+1*4(r14)
        lwz	    r5,PP_REGS+8*4(r14)
        lwz	    r6,PP_REGS+9*4(r14)
        lwz	    r22,PP_REGS+2*4(r14)
        lwz	    r23,PP_REGS+3*4(r14)
        lwz	    r24,PP_REGS+4*4(r14)
        lwz	    r25,PP_REGS+5*4(r14)
        lwz	    r26,PP_REGS+6*4(r14)
        lwz	    r27,PP_REGS+7*4(r14)
        lwz	    r28,PP_REGS+10*4(r14)
        lwz	    r29,PP_REGS+11*4(r14)
        lwz	    r30,PP_REGS+13*4(r14)
        lwz	    r31,PP_REGS+14*4(r14)
        lfd	    f1,PP_FREGS+0*8(r14)
        lfd	    f2,PP_FREGS+1*8(r14)
        lfd	    f3,PP_FREGS+2*8(r14)
        lfd	    f4,PP_FREGS+3*8(r14)
        lfd	    f5,PP_FREGS+4*8(r14)
        lfd	    f6,PP_FREGS+5*8(r14)
        lfd	    f7,PP_FREGS+6*8(r14)
        lfd	    f8,PP_FREGS+7*8(r14)

        li      r0,0
        mr      r7,r0
        mr      r8,r0
        mr      r9,r0
        mr      r10,r0
        mr      r11,r0
        mr      r12,r0
        mr      r20,r0
        mr      r21,r0

        lwz     r15,PP_FLAGS(r14)
        rlwinm. r16,r15,(32-PPB_LINEAR),31,31
        beq     .notLinear

        mr      r5,r22
        mr      r6,r23
        mr      r7,r24
        mr      r8,r25
        mr      r9,r26
        mr      r10,r27

.notLinear:
        mr      r14,r0
        mr      r16,r0
        mr      r17,r0
        mr      r18,r0
        mr      r19,r0

        rlwinm. r15,r15,(32-PPB_THROW),31,31
        mr      r15,r0
        loadreg r0,'WARP'
        beq     .noThrow

        trap

.noThrow:
        blrl

        lwz     r1,0(r1)
        lwz     r14,200(r1)         #frame
        la      r14,IF_CONTEXT_GPR(r14)
        lwz     r15,5*4(r14)        #args
        lwz     r16,PP_FLAGS(r15)
        rlwinm. r16,r16,(32-PPB_LINEAR),31,31
        beq     .notLinear2

        mr      r22,r5
        mr      r23,r6
        mr      r24,r7
        mr      r25,r8
        mr      r26,r9
        mr      r27,r10

.notLinear2:
        stw     r2,PP_REGS+12*4(r15)
        stw     r3,PP_REGS+0*4(r15)
        stw     r4,PP_REGS+1*4(r15)
        stw     r5,PP_REGS+8*4(r15)
        stw     r6,PP_REGS+9*4(r15)
        stw     r22,PP_REGS+2*4(r15)
        stw     r23,PP_REGS+3*4(r15)
        stw     r24,PP_REGS+4*4(r15)
        stw     r25,PP_REGS+5*4(r15)
        stw     r26,PP_REGS+6*4(r15)
        stw     r27,PP_REGS+7*4(r15)
        stw     r28,PP_REGS+10*4(r15)
        stw     r29,PP_REGS+11*4(r15)
        stw     r30,PP_REGS+13*4(r15)
        stw     r31,PP_REGS+14*4(r15)
        stfd    f1,PP_FREGS+0*8(r15)
        stfd    f2,PP_FREGS+1*8(r15)
        stfd    f3,PP_FREGS+2*8(r15)
        stfd    f4,PP_FREGS+3*8(r15)
        stfd    f5,PP_FREGS+4*8(r15)
        stfd    f6,PP_FREGS+5*8(r15)
        stfd    f7,PP_FREGS+6*8(r15)
        stfd    f8,PP_FREGS+7*8(r15)

        mr      r3,r14
        lwz     r0,0(r3)
        lwzu    r14,4(r3)
        mtlr    r14
        lwzu    r2,4(r3)
        lwzu    r4,8(r3)
        lwzu    r5,4(r3)
        lwzu    r6,4(r3)
        lwzu    r7,4(r3)
        lwzu    r8,4(r3)
        lwzu    r9,4(r3)
        lwzu    r10,4(r3)
        lwzu    r11,4(r3)
        lwzu    r12,4(r3)
        lwzu    r13,4(r3)
        lwzu    r14,4(r3)
        lwzu    r15,4(r3)
        lwzu    r16,4(r3)
        lwzu    r17,4(r3)
        lwzu    r18,4(r3)
        lwzu    r19,4(r3)
        lwzu    r20,4(r3)
        lwzu    r21,4(r3)
        lwzu    r22,4(r3)
        lwzu    r23,4(r3)
        lwzu    r24,4(r3)
        lwzu    r25,4(r3)
        lwzu    r26,4(r3)
        lwzu    r27,4(r3)
        lwzu    r28,4(r3)
        lwzu    r29,4(r3)
        lwzu    r30,4(r3)
        lwzu    r31,4(r3)

        lfdu    f0,4(r3)
        lfdu    f1,8(r3)
        lfdu    f2,8(r3)
        lfdu    f3,8(r3)
        lfdu    f4,8(r3)
        lfdu    f5,8(r3)
        lfdu    f6,8(r3)
        lfdu    f7,8(r3)
        lfdu    f8,8(r3)
        lfdu    f9,8(r3)
        lfdu    f10,8(r3)
        lfdu    f11,8(r3)
        lfdu    f12,8(r3)
        lfdu    f13,8(r3)
        lfdu    f14,8(r3)
        lfdu    f15,8(r3)
        lfdu    f16,8(r3)
        lfdu    f17,8(r3)
        lfdu    f18,8(r3)
        lfdu    f19,8(r3)
        lfdu    f20,8(r3)
        lfdu    f21,8(r3)
        lfdu    f22,8(r3)
        lfdu    f23,8(r3)
        lfdu    f24,8(r3)
        lfdu    f25,8(r3)
        lfdu    f26,8(r3)
        lfdu    f27,8(r3)
        lfdu    f28,8(r3)
        lfdu    f29,8(r3)
        lfdu    f30,8(r3)
        lfdu    f31,8(r3)

        lwz     r3,200(r1)
        lwz     r1,0(r1)
        blr

#********************************************************************************************
#
#
#
#********************************************************************************************

_GetDecTable:
       mflr      r4
       bl        .skipTable

       .long     0x3b9aca00,0x05f5e100,0x00989680,0x000f4240,0x000186a0
       .long     0x00002710,0x000003e8,0x00000064,0x0000000a,0x00000000

.skipTable:
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
#
#
#
#********************************************************************************************

_Calculator:
        mfctr   r8
        li      r9,32
        mtctr   r9
        li      r6,0
        mr.     r3,r3

.DoCalcLoop:	
        bge-    .XNotNeg

        addc    r4,r4,r4
        adde    r3,r3,r3
        add     r6,r6,r6
        b       .XWasNeg

.XNotNeg:	
        addc    r4,r4,r4
        adde    r3,r3,r3
        add     r6,r6,r6
        cmplw   r5,r3
        bgt-    .SkipNext

.XWasNeg:	
        sub.    r3,r3,r5
        addi    r6,r6,1
.SkipNext:	
        bdnz+   .DoCalcLoop

        mtctr   r8
        mr	r3,r6

		blr

#********************************************************************************************
#
#
#
#********************************************************************************************

_FinalCalc:
        mfctr   r9
        mtctr   r3
        mr.     r3,r3
        beq-    .TimeCalced

.CalcLoop:	
        addc    r5,r5,r6
        addze   r4,r4
        bdnz+   .CalcLoop

.TimeCalced:
        addc    r5,r5,r7
        addze   r4,r4

        stw     r4,WAITTIME_UPPER(r8)
        stw     r5,WAITTIME_LOWER(r8)
        mfctr   r9
        blr

#********************************************************************************************
#
#
#
#********************************************************************************************

_FPE_Enable:
        mflr    r4
        andi.	r0,r3,1
        bnel-	.Bit0
        rlwinm.	r0,r3,31,31,31
        bnel-	.Bit1
        rlwinm.	r0,r3,30,31,31
        bnel-	.Bit2
        rlwinm.	r0,r3,29,31,31
        bnel-	.Bit3
        rlwinm.	r0,r3,28,31,31
        bnel-	.Bit4
        mtlr    r4
        blr

.Bit0:  mtfsb0	3
        mtfsb1	25
        blr

.Bit1:  mtfsb0	4
        mtfsb1	26
        blr

.Bit2:  mtfsb0	5
        mtfsb1	27
        blr

.Bit3:  mtfsb0	6
        mtfsb1	28
        blr

.Bit4:  mtfsb0	7
        mtfsb0	8
        mtfsb0	9
        mtfsb0	10
        mtfsb0	11
        mtfsb0	12
        mtfsb0	21
        mtfsb0	22
        mtfsb0	23
        mtfsb1	24
        blr

#********************************************************************************************
#
#
#
#********************************************************************************************


_FPE_Disable:
        mflr    r4
        andi.	r0,r3,1
        bnel-	.Bit_0
        rlwinm.	r0,r3,31,31,31
        bnel-	.Bit_1
        rlwinm.	r0,r3,30,31,31
        bnel-	.Bit_2
        rlwinm.	r0,r3,29,31,31
        bnel-	.Bit_3
        rlwinm.	r0,r3,28,31,31
        bnel-	.Bit_4
        mtlr    r4
        blr

.Bit_0: mtfsb0	25
        blr

.Bit_1: mtfsb0	26
        blr

.Bit_2: mtfsb0	27
        blr

.Bit_3: mtfsb0	28
        blr

.Bit_4: mtfsb0	24
        blr

#********************************************************************************************

