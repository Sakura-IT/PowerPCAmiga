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

#include "constants.h"
#include "libstructs.h"
#include "Internalsppc.h"
#include <proto/powerpc.h>
#include <powerpc/powerpc.h>

/********************************************************************************************
*
*	Entry point after kernel.s. This directs to the correct exception code.
*
*********************************************************************************************/

PPCKERNEL void Exception_Entry(struct PrivatePPCBase* PowerPCBase, int vector, struct iframe* iframe)
{
    switch (vector)
    {
        case VEC_EXTERNAL:
        {
            PreparePPC(PowerPCBase, iframe);
            SwitchPPC(PowerPCBase, iframe);
            break;
        }
        case VEC_DECREMENTER:
        {
            SwitchPPC(PowerPCBase, iframe);
            break;
        }
        case VEC_DATASTORAGE:
        {
            break;
        }
        case VEC_PROGRAM:
        {
            break;
        }
        case VEC_ALIGNMENT:
        {
            break;
        }
        case VEC_ALTIVECUNAV:
        {
            break;
        }
        default:
        {
            break;
        }
    }
    return;
}

/********************************************************************************************
*
*	This function picks up messages from the 68K and prepares them for the SwitchPPC function
*
*********************************************************************************************/

PPCKERNEL void PreparePPC(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe)
{
    return;
}

/********************************************************************************************
*
*	This function dispatches new PPC tasks, switches between tasks and redirect signals.
*
*********************************************************************************************/

PPCKERNEL void SwitchPPC(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe)
{
    return;
}
