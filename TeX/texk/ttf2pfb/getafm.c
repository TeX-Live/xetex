/* getafm.c: this program get the afm metrics file 
   from the font file specified.

Copyright (C) 1997 Fabrice POPINEAU.

Time-stamp: <03/01/19 10:24:52 popineau>

*/

#include <win32lib.h>
#include <gs32lib.h>
#include <kpathsea/config.h>
#include <kpathsea/c-fopen.h>
#include <kpathsea/lib.h>
#include <kpathsea/tex-file.h>
#include <kpathsea/progname.h>
#include <kpathsea/line.h>

unsigned char *gs_device = NULL;

void usage()
{
  fprintf(stderr, "usage: %s font-name > font-name.afm\n", kpse_program_name);
}

int __cdecl
gsdll_callback(int message, char *str, unsigned long count)
{
  
  char *p;
  
  switch (message) {
  case GSDLL_STDIN:
    p = fgets(str, count, stdin);
#if _DEBUG
    fprintf(stderr, "gs read: %s\n", p);
#endif
    if (p)
      return strlen(str);
    else
      return 0;
  case GSDLL_STDOUT:
    if (str != (char *)NULL) {
      fwrite(str, 1, count, stdout);
    }
    return count;
  case GSDLL_DEVICE:
    gs_device = str;
#if _DEBUG
    fprintf(stdout,"Callback: DEVICE %p %s\n", str,
	    count ? "open" : "close");
#endif
    break;
  case GSDLL_SYNC:
#if _DEBUG
    fprintf(stdout,"Callback: SYNC %p\n", str);
#endif
    break;
  case GSDLL_PAGE:
    fprintf(stdout,"Callback: PAGE %p\n", str);
    break;
  case GSDLL_SIZE:
#if _DEBUG
    fprintf(stdout,"Callback: SIZE %p width=%d height=%d\n", str,
	    (int)(count & 0xffff), (int)((count>>16) & 0xffff) );
#endif
    break;
  case GSDLL_POLL:
#if _DEBUG
    fprintf(stderr, "GS: Poll sent!\n");
#endif
    return 0; /* no error */
  default:
    fprintf(stdout,"Callback: Unknown message=%d\n",message);
    break;
  }
  return 0;
}

void
gs_io(const char *cp)
{
  int ret;

#if 0
  if (cp && !(*cp == '\n' && *(cp+1) == '\0'))
    fprintf(stderr, "gs: sending %s\n", cp);
#endif
  if ((ret = (*pgsdll_execute_cont)(cp, strlen(cp))) != 0) {
    fprintf(stderr, "gs: error in executing\n%s\n", cp);
    if (ret <= -100) {
      fprintf(stderr, "gs: fatal error, exiting\n");
      (*pgsdll_exit)();
    }
    else if (ret < 0) {
      fprintf(stderr, "gs: error, exiting\n");
      (*pgsdll_execute_end)();
      (*pgsdll_exit)();
    }
  }
}

void
feed_gs_with (char *filename)
{
  FILE *f;
  static char buf[1024];
  int c, c_prev;
  unsigned long i = 0, doseps = 0, dosepsbegin = 0, dosepsend = 0;

  if (!filename) {
    fprintf(stderr, "Can't feed gs with (null)!\n");
    return;
  }
  f = fopen(filename, FOPEN_RBIN_MODE);
  if (!f) {
    fprintf(stderr, "Can't feed gs with %s: %s\n", filename, strerror(errno));
    return;
  }
#if 0
  fprintf(stderr, "opening %s\n", filename);
#endif

  i = 0; c_prev = '\0';
  while ((c = fgetc(f)) != EOF) {
    if (c == '\n') {
      if (c_prev == '\r') {
	i--;
      }
      buf[i++] = '\n';
      buf[i] = '\0';
      gs_io(buf);
      i = 0;
      c_prev = '\0';
    }
    else if (i == sizeof(buf) - 1) {
      buf[i] = '\0';
      gs_io(buf);
      i = 0;
      buf[i] = c;
      c_prev = c;
    }
    else {
      buf[i++] = c;
      c_prev = c;
    }
  }
  if (!feof(f)) {
    fprintf(stderr, "Can't feed gs with %s: %s\n", filename, strerror(errno));
  }
  fclose(f);
#if 0
  fprintf(stderr, "closing %s\n", filename);
#endif
}

main(int argc, char *argv[])
{
  string gs_argv[] = { "gswin32c.exe",
		       "-q",
		       "-dNODISPLAY",
		       "-sOutputFile=-",
		       "-dNOEPS",
		       "-dNOPAUSE",
		       NULL
  };
  string getafm_psfile;
  int ret;
  char cmd[256];

  kpse_set_progname(argv[0]);

  if (argc != 2) {
    usage();
    return EXIT_FAILURE;
  }

  if ((getafm_psfile = kpse_find_file("getafm.ps", kpse_tex_ps_header_format, true)) == NULL) {
    fprintf(stderr, "Can't find `getafm.ps' file.\n");
    return EXIT_FAILURE;
  }

  if (gs_locate() == NULL) {
    fprintf(stderr, "Can't locate Ghostscript ! Exiting ...\n");
    return EXIT_FAILURE;
  }

  gs_dll_initialize();

  if ((ret = (*pgsdll_init)(gsdll_callback, 
			    NULL, 
			    sizeof(gs_argv) / sizeof(char *) - 1,
			    gs_argv)) != 0) {
    fprintf(stderr, "gsdll_init returned %d\n", ret);
    gs_dll_release();
    return EXIT_FAILURE;
  }

  (*pgsdll_execute_begin)();

#if 1
  feed_gs_with(getafm_psfile);
#else
  sprintf(cmd, "(%s) run\n", getafm_psfile);
  gs_io(cmd);
#endif

  sprintf(cmd, "/%s getafm", argv[1]);
  gs_io(cmd);

  (*pgsdll_execute_end)();

  gs_dll_release();

  return EXIT_SUCCESS;
}
