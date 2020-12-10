#ifndef COGNATE_C
#define COGNATE_C

#include "cognate.h"
#include "stack.c"
#include "io.c"
#include "error.c"
#include "type.c"
#include "func.c"
#include <time.h>
#include <sys/resource.h>
#include <libgen.h>
#include <Block.h>
#include <locale.h>

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);

static const char *stack_start;
static struct rlimit stack_max;

static void get_params(int argc, char** argv)
{
  params.start = (cognate_object*) cognate_malloc (sizeof(cognate_object) * (argc-1));
  params.top = params.start + argc - 1;
  while (--argc >= 1)
  {
    char* str = argv[argc];
    params.start[argc-1] = (cognate_object){.type=string, .string=str};
  }
}

static void init(int argc, char** argv)
{
  // Get return stack limit
  char a;
  stack_start = &a;
  if (getrlimit(RLIMIT_STACK, &stack_max) == -1)
  {
    throw_error("Cannot get return stack limit!");
  }
  // Set locale for strings.
  if (setlocale(LC_ALL, "") == NULL)
  {
    throw_error("Cannot set locale!");
  }
  // Get executable path stuff.
#ifdef __APPLE__ // '/proc/self/exe' doesn't exist on macos
  short bufsize = PATH_MAX;
  if (_NSGetExecutablePath(file_name_buf, &bufsize) == -1);
#else
  if (readlink("/proc/self/exe", file_name_buf, PATH_MAX) == -1)
#endif
  {
    throw_error("Cannot get executable directory!");
  }
  // Need to use strdup here, since basename and dirname modify their arguments.
  exe_path = strdup(file_name_buf);
  exe_name = basename(strdup(file_name_buf));
  exe_dir  = dirname(file_name_buf);
  // Seed the random number generator properly.
  struct timespec ts;
  if (timespec_get(&ts, TIME_UTC) == 0)
  {
    throw_error("Cannot get system time!");
  }
  srand(ts.tv_nsec ^ ts.tv_sec); // TODO make random more random.
  // Init GC
#ifndef noGC
  GC_INIT();
#endif
  // Generate a stack.
  init_stack();
  get_params(argc, argv);
}

static void cleanup()
{
  if (unlikely(stack.items.top != stack.items.start))
  {
    throw_error_fmt("Program exiting with non-empty stack of length %ti", stack.items.top - stack.items.start);
  }
}

static cognate_object check_block(cognate_object obj)
{
  (obj.type==block) && (obj.block = Block_copy(obj.block));
  return obj;
}

static void copy_blocks()
{
  for (; stack.modified != stack.items.top; stack.modified++)
  {
    (stack.modified->type==block) && (stack.modified->block = Block_copy(stack.modified->block)); // Copy block to heap.
  }
}

static void check_call_stack()
{
  // Performance here is not great.
  static unsigned short calls = 1024;
  if (unlikely(!--calls))
  {
    calls = 1024;
    static long old_stack_size;
    char b;
    // if (how much stack left < stack change between checks)
    if (unlikely(stack_max.rlim_cur + &b - stack_start < stack_start - &b - old_stack_size))
    {
      throw_error_fmt("Call stack overflow - too much recursion! (call stack is %ti bytes)", stack_start - &b);
    }
    old_stack_size = stack_start - &b;
  }
}

#endif
