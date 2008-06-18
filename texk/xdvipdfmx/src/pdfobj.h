/*  $Header: /home/cvsroot/dvipdfmx/src/pdfobj.h,v 1.28 2008/06/07 09:54:38 chofchof Exp $

    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2007 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team <dvipdfmx@project.ktug.or.kr>
    
    Copyright (C) 1998, 1999 by Mark A. Wicks <mwicks@kettering.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#ifndef _PDFOBJ_H_
#define _PDFOBJ_H_

#include <stdio.h>

/* Here is the complete list of PDF object types */

#define	PDF_BOOLEAN	1
#define	PDF_NUMBER	2
#define PDF_STRING	3
#define PDF_NAME	4
#define PDF_ARRAY	5
#define PDF_DICT	6
#define PDF_STREAM	7
#define PDF_NULL        8
#define PDF_INDIRECT	9

#define PDF_OBJ_INVALID 0
#define PDF_UNDEFINED   PDF_OBJ_INVALID

#define STREAM_COMPRESS (1 << 0)

typedef struct pdf_obj  pdf_obj;
typedef struct pdf_file pdf_file;

/* External interface to pdf routines */

extern int      pdf_obj_get_verbose (void);
extern void     pdf_obj_set_verbose (void);
extern void     pdf_error_cleanup   (void);

extern void     pdf_out_init      (const char *filename, int do_encryption);
extern void     pdf_out_flush     (void);
extern void     pdf_set_version   (unsigned version);
extern unsigned pdf_get_version   (void);

extern pdf_obj *pdf_new_obj     (int type);
extern void     pdf_release_obj (pdf_obj *object);
extern int      pdf_obj_typeof  (pdf_obj *object);

#define PDF_OBJ_NUMBERTYPE(o)   ((o) && pdf_obj_typeof((o)) == PDF_NUMBER)
#define PDF_OBJ_BOOLEANTYPE(o)  ((o) && pdf_obj_typeof((o)) == PDF_BOOLEAN)
#define PDF_OBJ_STRINGTYPE(o)   ((o) && pdf_obj_typeof((o)) == PDF_STRING)
#define PDF_OBJ_NAMETYPE(o)     ((o) && pdf_obj_typeof((o)) == PDF_NAME)
#define PDF_OBJ_ARRAYTYPE(o)    ((o) && pdf_obj_typeof((o)) == PDF_ARRAY)
#define PDF_OBJ_NULLTYPE(o)     ((o) && pdf_obj_typeof((o)) == PDF_NULL)
#define PDF_OBJ_DICTTYPE(o)     ((o) && pdf_obj_typeof((o)) == PDF_DICT)
#define PDF_OBJ_STREAMTYPE(o)   ((o) && pdf_obj_typeof((o)) == PDF_STREAM)
#define PDF_OBJ_INDIRECTTYPE(o) ((o) && pdf_obj_typeof((o)) == PDF_INDIRECT)

#define PDF_OBJ_TYPEOF(o)       pdf_obj_typeof((o))


extern pdf_obj *pdf_ref_obj        (pdf_obj *object);
extern pdf_obj *pdf_link_obj       (pdf_obj *object);

extern pdf_obj *pdf_new_null       (void);

extern pdf_obj *pdf_new_boolean    (char value);
extern void     pdf_set_boolean    (pdf_obj *object, char value);
extern char     pdf_boolean_value  (pdf_obj *object);

extern pdf_obj *pdf_new_number     (double value);
extern void     pdf_set_number     (pdf_obj *object, double value);
extern double   pdf_number_value   (pdf_obj *number);

extern pdf_obj  *pdf_new_string    (const void *str, unsigned length);
extern void      pdf_set_string    (pdf_obj *object, unsigned char *str, unsigned length);
extern void     *pdf_string_value  (pdf_obj *object);
extern unsigned  pdf_string_length (pdf_obj *object);

/* Name does not include the / */
extern pdf_obj *pdf_new_name   (const char *name);
extern void     pdf_set_name   (pdf_obj *object, const char *name);
extern char    *pdf_name_value (pdf_obj *object);

extern pdf_obj *pdf_new_array     (void);
/* pdf_add_dict requires key but pdf_add_array does not.
 * pdf_add_array always append elements to array.
 * They should be pdf_put_array(array, idx, element) and
 * pdf_put_dict(dict, key, value)
 */
extern void     pdf_add_array     (pdf_obj *array, pdf_obj *object);
extern void     pdf_put_array     (pdf_obj *array, unsigned idx, pdf_obj *object);
extern pdf_obj *pdf_get_array     (pdf_obj *array, long idx);
extern unsigned pdf_array_length  (pdf_obj *array);

extern void     pdf_unshift_array (pdf_obj *array, pdf_obj *object);
extern pdf_obj *pdf_shift_array   (pdf_obj *array);
extern pdf_obj *pdf_pop_array     (pdf_obj *array);

extern pdf_obj *pdf_new_dict    (void);
extern void     pdf_remove_dict (pdf_obj *dict,  const char *key);
extern void     pdf_merge_dict  (pdf_obj *dict1, pdf_obj *dict2);
extern pdf_obj *pdf_lookup_dict (pdf_obj *dict,  const char *key);
extern pdf_obj *pdf_dict_keys   (pdf_obj *dict);

/* pdf_add_dict() want pdf_obj as key, however, key must always be name
 * object and pdf_lookup_dict() and pdf_remove_dict() uses const char as
 * key. This strange difference seems come from pdfdoc that first allocate
 * name objects frequently used (maybe 1000 times) such as /Type and does
 * pdf_link_obj() it rather than allocate/free-ing them each time. But I
 * already removed that.
 */
extern int      pdf_add_dict     (pdf_obj *dict, pdf_obj *key,    pdf_obj *value); 
extern void     pdf_put_dict     (pdf_obj *dict, const char *key, pdf_obj *value); 

/* Apply proc(key, value, pdata) for each key-value pairs in dict, stop if proc()
 * returned non-zero value (and that value is returned). PDF object is passed for
 * key to allow modification (fix) of key.
 */
extern int      pdf_foreach_dict (pdf_obj *dict,
				  int (*proc) (pdf_obj *, pdf_obj *, void *),
				  void *pdata);

extern pdf_obj    *pdf_new_stream        (int flags);
extern void        pdf_add_stream        (pdf_obj *stream,
					  const void *stream_data_ptr,
					  long stream_data_len);
#if HAVE_ZLIB
extern int         pdf_add_stream_flate  (pdf_obj *stream,
					  const void *stream_data_ptr,
					  long stream_data_len);
#endif
extern int         pdf_concat_stream     (pdf_obj *dst, pdf_obj *src);
extern pdf_obj    *pdf_stream_dict       (pdf_obj *stream);
extern long        pdf_stream_length     (pdf_obj *stream);
extern void        pdf_stream_set_flags  (pdf_obj *stream, int flags);
extern int         pdf_stream_get_flags  (pdf_obj *stream);
extern const void *pdf_stream_dataptr    (pdf_obj *stream);

#if 0
extern int         pdf_stream_pop_filter (pdf_obj *stream);
#endif

/* Compare label of two indirect reference object.
 */
extern int         pdf_compare_reference (pdf_obj *ref1, pdf_obj *ref2);

/* The following routines are not appropriate for pdfobj.
 */

extern void      pdf_set_compression (int level);

extern void      pdf_set_info     (pdf_obj *obj);
extern void      pdf_set_root     (pdf_obj *obj);
extern void      pdf_set_encrypt  (pdf_obj *encrypt, pdf_obj *id);

extern void      pdf_files_init    (void);
extern void      pdf_files_close   (void);
extern int      check_for_pdf     (FILE *file);
extern pdf_file *pdf_open          (char *ident, FILE *file);
extern void      pdf_close         (pdf_file *pf);
extern pdf_obj  *pdf_file_get_trailer (pdf_file *pf);

extern pdf_obj *pdf_deref_obj     (pdf_obj *object);
extern pdf_obj *pdf_import_object (pdf_obj *object);

extern int      pdfobj_escape_str (char *buffer, int size, const unsigned char *s, int len);

extern pdf_obj *pdf_new_indirect  (pdf_file *pf, unsigned long label, unsigned short generation);
extern void     pdf_copy_object   (pdf_obj *dst, pdf_obj *src);

#endif  /* _PDFOBJ_H_ */
