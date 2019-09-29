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

#define HW_CPUTYPE		        11	//Private
#define HW_SETDEBUGMODE		    12	//Private
#define HW_PPCSTATE		        13	//Private

#define MEMF_PPC                0x2000
#define MEM_GAP                 0x480000
#define TF_PPC			        1<<2
#define VENDOR_ELBOX            0x89e
#define MEDIATOR_MKII           33
#define MEDIATOR_1200TX         60
#define MEDIATOR_LOGIC          161
#define MEDIATOR_1200LOGIC      188

#define MAX_PCI_SLOTS           6
#define PCI_OFFSET_ID           0
#define PCI_OFFSET_COMMAND      4
#define DEVICENUMBER_SHIFT      3
#define BUSNUMBER_SHIFT         8
#define BUSMASTER_ENABLE        1<<2

#define DEVICE_MPC107           0x0004
#define VENDOR_MOTOROLA         0x1057
#define DEVICE_HARRIER          0x480B
#define DEVICE_MPC8343E         0x0086
#define VENDOR_FREESCALE        0x1957

#define VENDOR_ATI              0x1002
#define VENDOR_3DFX             0x121a
#define DEVICE_VOODOO45         0x0009
#define DEVICE_VOODOO3          0x0005
#define DEVICE_RV280PRO         0x5960
#define DEVICE_RV280            0x5961
#define DEVICE_RV280_2          0x5962
#define DEVICE_RV280MOB         0x5C63
#define DEVICE_RV280SE          0x5964

#define OFFSET_KERNEL           0x3000
#define OFFSET_ZEROPAGE         0x10000

#define IMMR_ADDR_DEFAULT       0xFF400000
#define IMMR_IMMRBAR            0x0

#define IMMR_RSR                0x910
#define IMMR_RPR                0x918
#define IMMR_RCR                0x91C
#define IMMR_RCER               0x920

#define IMMR_PCILAWBAR0         0x60
#define IMMR_PCILAWAR0          0x64
#define IMMR_PCILAWBAR1         0x68
#define IMMR_PCILAWAR1          0x6C

#define IMMR_PITAR0             0x8568
#define IMMR_PIBAR0             0x8570
#define IMMR_PIWAR0             0x8578

#define IMMR_POTAR0             0x8400
#define IMMR_POTAR1             0x8418
#define IMMR_POTAR2             0x8430
#define IMMR_POBAR0             0x8408
#define IMMR_POBAR1             0x8420
#define IMMR_POBAR2             0x8438

#define IMMR_POCMR0             0x8410
#define IMMR_POCMR1             0x8428
#define IMMR_POCMR2             0x8440
#define IMMR_POCMR3             0x8458
#define IMMR_POCMR4             0x8470
#define IMMR_POCMR5             0x8488

#define PIWAR_EN                0x80000000
#define PIWAR_PF                0x20000000
#define PIWAR_RTT_SNOOP         0x00060000
#define PIWAR_WTT_SNOOP         0x00006000
#define PIWAR_IWS_64MB          0x00000019

#define POCMR_EN                0x80000000
#define POCMR_CM_256MB          0x000F0000
#define POCMR_CM_128MB          0x000F8000
#define POCMR_CM_64MB           0x000FC000
#define POCMR_CM_64KB           0x000FFFF0

#define LAWAR_EN                0x80000000
#define LAWAR_64MB              0x00000019
#define LAWAR_128MB             0x0000001a
#define LAWAR_256MB             0x0000001b
#define LAWAR_512MB             0x0000001c

#define IMMR_SIMSR_L            0x724
#define SIMSR_L_MU              0x04000000

