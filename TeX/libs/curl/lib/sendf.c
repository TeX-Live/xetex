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
 * $Id: sendf.c,v 1.44 2002/02/05 15:33:00 bagder Exp $
 *****************************************************************************/

#include "setup.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>	/* required for send() & recv() prototypes */
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(WIN32) && !defined(__GNUC__) || defined(__MINGW32__)
#include <winsock.h>
#endif

#include <curl/curl.h>
#include "urldata.h"
#include "sendf.h"

#define _MPRINTF_REPLACE /* use the internal *printf() functions */
#include <curl/mprintf.h>

#ifdef KRB4
#include "security.h"
#endif
#include <string.h>
/* The last #include file should be: */
#ifdef MALLOCDEBUG
#include "memdebug.h"
#endif

/* returns last node in linked list */
static struct curl_slist *slist_get_last(struct curl_slist *list)
{
	struct curl_slist	*item;

	/* if caller passed us a NULL, return now */
	if (!list)
		return NULL;

	/* loop through to find the last item */
	item = list;
	while (item->next) {
		item = item->next;
	}
	return item;
}

/* append a struct to the linked list. It always retunrs the address of the
 * first record, so that you can sure this function as an initialization
 * function as well as an append function. If you find this bothersome,
 * then simply create a separate _init function and call it appropriately from
 * within the proram. */
struct curl_slist *curl_slist_append(struct curl_slist *list,
                                     const char *data)
{
	struct curl_slist	*last;
	struct curl_slist	*new_item;

	new_item = (struct curl_slist *) malloc(sizeof(struct curl_slist));
	if (new_item) {
		new_item->next = NULL;
		new_item->data = strdup(data);
	}
	else {
		fprintf(stderr, "Cannot allocate memory for QUOTE list.\n");
		return NULL;
	}

	if (list) {
		last = slist_get_last(list);
		last->next = new_item;
		return list;
	}

	/* if this is the first item, then new_item *is* the list */
	return new_item;
}

/* be nice and clean up resources */
void curl_slist_free_all(struct curl_slist *list)
{
	struct curl_slist	*next;
	struct curl_slist	*item;

	if (!list)
		return;

	item = list;
	do {
		next = item->next;
		
		if (item->data) {
			free(item->data);
		}
		free(item);
		item = next;
	} while (next);
}


/* Curl_infof() is for info message along the way */

void Curl_infof(struct SessionHandle *data, const char *fmt, ...)
{
  va_list ap;
  if(data->set.verbose) {
    va_start(ap, fmt);
    fputs("* ", data->set.err);
    vfprintf(data->set.err, fmt, ap);
    va_end(ap);
  }
}

/* Curl_failf() is for messages stating why we failed.
 * The message SHALL NOT include any LF or CR.
 */

void Curl_failf(struct SessionHandle *data, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  if(data->set.errorbuffer && !data->state.errorbuf) {
    vsnprintf(data->set.errorbuffer, CURL_ERROR_SIZE, fmt, ap);
    data->state.errorbuf = TRUE; /* wrote error string */
  }
  va_end(ap);
}

/* Curl_sendf() sends formated data to the server */
CURLcode Curl_sendf(int sockfd, struct connectdata *conn,
                    const char *fmt, ...)
{
  struct SessionHandle *data = conn->data;
  ssize_t bytes_written;
  CURLcode result;
  char *s;
  va_list ap;
  va_start(ap, fmt);
  s = vaprintf(fmt, ap); /* returns an allocated string */
  va_end(ap);
  if(!s)
    return 0; /* failure */
  if(data->set.verbose)
    fprintf(data->set.err, "> %s", s);

  /* Write the buffer to the socket */
  result = Curl_write(conn, sockfd, s, strlen(s), &bytes_written);

  free(s); /* free the output string */

  return result;
}

/*
 * Curl_write() is an internal write function that sends plain (binary) data
 * to the server. Works with plain sockets, SSL or kerberos.
 *
 */
CURLcode Curl_write(struct connectdata *conn, int sockfd,
                    void *mem, size_t len,
                    ssize_t *written)
{
  ssize_t bytes_written;

#ifdef USE_SSLEAY
  /* SSL_write() is said to return 'int' while write() and send() returns
     'size_t' */
  if (conn->ssl.use) {
    int err;
    int rc = SSL_write(conn->ssl.handle, mem, len);

    if(rc < 0) {
      err = SSL_get_error(conn->ssl.handle, rc);
    
      switch(err) {
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        /* this is basicly the EWOULDBLOCK equivalent */
        *written = 0;
        return CURLE_OK;
      }
      /* a true error */
      failf(conn->data, "SSL_write() return error %d\n", err);
      return CURLE_WRITE_ERROR;
    }
    bytes_written = rc;
  }
  else {
#endif
#ifdef KRB4
    if(conn->sec_complete) {
      bytes_written = Curl_sec_write(conn, sockfd, mem, len);
    }
    else
#endif /* KRB4 */
    {
      bytes_written = swrite(sockfd, mem, len);
    }
    if(-1 == bytes_written) {
#ifdef WIN32
      if(WSAEWOULDBLOCK == GetLastError())
#else
      if(EWOULDBLOCK == errno)
#endif
      {
        /* this is just a case of EWOULDBLOCK */
        *written=0;
        return CURLE_OK;
      }
    }
#ifdef USE_SSLEAY
  }
#endif

  *written = bytes_written;
  return (-1 != bytes_written)?CURLE_OK:CURLE_WRITE_ERROR;
}

/* client_write() sends data to the write callback(s)

   The bit pattern defines to what "streams" to write to. Body and/or header.
   The defines are in sendf.h of course.
 */
CURLcode Curl_client_write(struct SessionHandle *data,
                           int type,
                           char *ptr,
                           size_t len)
{
  size_t wrote;

  if(0 == len)
    len = strlen(ptr);

  if(type & CLIENTWRITE_BODY) {
    wrote = data->set.fwrite(ptr, 1, len, data->set.out);
    if(wrote != len) {
      failf (data, "Failed writing body");
      return CURLE_WRITE_ERROR;
    }
  }
  if((type & CLIENTWRITE_HEADER) &&
     (data->set.fwrite_header || data->set.writeheader) ) {
    /*
     * Write headers to the same callback or to the especially setup
     * header callback function (added after version 7.7.1).
     */
    curl_write_callback writeit=
      data->set.fwrite_header?data->set.fwrite_header:data->set.fwrite;

    wrote = writeit(ptr, 1, len, data->set.writeheader);
    if(wrote != len) {
      failf (data, "Failed writing header");
      return CURLE_WRITE_ERROR;
    }
  }
  
  return CURLE_OK;
}

/*
 * Internal read-from-socket function. This is meant to deal with plain
 * sockets, SSL sockets and kerberos sockets.
 *
 * If the read would block (EWOULDBLOCK) we return -1. Otherwise we return
 * a regular CURLcode value.
 */
int Curl_read(struct connectdata *conn,
              int sockfd,
              char *buf,
              size_t buffersize,
              ssize_t *n)
{
  ssize_t nread;

#ifdef USE_SSLEAY
  if (conn->ssl.use) {
    bool loop=TRUE;
    int err;
    do {
      nread = SSL_read(conn->ssl.handle, buf, buffersize);

      if(nread >= 0)
        /* successful read */
        break;

      err = SSL_get_error(conn->ssl.handle, nread);

      switch(err) {
      case SSL_ERROR_NONE: /* this is not an error */
      case SSL_ERROR_ZERO_RETURN: /* no more data */
        loop=0; /* get out of loop */
        break;
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        /* if there's data pending, then we re-invoke SSL_read() */
        break;
      }
    } while(loop);
    if(loop && SSL_pending(conn->ssl.handle))
      return -1; /* basicly EWOULDBLOCK */
  }
  else {
#endif
#ifdef KRB4
    if(conn->sec_complete)
      nread = Curl_sec_read(conn, sockfd, buf, buffersize);
    else
#endif
      nread = sread (sockfd, buf, buffersize);

    if(-1 == nread) {
#ifdef WIN32
      if(WSAEWOULDBLOCK == GetLastError())
#else
      if(EWOULDBLOCK == errno)
#endif
        return -1;
    }

#ifdef USE_SSLEAY
  }
#endif /* USE_SSLEAY */
  *n = nread;
  return CURLE_OK;
}


/*
 * local variables:
 * eval: (load-file "../curl-mode.el")
 * end:
 * vim600: fdm=marker
 * vim: et sw=2 ts=2 sts=2 tw=78
 */
