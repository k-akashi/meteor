
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
 * File name: stack.h
 * Function:  Header file of stack.c 
 *
 * Author: Razvan Beuran
 *
 * $Id: stack.h 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/


#ifndef __STACK_H
#define __STACK_H


/////////////////////////////////////////
// Stack related constants
/////////////////////////////////////////

#define MAX_ELEMENTS       1000
#define EMPTY_STACK        -1


/////////////////////////////////////////
// Stack related structures
/////////////////////////////////////////

struct element_class
{
  void *element;
  int element_type;
};

struct stack_class
{
  struct element_class elements[MAX_ELEMENTS];
  int top;
};


/////////////////////////////////////////
// Stack functions
/////////////////////////////////////////

void stack_init (struct stack_class *stack);

int stack_is_full (struct stack_class *stack);

int stack_is_empty (struct stack_class *stack);

int stack_push (struct stack_class *stack, void *element, int element_type);

struct element_class *stack_pop (struct stack_class *stack);

struct element_class *stack_top (struct stack_class *stack);


#endif
