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

#include <exec/types.h>
#include "libstructs.h"
#include "constants.h"


void    Reset(void);
ULONG   ReadPVR(void);

__section (".setupppc","acrx") void setupPPC(struct InitData* initData)
{
    ULONG mem = 0;

    initData->id_Status = 0x496e6974;    //Init

    Reset();

    for (LONG i=0; i<64; i++)
    {
        *((ULONG*)(mem)) = 0;
        mem += 4;
    }

    ULONG myPVR = ReadPVR();

    if ((myPVR>>16) == ID_MPC834X)
    {
        initData->id_Status = 0xaaaaaaaa;
    }
    else
    {
        initData->id_Status = 0xbbbbbbbb;
    }

fakeEnd:
    goto fakeEnd;
}

