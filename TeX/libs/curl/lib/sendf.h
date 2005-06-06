#ifndef __SENDF_H
#define __SENDF_H
/*****************************************************************************
 *                                  _   _ ____  _     
 *  Project                     ___| | | |  _ \| |    
 *                             / __| | | | |_) | |    
 *                            | (__| |_| |  _ <| |___ 
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2000, Daniel Stenberg, <daniel@haxx.se>, et al.
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
 * $Id: sendf.h,v 1.16 2002/01/16 14:49:51 bagder Exp $
 *****************************************************************************/

CURLcode Curl_sendf(int fd, struct connectdata *, const char *fmt, ...);
void Curl_infof(struct SessionHandle *, const char *fmt, ...);
void Curl_failf(struct SessionHandle *, const char *fmt, ...);

#define infof Curl_infof
#define failf Curl_failf

struct send_buffer {
  char *buffer;
  size_t size_max;
  size_t size_used;
};
typedef struct send_buffer send_buffer;

#define CLIENTWRITE_BODY   1
#define CLIENTWRITE_HEADER 2
#define CLIENTWRITE_BOTH   (CLIENTWRITE_BODY|CLIENTWRITE_HEADER)

CURLcode Curl_client_write(struct SessionHandle *data, int type, char *ptr,
                           size_t len);

/* internal read-function, does plain socket, SSL and krb4 */
int Curl_read(struct connectdata *conn, int sockfd,
              char *buf, size_t buffersize,
              ssize_t *n);
/* internal write-function, does plain socket, SSL and krb4 */
CURLcode Curl_write(struct connectdata *conn, int sockfd,
                    void *mem, size_t len,
                    ssize_t *written);

#endif
