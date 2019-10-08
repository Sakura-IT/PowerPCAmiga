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
