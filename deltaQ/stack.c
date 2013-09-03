
/*
 * Copyright (c) 2006-2013 The StarBED Project  All rights reserved.
 *
 * See the file 'LICENSE' for licensing information.
 *
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
 * $Id: stack.c 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/

#include <stdio.h>

#include "stack.h"
#include "global.h"
#include "message.h"


/////////////////////////////////////////
// Stack functions
/////////////////////////////////////////

void
stack_init (struct stack_class *stack)
{
  stack->top = EMPTY_STACK;
}

int
stack_is_full (struct stack_class *stack)
{
  if (stack->top < MAX_ELEMENTS - 1)
    return FALSE;
  else
    return TRUE;
}

int
stack_is_empty (struct stack_class *stack)
{
  if (stack->top == EMPTY_STACK)
    return TRUE;
  else
    return FALSE;
}

int
stack_push (struct stack_class *stack, void *element, int element_type)
{
  if (stack_is_full (stack))
    {
      WARNING ("Cannot push element because stack is full");
      return ERROR;
    }

  // update top
  stack->top++;
  // add element to stack
  (stack->elements[stack->top]).element = element;
  (stack->elements[stack->top]).element_type = element_type;

  return SUCCESS;
}

struct element_class *
stack_pop (struct stack_class *stack)
{
  if (stack_is_empty (stack))
    {
      WARNING ("Cannot pop element because stack is empty");
      return NULL;
    }

  return &(stack->elements[stack->top--]);
}

struct element_class *
stack_top (struct stack_class *stack)
{
  if (stack_is_empty (stack))
    {
      WARNING ("Cannot return top element because stack is empty");
      return NULL;
    }

  return &(stack->elements[stack->top]);
}
