#ifndef __CURL_CURL_H
#define __CURL_CURL_H
/*****************************************************************************
 *                                  _   _ ____  _     
 *  Project                     ___| | | |  _ \| |    
 *                             / __| | | | |_) | |    
 *                            | (__| |_| |  _ <| |___ 
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2002, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * In order to be useful for every potential user, curl and libcurl are
 * dual-licensed under the MPL and the MIT/X-derivate licenses.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the MPL or the MIT/X-derivate
 * licenses. You may pick one of these licenses.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * $Id: curl.h,v 1.127 2002/03/05 10:15:38 bagder Exp $
 *****************************************************************************/

#include <stdio.h>
/* The include stuff here is mainly for time_t! */
#ifdef vms
# include <types.h>
# include <time.h>
#else
# include <sys/types.h>
# ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#  ifdef HAVE_SYS_TIME_H
#   include <sys/time.h>
#  else
#   include <time.h>
#  endif
# endif
#endif /* defined (vms) */

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#include <curl/types.h>

#ifdef  __cplusplus
extern "C" {
#endif

struct HttpPost {
  struct HttpPost *next; /* next entry in the list */
  char *name;     /* pointer to allocated name */
  long namelength; /* length of name length */
  char *contents; /* pointer to allocated data contents */
  long contentslength; /* length of contents field */
  char *contenttype; /* Content-Type */
  struct curl_slist* contentheader; /* list of extra headers for this form */
  struct HttpPost *more; /* if one field name has more than one file, this
			    link should link to following files */
  long flags;     /* as defined below */
#define HTTPPOST_FILENAME (1<<0) /* specified content is a file name */
#define HTTPPOST_READFILE (1<<1) /* specified content is a file name */
#define HTTPPOST_PTRNAME (1<<2) /* name is only stored pointer
                                   do not free in formfree */
#define HTTPPOST_PTRCONTENTS (1<<3) /* contents is only stored pointer
                                       do not free in formfree */
};

typedef int (*curl_progress_callback)(void *clientp,
                                      double dltotal,
                                      double dlnow,
                                      double ultotal,
                                      double ulnow);

typedef size_t (*curl_write_callback)(char *buffer,
                                      size_t size,
                                      size_t nitems,
                                      void *outstream);

typedef size_t (*curl_read_callback)(char *buffer,
                                     size_t size,
                                     size_t nitems,
                                     void *instream);

typedef int (*curl_passwd_callback)(void *clientp,
                                    const char *prompt,
                                    char *buffer,
                                    int buflen);

/* All possible error codes from all sorts of curl functions. Future versions
   may return other values, stay prepared.

   Always add new return codes last. Never *EVER* remove any. The return
   codes must remain the same!
 */

typedef enum {
  CURLE_OK = 0,
  CURLE_UNSUPPORTED_PROTOCOL,    /* 1 */
  CURLE_FAILED_INIT,             /* 2 */
  CURLE_URL_MALFORMAT,           /* 3 */
  CURLE_URL_MALFORMAT_USER,      /* 4 */
  CURLE_COULDNT_RESOLVE_PROXY,   /* 5 */
  CURLE_COULDNT_RESOLVE_HOST,    /* 6 */
  CURLE_COULDNT_CONNECT,         /* 7 */
  CURLE_FTP_WEIRD_SERVER_REPLY,  /* 8 */
  CURLE_FTP_ACCESS_DENIED,       /* 9 */
  CURLE_FTP_USER_PASSWORD_INCORRECT, /* 10 */
  CURLE_FTP_WEIRD_PASS_REPLY,    /* 11 */
  CURLE_FTP_WEIRD_USER_REPLY,    /* 12 */
  CURLE_FTP_WEIRD_PASV_REPLY,    /* 13 */
  CURLE_FTP_WEIRD_227_FORMAT,    /* 14 */
  CURLE_FTP_CANT_GET_HOST,       /* 15 */
  CURLE_FTP_CANT_RECONNECT,      /* 16 */
  CURLE_FTP_COULDNT_SET_BINARY,  /* 17 */
  CURLE_PARTIAL_FILE,            /* 18 */
  CURLE_FTP_COULDNT_RETR_FILE,   /* 19 */
  CURLE_FTP_WRITE_ERROR,         /* 20 */
  CURLE_FTP_QUOTE_ERROR,         /* 21 */
  CURLE_HTTP_NOT_FOUND,          /* 22 */
  CURLE_WRITE_ERROR,             /* 23 */
  CURLE_MALFORMAT_USER,          /* 24 - user name is illegally specified */
  CURLE_FTP_COULDNT_STOR_FILE,   /* 25 - failed FTP upload */
  CURLE_READ_ERROR,              /* 26 - could open/read from file */
  CURLE_OUT_OF_MEMORY,           /* 27 */
  CURLE_OPERATION_TIMEOUTED,     /* 28 - the timeout time was reached */
  CURLE_FTP_COULDNT_SET_ASCII,   /* 29 - TYPE A failed */
  CURLE_FTP_PORT_FAILED,         /* 30 - FTP PORT operation failed */
  CURLE_FTP_COULDNT_USE_REST,    /* 31 - the REST command failed */
  CURLE_FTP_COULDNT_GET_SIZE,    /* 32 - the SIZE command failed */
  CURLE_HTTP_RANGE_ERROR,        /* 33 - RANGE "command" didn't work */
  CURLE_HTTP_POST_ERROR,         /* 34 */
  CURLE_SSL_CONNECT_ERROR,       /* 35 - wrong when connecting with SSL */
  CURLE_FTP_BAD_DOWNLOAD_RESUME, /* 36 - couldn't resume download */
  CURLE_FILE_COULDNT_READ_FILE,  /* 37 */
  CURLE_LDAP_CANNOT_BIND,        /* 38 */
  CURLE_LDAP_SEARCH_FAILED,      /* 39 */
  CURLE_LIBRARY_NOT_FOUND,       /* 40 */
  CURLE_FUNCTION_NOT_FOUND,      /* 41 */
  CURLE_ABORTED_BY_CALLBACK,     /* 42 */
  CURLE_BAD_FUNCTION_ARGUMENT,   /* 43 */
  CURLE_BAD_CALLING_ORDER,       /* 44 */
  CURLE_HTTP_PORT_FAILED,        /* 45 - HTTP Interface operation failed */
  CURLE_BAD_PASSWORD_ENTERED,    /* 46 - my_getpass() returns fail */
  CURLE_TOO_MANY_REDIRECTS ,     /* 47 - catch endless re-direct loops */
  CURLE_UNKNOWN_TELNET_OPTION,   /* 48 - User specified an unknown option */
  CURLE_TELNET_OPTION_SYNTAX ,   /* 49 - Malformed telnet option */
  CURLE_OBSOLETE,	         /* 50 - removed after 7.7.3 */
  CURLE_SSL_PEER_CERTIFICATE,    /* 51 - peer's certificate wasn't ok */
  CURLE_GOT_NOTHING,             /* 52 - when this is a specific error */
  CURLE_SSL_ENGINE_NOTFOUND,     /* 53 - SSL crypto engine not found */
  CURLE_SSL_ENGINE_SETFAILED,    /* 54 - can not set SSL crypto engine as default */

  CURL_LAST /* never use! */
} CURLcode;

/* this was the error code 50 in 7.7.3 and a few earlier versions, this
   is no longer used by libcurl but is instead #defined here only to not
   make programs break */
#define CURLE_ALREADY_COMPLETE 99999

/* This is just to make older programs not break: */
#define CURLE_FTP_PARTIAL_FILE CURLE_PARTIAL_FILE

#define CURL_ERROR_SIZE 256

/* name is uppercase CURLOPT_<name>,
   type is one of the defined CURLOPTTYPE_<type>
   number is unique identifier */
#ifdef CINIT
#undef CINIT
#endif
#define CINIT(name,type,number) CURLOPT_ ## name = CURLOPTTYPE_ ## type + number

/* long may be 32 or 64 bits, but we should never depend on anything else
   but 32 */
#define CURLOPTTYPE_LONG          0
#define CURLOPTTYPE_OBJECTPOINT   10000
#define CURLOPTTYPE_FUNCTIONPOINT 20000

typedef enum {
  CINIT(NOTHING, LONG, 0), /********* the first one is unused ************/
  
  /* This is the FILE * or void * the regular output should be written to. */
  CINIT(FILE, OBJECTPOINT, 1),

  /* The full URL to get/put */
  CINIT(URL,  OBJECTPOINT, 2),

  /* Port number to connect to, if other than default. Specify the CONF_PORT
     flag in the CURLOPT_FLAGS to activate this */
  CINIT(PORT, LONG, 3),

  /* Name of proxy to use. Specify the CONF_PROXY flag in the CURLOPT_FLAGS to
     activate this */
  CINIT(PROXY, OBJECTPOINT, 4),
  
  /* Name and password to use when fetching. Specify the CONF_USERPWD flag in
     the CURLOPT_FLAGS to activate this */
  CINIT(USERPWD, OBJECTPOINT, 5),

  /* Name and password to use with Proxy. Specify the CONF_PROXYUSERPWD 
     flag in the CURLOPT_FLAGS to activate this */
  CINIT(PROXYUSERPWD, OBJECTPOINT, 6),

  /* Range to get, specified as an ASCII string. Specify the CONF_RANGE flag
     in the CURLOPT_FLAGS to activate this */
  CINIT(RANGE, OBJECTPOINT, 7),

  /* not used */

  /* Specified file stream to upload from (use as input): */
  CINIT(INFILE, OBJECTPOINT, 9),

  /* Buffer to receive error messages in, must be at least CURL_ERROR_SIZE
   * bytes big. If this is not used, error messages go to stderr instead: */
  CINIT(ERRORBUFFER, OBJECTPOINT, 10),

  /* Function that will be called to store the output (instead of fwrite). The
   * parameters will use fwrite() syntax, make sure to follow them. */
  CINIT(WRITEFUNCTION, FUNCTIONPOINT, 11),

  /* Function that will be called to read the input (instead of fread). The
   * parameters will use fread() syntax, make sure to follow them. */
  CINIT(READFUNCTION, FUNCTIONPOINT, 12),

  /* Time-out the read operation after this amount of seconds */
  CINIT(TIMEOUT, LONG, 13),

  /* If the CURLOPT_INFILE is used, this can be used to inform libcurl about
   * how large the file being sent really is. That allows better error
   * checking and better verifies that the upload was succcessful. -1 means
   * unknown size. */
  CINIT(INFILESIZE, LONG, 14),

  /* POST input fields. */
  CINIT(POSTFIELDS, OBJECTPOINT, 15),

  /* Set the referer page (needed by some CGIs) */
  CINIT(REFERER, OBJECTPOINT, 16),

  /* Set the FTP PORT string (interface name, named or numerical IP address)
     Use i.e '-' to use default address. */
  CINIT(FTPPORT, OBJECTPOINT, 17),

  /* Set the User-Agent string (examined by some CGIs) */
  CINIT(USERAGENT, OBJECTPOINT, 18),

  /* If the download receives less than "low speed limit" bytes/second
   * during "low speed time" seconds, the operations is aborted.
   * You could i.e if you have a pretty high speed connection, abort if
   * it is less than 2000 bytes/sec during 20 seconds.   
   */

  /* Set the "low speed limit" */
  CINIT(LOW_SPEED_LIMIT, LONG , 19),

  /* Set the "low speed time" */
  CINIT(LOW_SPEED_TIME, LONG, 20),

  /* Set the continuation offset */
  CINIT(RESUME_FROM, LONG, 21),

  /* Set cookie in request: */
  CINIT(COOKIE, OBJECTPOINT, 22),

  /* This points to a linked list of headers, struct curl_slist kind */
  CINIT(HTTPHEADER, OBJECTPOINT, 23),

  /* This points to a linked list of post entries, struct HttpPost */
  CINIT(HTTPPOST, OBJECTPOINT, 24),

  /* name of the file keeping your private SSL-certificate */
  CINIT(SSLCERT, OBJECTPOINT, 25),

  /* password for the SSL-private key, keep this for compatibility */
  CINIT(SSLCERTPASSWD, OBJECTPOINT, 26),
  /* password for the SSL private key */
  CINIT(SSLKEYPASSWD, OBJECTPOINT, 26),
  
  /* send TYPE parameter? */
  CINIT(CRLF, LONG, 27),

  /* send linked-list of QUOTE commands */
  CINIT(QUOTE, OBJECTPOINT, 28),

  /* send FILE * or void * to store headers to, if you use a callback it
     is simply passed to the callback unmodified */
  CINIT(WRITEHEADER, OBJECTPOINT, 29),

  /* point to a file to read the initial cookies from, also enables
     "cookie awareness" */
  CINIT(COOKIEFILE, OBJECTPOINT, 31),

  /* What version to specifly try to use.
     See CURL_SSLVERSION defines below. */
  CINIT(SSLVERSION, LONG, 32),

  /* What kind of HTTP time condition to use, see defines */
  CINIT(TIMECONDITION, LONG, 33),

  /* Time to use with the above condition. Specified in number of seconds
     since 1 Jan 1970 */
  CINIT(TIMEVALUE, LONG, 34),

  /* HTTP request, for odd commands like DELETE, TRACE and others */
  /* OBSOLETE DEFINE, left for tradition only */
  CINIT(HTTPREQUEST, OBJECTPOINT, 35),

  /* Custom request, for customizing the get command like
     HTTP: DELETE, TRACE and others
     FTP: to use a different list command
     */
  CINIT(CUSTOMREQUEST, OBJECTPOINT, 36),

  /* HTTP request, for odd commands like DELETE, TRACE and others */
  CINIT(STDERR, OBJECTPOINT, 37),

  /* 38 is not used */

  /* send linked-list of post-transfer QUOTE commands */
  CINIT(POSTQUOTE, OBJECTPOINT, 39),

  /* Pass a pointer to string of the output using full variable-replacement
     as described elsewhere. */
  CINIT(WRITEINFO, OBJECTPOINT, 40),

  /* Previous FLAG bits */
  CINIT(VERBOSE, LONG, 41),      /* talk a lot */
  CINIT(HEADER, LONG, 42),       /* throw the header out too */
  CINIT(NOPROGRESS, LONG, 43),   /* shut off the progress meter */
  CINIT(NOBODY, LONG, 44),       /* use HEAD to get http document */
  CINIT(FAILONERROR, LONG, 45),  /* no output on http error codes >= 300 */
  CINIT(UPLOAD, LONG, 46),       /* this is an upload */
  CINIT(POST, LONG, 47),         /* HTTP POST method */
  CINIT(FTPLISTONLY, LONG, 48),  /* Use NLST when listing ftp dir */

  CINIT(FTPAPPEND, LONG, 50),    /* Append instead of overwrite on upload! */
  CINIT(NETRC, LONG, 51),        /* read user+password from .netrc */
  CINIT(FOLLOWLOCATION, LONG, 52),  /* use Location: Luke! */

  /* This FTPASCII name is now obsolete, to be removed, use the TRANSFERTEXT
     instead. It goes for more protocols than just ftp... */
  CINIT(FTPASCII, LONG, 53),     /* use TYPE A for transfer */

  CINIT(TRANSFERTEXT, LONG, 53), /* transfer data in text/ASCII format */
  CINIT(PUT, LONG, 54),          /* PUT the input file */

  CINIT(MUTE, LONG, 55),         /* OBSOLETE OPTION, removed in 7.8 */

  /* Function that will be called instead of the internal progress display
   * function. This function should be defined as the curl_progress_callback
   * prototype defines. */
  CINIT(PROGRESSFUNCTION, FUNCTIONPOINT, 56),

  /* Data passed to the progress callback */
  CINIT(PROGRESSDATA, OBJECTPOINT, 57),

  /* We want the referer field set automatically when following locations */
  CINIT(AUTOREFERER, LONG, 58),

  /* Port of the proxy, can be set in the proxy string as well with:
     "[host]:[port]" */
  CINIT(PROXYPORT, LONG, 59),

  /* size of the POST input data, if strlen() is not good to use */
  CINIT(POSTFIELDSIZE, LONG, 60),

  /* tunnel non-http operations through a HTTP proxy */
  CINIT(HTTPPROXYTUNNEL, LONG, 61),

  /* Set the interface string to use as outgoing network interface */
  CINIT(INTERFACE, OBJECTPOINT, 62),

  /* Set the krb4 security level, this also enables krb4 awareness.  This is a
   * string, 'clear', 'safe', 'confidential' or 'private'.  If the string is
   * set but doesn't match one of these, 'private' will be used.  */
  CINIT(KRB4LEVEL, OBJECTPOINT, 63),

  /* Set if we should verify the peer in ssl handshake, set 1 to verify. */
  CINIT(SSL_VERIFYPEER, LONG, 64),
  
  /* The CApath or CAfile used to validate the peer certificate
     this option is used only if SSL_VERIFYPEER is true */
  CINIT(CAINFO, OBJECTPOINT, 65),

  /* Function pointer to replace the internal password prompt */
  CINIT(PASSWDFUNCTION, FUNCTIONPOINT, 66),

  /* Custom pointer that gets passed as first argument to the password
     function */
  CINIT(PASSWDDATA, OBJECTPOINT, 67),
  
  /* Maximum number of http redirects to follow */
  CINIT(MAXREDIRS, LONG, 68),

  /* Pass a pointer to a time_t to get a possible date of the requested
     document! Pass a NULL to shut it off. */
  CINIT(FILETIME, OBJECTPOINT, 69),

  /* This points to a linked list of telnet options */
  CINIT(TELNETOPTIONS, OBJECTPOINT, 70),

  /* Max amount of cached alive connections */
  CINIT(MAXCONNECTS, LONG, 71),

  /* What policy to use when closing connections when the cache is filled
     up */
  CINIT(CLOSEPOLICY, LONG, 72),

  /* Callback to use when CURLCLOSEPOLICY_CALLBACK is set */
  CINIT(CLOSEFUNCTION, FUNCTIONPOINT, 73),

  /* Set to explicitly use a new connection for the upcoming transfer.
     Do not use this unless you're absolutely sure of this, as it makes the
     operation slower and is less friendly for the network. */
  CINIT(FRESH_CONNECT, LONG, 74),

  /* Set to explicitly forbid the upcoming transfer's connection to be re-used
     when done. Do not use this unless you're absolutely sure of this, as it
     makes the operation slower and is less friendly for the network. */
  CINIT(FORBID_REUSE, LONG, 75),

  /* Set to a file name that contains random data for libcurl to use to
     seed the random engine when doing SSL connects. */
  CINIT(RANDOM_FILE, OBJECTPOINT, 76),

  /* Set to the Entropy Gathering Daemon socket pathname */
  CINIT(EGDSOCKET, OBJECTPOINT, 77),

  /* Time-out connect operations after this amount of seconds, if connects
     are OK within this time, then fine... This only aborts the connect
     phase. [Only works on unix-style/SIGALRM operating systems] */
  CINIT(CONNECTTIMEOUT, LONG, 78),

  /* Function that will be called to store headers (instead of fwrite). The
   * parameters will use fwrite() syntax, make sure to follow them. */
  CINIT(HEADERFUNCTION, FUNCTIONPOINT, 79),

  /* Set this to force the HTTP request to get back to GET. Only really usable
     if POST, PUT or a custom request have been used first.
   */
  CINIT(HTTPGET, LONG, 80),

  /* Set if we should verify the Common name from the peer certificate in ssl
   * handshake, set 1 to check existence, 2 to ensure that it matches the
   * provided hostname. */
  CINIT(SSL_VERIFYHOST, LONG, 81),

  /* Specify which file name to write all known cookies in after completed
     operation. Set file name to "-" (dash) to make it go to stdout. */
  CINIT(COOKIEJAR, OBJECTPOINT, 82),

  /* Specify which SSL ciphers to use */
  CINIT(SSL_CIPHER_LIST, OBJECTPOINT, 83),

  /* Specify which HTTP version to use! This must be set to one of the
     CURL_HTTP_VERSION* enums set below. */
  CINIT(HTTP_VERSION, LONG, 84),

  /* Specificly switch on or off the FTP engine's use of the EPSV command. By
     default, that one will always be attempted before the more traditional
     PASV command. */     
  CINIT(FTP_USE_EPSV, LONG, 85),

  /* type of the file keeping your SSL-certificate ("DER", "PEM", "ENG") */
  CINIT(SSLCERTTYPE, OBJECTPOINT, 86),

  /* name of the file keeping your private SSL-key */
  CINIT(SSLKEY, OBJECTPOINT, 87),

  /* type of the file keeping your private SSL-key ("DER", "PEM", "ENG") */
  CINIT(SSLKEYTYPE, OBJECTPOINT, 88),

  /* crypto engine for the SSL-sub system */
  CINIT(SSLENGINE, OBJECTPOINT, 89),

  /* set the crypto engine for the SSL-sub system as default
     the param has no meaning...
   */
  CINIT(SSLENGINE_DEFAULT, LONG, 90),

  /* Non-zero value means to use the global dns cache */
  CINIT(DNS_USE_GLOBAL_CACHE, LONG, 91),

  /* DNS cache timeout */
  CINIT(DNS_CACHE_TIMEOUT, LONG, 92),

  /* send linked-list of pre-transfer QUOTE commands (Wesley Laxton)*/
  CINIT(PREQUOTE, OBJECTPOINT, 93),
  
  CURLOPT_LASTENTRY /* the last unusued */
} CURLoption;

  /* These enums are for use with the CURLOPT_HTTP_VERSION option. */
enum {
  CURL_HTTP_VERSION_NONE, /* setting this means we don't care, and that we'd
                             like the library to choose the best possible
                             for us! */
  CURL_HTTP_VERSION_1_0,  /* please use HTTP 1.0 in the request */
  CURL_HTTP_VERSION_1_1,  /* please use HTTP 1.1 in the request */
  
  CURL_HTTP_VERSION_LAST /* *ILLEGAL* http version */
};

enum {
  CURL_SSLVERSION_DEFAULT,
  CURL_SSLVERSION_TLSv1,
  CURL_SSLVERSION_SSLv2,
  CURL_SSLVERSION_SSLv3,

  CURL_SSLVERSION_LAST /* never use, keep last */
};


typedef enum {
  TIMECOND_NONE,

  TIMECOND_IFMODSINCE,
  TIMECOND_IFUNMODSINCE,
  TIMECOND_LASTMOD,

  TIMECOND_LAST
} curl_TimeCond;

#ifdef __BEOS__
#include <support/SupportDefs.h>
#endif


/* These functions are in the libcurl, they're here for portable reasons and
   they are used by the 'curl' client. They really should be moved to some kind
   of "portability library" since it has nothing to do with file transfers and
   might be usable to other programs...

   NOTE: they return TRUE if the strings match *case insensitively*.
 */
extern int (curl_strequal)(const char *s1, const char *s2);
extern int (curl_strnequal)(const char *s1, const char *s2, size_t n);
#define strequal(a,b) curl_strequal(a,b)
#define strnequal(a,b,c) curl_strnequal(a,b,c)

/* external form function */
int curl_formparse(char *string,
                   struct HttpPost **httppost,
                   struct HttpPost **last_post);

/* name is uppercase CURLFORM_<name> */
#ifdef CFINIT
#undef CFINIT
#endif
#define CFINIT(name) CURLFORM_ ## name

typedef enum {
  CFINIT(NOTHING),        /********* the first one is unused ************/
  
  /*  */
  CFINIT(COPYNAME),
  CFINIT(PTRNAME),
  CFINIT(NAMELENGTH),
  CFINIT(COPYCONTENTS),
  CFINIT(PTRCONTENTS),
  CFINIT(CONTENTSLENGTH),
  CFINIT(FILECONTENT),
  CFINIT(ARRAY),
  CFINIT(ARRAY_START), /* below are the options allowed within a array */
  CFINIT(FILE),
  CFINIT(CONTENTTYPE),
  CFINIT(CONTENTHEADER),
  CFINIT(END),
  CFINIT(ARRAY_END),   /* up are the options allowed within a array */

  CURLFORM_LASTENTRY /* the last unusued */
} CURLformoption;

/* structure to be used as parameter for CURLFORM_ARRAY */
struct curl_forms {
	CURLformoption		option;
	const char		*value;
};

/* new external form function */
int curl_formadd(struct HttpPost **httppost,
                 struct HttpPost **last_post,
                 ...);

/* cleanup a form: */
void curl_formfree(struct HttpPost *form);

/* Unix and Win32 getenv function call, this returns a malloc()'ed string that
   MUST be free()ed after usage is complete. */
char *curl_getenv(const char *variable);

/* Returns a static ascii string of the libcurl version. */
char *curl_version(void);

/* Escape and unescape URL encoding in strings. The functions return a new
 * allocated string or NULL if an error occurred.  */
char *curl_escape(const char *string, int length);
char *curl_unescape(const char *string, int length);

/* curl_global_init() should be invoked exactly once for each application that
   uses libcurl */
CURLcode curl_global_init(long flags);

/* curl_global_cleanup() should be invoked exactly once for each application
   that uses libcurl */
void curl_global_cleanup(void);

/* This is the version number */
#define LIBCURL_VERSION "7.9.5"
#define LIBCURL_VERSION_NUM 0x070905

/* linked-list structure for the CURLOPT_QUOTE option (and other) */
struct curl_slist {
	char			*data;
	struct curl_slist	*next;
};

struct curl_slist *curl_slist_append(struct curl_slist *, const char *);
void curl_slist_free_all(struct curl_slist *);

/*
 * NAME curl_getdate()
 *
 * DESCRIPTION
 *
 * Returns the time, in seconds since 1 Jan 1970 of the time string given in
 * the first argument. The time argument in the second parameter is for cases
 * where the specified time is relative now, like 'two weeks' or 'tomorrow'
 * etc.
 */
time_t curl_getdate(const char *p, const time_t *now);


#define CURLINFO_STRING   0x100000
#define CURLINFO_LONG     0x200000
#define CURLINFO_DOUBLE   0x300000
#define CURLINFO_MASK     0x0fffff
#define CURLINFO_TYPEMASK 0xf00000

typedef enum {
  CURLINFO_NONE, /* first, never use this */
  CURLINFO_EFFECTIVE_URL    = CURLINFO_STRING + 1,
  CURLINFO_HTTP_CODE        = CURLINFO_LONG   + 2,
  CURLINFO_TOTAL_TIME       = CURLINFO_DOUBLE + 3,
  CURLINFO_NAMELOOKUP_TIME  = CURLINFO_DOUBLE + 4,
  CURLINFO_CONNECT_TIME     = CURLINFO_DOUBLE + 5,
  CURLINFO_PRETRANSFER_TIME = CURLINFO_DOUBLE + 6,
  CURLINFO_SIZE_UPLOAD      = CURLINFO_DOUBLE + 7,
  CURLINFO_SIZE_DOWNLOAD    = CURLINFO_DOUBLE + 8,
  CURLINFO_SPEED_DOWNLOAD   = CURLINFO_DOUBLE + 9,
  CURLINFO_SPEED_UPLOAD     = CURLINFO_DOUBLE + 10,
  CURLINFO_HEADER_SIZE      = CURLINFO_LONG   + 11,
  CURLINFO_REQUEST_SIZE     = CURLINFO_LONG   + 12,
  CURLINFO_SSL_VERIFYRESULT = CURLINFO_LONG   + 13,
  CURLINFO_FILETIME         = CURLINFO_LONG   + 14,

  CURLINFO_CONTENT_LENGTH_DOWNLOAD   = CURLINFO_DOUBLE + 15,
  CURLINFO_CONTENT_LENGTH_UPLOAD     = CURLINFO_DOUBLE + 16,

  CURLINFO_STARTTRANSFER_TIME = CURLINFO_DOUBLE + 17,

  CURLINFO_CONTENT_TYPE = CURLINFO_STRING + 18,

  /* Fill in new entries here! */

  CURLINFO_LASTONE          = 19
} CURLINFO;

/* unfortunately, the easy.h include file needs the options and info stuff
   before it can be included! */
#include <curl/easy.h> /* nothing in curl is fun without the easy stuff */

typedef enum {
  CURLCLOSEPOLICY_NONE, /* first, never use this */

  CURLCLOSEPOLICY_OLDEST,
  CURLCLOSEPOLICY_LEAST_RECENTLY_USED,
  CURLCLOSEPOLICY_LEAST_TRAFFIC,
  CURLCLOSEPOLICY_SLOWEST,
  CURLCLOSEPOLICY_CALLBACK,
 
  CURLCLOSEPOLICY_LAST /* last, never use this */
} curl_closepolicy;

#define CURL_GLOBAL_SSL (1<<0)
#define CURL_GLOBAL_WIN32 (1<<1)
#define CURL_GLOBAL_ALL (CURL_GLOBAL_SSL|CURL_GLOBAL_WIN32)
#define CURL_GLOBAL_NOTHING 0
#define CURL_GLOBAL_DEFAULT CURL_GLOBAL_ALL

#ifdef  __cplusplus
}
#endif

#endif /* __CURL_CURL_H */
