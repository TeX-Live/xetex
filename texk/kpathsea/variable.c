/* variable.c: variable expansion.

    Copyright 1993, 1994, 1995, 1996, 2008 Karl Berry.
    Copyright 1997, 1999, 2001, 2002, 2005 Olaf Weber.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
 
    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, see <http://www.gnu.org/licenses/>.  */

#include <kpathsea/config.h>

#include <kpathsea/c-ctype.h>
#include <kpathsea/cnf.h>
#include <kpathsea/fn.h>
#include <kpathsea/tilde.h>
#include <kpathsea/variable.h>


/* Here's the simple one, when a program just wants a value.  */

string
kpse_var_value P1C(const_string, var)
{
  string vtry, ret;

  assert (kpse_program_name);

  /* First look for VAR.progname. */
  vtry = concat3 (var, ".", kpse_program_name);
  ret = getenv (vtry);
  free (vtry);

  if (!ret || !*ret) {
    /* Now look for VAR_progname. */
    vtry = concat3 (var, "_", kpse_program_name);
    ret = getenv (vtry);
    free (vtry);
  }

  /* Just plain VAR.  */
  if (!ret || !*ret)
    ret = getenv (var);

  /* Not in the environment; check a config file.  */
  if (!ret || !*ret)
    ret = kpse_cnf_get (var);

  /* We have a value; do variable and tilde expansion.  We want to use ~
     in the cnf files, to adapt nicely to Windows and to avoid extra /'s
     (see tilde.c), but we also want kpsewhich -var-value=foo to not
     have any literal ~ characters, so our shell scripts don't have to
     worry about doing the ~ expansion.  */
  if (ret) {
    string tmp = kpse_var_expand (ret);
    /* We don't want to free the previous value of ret here; apparently
       it's used later, somewhere, somehow.  (The end result was a crash
       when making tex.fmt.)  Sigh.  */
    ret = kpse_tilde_expand (tmp);
    if (ret != tmp) {
      free (tmp);
    }
  }

#ifdef KPSE_DEBUG
  if (KPSE_DEBUG_P (KPSE_DEBUG_VARS))
    DEBUGF2("variable: %s = %s\n", var, ret ? ret : "(nil)");
#endif

  return ret;
}

/* We have to keep track of variables being expanded, otherwise
   constructs like TEXINPUTS = $TEXINPUTS result in an infinite loop.
   (Or indirectly recursive variables, etc.)  Our simple solution is to
   add to a list each time an expansion is started, and check the list
   before expanding.  */

typedef struct {
  const_string var;
  boolean expanding;
} expansion_type;
static expansion_type *expansions; /* The sole variable of this type.  */
static unsigned expansion_len = 0;

static void
expanding P2C(const_string, var,  boolean, xp)
{
  unsigned e;
  for (e = 0; e < expansion_len; e++) {
    if (STREQ (expansions[e].var, var)) {
      expansions[e].expanding = xp;
      return;
    }
  }

  /* New variable, add it to the list.  */
  expansion_len++;
  XRETALLOC (expansions, expansion_len, expansion_type);
  expansions[expansion_len - 1].var = xstrdup (var);
  expansions[expansion_len - 1].expanding = xp;
}


/* Return whether VAR is currently being expanding.  */

static boolean
expanding_p P1C(const_string, var)
{
  unsigned e;
  for (e = 0; e < expansion_len; e++) {
    if (STREQ (expansions[e].var, var))
      return expansions[e].expanding;
  }
  
  return false;
}

/* Append the result of value of `var' to EXPANSION, where `var' begins
   at START and ends at END.  If `var' is not set, do not complain.
   This is a subroutine for the more complicated expansion function.  */

static void
expand P3C(fn_type *, expansion,  const_string, start,  const_string, end)
{
  string value;
  unsigned len = end - start + 1;
  string var = (string)xmalloc (len + 1);
  strncpy (var, start, len);
  var[len] = 0;
  
  if (expanding_p (var)) {
    WARNING1 ("kpathsea: variable `%s' references itself (eventually)", var);
  } else {
    string vtry = concat3 (var, "_", kpse_program_name);
    /* Check for an environment variable.  */
    value = getenv (vtry);
    free (vtry);
    
    if (!value || !*value)
      value = getenv (var);

    /* If no envvar, check the config files.  */
    if (!value || !*value)
      value = kpse_cnf_get (var);

    if (value) {
      expanding (var, true);
      value = kpse_var_expand (value);
      expanding (var, false);
      
      { /* Do tilde expansion; see explanation above in kpse_var_value.  */
        string tmp = kpse_tilde_expand (value);
        if (value != tmp) {
          free (value);
          value = tmp;
        }
      }
      
      fn_grow (expansion, value, strlen (value));
      free (value);
    }

    free (var);
  }
}

/* Can't think of when it would be useful to change these (and the
   diagnostic messages assume them), but ... */
#ifndef IS_VAR_START /* starts all variable references */
#define IS_VAR_START(c) ((c) == '$')
#endif
#ifndef IS_VAR_CHAR  /* variable name constituent */
#define IS_VAR_CHAR(c) (ISALNUM (c) || (c) == '_')
#endif
#ifndef IS_VAR_BEGIN_DELIMITER /* start delimited variable name (after $) */
#define IS_VAR_BEGIN_DELIMITER(c) ((c) == '{')
#endif
#ifndef IS_VAR_END_DELIMITER
#define IS_VAR_END_DELIMITER(c) ((c) == '}')
#endif


/* Maybe we should support some or all of the various shell ${...}
   constructs, especially ${var-value}.  We do do ~ expansion.  */

string
kpse_var_expand P1C(const_string, src)
{
  const_string s;
  string ret;
  fn_type expansion;
  expansion = fn_init ();
  
  /* Copy everything but variable constructs.  */
  for (s = src; *s; s++) {
    if (IS_VAR_START (*s)) {
      s++;

      /* Three cases: `$VAR', `${VAR}', `$<anything-else>'.  */
      if (IS_VAR_CHAR (*s)) {
        /* $V: collect name constituents, then expand.  */
        const_string var_end = s;

        do {
          var_end++;
        } while (IS_VAR_CHAR (*var_end));

        var_end--; /* had to go one past */
        expand (&expansion, s, var_end);
        s = var_end;

      } else if (IS_VAR_BEGIN_DELIMITER (*s)) {
        /* ${: scan ahead for matching delimiter, then expand.  */
        const_string var_end = ++s;

        while (*var_end && !IS_VAR_END_DELIMITER (*var_end))
          var_end++;

        if (! *var_end) {
          WARNING1 ("%s: No matching } for ${", src);
          s = var_end - 1; /* will incr to null at top of loop */
        } else {
          expand (&expansion, s, var_end - 1);
          s = var_end; /* will incr past } at top of loop*/
        }

      } else {
        /* $<something-else>: error.  */
        WARNING2 ("%s: Unrecognized variable construct `$%c'", src, *s);
        /* Just ignore those chars and keep going.  */
      }
    } else
     fn_1grow (&expansion, *s);
  }
  fn_1grow (&expansion, 0);
          
  ret = FN_STRING (expansion);
  return ret;
}

#ifdef TEST

static void
test_var (string test, string right_answer)
{
  string result = kpse_var_expand (test);
  
  printf ("expansion of `%s'\t=> %s", test, result);
  if (!STREQ (result, right_answer))
    printf (" [should be `%s']", right_answer);
  putchar ('\n');
}


int
main ()
{
  test_var ("a", "a");
  test_var ("$foo", "");
  test_var ("a$foo", "a");
  test_var ("$foo a", " a");
  test_var ("a$foo b", "a b");

  xputenv ("FOO", "foo value");
  test_var ("a$FOO", "afoo value");

  xputenv ("Dollar", "$");
  test_var ("$Dollar a", "$ a");

  test_var ("a${FOO}b", "afoo valueb");
  test_var ("a${}b", "ab");

  test_var ("$$", ""); /* and error */
  test_var ("a${oops", "a"); /* and error */

  return 0;
}

#endif /* TEST */


/*
Local variables:
standalone-compile-command: "gcc -g -I. -I.. -DTEST variable.c kpathsea.a"
End:
*/
