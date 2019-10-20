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

void illegal(void) = "\t.long\t0\n";            //debug function


void    Reset  (void);
ULONG   ReadPVR(void);
ULONG   SetIdle(void);

ULONG getLeadZ(ULONG value)               = "\tcntlzw\tr3,r3\n";
ULONG getPVR(void)                        = "\tmfpvr\tr3\n";
ULONG getSDR1(void)                       = "\tmfsdr1\tr3\n";
void  setSDR1(ULONG value)                = "\tmtsdr1\tr3\n";
void  setSRIn(ULONG keyVal, ULONG segVal) = "\tmtsrin\tr3,r4\n";
ULONG getSRIn(ULONG address)              = "\tmfsrin\tr3,r3\n";
void  setMSR(ULONG value)                 = "\tsync\n\tmtmsr\tr3\n\tsync\n";
void  tlbSync(void)                       = "\ttlbsync\n";
void  tlbIe(ULONG value)                  = "\ttlbie\tr3\n";

void setBAT0(ULONG ibatl, ULONG ibatu, ULONG dbatl, ULONG dbatu)
        ="\tmtibatl\t0,r3\n\tmtibatu\t0,r4\n\tmtdbatl\t0,r5\n\tmtdbatu\t0,r6\n";
void setBAT1(ULONG ibatl, ULONG ibatu, ULONG dbatl, ULONG dbatu)
        ="\tmtibatl\t1,r3\n\tmtibatu\t1,r4\n\tmtdbatl\t1,r5\n\tmtdbatu\t1,r6\n";
void setBAT2(ULONG ibatl, ULONG ibatu, ULONG dbatl, ULONG dbatu)
        ="\tmtibatl\t2,r3\n\tmtibatu\t2,r4\n\tmtdbatl\t2,r5\n\tmtdbatu\t2,r6\n";
void setBAT3(ULONG ibatl, ULONG ibatu, ULONG dbatl, ULONG dbatu)
        ="\tmtibatl\t3,r3\n\tmtibatu\t3,r4\n\tmtdbatl\t3,r5\n\tmtdbatu\t3,r6\n";

void mSync(void)                          = "\tsync\n\tisync\n";
