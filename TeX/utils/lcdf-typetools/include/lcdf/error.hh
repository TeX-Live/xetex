// -*- related-file-name: "../../liblcdf/error.cc" -*-
#ifndef LCDF_ERROR_HH
#define LCDF_ERROR_HH
#include <lcdf/string.hh>
#ifndef __KERNEL__
# include <stdio.h>
#endif
#include <stdarg.h>
#if HAVE_ADDRESSABLE_VA_LIST
# define VA_LIST_REF_T		va_list *
# define VA_LIST_DEREF(val)	(*(val))
# define VA_LIST_REF(val)	(&(val))
#else
# define VA_LIST_REF_T		va_list
# define VA_LIST_DEREF(val)	(val)
# define VA_LIST_REF(val)	(val)
#endif

class ErrorHandler { public:
  
  enum Seriousness {
    ERRVERBOSITY_CONTEXT= 0x8000,
    ERRVERBOSITY_MAX	= 0xFFFF,
    ERRVERBOSITY_DEFAULT= ERRVERBOSITY_MAX,
    ERRVERBOSITY_MASK	= 0x0000FFFF,
    ERRVERBOSITY_SHIFT	= 16,

    ERR_MIN_DEBUG	= 0x00000000,
    ERR_MIN_MESSAGE	= 0x00010000,
    ERR_MIN_WARNING	= 0x00020000,
    ERR_MIN_ERROR	= 0x00030000,
    ERR_MIN_FATAL	= 0x00040000,

    // fatal() with no explicit exit status exits with this status
    FATAL_EXITSTATUS	= 1,
    
    ERR_DEBUG		= ERR_MIN_DEBUG + ERRVERBOSITY_DEFAULT,
    ERR_CONTEXT_MESSAGE	= ERR_MIN_MESSAGE + ERRVERBOSITY_CONTEXT,
    ERR_MESSAGE		= ERR_MIN_MESSAGE + ERRVERBOSITY_DEFAULT,
    ERR_WARNING		= ERR_MIN_WARNING + ERRVERBOSITY_DEFAULT,
    ERR_CONTEXT_ERROR	= ERR_MIN_ERROR + ERRVERBOSITY_CONTEXT,
    ERR_ERROR		= ERR_MIN_ERROR + ERRVERBOSITY_DEFAULT,
    ERR_FATAL		= ERR_MIN_FATAL + ERRVERBOSITY_DEFAULT + (FATAL_EXITSTATUS << ERRVERBOSITY_SHIFT)
  };
  
  ErrorHandler()			{ }
  virtual ~ErrorHandler()		{ }
  
  static ErrorHandler *static_initialize(ErrorHandler *errh); // returns errh
  static void static_cleanup();

  static ErrorHandler *default_handler();
  static ErrorHandler *silent_handler();
 
  static bool has_default_handler(); 
  static void set_default_handler(ErrorHandler *);
  
  virtual int nwarnings() const = 0;
  virtual int nerrors() const = 0;
  virtual void reset_counts() = 0;
  virtual int min_verbosity() const;

  // seriousness < ERR_MIN_WARNING returns OK_RESULT, which is 0
  // seriousness >= ERR_MIN_WARNING returns ERROR_RESULT, which is -EINVAL
  static const int OK_RESULT;
  static const int ERROR_RESULT;

  void debug(const char *format, ...);
  void message(const char *format, ...);
  int warning(const char *format, ...);
  int error(const char *format, ...);
  int fatal(const char *format, ...);
  int fatal(int exit_status, const char *format, ...);

  void ldebug(const String &landmark, const char *format, ...);
  void lmessage(const String &landmark, const char *format, ...);
  int lwarning(const String &landmark, const char *format, ...);
  int lerror(const String &landmark, const char *format, ...);
  int lfatal(const String &landmark, const char *format, ...);
  int lfatal(int exit_status, const String &landmark, const char *format, ...);

  int verror(Seriousness, const String &landmark, const char *format, va_list);
  int verror_text(Seriousness, const String &landmark, const String &text);
  
  String make_text(Seriousness, const char *, ...);
  virtual String make_text(Seriousness, const char *, va_list);
  virtual String decorate_text(Seriousness, const String &, const String &);
  virtual void handle_text(Seriousness, const String &) = 0;
  virtual int count_error(Seriousness, const String &) = 0;

  virtual void set_error_code(int);

  static String prepend_lines(const String &prefix, const String &text);

  // error conversions
  struct Conversion;
  typedef String (*ConversionHook)(int flags, VA_LIST_REF_T);
  enum ConversionFlags {
    ZERO_PAD = 1, PLUS_POSITIVE = 2, SPACE_POSITIVE = 4, LEFT_JUST = 8,
    ALTERNATE_FORM = 16, UPPERCASE = 32, SIGNED = 64, NEGATIVE = 128
  };
  static Conversion *add_conversion(const String &, ConversionHook);
  static int remove_conversion(Conversion *);
  
};

class BaseErrorHandler : public ErrorHandler { public:
  BaseErrorHandler()			: _nwarnings(0), _nerrors(0) { }
  int nwarnings() const			{ return _nwarnings; }
  int nerrors() const			{ return _nerrors; }
  void reset_counts()			{ _nwarnings = _nerrors = 0; }
  int count_error(Seriousness, const String &);
 private:
  int _nwarnings, _nerrors;
};

#ifndef __KERNEL__
class FileErrorHandler : public BaseErrorHandler { public:
  FileErrorHandler(FILE *, const String & = String());
  void handle_text(Seriousness, const String &);
 private:
  FILE *_f;
  String _context;
};
#endif

class ErrorVeneer : public ErrorHandler { public:

  ErrorVeneer(ErrorHandler *errh)	: _errh(errh) { }

  int nwarnings() const;
  int nerrors() const;
  void reset_counts();

  String make_text(Seriousness, const char *, va_list);
  String decorate_text(Seriousness, const String &, const String &);
  void handle_text(Seriousness, const String &);
  int count_error(Seriousness, const String &);

 protected:

  ErrorHandler *_errh;
 
};

class ContextErrorHandler : public ErrorVeneer { public:
  ContextErrorHandler(ErrorHandler *, const String &context, const String &indent = "  ", const String &context_landmark = "");
  String decorate_text(Seriousness, const String &, const String &);
 private:
  String _context;
  String _indent;
  String _context_landmark;
};

class PrefixErrorHandler : public ErrorVeneer { public:
  PrefixErrorHandler(ErrorHandler *, const String &prefix);
  String decorate_text(Seriousness, const String &, const String &);
 private:
  String _prefix;
};

class LandmarkErrorHandler : public ErrorVeneer { public:
  LandmarkErrorHandler(ErrorHandler *, const String &);
  void set_landmark(const String &s)	{ _landmark = s; }
  String decorate_text(Seriousness, const String &, const String &);
 private:
  String _landmark;
};

class VerboseFilterErrorHandler : public ErrorVeneer { public:
  VerboseFilterErrorHandler(ErrorHandler *, int min_verbosity);
  int min_verbosity() const;
  void handle_text(Seriousness, const String &);
 private:
  int _min_verbosity;
};

#ifndef __KERNEL__
class BailErrorHandler : public ErrorVeneer { public:
  BailErrorHandler(ErrorHandler *, Seriousness = ERR_MIN_ERROR);
  void handle_text(Seriousness, const String &);
 private:
  int _exit_seriousness;
};
#endif

#endif
