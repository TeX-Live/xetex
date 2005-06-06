/*****************************************************************************
 *                                  _   _ ____  _     
 *  Project                     ___| | | |  _ \| |    
 *                             / __| | | | |_) | |    
 *                            | (__| |_| |  _ <| |___ 
 *                             \___|\___/|_| \_\_____|
 *
 * $Id: sepheaders.c,v 1.3 2001/05/15 12:55:35 bagder Exp $
 */

/* to make this work under windows, use the win32-functions from the
   win32socket.c file as well */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

int main(int argc, char **argv)
{
  CURL *curl_handle;
  char *headerfilename = "head.out";
  FILE *headerfile;
  char *bodyfilename = "body.out";
  FILE *bodyfile;

  /* init the curl session */
  curl_handle = curl_easy_init();

  /* set URL to get */
  curl_easy_setopt(curl_handle, CURLOPT_URL, "http://curl.haxx.se");

  /* no progress meter please */
  curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1);

  /* shut up completely */
  curl_easy_setopt(curl_handle, CURLOPT_MUTE, 1);

  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

  /* open the files */
  headerfile = fopen(headerfilename,"w");
  if (headerfile == NULL) {
    curl_easy_cleanup(curl_handle);
    return -1;
  }
  bodyfile = fopen(bodyfilename,"w");
  if (bodyfile == NULL) {
    curl_easy_cleanup(curl_handle);
    return -1;
  }

  /* we want the headers to this file handle */
  curl_easy_setopt(curl_handle,   CURLOPT_WRITEHEADER ,headerfile);

  /* get it! */
  curl_easy_perform(curl_handle);

  /* close the header file */
  fclose(headerfile);

  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);

  return 0;
}
