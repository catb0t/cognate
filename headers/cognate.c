#ifndef COGNATE_C
#define COGNATE_C

// Macro to define internal cognate function.
// __block attribute allows recursion at performance cost.
#define cognate_define(name, body) \
  const __block void(^ cognate_func_ ## name)(void) = ^body

#define cognate_redefine(name, body) \
  cognate_func_ ## name = ^body

// Macro for defining internal cognate variables.
// __block attribute allows mutation at performance cost.
#define cognate_let(name) \
  const cognate_object cognate_variable_ ## name = pop_object(); \
  const __block void(^ cognate_func_ ## name)(void) = ^{push_object(cognate_variable_ ## name);};

#define cognate_set(name) \
  const cognate_object cognate_variable_ ## name = pop_object(); \
  cognate_func_ ## name = ^{push_object(cognate_variable_ ## name);};

// This macro attempts to prevent unnecessary use of the return stack. It should only be used at the end of a block.
// Keep for later.
#define attempt_tco(name) \
  if ( &&lbl_call_##name == __builtin_return_address(0) ) \
  { \
    goto lbl_def_##name; \
  } \
  else { \
    cognate_func_##name (); \
    lbl_call_##name:;\
  }

#define tco_call(name) \
  goto lbl_def_##name;

/*
#define MAX_RECURSION_DEPTH 1048576

static void init_recursion_depth_check();
static void check_recursion_depth();
*/

#include "debug.c"
#include <time.h>
#include "stack.c"
#include "func.c"
#include "io.c"
#include "error.c"
#include "type.c"

static void init()
{
  // Mark top of RETURN stack (for recursion depth checking).
  //init_recursion_depth_check();

  // Seed the random number generator properly.
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  srand(ts.tv_nsec ^ ts.tv_sec);
  // Generate a stack.
  init_stack();
}

/*
static char* return_stack_start;
static void check_recursion_depth()
{
  // Gets the size of the return stack (not cognate's stack!).
  // Can be used to 'catch' stack overflow errors.
  char var; // Address of local var is top of return stack!!!
  int return_stack_size = (&var) - return_stack_start;
  if (return_stack_size > MAX_RECURSION_DEPTH || return_stack_size < -MAX_RECURSION_DEPTH)
  {
    throw_error("Maximum recursion depth exceeded!");
  }
}
static void init_recursion_depth_check()
{
  char var;
  return_stack_start = &var;
}
*/

#endif
