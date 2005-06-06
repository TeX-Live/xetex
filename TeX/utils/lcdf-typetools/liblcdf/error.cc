// -*- c-basic-offset: 2; related-file-name: "../include/lcdf/error.hh" -*-
/*
 * error.{cc,hh} -- flexible classes for error reporting
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2001-2004 Eddie Kohler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <lcdf/error.hh>
#include <lcdf/straccum.hh>
#include <lcdf/hashmap.hh>
#include <errno.h>
#include <ctype.h>
#include <algorithm>
#ifndef __KERNEL__
# include <stdlib.h>
#endif

struct ErrorHandler::Conversion {
  String name;
  ConversionHook hook;
  Conversion *next;
};
static ErrorHandler::Conversion *error_items;

const int ErrorHandler::OK_RESULT = 0;
const int ErrorHandler::ERROR_RESULT = -EINVAL;

int
ErrorHandler::min_verbosity() const
{
  return 0;
}

void
ErrorHandler::debug(const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(ERR_DEBUG, String(), format, val);
  va_end(val);
}

void
ErrorHandler::message(const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(ERR_MESSAGE, String(), format, val);
  va_end(val);
}

int
ErrorHandler::warning(const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(ERR_WARNING, String(), format, val);
  va_end(val);
  return ERROR_RESULT;
}

int
ErrorHandler::error(const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(ERR_ERROR, String(), format, val);
  va_end(val);
  return ERROR_RESULT;
}

int
ErrorHandler::fatal(const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(ERR_FATAL, String(), format, val);
  va_end(val);
  return ERROR_RESULT;
}

int
ErrorHandler::fatal(int exit_status, const char *format, ...)
{
  assert(exit_status >= 0 && exit_status < 0x1000);
  va_list val;
  va_start(val, format);
  verror((Seriousness) (ERR_MIN_FATAL + (exit_status << ERRVERBOSITY_SHIFT)), String(), format, val);
  va_end(val);
  return ERROR_RESULT;
}

void
ErrorHandler::ldebug(const String &where, const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(ERR_DEBUG, where, format, val);
  va_end(val);
}

void
ErrorHandler::lmessage(const String &where, const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(ERR_MESSAGE, where, format, val);
  va_end(val);
}

int
ErrorHandler::lwarning(const String &where, const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(ERR_WARNING, where, format, val);
  va_end(val);
  return ERROR_RESULT;
}

int
ErrorHandler::lerror(const String &where, const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(ERR_ERROR, where, format, val);
  va_end(val);
  return ERROR_RESULT;
}

int
ErrorHandler::lfatal(const String &where, const char *format, ...)
{
  va_list val;
  va_start(val, format);
  verror(ERR_FATAL, where, format, val);
  va_end(val);
  return ERROR_RESULT;
}

int
ErrorHandler::lfatal(int exit_status, const String &where, const char *format, ...)
{
  assert(exit_status >= 0 && exit_status < 0x1000);
  va_list val;
  va_start(val, format);
  verror((Seriousness) (ERR_MIN_FATAL + (exit_status << ERRVERBOSITY_SHIFT)), where, format, val);
  va_end(val);
  return ERROR_RESULT;
}

String
ErrorHandler::make_text(Seriousness seriousness, const char *format, ...)
{
  va_list val;
  va_start(val, format);
  String s = make_text(seriousness, format, val);
  va_end(val);
  return s;
}

#define NUMBUF_SIZE	128
#define ErrH		ErrorHandler

static char *
do_number(unsigned long num, char *after_last, int base, int flags)
{
  const char *digits =
    ((flags & ErrH::UPPERCASE) ? "0123456789ABCDEF" : "0123456789abcdef");
  char *pos = after_last;
  while (num) {
    *--pos = digits[num % base];
    num /= base;
  }
  if (pos == after_last)
    *--pos = '0';
  return pos;
}

static char *
do_number_flags(char *pos, char *after_last, int base, int flags,
		int precision, int field_width)
{
  // remove ALTERNATE_FORM for zero results in base 16
  if ((flags & ErrH::ALTERNATE_FORM) && base == 16 && *pos == '0')
    flags &= ~ErrH::ALTERNATE_FORM;
  
  // account for zero padding
  if (precision >= 0)
    while (after_last - pos < precision)
      *--pos = '0';
  else if (flags & ErrH::ZERO_PAD) {
    if ((flags & ErrH::ALTERNATE_FORM) && base == 16)
      field_width -= 2;
    if ((flags & ErrH::NEGATIVE)
	|| (flags & (ErrH::PLUS_POSITIVE | ErrH::SPACE_POSITIVE)))
      field_width--;
    while (after_last - pos < field_width)
      *--pos = '0';
  }
  
  // alternate forms
  if ((flags & ErrH::ALTERNATE_FORM) && base == 8 && pos[1] != '0')
    *--pos = '0';
  else if ((flags & ErrH::ALTERNATE_FORM) && base == 16) {
    *--pos = ((flags & ErrH::UPPERCASE) ? 'X' : 'x');
    *--pos = '0';
  }
  
  // sign
  if (flags & ErrH::NEGATIVE)
    *--pos = '-';
  else if (flags & ErrH::PLUS_POSITIVE)
    *--pos = '+';
  else if (flags & ErrH::SPACE_POSITIVE)
    *--pos = ' ';
  
  return pos;
}

String
ErrorHandler::make_text(Seriousness, const char *s, va_list val)
{
  StringAccum msg;
  char numbuf[NUMBUF_SIZE];	// for numerics
  String placeholder;		// to ensure temporaries aren't destroyed
  numbuf[NUMBUF_SIZE-1] = 0;
  
  // declare and initialize these here to make gcc shut up about possible 
  // use before initialization
  int flags = 0;
  int field_width = -1;
  int precision = -1;
  int width_flag = 0;
  int base = 10;
  while (1) {
    
    const char *pct = strchr(s, '%');
    if (!pct) {
      if (*s) msg << s;
      break;
    }
    if (pct != s) {
      memcpy(msg.extend(pct - s), s, pct - s);
      s = pct;
    }
    
    // parse flags
    flags = 0;
   flags:
    switch (*++s) {
     case '#': flags |= ALTERNATE_FORM; goto flags;
     case '0': flags |= ZERO_PAD; goto flags;
     case '-': flags |= LEFT_JUST; goto flags;
     case ' ': flags |= SPACE_POSITIVE; goto flags;
     case '+': flags |= PLUS_POSITIVE; goto flags;
    }
    
    // parse field width
    field_width = -1;
    if (*s == '*') {
      field_width = va_arg(val, int);
      if (field_width < 0) {
	field_width = -field_width;
	flags |= LEFT_JUST;
      }
      s++;
    } else if (*s >= '0' && *s <= '9')
      for (field_width = 0; *s >= '0' && *s <= '9'; s++)
	field_width = 10*field_width + *s - '0';
    
    // parse precision
    precision = -1;
    if (*s == '.') {
      s++;
      precision = 0;
      if (*s == '*') {
	precision = va_arg(val, int);
	s++;
      } else if (*s >= '0' && *s <= '9')
	for (; *s >= '0' && *s <= '9'; s++)
	  precision = 10*precision + *s - '0';
    }
    
    // parse width flags
    width_flag = 0;
   width_flags:
    switch (*s) {
     case 'h':
      width_flag = 'h'; s++; goto width_flags;
     case 'l':
      width_flag = (width_flag == 'l' ? 'q' : 'l'); s++; goto width_flags;
     case 'L': case 'q':
      width_flag = 'q'; s++; goto width_flags;
    }
    
    // conversion character
    // after switch, data lies between `s1' and `s2'
    const char *s1 = 0, *s2 = 0;
    base = 10;
    switch (*s++) {
      
     case 's': {
       s1 = va_arg(val, const char *);
       if (!s1)
	 s1 = "(null)";
       if (flags & ALTERNATE_FORM) {
	 placeholder = String(s1).printable();
	 s1 = placeholder.c_str();
       }
       for (s2 = s1; *s2 && precision != 0; s2++)
	 if (precision > 0)
	   precision--;
       break;
     }

     case 'c': {
       int c = va_arg(val, int);
       // check for extension of 'signed char' to 'int'
       if (c < 0)
	 c += 256;
       // assume ASCII
       if (c == '\n')
	 strcpy(numbuf, "\\n");
       else if (c == '\t')
	 strcpy(numbuf, "\\t");
       else if (c == '\r')
	 strcpy(numbuf, "\\r");
       else if (c == '\0')
	 strcpy(numbuf, "\\0");
       else if (c < 0 || c >= 256)
	 strcpy(numbuf, "(bad char)");
       else if (c < 32 || c >= 0177)
	 sprintf(numbuf, "\\%03o", c);
       else
	 sprintf(numbuf, "%c", c);
       s1 = numbuf;
       s2 = strchr(numbuf, 0);
       break;
     }
     
     case '%': {
       numbuf[0] = '%';
       s1 = numbuf;
       s2 = s1 + 1;
       break;
     }

     case 'd':
     case 'i':
      flags |= SIGNED;
     case 'u':
     number: {
       // protect numbuf from overflow
       if (field_width > NUMBUF_SIZE)
	 field_width = NUMBUF_SIZE;
       if (precision > NUMBUF_SIZE-4)
	 precision = NUMBUF_SIZE-4;
       
       s2 = numbuf + NUMBUF_SIZE;
       
       unsigned long num;
#ifdef HAVE_INT64_TYPES
       if (width_flag == 'q') {
	 uint64_t qnum = va_arg(val, uint64_t);
	 if ((flags & SIGNED) && (int64_t)qnum < 0)
	   qnum = -(int64_t)qnum, flags |= NEGATIVE;
	 String q = cp_unparse_unsigned64(qnum, base, flags & UPPERCASE);
	 s1 = s2 - q.length();
	 memcpy((char *)s1, q.data(), q.length());
	 goto got_number;
       }
#endif
       if (width_flag == 'h') {
	 num = (unsigned short)va_arg(val, int);
	 if ((flags & SIGNED) && (short)num < 0)
	   num = -(short)num, flags |= NEGATIVE;
       } else if (width_flag == 'l') {
	 num = va_arg(val, unsigned long);
	 if ((flags & SIGNED) && (long)num < 0)
	   num = -(long)num, flags |= NEGATIVE;
       } else {
	 num = va_arg(val, unsigned int);
	 if ((flags & SIGNED) && (int)num < 0)
	   num = -(int)num, flags |= NEGATIVE;
       }
       s1 = do_number(num, (char *)s2, base, flags);

#ifdef HAVE_INT64_TYPES
      got_number:
#endif
       s1 = do_number_flags((char *)s1, (char *)s2, base, flags,
			    precision, field_width);
       break;
     }
     
     case 'o':
      base = 8;
      goto number;
      
     case 'X':
      flags |= UPPERCASE;
     case 'x':
      base = 16;
      goto number;
      
     case 'p': {
       void *v = va_arg(val, void *);
       s2 = numbuf + NUMBUF_SIZE;
       s1 = do_number((unsigned long)v, (char *)s2, 16, flags);
       s1 = do_number_flags((char *)s1, (char *)s2, 16, flags | ALTERNATE_FORM,
			    precision, field_width);
       break;
     }

#ifndef __KERNEL__
     case 'e': case 'f': case 'g':
     case 'E': case 'F': case 'G': {
       char format[80], *f = format, new_numbuf[NUMBUF_SIZE];
       *f++ = '%';
       if (flags & ALTERNATE_FORM)
	 *f++ = '#';
       if (precision >= 0)
	 f += sprintf(f, ".%d", precision);
       *f++ = s[-1];
       *f++ = 0;

       int len = sprintf(new_numbuf, format, va_arg(val, double));

       s2 = numbuf + NUMBUF_SIZE;
       s1 = s2 - len;
       memcpy((char *)s1, new_numbuf, len); // note: no terminating \0
       s1 = do_number_flags((char *)s1, (char *)s2, 10, flags & ~ALTERNATE_FORM, -1, field_width);
       break;
     }
#endif
     
     case '{': {
       const char *rbrace = strchr(s, '}');
       if (!rbrace || rbrace == s)
	 assert(0 /* Bad %{ in error */);
       String name(s, rbrace - s);
       s = rbrace + 1;
       for (Conversion *item = error_items; item; item = item->next)
	 if (item->name == name) {
	   placeholder = item->hook(flags, VA_LIST_REF(val));
	   s1 = placeholder.data();
	   s2 = s1 + placeholder.length();
	   goto got_result;
	 }
       assert(0 /* Bad %{ in error */);
       break;
     }
     
     default:
      assert(0 /* Bad % in error */);
      break;
      
    }

    // add result of conversion
   got_result:
    int slen = s2 - s1;
    if (slen > field_width) field_width = slen;
    char *dest = msg.extend(field_width);
    if (flags & LEFT_JUST) {
      memcpy(dest, s1, slen);
      memset(dest + slen, ' ', field_width - slen);
    } else {
      memcpy(dest + field_width - slen, s1, slen);
      memset(dest, (flags & ZERO_PAD ? '0' : ' '), field_width - slen);
    }
  }

  return msg.take_string();
}

String
ErrorHandler::decorate_text(Seriousness seriousness, const String &landmark, const String &text)
{
  String new_text;
  
  // prepend 'warning: ' to every line if appropriate
  if (seriousness >= ERR_MIN_WARNING && seriousness < ERR_MIN_ERROR)
    new_text = prepend_lines("warning: ", text);
  else
    new_text = text;

  // handle landmark
  if (landmark
      && !(landmark.length() > 2 && landmark[0] == '\\'
	   && landmark.back() == '\\')) {
    // ignore special-purpose landmarks (begin and end with a backslash `\')
    // fix landmark: skip trailing spaces and trailing colon
    int i, len = landmark.length();
    for (i = len - 1; i >= 0; i--)
      if (!isspace(landmark[i]))
	break;
    if (i >= 0 && landmark[i] == ':')
      i--;

    // prepend landmark, unless all spaces
    if (i >= 0)
      new_text = prepend_lines(landmark.substring(0, i+1) + ": ", new_text);
  }

  return new_text;
}

int
ErrorHandler::verror(Seriousness seriousness, const String &where,
		     const char *s, va_list val)
{
  String text = make_text(seriousness, s, val);
  text = decorate_text(seriousness, where, text);
  handle_text(seriousness, text);
  return count_error(seriousness, text);
}

int
ErrorHandler::verror_text(Seriousness seriousness, const String &where,
			  const String &text)
{
  // text is already made
  String dec_text = decorate_text(seriousness, where, text);
  handle_text(seriousness, dec_text);
  return count_error(seriousness, dec_text);
}

void
ErrorHandler::set_error_code(int)
{
}

String
ErrorHandler::prepend_lines(const String &prepend, const String &text)
{
  if (!prepend)
    return text;
  
  StringAccum sa;
  const char *begin = text.begin();
  const char *end = text.end();
  const char *nl;
  while ((nl = std::find(begin, end, '\n')) < end) {
    sa << prepend << text.substring(begin, nl + 1);
    begin = nl + 1;
  }
  if (begin < end)
    sa << prepend << text.substring(begin, end);
  
  return sa.take_string();
}


//
// BASE ERROR HANDLER
//

int
BaseErrorHandler::count_error(Seriousness seriousness, const String &)
{
  if (seriousness < ERR_MIN_WARNING)
    /* do nothing */;
  else if (seriousness < ERR_MIN_ERROR)
    _nwarnings++;
  else
    _nerrors++;
  return (seriousness < ERR_MIN_WARNING ? OK_RESULT : ERROR_RESULT);
}


#ifndef __KERNEL__
//
// FILE ERROR HANDLER
//

FileErrorHandler::FileErrorHandler(FILE *f, const String &context)
  : _f(f), _context(context)
{
}

void
FileErrorHandler::handle_text(Seriousness seriousness, const String &message)
{
  String text = prepend_lines(_context, message);
  if (text && text.back() != '\n')
    text += '\n';
  fwrite(text.data(), 1, text.length(), _f);
  
  if (seriousness >= ERR_MIN_FATAL) {
    int exit_status = (seriousness - ERR_MIN_FATAL) >> ERRVERBOSITY_SHIFT;
    exit(exit_status);
  }
}
#endif


//
// SILENT ERROR HANDLER
//

class SilentErrorHandler : public BaseErrorHandler { public:
  SilentErrorHandler()			{ }
  void handle_text(Seriousness, const String &);  
};

void
SilentErrorHandler::handle_text(Seriousness, const String &)
{
}


//
// STATIC ERROR HANDLERS
//

static ErrorHandler *the_default_handler = 0;
static ErrorHandler *the_silent_handler = 0;

ErrorHandler::Conversion *
ErrorHandler::add_conversion(const String &name, ConversionHook hook)
{
  if (Conversion *c = new Conversion) {
    c->name = name;
    c->hook = hook;
    c->next = error_items;
    error_items = c;
    return c;
  } else
    return 0;
}

int
ErrorHandler::remove_conversion(ErrorHandler::Conversion *conv)
{
  Conversion **pprev = &error_items;
  for (Conversion *c = error_items; c; pprev = &c->next, c = *pprev)
    if (c == conv) {
      *pprev = c->next;
      delete c;
      return 0;
    }
  return -1;
}

ErrorHandler *
ErrorHandler::static_initialize(ErrorHandler *default_handler)
{
  the_default_handler = default_handler;
  return the_default_handler;
}

void
ErrorHandler::static_cleanup()
{
  delete the_default_handler;
  delete the_silent_handler;
  the_default_handler = the_silent_handler = 0;
  while (error_items) {
    Conversion *next = error_items->next;
    delete error_items;
    error_items = next;
  }
}

bool
ErrorHandler::has_default_handler()
{
  return the_default_handler ? true : false;
}

ErrorHandler *
ErrorHandler::default_handler()
{
  assert(the_default_handler != 0);
  return the_default_handler;
}

ErrorHandler *
ErrorHandler::silent_handler()
{
  if (!the_silent_handler)
    the_silent_handler = new SilentErrorHandler;
  return the_silent_handler;
}

void
ErrorHandler::set_default_handler(ErrorHandler *errh)
{
  the_default_handler = errh;
}


//
// ERROR VENEER
//

int
ErrorVeneer::nwarnings() const
{
  return _errh->nwarnings();
}

int
ErrorVeneer::nerrors() const
{
  return _errh->nerrors();
}

void
ErrorVeneer::reset_counts()
{
  _errh->reset_counts();
}

String
ErrorVeneer::make_text(Seriousness seriousness, const char *s, va_list val)
{
  return _errh->make_text(seriousness, s, val);
}

String
ErrorVeneer::decorate_text(Seriousness seriousness, const String &landmark, const String &text)
{
  return _errh->decorate_text(seriousness, landmark, text);
}

void
ErrorVeneer::handle_text(Seriousness seriousness, const String &text)
{
  _errh->handle_text(seriousness, text);
}

int
ErrorVeneer::count_error(Seriousness seriousness, const String &text)
{
  return _errh->count_error(seriousness, text);
}


//
// CONTEXT ERROR HANDLER
//

ContextErrorHandler::ContextErrorHandler(ErrorHandler *errh,
					 const String &context,
					 const String &indent,
					 const String &context_landmark)
  : ErrorVeneer(errh), _context(context), _indent(indent),
    _context_landmark(context_landmark)
{
}

String
ContextErrorHandler::decorate_text(Seriousness seriousness, const String &landmark, const String &text)
{
  String context_lines;
  if (_context) {
    // do not print context or indent if underlying ErrorHandler doesn't want
    // context
    if (_errh->min_verbosity() > ERRVERBOSITY_CONTEXT)
      _context = _indent = String();
    else {
      context_lines = _errh->decorate_text(ERR_MESSAGE, (_context_landmark ? _context_landmark : landmark), _context);
      if (context_lines && context_lines.back() != '\n')
	context_lines += '\n';
      _context = String();
    }
  }
  return context_lines + _errh->decorate_text(seriousness, (landmark ? landmark : _context_landmark), prepend_lines(_indent, text));
}


//
// PREFIX ERROR HANDLER
//

PrefixErrorHandler::PrefixErrorHandler(ErrorHandler *errh,
				       const String &prefix)
  : ErrorVeneer(errh), _prefix(prefix)
{
}

String
PrefixErrorHandler::decorate_text(Seriousness seriousness, const String &landmark, const String &text)
{
  return _errh->decorate_text(seriousness, landmark, prepend_lines(_prefix, text));
}


//
// LANDMARK ERROR HANDLER
//

LandmarkErrorHandler::LandmarkErrorHandler(ErrorHandler *errh, const String &landmark)
  : ErrorVeneer(errh), _landmark(landmark)
{
}

String
LandmarkErrorHandler::decorate_text(Seriousness seriousness, const String &lm, const String &text)
{
  if (lm)
    return _errh->decorate_text(seriousness, lm, text);
  else
    return _errh->decorate_text(seriousness, _landmark, text);
}


//
// VERBOSE FILTER ERROR HANDLER
//

VerboseFilterErrorHandler::VerboseFilterErrorHandler(ErrorHandler *errh, int min_verbosity)
  : ErrorVeneer(errh), _min_verbosity(min_verbosity)
{
}

int
VerboseFilterErrorHandler::min_verbosity() const
{
  int m = _errh->min_verbosity();
  return (m >= _min_verbosity ? m : _min_verbosity);
}

void
VerboseFilterErrorHandler::handle_text(Seriousness s, const String &text)
{
  if ((s & ERRVERBOSITY_MASK) >= _min_verbosity)
    _errh->handle_text(s, text);
}


//
// BAIL ERROR HANDLER
//

#ifndef __KERNEL__

BailErrorHandler::BailErrorHandler(ErrorHandler *errh, Seriousness s)
  : ErrorVeneer(errh), _exit_seriousness(s)
{
}

void
BailErrorHandler::handle_text(Seriousness s, const String &text)
{
  _errh->handle_text(s, text);
  if (s >= _exit_seriousness)
    exit(1);
}

#endif
