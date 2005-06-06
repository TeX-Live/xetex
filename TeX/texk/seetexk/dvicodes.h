/*
 * Copyright (c) 1987, 1989 University of Maryland
 * Department of Computer Science.  All rights reserved.
 * Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

/* DVI opcodes */

#define DVI_VERSION	2	/* version number that should appear in
				   pre- and post-ambles */

#define DVI_SET1	128	/* set character, 1 byte param */
#define DVI_SET2	129	/* set character, 2 byte param */
#define DVI_SET3	130	/* set character, 3 byte param */
#define DVI_SET4	131	/* set character, 4 byte param */
#define DVI_SETRULE	132	/* set a rule */
#define DVI_PUT1	133	/* put char, don't move right */
#define DVI_PUT2	134	/* put char, 2 byte */
#define DVI_PUT3	135	/* etc */
#define DVI_PUT4	136
#define DVI_PUTRULE	137	/* put rule, don't move right */
#define DVI_NOP		138	/* no-op */
#define DVI_BOP		139	/* begin page */
#define DVI_EOP		140	/* end page */
#define DVI_PUSH	141	/* push h,v,w,x,y,z */
#define DVI_POP		142	/* pop  h,v,w,x,y,z */
#define DVI_RIGHT1	143	/* move right, 1 byte signed param */
#define DVI_RIGHT2	144	/* move right, 2 byte signed param */
#define DVI_RIGHT3	145	/* etc */
#define DVI_RIGHT4	146
#define DVI_W0		147	/* h += w */
#define DVI_W1		148	/* w = 1 byte signed param, h += w */
#define DVI_W2		149	/* w = 2 byte etc, h += w */
#define DVI_W3		150
#define DVI_W4		151
#define DVI_X0		152	/* like DVI_W0 but for x */
#define DVI_X1		153	/* etc */
#define DVI_X2		154
#define DVI_X3		155
#define DVI_X4		156
#define DVI_DOWN1	157	/* v += 1 byte signed param */
#define DVI_DOWN2	158	/* v += 2 byte signed param */
#define DVI_DOWN3	159	/* etc */
#define DVI_DOWN4	160
#define DVI_Y0		161	/* y = 1 byte signed param, v += y */
#define DVI_Y1		162	/* etc */
#define DVI_Y2		163
#define DVI_Y3		164
#define DVI_Y4		165
#define DVI_Z0		166	/* z = 1 byte signed param, v += z */
#define DVI_Z1		167	/* etc */
#define DVI_Z2		168
#define DVI_Z3		169
#define DVI_Z4		170
#define DVI_FNTNUM0	171

#define DVI_FNT1	235	/* select font, 1 byte param */
#define DVI_FNT2	236	/* etc */
#define DVI_FNT3	237
#define DVI_FNT4	238
#define DVI_XXX1	239	/* for \special: if length < 256 */
#define DVI_XXX2	240	/* etc */
#define DVI_XXX3	241
#define DVI_XXX4	242
#define DVI_FNTDEF1	243	/* Define font, 1 byte param (0 to 63) */
#define DVI_FNTDEF2	244	/* etc */
#define DVI_FNTDEF3	245
#define DVI_FNTDEF4	246
#define DVI_PRE		247	/* preamble */
#define DVI_POST	248	/* postamble */
#define DVI_POSTPOST	249	/* end of postamble */
#define DVI_FILLER	223	/* filler bytes at end of dvi file */
