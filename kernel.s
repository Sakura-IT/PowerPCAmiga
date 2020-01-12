## Copyright (c) 2019 Dennis van der Boon
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

.global     _Exception_Entry, _SmallExcHandler, _DoAlign

.section "kernel","acrx"

#********************************************************************************************

_ExcCommon:

        b       ExcITLBMiss
        b       ExcDLoadTLBMiss
        b       ExcDStoreTLBMiss

        mtsprg2 r0                           #LR; sprg3 = r0
		
        mfmsr   r0
        ori     r0,r0,(PSL_IR|PSL_DR|PSL_FP)
        mtmsr	r0				             #Reenable MMU
        isync					             #Also reenable FPU
        sync

        mtsprg0 r3

        stwu	r1,-2048(r1)

        la      r3,804(r1)                   #256 nothing 512 altivec 40 EXC header 128 GPR 256 FPR

        mfsprg3 r0
        stwu    r0,4(r3)
        lwz     r0,0(r1)
        stwu    r0,4(r3)                     #r1
        stwu    r2,4(r3)
        mfsprg0 r0
        stwu    r0,4(r3)                     #r3
        stwu    r4,4(r3)
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

        mflr    r4
        subi    r4,r4,PPC_VECLEN*4
        stw     r4,8(r3)

        la      r3,764(r1)

        rlwinm  r0,r4,24,24,31
        stwu    r0,4(r3)                     #ec_ExcID; does not work for Altivec exception
        mfsrr0  r0
        stwu    r0,4(r3)
        mfsrr1  r0
        stwu    r0,4(r3)
        mfdar   r0
        stwu    r0,4(r3)
        mfdsisr r0
        stwu    r0,4(r3)
        mfcr    r0
        stwu    r0,4(r3)
        mfctr   r0
        stwu    r0,4(r3)
        mfsprg2 r0
        stwu    r0,4(r3)                     #LR
        mfxer   r0
        stwu    r0,4(r3)
        mffs    f0
        stfsu   f0,4(r3)

        lwz     r3,PowerPCBase(r0)           #Loads PowerPCBase
        la      r4,256(r1)
        mtsprg0 r4

        bl _Exception_Entry

        mfsprg0 r31
        addi    r31,r31,512

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
        lwzu    r3,4(r31)
        mtlr    r3
        lwzu    r3,4(r31)
        mtxer   r3
        lfsu    f0,4(r31)
        mtfsf   0xff,f0

        lwzu    r0,4(r31)
        lwzu    r1,4(r31)
        lwzu    r2,4(r31)
        lwzu    r3,4(r31)
        lwzu    r4,4(r31)
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

        mtsprg2 r30
        lwzu    r30,4(r31)

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

        mr      r31,r30
        mfsprg2 r30

        rfi

#********************************************************************************************

_SmallExcHandler:

        stwu    r1,-256(r1)

        lwz     r0,516(r4)
        mtcr    r0
        lwz     r0,520(r4)
        mtctr   r0
        lwz     r0,524(r4)
        mtsprg1 r0
        lwz     r0,528(r4)
        mtxer   r0

        la      r12,548(r4)
        stw     r4,208(r1)

        lwz     r0,14(r3)               #code
        mtlr    r0
        lwz     r2,18(r3)               #data
        la      r3,200(r1)              #XCONTEXT

        lwzu    r0,4(r12)               #volatile regs
        lwzu    r4,4(r12)
        lwzu    r5,4(r12)
        lwzu    r6,4(r12)
        mtsprg2 r4
        mtsprg3 r5
        stw     r6,0(r3)
        lwzu    r4,4(r12)
        lwzu    r5,4(r12)
        lwzu    r6,4(r12)
        lwzu    r7,4(r12)
        lwzu    r8,4(r12)
        lwzu    r9,4(r12)
        lwzu    r10,4(r12)
        lwzu    r11,4(r12)
        lwz     r12,4(r12)

        blrl

        stw     r3,204(r1)             #status
        stw     r4,212(r1)
        mfcr    r3
        lwz     r4,208(r1)             #iframe
        stw     r3,516(r4)
        mfctr   r3
        stw     r3,520(r4)
        mfsprg1 r3
        stw     r3,524(r4)             #lr
        mfxer   r3
        stw     r3,528(r4)

        la      r3,548(r4)
        stwu    r0,4(r3)
        mfsprg2 r0
        stwu    r0,4(r3)               #r1
        mfsprg3 r0
        stwu    r0,4(r3)               #r2
        lwz     r0,200(r1)
        stwu    r0,4(r3)               #r3
        lwz     r4,212(r1)
        stwu    r4,4(r3)
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

        lwz     r3,204(r1)
        lwz     r1,0(r1)

        blr

#********************************************************************************************

_DoAlign:

        lwz     r5,0(r4)
        la      r11,552(r3)            #reg table
        la      r12,680(r3)            #freg table

		rlwinm  r6,r5,14,24,28         #get floating point register offset
		rlwinm  r7,r5,18,25,29         #get destination register offset
		mr.     r10,r7
		beq     .ItsR0
		lwzx    r10,r11,r7             #get address from destination register

.ItsR0:	
    	rlwinm  r8,r5,0,16,31          #get displacement
		rlwinm  r0,r5,6,26,31
		cmpwi   r0,0x34                #test for stfs
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
        lfdx    f1,r12,r6              #get value from fp register
		stfs    f1,936(r3)             #store it on correct aligned spot
		lwz     r6,936(r3)             #Get the correct 32 bit value
		stwx    r6,r10,r8              #Store correct value
		b       .AligExit

.lfd:
        lwzx    r9,r10,r8
        stw	    r9,936(r3)
        addi    r8,r8,4
        lwzx    r9,r10,r8
        stw     r9,936(r3)
        lfd     f1,936(r3)
        stfdx   f1,r12,r6
        b       .AligExit

.lfsu:	
        add     r5,r10,r8              #Add displacement
        stwx    r5,r11,r7

.lfs:	
        lwzx    r9,r10,r8              #Get 32 bit value
        stw     r9,936(r3)             #Store it on aligned spot
        lfs     f1,936(r3)             #Get it and convert it to 64 bit
        stfdx   f1,r12,r6              #Store the 64 bit value
        b       .AligExit

.lstfsx:
        rlwinm. r0,r5,25,31,31         #0 = s; 1 = d
        bne     .HaltAlign
        rlwinm. r0,r5,26,31,31         #0 = x; 1 = ux
        bne     .HaltAlign

        rlwinm  r8,r5,23,25,29         #get index register
        lwzx    r8,r11,r8              #get index register value
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
		ba      0x400                   #go to instr. access interrupt

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
