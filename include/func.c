#ifndef FUNC_C
#define FUNC_C

#include "cognate.h"
#include "types.h"

static const cognate_list* params;

#define call(name, ...) \
  word_name = #name; \
  cognate_function_ ## name(__VA_ARGS__);

// I'm not putting type signatures for every single function here.

#include "type.c"
#include "stack.c"
#include "table.c"
#include "io.c"

#include <unistd.h>
#include <regex.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#ifndef noGC
#include <gc/gc.h>
#endif

static void cognate_function_if(cognate_object cond, cognate_object a, cognate_object b)
{
  check_type(block, cond);
  check_type(block, a);
  check_type(block, b);
  cond.block();
  if (pop(boolean))
  {
    push_any(a);
  }
  else
  {
    push_any(b);
  }
}

static void cognate_function_while() {
  cognate_block cond = pop(block);
  cognate_block body = pop(block);
  cond();
  while (pop(boolean))
  {
    body();
    cond();
  }
}

static void cognate_function_do(cognate_object blk) { check_type(block, blk).block(); }

static void cognate_function_put(cognate_object a)   { print_object(a, stdout, 0); fflush(stdout); }
static void cognate_function_print(cognate_object a) { print_object(a, stdout, 0); puts(""); }


static void cognate_function_sum(cognate_object a, cognate_object b)      { push(number, check_type(number, a).number + check_type(number, b).number); }
static void cognate_function_multiply(cognate_object a, cognate_object b) { push(number, check_type(number, a).number * check_type(number, b).number); }
static void cognate_function_divide(cognate_object a, cognate_object b)   { push(number, check_type(number, b).number / check_type(number, a).number); }
static void cognate_function_subtract(cognate_object a, cognate_object b) { push(number, check_type(number, b).number - check_type(number, a).number); }

//static void cognate_function_sum()      { push(number, pop(number) + pop(number)); }
//static void cognate_function_multiply() { push(number, pop(number) * pop(number)); }
//static void cognate_function_divide()   { push(number, (1 / pop(number) * pop(number))); }
//static void cognate_function_subtract() { push(number, (-pop(number) + pop(number))); }

static void cognate_function_modulo(cognate_object a, cognate_object b) {
  check_type(number, a);
  check_type(number, b);
  push(number, fmod(b.number, a.number));
}

static void cognate_function_random() {
  const double low  = pop(number);
  const double high = pop(number);
  const double step = pop(number);
  if unlikely((high - low) * step < 0 || !step)
  {
    throw_error("Cannot generate random number in range %.14g..%.14g with step %.14g", low, high, step);
  }
  else if ((high - low) / step < 1)
  {
    push(number, low);
    return;
  }
  // This is not cryptographically secure btw.
  // Since RAND_MAX may only be 2^15, we need to do this:
  const long num
    = ((long)(short)random())
    | ((long)(short)random() << 15)
    | ((long)(short)random() << 30)
    | ((long)(short)random() << 45)
    | ((long)       random() << 60);
  push(number, low + (double)(num % (unsigned long)((high - low) / step)) * step);
}

static void cognate_function_drop()    { pop_any(); } // These can be defined within cognate.
static void cognate_function_twin()    { push_any(peek_any()); }
static void cognate_function_triplet() { const cognate_object a = peek_any(); push_any(a); push_any(a); }
static void cognate_function_swap()    { const cognate_object a = pop_any(); const cognate_object b = pop_any(); push_any(a); push_any(b); }
static void cognate_function_clear()   { stack.top=stack.start; }

static void cognate_function_true()  { push(boolean, 1); }
static void cognate_function_false() { push(boolean, 0); }

static void cognate_function_either() { push(boolean, pop(boolean) + pop(boolean)); } // Use unconventional operators to avoid short-circuits.
static void cognate_function_both()   { push(boolean, pop(boolean) & pop(boolean)); }
static void cognate_function_one_of() { push(boolean, pop(boolean) ^ pop(boolean)); }
static void cognate_function_not(cognate_object a)    { push(boolean,!check_type(boolean, a).boolean); }


static void cognate_function_equal(cognate_object a, cognate_object b)      { push(boolean, check_type(number, a).number == check_type(number, b).number); }
static void cognate_function_unequal(cognate_object a, cognate_object b)      { push(boolean, check_type(number, a).number != check_type(number, b).number); }
static void cognate_function_exceed(cognate_object a, cognate_object b)      { push(boolean, check_type(number, a).number < check_type(number, b).number); }
static void cognate_function_preceed(cognate_object a, cognate_object b)      { push(boolean, check_type(number, a).number > check_type(number, b).number); }
static void cognate_function_equalorpreceed(cognate_object a, cognate_object b)      { push(boolean, check_type(number, a).number >= check_type(number, b).number); }
static void cognate_function_equalorexceed(cognate_object a, cognate_object b)      { push(boolean, check_type(number, a).number <= check_type(number, b).number); }

static void cognate_function_number_()  { push(boolean, pop_any().type & number);  } // Question marks are converted to underscores.
static void cognate_function_list_()    { push(boolean, pop_any().type & list);    } // However all other symbols are too.
static void cognate_function_string_()  { push(boolean, pop_any().type & string);  } // So this is a temporary hack!
static void cognate_function_block_()   { push(boolean, pop_any().type & block);   }
static void cognate_function_boolean_() { push(boolean, pop_any().type & boolean); }

static void cognate_function_head(cognate_object a) {
  // Returns a list's first element. O(1).
  const cognate_list* lst = check_type(list, a).list;
  if unlikely(!lst) throw_error("Cannot return the First element of an empty list!");
  push_any(lst->object);
}

static void cognate_function_tail(cognate_object a) {
  // Returns the tail portion of a list. O(1).
  const cognate_list* lst = check_type(list, a).list;
  if unlikely(!lst) throw_error("Cannot return the Tail elements of an empty list!");
  push(list, lst->next);
}

static void cognate_function_push() {
  // Pushes an object from the stack onto the list's first element. O(1).
  // TODO: Better name? Inconsistent with List where pushing to the stack adds to the END.
  cognate_list* lst = GC_NEW (cognate_list);
  lst->object = pop_any();
  lst->next   = pop(list);
  push(list, lst);
}

static void cognate_function_empty_() {
  // Returns true is a list is empty. O(1).
  // Can be used to to write a Length function.
  push(boolean, !pop(list));
}

static void cognate_function_list(cognate_object a) {
  // Get the block argument
  const cognate_block expr = check_type(block, a).block;
  // Move the stack to temporary storage
  const cognate_stack temp_stack = stack;
  // Allocate a list as the stack
  init_stack();
  // Eval expr
  expr();
  // Move to a list.
  const cognate_list* lst = NULL;
  while (stack.top != stack.start)
  {
    // TODO: Allocate the list as an array for the sake of memory locality.
    // This can just be Swap, Push;
    cognate_list* tmp = GC_NEW (cognate_list);
    tmp -> object = pop_any();
    tmp -> next = lst;
    lst = tmp;
  }
  // Restore the stack.
  stack = temp_stack;
  push(list, lst);
}

static void cognate_function_characters() {
  // Can be rewritten using substring, albeit O(n^2)
  /*
  const char* str = pop(string);
  const cognate_list* lst = NULL;
  for (size_t i = 0; *str ; ++i)
  {
    size_t char_len = mblen(str, MB_CUR_MAX);
    cognate_list* tmp = GC_NEW (cognate_list);
    tmp->object = (cognate_object) {.type=string, .string=GC_STRNDUP(str, char_len)};
    tmp->next = lst;
    lst = tmp;
    str += char_len;
  }
  push(list, lst);
  */ // TODO: Currently returning list backwards.
}

static void cognate_function_split() {
  // Can be rewritten using Substring.
  /*
  const char* const delimiter = pop(string);
  const size_t delim_size     = strlen(delimiter);
  if unlikely(!delim_size) throw_error("Cannot Split a string with a zero-length delimiter!");
  const char* str = pop(string);
  size_t length = 1;
    for (const char* temp = str; (temp = strstr(temp, delimiter) + delim_size) - delim_size; ++length);
  cognate_list* const lst = GC_NEW(cognate_list);
  lst->top = lst->start   = (cognate_object*) GC_MALLOC (sizeof(cognate_object) * length);
  lst->top += length;
  size_t i = 0;
  for (const char* c; (c = strstr(str, delimiter)); i++)
  {
    lst->start[i] = (cognate_object) {.type=string, .string=GC_STRNDUP(str, c - str)};
    str = c + delim_size;
  }
  lst->start[i] = (cognate_object) {.type=string, .string=str};
  push(list, lst);
  */ // TODO
}

static void cognate_function_suffix() {
  // Joins a string to the end of another string.
  // Define Prefix (Swap, Suffix);
  const char* const str1 = pop(string);
  const char* const str2 = pop(string);
  const size_t str1_size = strlen(str1);
  const size_t str2_size = strlen(str2);
  const size_t new_string_size = str1_size + str2_size;
  char* const new_str = (char* const) GC_MALLOC (new_string_size);
  memmove(new_str, str2, str2_size);
  memmove(new_str+str2_size, str1, str1_size);
  push(string, new_str);
}

static void cognate_function_string_length() {
  push(number, mbstrlen(pop(string)));
}

static void cognate_function_substring() {
  // O(end).
  // Only allocates a new string if it has to.
  /* TODO: Would it be better to have a simpler and more minimalist set of string functions, like lists do?
   * The only real difference between NULL terminated strings and linked lists is that appending to strings is harder.
   * Maybe also a 'Join N Str1 Str2 Str3 ... StrN' function.
   */
  const double startf = pop(number);
  const double endf   = pop(number);
  size_t start  = startf;
  size_t end    = endf;
  if unlikely(start != startf || end != endf || start > end)
    throw_error("Cannot substring with character range %.14g...%.14g!", startf, endf);
  const char* str = pop(string);
  size_t str_size = 0;
  end -= start;
  for (;start != 0; --start)
  {
    if unlikely(*str == '\0') throw_error("String is too small to take substring from!"); // TODO Show more info here.
    str += mblen(str, MB_CUR_MAX);
  }
  for (;end != 0; --end)
  {
    if unlikely(str[str_size] == '\0') throw_error("String is too small to take substring from!");
    str_size += mblen(str+str_size, MB_CUR_MAX);
  }
  if unlikely(str[str_size] == '\0')
  {
    // We don't need to make a new string here.
    push(string, str);
    return;
  }
  push(string, GC_STRNDUP(str, str_size));
}


static void cognate_function_input() {
  // Read user input to a string.
  size_t size = 0;
  char* buf;
  size_t chars = getline(&buf, &size, stdin);
  push(string, GC_STRNDUP(buf, chars-1)); // Don't copy trailing newline.
  free(buf);
}

static void cognate_function_read() {
  // Read a file to a string.
  const char* const filename = pop(string);
  FILE *fp = fopen(filename, "ro");
  if unlikely(fp == NULL) throw_error("Cannot open file '%s'. It probably doesn't exist.", filename);
  fseek(fp, 0L, SEEK_END);
  size_t file_size = ftell(fp);
  rewind(fp);
  char *text = (char*) GC_MALLOC (file_size);
  fread(text, sizeof(char), file_size, fp);
  fclose(fp);
  text[file_size-1] = '\0'; // Remove trailing eof.
  push(string, text);
  // TODO: single line (or delimited) file read function for better IO performance?
}

static void cognate_function_number() {
  // casts string to number.
  const char* const str = pop(string);
  char* end;
  double num = strtod(str, &end);
  if (end == str || *end != '\0')
  {
    throw_error("Cannot parse '%s' to a number!", str);
  }
  push(number, num);
}

static void cognate_function_path() {
  char buf[FILENAME_MAX];
  if (!getcwd(buf, FILENAME_MAX))
  {
    throw_error("Cannot get executable path!");
  }
  push(string, GC_STRDUP(buf));
  free(buf);
}

static void cognate_function_stack() {
  copy_blocks();
  const cognate_list* lst = NULL;
  for (cognate_object* i = stack.top - 1 ; i >= stack.start ; --i)
  {
    // TODO: Allocate the list as an array for the sake of memory locality.
    cognate_list* tmp = GC_NEW (cognate_list);
    tmp -> object = *i;
    tmp -> next = lst;
    lst = tmp;
  }
  // Restore the stack.
  push(list, lst);

}

static void cognate_function_write() {
  // Write string to end of file, without a newline.
  // TODO: Allow writing of any writable object.
  const char* const filename = pop(string);
  FILE* const fp = fopen(filename, "a");
  if unlikely(fp == NULL) throw_error("Cannot open file '%s'. It probably doesn't exist.", filename);
  print_object(pop_any(), fp, 0);
  fclose(fp);
}

static void cognate_function_parameters() {
  push(list, params);
}

static void cognate_function_stop() {
  // Don't check stack length, because it probably wont be empty.
  exit(0);
}

static void cognate_function_table() {
  /*
  cognate_function_list();
  const cognate_list init = *pop(list);
  const size_t table_size = ((init.top - init.start) * LIST_GROWTH_FACTOR);
  cognate_table* const tab = GC_NEW(cognate_table); // Need to allocate list here.
  tab->items.start = (cognate_object*) GC_MALLOC (sizeof(cognate_object) * table_size);
  tab->items.top = tab->items.start + table_size;
  tab->confirmation_hash = (unsigned long*) GC_MALLOC (sizeof(unsigned long) * table_size);
  const char *key;
  cognate_object value;
  for (const cognate_object *i = init.start + 1; i < init.top; i += 2)
  {
    key = check_type(string, *i).string;
    value = *(i-1);
    *tab = table_add(hash(key), value, *tab);
  }
  push(table, tab);
  */ // TODO
}

static void cognate_function_insert() {
  // O(n) :(
  /*
  const char* const key = pop(string);
  const cognate_object value = pop_any();
  cognate_table* const tab = GC_NEW(cognate_table);
  *tab = table_add(hash(key), value, table_copy(*pop(table)));
  push(table, tab);
  */
}

static void cognate_function_get() {
  // O(1) mostly;
  /*
  const char* const key = pop(string);
  const cognate_table tab = *pop(table);
  push_any(table_get(key, tab));
  */
}

static void cognate_function_values() {
  // O(n)
  // Resulting list is NOT in any order at all.
  // Equivilant tables may give differently ordered lists.
  /*
  const cognate_table tab = *pop(table);
  cognate_list* const lst = GC_NEW(cognate_list);
  const long table_size = tab.items.top - tab.items.start;
  lst->start = (cognate_object*) GC_MALLOC (sizeof(cognate_object) * table_size);
  int j = 0;
  for (int i = 0; i < table_size; ++i)
  {
    if (tab.items.start[i].type != NOTHING)
    {
      lst->start[j++] = tab.items.start[i];
    }
  }
  lst->top = lst->start + j;
  push(list, lst);
  */ // TODO
}

static void cognate_function_match() {
  // Returns true if string matches regex.
  static const char *old_str = NULL;
  const char* const reg_str = pop(string);
  static regex_t reg;
  if (old_str == NULL || strcmp(reg_str, old_str) != 0)
  {
    // Technically, the last regex to be used in the program will leak memory.
    // However, this is minor, since only a limited amount of memory can be leaked.
    regfree(&reg); // Apparently freeing an unallocated regex is fine.
    if unlikely(*reg_str == '\0' || regcomp(&reg, reg_str, REG_EXTENDED | REG_NEWLINE | REG_NOSUB))
    {
      throw_error("Cannot compile invalid regular expression! ('%s')", reg_str);
    }
    old_str = reg_str; /* This should probably be strcpy, but I trust that reg_str is either
                          allocated with the garbage collector, or read only in the data segment. */
  }
  const int found = regexec(&reg, pop(string), 0, NULL, 0);
  if unlikely(found != 0 && found != REG_NOMATCH)
  {
    throw_error("Regex match error! (%s)", reg_str);
    // If this error ever actually appears, use regerror to get the full text.
  }
  push(boolean, !found);
}

static void cognate_function_ordinal() {
  const char* const str = pop(string);
  if unlikely(mbstrlen(str) != 1)
  {
    throw_error("Ordinal requires string of length 1. String '%s' is not of length 1!", str);
  }
  int chr = 0;
  mbtowc(&chr, str, MB_CUR_MAX);
  push(number, chr);
}

static void cognate_function_character() {
  const double d = pop(number);
  const int i = d;
  if unlikely(i != d) throw_error("Cannot convert %.14g to UTF8 character!", d);
  char* const str = (char* const) GC_MALLOC (MB_CUR_MAX + 1);
  wctomb(str, i);
  str[mblen(str, MB_CUR_MAX)] = '\0';
  push(string, str);
}

static void cognate_function_floor() {
  push(number, floor(pop(number)));
}

static void cognate_function_round() {
  push(number, round(pop(number)));
}

static void cognate_function_ceiling() {
  push(number, ceil(pop(number)));
}

static void cognate_function_assert() {
  const char* const name = pop(string);
  if unlikely(!pop(boolean))
  {
    throw_error("Assertion '%s' has failed!", name);
  }
}

static void cognate_function_error() {
  word_name = NULL;
  errno = 0;
  throw_error("%s", pop(string));
}

#endif
