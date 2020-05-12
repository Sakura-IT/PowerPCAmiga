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

.global     _Exception_Entry, _SmallExcHandler, _DoAlign, _DoDataStore, _FinDataStore
.global     _FlushICache

.section "kernel","acrx"

#********************************************************************************************
#
#
#
#********************************************************************************************

_ExcCommon:

        b       ExcITLBMiss
        b       ExcDLoadTLBMiss
        b       ExcDStoreTLBMiss


        #256 nothing 512 altivec 24 altivec support 40 EXC header 128 GPR 256 FPR 64 BATs 64 Segments

        mtsprg2 r0                                         #LR; sprg3 = r0

        mfsrr0  r0
        mtsprg0 r0
        mfsrr1  r0
        mtsprg1 r0

        mfmsr   r0
        ori     r0,r0,(PSL_IR|PSL_DR|PSL_FP)
        mtmsr   r0			                               #Reenable MMU
        sync                                               #Also reenable FPU
        isync

        mr      r0,r1
        subi    r1,r1,2048
        rlwinm  r1,r1,0,0,26
        stw     r0,0(r1)                                   #Make room for struct iframe and align on 0x20

        stw     r4,IF_GAP+IF_CONTEXT_GPR+GPR4(r1)          #GPR[4]

        lwz     r4,0xa0(r0)
        addi    r4,r4,1
        stw     r4,0xa0(r0)
        mfsprg0 r4
        stw     r4,0xa4(r0)

        mfsprg3 r0
        la      r4,IF_GAP(r1)                              #iFrame
        stw     r3,IF_CONTEXT_GPR+GPR3(r4)                 #GPR[3]
        stw     r0,IF_CONTEXT_GPR+GPR0(r4)                 #GPR[0]
        mflr    r3
        rlwinm  r0,r3,24,24,31
        stw     r3,-4(r4)
        stw     r0,IF_EXCNUM(r4)                           #EXCB
        li      r3,1
        rlwnm   r0,r3,r0,0,31
        stw     r0,IF_CONTEXT_EXCID(r4)                    #EXC_ID (EXCF)
        la      r3,IF_CONTEXT(r4)
        mfsprg2 r0
        stw     r0,-8(r4)                                  #LR

        bl      _StoreFrame                  #r0, r3 and r4 are skipped in this routine and were saved above

        la      r4,IF_GAP(r1)                              #iFrame
        lwz     r5,-4(r4)
        subi    r5,r5,PPC_VECLEN*4
        stw     r5,IF_EXCEPTIONVECTOR(r4)                  #if_ExceptionVector
        lwz     r3,PowerPCBase(r0)                         #Loads PowerPCBase

        bl      _Exception_Entry

        stw     r1,0x90(r0)

        la      r31,IF_GAP+IF_CONTEXT(r1)

        bl      _LoadFrame                   #LR, r0,r1 and r3 are skipped in this routine and are loaded below

        bl      _FlushICache

        lwz     r0,IF_GAP+IF_CONTEXT_LR(r1)                #EXC_LR
        mtlr    r0
        lwz     r0,IF_GAP+IF_CONTEXT_GPR+GPR0(r1)          #GPR[0]
        lwz     r3,IF_GAP+IF_CONTEXT_GPR+GPR3(r1)          #GPR[3]
        lwz     r1,IF_GAP+IF_CONTEXT_GPR+GPR1(r1)          #GPR[1]

        rfi

#********************************************************************************************

_StoreFrame:

        stfd    f0,IF_CONTEXT_FPR-IF_CONTEXT(r3)
        mfsprg0 r0
        stwu    r0,4(r3)
        mfsprg1 r0
        stwu    r0,4(r3)
        mfdar   r0
        stwu    r0,4(r3)
        mfdsisr r0
        stwu    r0,4(r3)
        mfcr    r0
        stwu    r0,4(r3)
        mfctr   r0
        stwu    r0,4(r3)
        lwz     r0,-8(r4)
        stwu    r0,4(r3)                     #LR
        mffs    f0
        stfdu   f0,4(r3)
        mfxer   r0
        stw     r0,0(r3)

        lwz     r0,0(r1)
        stwu    r0,12(r3)                    #r1, skipped r0
        stwu    r2,4(r3)
        stwu    r5,12(r3)                    #skipped r3 and r4. Need to be stored seperately
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

        stfdu   f1,12(r3)                    #skipped f0 (see above)
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

        mfibatu r0,0
        stwu    r0,8(r3)
        mfibatl r0,0
        stwu    r0,4(r3)
        mfdbatu r0,0
        stwu    r0,4(r3)
        mfdbatl r0,0
        stwu    r0,4(r3)
        mfibatu r0,1
        stwu    r0,4(r3)
        mfibatl r0,1
        stwu    r0,4(r3)
        mfdbatu r0,1
        stwu    r0,4(r3)
        mfdbatl r0,1
        stwu    r0,4(r3)
        mfibatu r0,2
        stwu    r0,4(r3)
        mfibatl r0,2
        stwu    r0,4(r3)
        mfdbatu r0,2
        stwu    r0,4(r3)
        mfdbatl r0,2
        stwu    r0,4(r3)
        mfibatu r0,3
        stwu    r0,4(r3)
        mfibatl r0,3
        stwu    r0,4(r3)
        mfdbatu r0,3
        stwu    r0,4(r3)
        mfdbatl r0,3
        stwu    r0,4(r3)

        mfsr    r0,0
        stwu    r0,4(r3)
        mfsr    r0,1
        stwu    r0,4(r3)
        mfsr    r0,2
        stwu    r0,4(r3)
        mfsr    r0,3
        stwu    r0,4(r3)
        mfsr    r0,4
        stwu    r0,4(r3)
        mfsr    r0,5
        stwu    r0,4(r3)
        mfsr    r0,6
        stwu    r0,4(r3)
        mfsr    r0,7
        stwu    r0,4(r3)
        mfsr    r0,8
        stwu    r0,4(r3)
        mfsr    r0,9
        stwu    r0,4(r3)
        mfsr    r0,10
        stwu    r0,4(r3)
        mfsr    r0,11
        stwu    r0,4(r3)
        mfsr    r0,12
        stwu    r0,4(r3)
        mfsr    r0,13
        stwu    r0,4(r3)
        mfsr    r0,14
        stwu    r0,4(r3)
        mfsr    r0,15
        stwu    r0,4(r3)

        blr

#********************************************************************************************

_LoadFrame:

        lwzu    r3,4(r31)
        mtsrr0  r3
        lwzu    r3,4(r31)
        mtsrr1  r3
        lwzu    r3,4(r31)
        mtdar   r3
        lwzu    r3,4(r31)
        mtdsisr r3
        lwzu    r3,4(r31)
        mtcr    r3
        lwzu    r3,4(r31)
        mtctr   r3
        lwzu    r3,8(r31)                    #skip lr, is loaded seperately
        mtxer   r3
        lfd     f0,0(r31)
        mtfsf   0xff,f0

        lwzu    r0,8(r31)
        lwzu    r2,8(r31)                    #skip r1, is loaded seperately
        lwzu    r4,8(r31)                    #skip r3, is loaded seperately
        lwzu    r5,4(r31)
        lwzu    r6,4(r31)
        lwzu    r7,4(r31)
        lwzu    r8,4(r31)
        lwzu    r9,4(r31)
        lwzu    r10,4(r31)
        lwzu    r11,4(r31)
        lwzu    r12,4(r31)
        lwzu    r13,4(r31)
        lwzu    r14,4(r31)
        lwzu    r15,4(r31)
        lwzu    r16,4(r31)
        lwzu    r17,4(r31)
        lwzu    r18,4(r31)
        lwzu    r19,4(r31)
        lwzu    r20,4(r31)
        lwzu    r21,4(r31)
        lwzu    r22,4(r31)
        lwzu    r23,4(r31)
        lwzu    r24,4(r31)
        lwzu    r25,4(r31)
        lwzu    r26,4(r31)
        lwzu    r27,4(r31)
        lwzu    r28,4(r31)
        lwzu    r29,4(r31)
        lwzu    r30,4(r31)
        lwzu    r0,4(r31)

        lfdu    f0,4(r31)
        lfdu    f1,8(r31)
        lfdu    f2,8(r31)
        lfdu    f3,8(r31)
        lfdu    f4,8(r31)
        lfdu    f5,8(r31)
        lfdu    f6,8(r31)
        lfdu    f7,8(r31)
        lfdu    f8,8(r31)
        lfdu    f9,8(r31)
        lfdu    f10,8(r31)
        lfdu    f11,8(r31)
        lfdu    f12,8(r31)
        lfdu    f13,8(r31)
        lfdu    f14,8(r31)
        lfdu    f15,8(r31)
        lfdu    f16,8(r31)
        lfdu    f17,8(r31)
        lfdu    f18,8(r31)
        lfdu    f19,8(r31)
        lfdu    f20,8(r31)
        lfdu    f21,8(r31)
        lfdu    f22,8(r31)
        lfdu    f23,8(r31)
        lfdu    f24,8(r31)
        lfdu    f25,8(r31)
        lfdu    f26,8(r31)
        lfdu    f27,8(r31)
        lfdu    f28,8(r31)
        lfdu    f29,8(r31)
        lfdu    f30,8(r31)
        lfdu    f31,8(r31)

        lwzu    r3,8(r31)
        mtibatu 0,r3
        lwzu    r3,4(r31)
        mtibatl 0,r3
        lwzu    r3,4(r31)
        mtdbatu 0,r3
        lwzu    r3,4(r31)
        mtdbatl 0,r3
        lwzu    r3,4(r31)
        mtibatu 1,r3
        lwzu    r3,4(r31)
        mtibatl 1,r3
        lwzu    r3,4(r31)
        mtdbatu 1,r3
        lwzu    r3,4(r31)
        mtdbatl 1,r3
        lwzu    r3,4(r31)
        mtibatu 2,r3
        lwzu    r3,4(r31)
        mtibatl 2,r3
        lwzu    r3,4(r31)
        mtdbatu 2,r3
        lwzu    r3,4(r31)
        mtdbatl 2,r3
        lwzu    r3,4(r31)
        mtibatu 3,r3
        lwzu    r3,4(r31)
        mtibatl 3,r3
        lwzu    r3,4(r31)
        mtdbatu 3,r3
        lwzu    r3,4(r31)
        mtdbatl 3,r3

        lwzu    r3,4(r31)
        mtsr    0,r3
        lwzu    r3,4(r31)
        mtsr    1,r3
        lwzu    r3,4(r31)
        mtsr    2,r3
        lwzu    r3,4(r31)
        mtsr    3,r3
        lwzu    r3,4(r31)
        mtsr    4,r3
        lwzu    r3,4(r31)
        mtsr    5,r3
        lwzu    r3,4(r31)
        mtsr    6,r3
        lwzu    r3,4(r31)
        mtsr    7,r3
        lwzu    r3,4(r31)
        mtsr    8,r3
        lwzu    r3,4(r31)
        mtsr    9,r3
        lwzu    r3,4(r31)
        mtsr    10,r3
        lwzu    r3,4(r31)
        mtsr    11,r3
        lwzu    r3,4(r31)
        mtsr    12,r3
        lwzu    r3,4(r31)
        mtsr    13,r3
        lwzu    r3,4(r31)
        mtsr    14,r3
        lwzu    r3,4(r31)
        mtsr    15,r3

        mr      r31,r0
        blr

#********************************************************************************************
#
#
#
#********************************************************************************************

_SmallExcHandler:

        stwu    r1,-512(r1)                  #Enough?

        la      r31,IF_CONTEXT(r4)
        mtsprg0 r4
        mtsprg3 r3
        mflr    r3
        stw     r3,-12(r4)                   #Store this function's lr

        bl      _LoadFrame                   #LR, r0, r1 and r3 are skipped in this routine and are loaded below

        mfsprg0 r4
        lwz     r3,IF_CONTEXT_GPR+GPR3(r4)   #GPR[3]
        stw     r3,300(r1)
        mfsprg3 r3
        lwz     r0,IF_CONTEXT_LR(r4)         #EXC_LR
        mtsprg1 r0
        stw     r0,-8(r4)                    #Will be used later in StoreFrame
        lwz     r0,IF_CONTEXT_GPR+GPR1(r4)   #GPR[1]
        mtsprg2 r0
        stw     r2,-16(r4)
        lwz     r0,IF_CONTEXT_GPR+GPR2(r4)   #GPR[2]
        lwz     r2,18(r3)                    #DATA
        mtsprg3 r0
        lwz     r0,14(r3)                    #CODE
        mtlr    r0
        la      r3,300(r1)                   #XCONTEXT
        stw     r4,308(r1)
        lwz     r0,IF_CONTEXT_GPR+GPR0(r4)   #GPR[0]
        lwz     r4,IF_CONTEXT_GPR+GPR4(r4)   #GPR[4]

        blrl

        stw     r3,304(r1)                   #Status
        lwz     r3,300(r1)
        stw     r4,312(r1)                   #Temp store r4
        mfsprg0 r4
        stw     r3,IF_CONTEXT_GPR+GPR3(r4)   #GPR[3]
        stw     r0,IF_CONTEXT_GPR+GPR0(r4)   #GPR[0]

        bl      _StoreFrame

        lwz     r4,312(r1)                   #Really store r4
        mfsprg0 r5
        stw     r4,IF_CONTEXT_GPR+GPR4(r5)   #GPR[4]
        mfsprg1 r6
        lwz     r4,-12(r5)                   #Retrieve this function's lr
        lwz     r2,-16(r5)
        stw     r6,IF_CONTEXT_LR(r5)         #EXC_LR
        mfsprg2 r7
        mtlr    r4
        mfsprg3 r6
        stw     r7,IF_CONTEXT_GPR+GPR1(r5)   #GPR[1]
        lwz     r3,304(r1)                   #Return Status
        stw     r6,IF_CONTEXT_GPR+GPR2(r5)   #GPR[2]
        lwz     r1,0(r1)

        blr

#********************************************************************************************
#
#
#
#********************************************************************************************

_DoAlign:

        lwz     r5,0(r4)
        la      r11,IF_CONTEXT_GPR(r3)       #reg table
        la      r12,IF_CONTEXT_FPR(r3)       #freg table

        rlwinm  r6,r5,14,24,28               #get floating point register offset
        rlwinm  r7,r5,18,25,29               #get destination register offset
        mr.     r10,r7
        beq     .ItsR0
        lwzx    r10,r11,r7                   #get address from destination register

.ItsR0:	
        rlwinm  r8,r5,0,16,31                #get displacement
        rlwinm  r0,r5,6,26,31
        cmpwi   r0,0x34                      #test for stfs
        beq     .stfs
        cmpwi   r0,0x35
        beq     .stfsu
        cmpwi   r0,0x30
        beq     .lfs
        cmpwi   r0,0x31
        beq     .lfsu
        cmpwi   r0,0x1f
        beq     .lstfsx
        cmpwi   r0,0x32
        beq     .lfd
        cmpwi   r0,0x36
        beq     .stfd
        b       .HaltAlign

.stfd:		
        lwzx    r9,r12,r6
        stwx    r9,r10,r8
        addi    r6,r6,4
        addi    r8,r8,4
        lwzx    r9,r12,r6
        stwx    r9,r10,r8
        b       .AligExit

.stfsu:		
        add     r9,r10,r8
        stwx    r9,r11,r7

.stfs:		
        lfdx    f1,r12,r6                    #get value from fp register
        stfs    f1,IF_ALIGNSTORE(r3)         #store it on correct aligned spot
        lwz     r6,IF_ALIGNSTORE(r3)         #Get the correct 32 bit value
        stwx    r6,r10,r8                    #Store correct value
        b       .AligExit

.lfd:
        lwzx    r9,r10,r8
        stw     r9,IF_ALIGNSTORE(r3)
        addi    r8,r8,4
        lwzx    r9,r10,r8
        stw     r9,IF_ALIGNSTORE(r3)
        lfd     f1,IF_ALIGNSTORE(r3)
        stfdx   f1,r12,r6
        b       .AligExit

.lfsu:	
        add     r5,r10,r8                    #Add displacement
        stwx    r5,r11,r7

.lfs:	
        lwzx    r9,r10,r8                    #Get 32 bit value
        stw     r9,IF_ALIGNSTORE(r3)         #Store it on aligned spot
        lfs     f1,IF_ALIGNSTORE(r3)         #Get it and convert it to 64 bit
        stfdx   f1,r12,r6                    #Store the 64 bit value
        b       .AligExit

.lstfsx:
        rlwinm. r0,r5,25,31,31               #0 = s; 1 = d
        bne     .HaltAlign
        rlwinm. r0,r5,26,31,31               #0 = x; 1 = ux
        bne     .HaltAlign

        rlwinm  r8,r5,23,25,29               #get index register
        lwzx    r8,r11,r8                    #get index register value
        rlwinm. r0,r5,24,31,31
        bne     .stfs
        b       .lfs

.HaltAlign:
        li      r3,0
        blr
.AligExit:
        li      r3,1
        blr

#********************************************************************************************
#
#
#
#********************************************************************************************

_DoDataStore:

        li      r12,0                        #r3 = iframe, r4 = srr0, r5 = datastruct
        stw     r12,IF_ALIGNSTORE(r3)        #to get an offset of 0 (reusing an unused variable)
        la      r11,IF_CONTEXT_GPR(r3)       #get reg table in r11
        lwz     r12,0(r4)                    #get offending instruction in r12
        li      r10,0
        lis     r0,0xc000                    #check for load or store instruction
        mr      r7,r5
        and.    r0,r12,r0
        lis     r6,0x8000
        cmpw    r6,r0
        beq     .LoadStore

        rlwinm  r6,r12,0,25,5
        loadreg r8,0x7c00002e                #check for stbx/sthx/stwx/lbzx/lhzx/lwzx
        cmpw    r6,r8
        bne     .NotSupported

        li      r10,1
        rlwinm  r6,r12,13,25,29              #Source reg
        rlwinm  r8,r12,18,25,29              #Dest 1
        rlwinm  r4,r12,23,25,29              #Dest 2
        lwzx    r4,r11,r4                    #Displacement (word)
        li      r5,0                         #Update bit
        li      r9,1                         #Mark as Store instruction

        b       .GoStore

.LoadStore:	
        rlwinm  r6,r12,13,25,29              #Get Destination Reg (l) or Source (s)
        rlwinm  r8,r12,18,25,29              #Get Source Reg (l) or Destination (s)
        rlwinm  r9,r12,4,31,31               #Check load or store
        rlwinm  r5,r12,6,31,31               #Check for update bit
        rlwinm  r4,r12,0,16,31               #Displacement (halfword)
        extsh   r4,r4                        #Extend sign of displacement

.GoStore:
        mr.     r5,r5
        bne     .NoZero                      #When update r0 = r0 and not 0
        mr.     r8,r8
        bne     .NoZero
        li      r8,IF_ALIGNSTORE-IF_CONTEXT_GPR             #Point to 0 in frame

.NoZero:
        lwzx    r3,r11,r8                    #Get Destination Address
        add     r3,r3,r4                     #Add displacement
        mr.     r5,r5
        beq     .NoUpdate
        stwx    r3,r11,r8                    #Update Destination Reg

.NoUpdate:
        lwzx    r5,r11,r6                    #Get value to store
        mr.     r9,r9
        beq     .GoLoad                      #Load or store?

        mr.     r10,r10                      #Normal store or with index?
        beq     .NoStxx

        rlwinm  r0,r12,25,29,31              #Indexed store
        cmpwi   r0,6
        beq     .StoreHalf
        cmpwi   r0,3
        beq     .StoreByte
        cmpwi   r0,2
        beq     .StoreWord
        mr.     r0,r0
        beq     .GoLoadx                     #Lwzx
        cmpwi   r0,1
        beq     .GoLoadx                     #lbzx
        cmpwi   r0,4
        beq     .GoLoadx                     #lhzx
        cmpwi   r0,5
        beq     .GoLoadx                     #lhax
        b       .NotSupported                #Not Supported

.NoStxx:
        rlwinm. r0,r12,3,31,31               #Normal store
        bne     .StoreHalf
        rlwinm. r0,r12,5,31,31
        bne     .StoreByte

.StoreWord:	
        loadreg r9,'PUTW'
        b       .DoStore

.StoreHalf:	
        loadreg r9,'PUTH'
        b       .DoStore

.StoreByte:	
        loadreg r9,'PUTB'

.DoStore:
        stw     r9,0(r7)
        stw     r5,4(r7)
        stw     r3,8(r7)
        li      r4,1
        stw     r4,20(r7)
        b       .DoneDSI                     #We're done

.GoLoad:
        li      r5,0
        mr      r0,r5
        b       .NoX

.GoLoadx:	
        li      r5,1
.NoX:   stw     r0,12(r7)
        stw     r6,16(r7)
        loadreg r9,'GETV'
        stw     r9,0(r7)
        stw     r3,8(r7)
        li      r4,0
        stw     r4,20(r7)
        stw     r5,24(r7)
        b      .DoneDSI

.NotSupported:
        li      r3,0
        blr

#********************************************************************************************

_FinDataStore:
        lwz     r12,0(r5)                    #r3 = value, r4 = iframe, r5 = srr0. r6 = data
        la      r11,IF_CONTEXT_GPR(r4)
        lwz     r9,24(r6)
        lwz     r4,12(r6)
        mr      r10,r3
        lwz     r6,16(r6)
        mr.     r9,r9
        beq     .Normal

        mr.     r4,r4
        beq     .FixedValue                  #Word
        rlwinm  r10,r10,16,16,31
        cmpwi   r4,4
        beq     .FixedValue                  #halfword
        extsh   r10,r10
        cmpwi   r4,5                         #halfword algebraic
        beq     .FixedValue
        rlwinm  r10,r10,24,24,31
        b       .FixedValue                  #byte

.Normal:
        rlwinm  r9,r12,16,16,31
        andi.   r9,r9,0xa800
        cmplwi  r9,0x8000                    #lwz/lwzu 0x8000
        beq     .FixedValue
        rlwinm  r10,r10,16,16,31
        cmplwi  r9,0xa000                    #lhz/lhzu 0xa000
        beq     .FixedValue
        extsh   r10,r10
        cmplwi  r9,0xa800                    #lha/lhau 0xa800
        beq     .FixedValue
        rlwinm  r10,r10,24,24,31
        cmplwi  r9,0x8800                    #lbz/lbzu 0x8800
        bne     .NotSupported                #Not Supported

.FixedValue:
    	stwx    r10,r11,r6                   #Store gotten value in correct register

.DoneDSI:
        li      r3,1
        blr

#********************************************************************************************
#
#
#
#********************************************************************************************

ExcITLBMiss:
		mfspr	r2,HASH1                #get first pointer
		li      r1,8                    #load 8 for counter
		mfctr	r0                      #save counter
		mfspr	r3,ICMP                 #get first compare value
		addi	r2,r2,-8                #pre dec the pointer
im0:
		mtctr	r1                      #load counter
im1:
		lwzu	r1,8(r2)                #get next pte
		cmpw	r1,r3                   #see if found pte
		bdnzf	eq,im1                  #dec count br if cmp ne and if count not zero
		bne     instrSecHash            #if not found set up second hash or exit

		lwz     r1,4(r2)                #load tlb entry lower-word
		andi.	r3,r1,PTE_GUARDED<<3    #check G bit
		bne     doISIp                  #if guarded, take an ISI

		mtctr	r0                      #restore counter
		mfspr	r0,IMISS                #get the miss address for the tlbl
		mfsrr1	r3                      #get the saved cr0 bits
		mtcrf	0x80,r3                 #restore CR0
		mtspr	RPA,r1                  #set the pte
		ori     r1,r1,PTE_REFERENCED    #set reference bit
		srwi	r1,r1,8                 #get byte 7 of pte
		tlbli	r0                      #load the itlb
		stb     r1,6(r2)                #update page table
		rfi                             #return to executing program

instrSecHash:
		andi.	r1,r3,PTE_HASHID        #see if we have done second hash
		bne     doISI                   #if so, go to ISI interrupt

		mfspr	r2,HASH2                #get the second pointer
		ori     r3,r3,PTE_HASHID        #change the compare value
		li      r1,8                    #load 8 for counter
		addi	r2,r2,-8                #pre dec for update on load
		b       im0                     #try second hash

doISIp:
		mfsrr1	r3                      #get srr1
		andi.	r2,r3,0xffff            #clean upper srr1
		addis	r2,r2,DSISR_PROTECT@h   #or in srr<4> = 1 to flag prot violation
		b       isi1
doISI:
 		mfsrr1	r3                      #get srr1
 		andi.	r2,r3,0xffff            #clean srr1
		addis	r2,r2,DSISR_NOTFOUND@h  #or in srr1<1> = 1 to flag not found
isi1:
		mtctr	r0                      #restore counter
		mtsrr1	r2                      #set srr1
		mfmsr	r0                      #get msr
		xoris	r0,r0,PSL_TGPR@h        #flip the msr<tgpr> bit
		mtcrf	0x80,r3                 #restore CR0
		mtmsr	r0                      #flip back to the native gprs
        isync
		ba      0x400                   #go to instr. access interrupt

#********************************************************************************************
#
#
#
#********************************************************************************************

ExcDLoadTLBMiss:
		mfspr	r2,HASH1                #get first pointer
		li      r1,8                    #load 8 for counter
		mfctr	r0                      #save counter
		mfspr	r3,DCMP                 #get first compare value
		addi	r2,r2,-8                #pre dec the pointer
dm0:
		mtctr	r1                      #load counter
dm1:
		lwzu	r1,8(r2)                #get next pte
		cmpw	r1,r3                   #see if found pte
		bdnzf	eq,dm1                  #dec count br if cmp ne and if count not zero
		bne     dataSecHash             #if not found set up second hash or exit

		lwz     r1,4(r2)                #load tlb entry lower-word
		mtctr	r0                      #restore counter
		mfspr	r0,DMISS                #get the miss address for the tlbld
		mfsrr1	r3                      #get the saved cr0 bits
		mtcrf	0x80,r3                 #restore CR0
		mtspr	RPA,r1                  #set the pte
		ori     r1,r1,PTE_REFERENCED    #set reference bit
		srwi	r1,r1,8                 #get byte 7 of pte
		tlbld	r0                      #load the dtlb
		stb     r1,6(r2)                #update page table
		rfi                             #return to executing program

dataSecHash:
 		andi.	r1,r3,PTE_HASHID        #see if we have done second hash
		bne     doDSI                   #if so, go to DSI interrupt

		mfspr	r2,HASH2                #get the second pointer
		ori     r3,r3,PTE_HASHID        #change the compare value
		li      r1,8                    #load 8 for counter
		addi	r2,r2,-8                #pre dec for update on load
		b       dm0                     #try second hash

#********************************************************************************************
#
#
#
#********************************************************************************************

ExcDStoreTLBMiss:
		mfspr	r2,HASH1                #get first pointer
		li      r1,8                    #load 8 for counter
		mfctr	r0                      #save counter
		mfspr	r3,DCMP                 #get first compare value
		addi	r2,r2,-8                #pre dec the pointer
ceq0:
		mtctr	r1                      #load counter
ceq1:
		lwzu	r1,8(r2)                #get next pte
		cmpw	r1,r3                   #see if found pte
		bdnzf	eq,ceq1                 #dec count br if cmp ne and if count not zero
		bne     cEq0SecHash             #if not found set up second hash or exit

		lwz     r1,4(r2)                #load tlb entry lower-word
		andi.	r3,r1,PTE_CHANGED       #check the C-bit
		beq     cEq0ChkProt             #if (C==0) go check protection modes
ceq2:
		mtctr	r0                      #restore counter
		mfspr	r0,DMISS                #get the miss address for the tlbld
		mfsrr1	r3                      #get the saved cr0 bits
		mtcrf	0x80,r3                 #restore CR0
		mtspr	RPA,r1                  #set the pte
		tlbld	r0                      #load the dtlb
		rfi                             #return to executing program

cEq0SecHash:
		andi.	r1,r3,PTE_HASHID        #see if we have done second hash
		bne     doDSI                   #if so, go to DSI interrupt

		mfspr	r2,HASH2                #get the second pointer
		ori     r3,r3,PTE_HASHID        #change the compare value
		li      r1,8                    #load 8 for counter
		addi	r2,r2,-8                #pre dec for update on load
		b       ceq0                    #try second hash

cEq0ChkProt:
		rlwinm.	r3,r1,30,0,1            #test PP
		bge-    chk0                    #if (PP == 00 or PP == 01) goto chk0:

		andi.	r3,r1,1                 #test PP[0]
		beq+	chk2                    #return if PP[0] == 0
		b       doDSIp                  #else DSIp

chk0:
  		mfsrr1	r3                      #get old msr
		andis.	r3,r3,SRR1_KEY@h        #test the KEY bit (SRR1-bit 12)
		beq     chk2                    #if (KEY==0) goto chk2:
		b       doDSIp                  #else DSIp
chk2:
   		ori     r1,r1,(PTE_REFERENCED|PTE_CHANGED)    #set reference and change bit
		sth     r1,6(r2)                #update page table
		b       ceq2                    #and back we go

doDSI:
 		mfsrr1	r3                      #get srr1
		rlwinm	r1,r3,9,6,6             #get srr1<flag> to bit 6
		addis	r1,r1,DSISR_NOTFOUND@h  #or in dsisr<1> = 1 to flag not found
		b       dsi1
doDSIp:
		mfsrr1	r3                      #get srr1
		rlwinm	r1,r3,9,6,6             #get srr1<flag> to bit 6
		addis	r1,r1,DSISR_PROTECT@h   #or in dsisr<4> = 1 to flag prot violation
dsi1:
  		mtctr	r0                      #restore counter
		andi.	r2,r3,0xffff            #clear upper bits of srr1
		mtsrr1	r2                      #set srr1
		mtdsisr	r1                      #load the dsisr
		mfspr	r1,DMISS                #get miss address
		rlwinm.	r2,r2,0,31,31           #test LE bit
		beq     dsi2                    #if little endian then:

		xori	r1,r1,0x07              #de-mung the data address
dsi2:
  		mtdar	r1                      #put in dar
		mfmsr	r0                      #get msr
		xoris	r0,r0,PSL_TGPR@h        #flip the msr<tgpr> bit
		mtcrf	0x80,r3                 #restore CR0
		mtmsr	r0                      #flip back to the native gprs
		isync
		sync
		sync
		ba      0x300                   #branch to DSI interrupt

#********************************************************************************************
#
#
#
#********************************************************************************************

_FlushICache:
        b       .Mojo1					#Some L1 mojo

.Mojo2: mfspr   r0,HID0
        ori     r0,r0,HID0_ICFI
        mtspr   HID0,r0
        xori    r0,r0,HID0_ICFI
        mtspr   HID0,r0
        sync
        b       .Mojo3

.Mojo1: b       .Mojo2

.Mojo3: blr

#********************************************************************************************
