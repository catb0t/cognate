#ifndef STACK_C
#define STACK_C

#include "cognate.h"
#include "types.h"

static void init_stack();
static void push(const cognate_object);
static cognate_object pop();
static cognate_object peek();
static void expand_stack();

static cognate_stack stack;

#include "error.c"

#include <stdio.h>
#include <stdlib.h>
#ifndef noGC
#include <gc/gc.h>
#endif

static void init_stack()
{
  stack.uncopied_blocks = 0;
  stack.size = INITIAL_LIST_SIZE;
  stack.top = stack.start =
    (cognate_object*) GC_MALLOC (INITIAL_LIST_SIZE * sizeof(cognate_object));
}

static void push(cognate_object object)
{
  // Profiles says that this function is The Problem.
  // builtin_expect optimises because the stack hardly ever needs to expand.
  if unlikely(stack.start + stack.size == stack.top)
    expand_stack();
  stack.uncopied_blocks += (object.type == block);
  *stack.top++ = object;
}

static cognate_object pop()
{
  if unlikely(stack.top == stack.start)
    throw_error("Stack underflow!");
  const cognate_object object = *--stack.top;
  stack.uncopied_blocks -= (object.type == block);
  return object;
}

static cognate_object peek()
{
  if unlikely(stack.top == stack.start)
    throw_error("Stack underflow!");
  return *(stack.top - 1);
}

static void expand_stack()
{
  // New stack size = current stack size * growth factor.
  // Assumes that stack is currently of length stack.size.
  stack.start = (cognate_object*) GC_REALLOC (stack.start, stack.size * LIST_GROWTH_FACTOR * sizeof(cognate_object));
  stack.top = stack.start + stack.size;
  stack.size *= LIST_GROWTH_FACTOR;
}

#endif
