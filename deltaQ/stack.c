
/*
 * Copyright (c) 2006-2009 The StarBED Project  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/************************************************************************
 *
 * QOMET Emulator Implementation
 *
 * File name: stack.c
 * Function:  Stack structure implementation 
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/

#include <stdio.h>

#include "stack.h"
#include "global.h"
#include "message.h"


/////////////////////////////////////////
// Stack functions
/////////////////////////////////////////

void stack_init(stack_class *stack)
{
  stack->top = EMPTY_STACK;
}

int stack_is_full(stack_class *stack)
{
  if(stack->top<MAX_ELEMENTS-1)
    return FALSE;
  else
    return TRUE;
}

int stack_is_empty(stack_class *stack)
{
  if(stack->top==EMPTY_STACK)
    return TRUE;
  else
    return FALSE;
}

int stack_push(stack_class *stack, void *element, int element_type)
{
  if(stack_is_full(stack))
    {
      WARNING("Cannot push element because stack is full");
      return ERROR;
  }

  // update top
  stack->top++;
  // add element to stack
  (stack->elements[stack->top]).element = element;
  (stack->elements[stack->top]).element_type = element_type;

  return SUCCESS;
}

element_class *stack_pop(stack_class *stack)
{
  if(stack_is_empty(stack))
    {
      WARNING("Cannot pop element because stack is empty");
      return NULL;
  }

  return &(stack->elements[stack->top--]);
}

element_class *stack_top(stack_class *stack)
{
  if(stack_is_empty(stack))
    {
      WARNING("Cannot return top element because stack is empty");
      return NULL;
  }

  return &(stack->elements[stack->top]);
}
