%
% This file is part of the Omega project, which
% is based in the web2c distribution of TeX.
%
% Copyright (c) 1994--1998 John Plaice and Yannis Haralambous
% applies only to the changes to the original vftovp.ch.
%
% vftovp.ch for C compilation with web2c.
% Written by kb@cs.umb.edu.
% This file is in the public domain.

@x [0] WEAVE: print changes only.
\pageno=\contentspagenumber \advance\pageno by 1
@y
\pageno=\contentspagenumber \advance\pageno by 1
%\let\maybe=\iffalse
%\def\title{VF$\,$\lowercase{to}$\,$VP changes for C}
@z

% [2] We need to tell web2c about one special variable.
% Perhaps it would be better to allow @define's
% anywhere in a source file, but that seemed just as painful as this.
@x
@p program OVF2OVP(@!vf_file,@!tfm_file,@!vpl_file,@!output);
@y
@p
{Tangle doesn't recognize @@ when it's right after the \.=.}
@\@= @@define var tfm;@>@\
program OVF2OVP(@!vf_file,@!tfm_file,@!vpl_file,@!output);
@z

@x still [2] Set up for path reading.
procedure initialize; {this procedure gets things started properly}
  var @!k:integer; {all-purpose index for initialization}
  begin print_ln(banner);@/
@y
@<Define |parse_arguments|@>
procedure initialize; {this procedure gets things started properly}
  var @!k:integer; {all-purpose index for initialization}
  begin
    kpse_set_program_name (argv[0], nil);
    kpse_init_prog ('VFTOVP', 0, nil, nil);
    {We |xrealloc| when we know how big the file is.  The 1000 comes
     from the negative lower bound.}
    tfm_file_array := cast_to_byte_pointer (xmalloc (1009));
    parse_arguments;
@z

% [4] No name_length.
% Also, AIX defines `class' in <math.h>, so let's take this opportunity to
% define that away.
@x
@<Constants...@>=
@y
@d class == class_var
@<Constants...@>=
@z

@x [4] Drop unused constant.
@!tfm_size=2000000; {maximum length of |tfm| data, in bytes}
@y
@z

@x
@!name_length=50; {a file name shouldn't be longer than this}
@y
@z

@x [11] Open the files.
@ On some systems you may have to do something special to read a
packed file of bytes. For example, the following code didn't work
when it was first tried at Stanford, because packed files have to be
opened with a special switch setting on the \PASCAL\ that was used.
@^system dependencies@>

@<Set init...@>=
reset(tfm_file); reset(vf_file);
@y
@ We don't have to do anything special to read a packed file of bytes,
but we do want to use environment variables to find the input files.
@^system dependencies@>

@<Set init...@>=
{See comments at |kpse_find_vf| in \.{kpathsea/tex-file.h} for why we
 don't use it.}
vf_file := kpse_open_file (vf_name, kpse_ovf_format);
tfm_file := kpse_open_file (tfm_name, kpse_ofm_format);

if verbose then begin
  print (banner);
  print_ln (version_string);
end;
@z

@x [21] Open VPL file.
@!vpl_file:text;

@ @<Set init...@>=
rewrite(vpl_file);
@y
@!vpl_file:text;

@ If an explicit filename isn't given, we write to |stdout|.

@<Set init...@>=
if optind + 3 > argc then begin
  vpl_file := stdout;
end else begin
  vpl_name := extend_filename (cmdline (optind + 2), 'ovp');
  rewrite (vpl_file, vpl_name);
end;
@z

@x [23] `index' is not a good choice of identifier in C.
@<Types...@>=
@!index=0..tfm_size; {address of a byte in |tfm|}
@y
@d index == index_type

@<Types...@>=
@!index=integer; {address of a byte in |tfm|}
@z

@x [24] Make |tfm| be dynamically allocated.
@!tfm:array [-1000..tfm_size] of byte; {the input data all goes here}
@y
{Kludge here to define |tfm| as a macro which takes care of the negative
 lower bound.  We've defined |tfm| for the benefit of web2c above.}
@=#define tfm (tfmfilearray + 1001);@>@\
@!tfm_file_array: pointer_to_byte; {the input data all goes here}
@z

% [25] abort() should cause a bad exit code.
@x
@d abort(#)==begin print_ln(#);
  print_ln('Sorry, but I can''t go on; are you sure this is a OFM?');
  goto final_end;
  end
@y
@d abort(#)==begin print_ln(#);
  write_ln(stderr, 'Sorry, but I can''t go on; are you sure this is a OFM?');
  uexit(1);
  end
@z

@x [25] Allow arbitrarily large input files.
if 4*lf-1>tfm_size then abort('The file is bigger than I can handle!');
@.The file is bigger...@>
@y
tfm_file_array
  := cast_to_byte_pointer (xrealloc (tfm_file_array, 4 * lf - 1 + 1002));
@z

% [31] Ditto for vf_abort.
@x
@d vf_abort(#)==begin
  print_ln(#);
  print_ln('Sorry, but I can''t go on; are you sure this is a OVF?');
  goto final_end;
  end
@y
@d vf_abort(#)==begin
  write_ln(stderr, #);
  write_ln(stderr, 'Sorry, but I can''t go on; are you sure this is a OVF?');
  uexit(1);
  end
@z

@x [32] Be quiet if not -verbose.
for k:=0 to vf_ptr-1 do print(xchr[vf[k]]);
print_ln(' '); reg_count:=0;
@y
if verbose then begin
  for k:=0 to vf_ptr-1 do print(xchr[vf[k]]);
  print_ln(' ');
end;
reg_count:=0;
@z

@x [35] Be quiet if not -verbose.
@<Print the name of the local font@>;
@y
if verbose then begin
  @<Print the name of the local font@>;
end;
@z

@x [36] Output of real numbers.
print_ln(' at ',(((vf[k]*256+vf[k+1])*256+vf[k+2])/@'4000000)*real_dsize:2:2,
  'pt')
@y
print(' at ');
print_real((((vf[k]*256+vf[k+1])*256+vf[k+2])/@'4000000)*real_dsize, 2, 2);
print_ln('pt')
@z

@x [37] No arbitrary max on cur_name.
@!cur_name:packed array[1..name_length] of char; {external name,
  with no lower case letters}
@y
@!cur_name:^char; {external tfm name}
@z

@x [39] Open another TFM file.
reset(tfm_file,cur_name);
@y
tfm_file := kpse_open_file (cur_name, kpse_ofm_format);
@z

@x [40] Be quiet if not -verbose.
    print_ln('Check sum in OVF file being replaced by font metric check sum');
@y
    if verbose then
      print_ln('Check sum in OVF file being replaced by font metric check sum');
@z

@x [42] Remove initialization of now-defunct array.
@ @<Set init...@>=
default_directory:=default_directory_name;
@y
@ (No initialization to be done.  Keep this module to preserve numbering.)
@z

@x [44] Don't append `.tfm' here, and keep lowercase.
for k:=1 to name_length do cur_name[k]:=' ';
if a=0 then begin
  for k:=1 to default_directory_name_length do
    cur_name[k]:=default_directory[k];
  r:=default_directory_name_length;
  end
else r:=0;
for k:=font_start[font_ptr]+14 to vf_ptr-1 do begin
  incr(r);
  if r+4>name_length then vf_abort('Font name too long for me!');
@.Font name too long for me@>
  if (vf[k]>="a")and(vf[k]<="z") then
      cur_name[r]:=xchr[vf[k]-@'40]
  else cur_name[r]:=xchr[vf[k]];
  end;
cur_name[r+1]:='.'; cur_name[r+2]:='T'; cur_name[r+3]:='F'; cur_name[r+4]:='M'
@y
@ The string |cur_name| is supposed to be set to the external name of the
\.{TFM} file for the current font.  We do not impose an arbitrary limit
on the filename length.
@^system dependencies@>

@d name_start == (font_start[font_ptr] + 14)
@d name_end == vf_ptr

@<Move font name into the |cur_name| string@>=
r := name_end - name_start;
cur_name := xmalloc (r + 1);
{|strncpy| might be faster, but it's probably a good idea to keep the
 |xchr| translation.}
for k := name_start to name_end do begin
  cur_name[k - name_start] := xchr[vf[k]];
end;
cur_name[r] := 0; {Append null byte since this is C.}
@z

@x [49] Change strings to C char pointers, so we can initialize them.
@!ASCII_04,@!ASCII_10,@!ASCII_14,HEX: packed array [1..32] of char;
  {strings for output in the user's external character set}
@!xchr:packed array [0..255] of char;
@!MBL_string,@!RI_string,@!RCE_string:packed array [1..3] of char;
  {handy string constants for |face| codes}
@y
@!ASCII_04,@!ASCII_10,@!ASCII_14,HEX: const_c_string;
  {strings for output in the user's external character set}
@!ASCII_all: packed array[0..256] of char;
@!xchr:packed array [0..255] of char;
@!MBL_string,@!RI_string,@!RCE_string: const_c_string;
  {handy string constants for |face| codes}
@z

@x [50] The Pascal strings are indexed starting at 1, so we pad with a blank.
ASCII_04:=' !"#$%&''()*+,-./0123456789:;<=>?';@/
ASCII_10:='@@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_';@/
ASCII_14:='`abcdefghijklmnopqrstuvwxyz{|}~?';@/
HEX:='0123456789ABCDEF';@/
@y
ASCII_04:='  !"#$%&''()*+,-./0123456789:;<=>?';@/
ASCII_10:=' @@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_';@/
ASCII_14:=' `abcdefghijklmnopqrstuvwxyz{|}~?';@/
HEX:=' 0123456789ABCDEF';@/
strcpy (ASCII_all, ASCII_04);
strcat (ASCII_all, '@@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_');
strcat (ASCII_all, '`abcdefghijklmnopqrstuvwxyz{|}~');@/
@z

@x
MBL_string:='MBL'; RI_string:='RI '; RCE_string:='RCE';
@y
MBL_string:=' MBL'; RI_string:=' RI '; RCE_string:=' RCE';
@z

% [60] How we output the character code depends on |charcode_format|.
@x
begin if font_type>vanilla then
  out_hex_char(c)
else if (c>="0")and(c<="9") then
  out(' C ',c-"0":1)
else if (c>="A")and(c<="Z") then
  out(' C ',ASCII_10[c-"A"+2])
else if (c>="a")and(c<="z") then
  out(' C ',ASCII_14[c-"a"+2])
else out_hex_char(c);
end;
@y
begin if (font_type > vanilla) or (charcode_format = charcode_hex) then
  out_hex_char(c)
else if (charcode_format = charcode_ascii) and (c > " ") and (c <= "~")
        and (c <> "(") and (c <> ")") then
  out(' C ', ASCII_all[c - " " + 1])
{default case, use hex}
else out_hex_char(c);
end;
@z

% [61] Don't output the face code as an integer.
@x
  out(MBL_string[1+(b mod 3)]);
  out(RI_string[1+s]);
  out(RCE_string[1+(b div 3)]);
@y
  put_byte(MBL_string[1+(b mod 3)], vpl_file);
  put_byte(RI_string[1+s], vpl_file);
  put_byte(RCE_string[1+(b div 3)], vpl_file);
@z

@x [62] Force 32-bit constant arithmetic for 16-bit machines.
f:=((tfm[k+1] mod 16)*@'400+tfm[k+2])*@'400+tfm[k+3];
@y
f:=((tfm[k+1] mod 16)*intcast(@'400)+tfm[k+2])*@'400+tfm[k+3];
@z

% [101] No progress reports unless verbose.
@x
      incr(chars_on_line);
      end;
    for cprime:=c to (c+no_repeats(c)) do begin
      print_hex(cprime); {progress report}
@y
      if verbose then incr(chars_on_line);
      end;
    for cprime:=c to (c+no_repeats(c)) do begin
      if verbose then print_hex(cprime); {progress report}
@z

% [112] No nonlocal goto's.
@x
  print_ln('Sorry, I haven''t room for so many ligature/kern pairs!');
@.Sorry, I haven't room...@>
  goto final_end;
@y
  write_ln(stderr, 'Sorry, I haven''t room for so many ligature/kern pairs!');
@.Sorry, I haven't room...@>
  uexit(1);
@z

% still [112] We can't have a function named `f', because of the local
% variable in do_simple_things.  It would be better, but harder, to fix
% web2c.
@x
     r:=f(r,(hash[r]-1)div xmax_char,(hash[r]-1)mod xmax_char);
@y
     r:=lig_f(r,(hash[r]-1)div xmax_char,(hash[r]-1)mod xmax_char);
@z

@x
  out('(INFINITE LIGATURE LOOP MUST BE BROKEN!)'); goto final_end;
@y
  out('(INFINITE LIGATURE LOOP MUST BE BROKEN!)'); uexit(1);
@z

% [116] web2c can't handle these mutually recursive procedures.
% But let's do a fake definition of f here, so that it gets into web2c's
% symbol table...
@x
@p function f(@!h,@!x,@!y:index):index; forward;@t\2@>
  {compute $f$ for arguments known to be in |hash[h]|}
@y
@p
ifdef('notdef')
function lig_f(@!h,@!x,@!y:index):index; begin end;@t\2@>
  {compute $f$ for arguments known to be in |hash[h]|}
endif('notdef')
@z

@x
else eval:=f(h,x,y);
@y
else eval:=lig_f(h,x,y);
@z

@x [117] ... and then really define it now.
@p function f;
@y
@p function lig_f(@!h,@!x,@!y:index):index;
@z

@x
f:=lig_z[h];
@y
lig_f:=lig_z[h];
@z

@x [124] Some cc's can't handle 136 case labels.
    o:=vf[vf_ptr]; incr(vf_ptr);
    case o of
@y
    o:=vf[vf_ptr]; incr(vf_ptr);
    if (o<=set1+3)or((o>=put1)and(o<=put1+3)) then
      @<Special cases of \.{DVI} instructions to typeset characters@>@;
    else case o of
@z

@x [125] `signed' is a reserved word in ANSI C.
@p function get_bytes(@!k:integer;@!signed:boolean):integer;
@y
@d signed == is_signed {|signed| is a reserved word in ANSI C}
@p function get_bytes(@!k:integer;@!signed:boolean):integer;
@z

@x [126] No nonlocal goto's.
    begin print_ln('Stack overflow!'); goto final_end;
@y
    begin write_ln(stderr, 'Stack overflow!'); uexit(1);
@z

@x [129] This block of code moved outside the case statement.
@ Before we typeset a character we make sure that it exists.

@<Cases...@>=
sixty_four_cases(set_char_0),sixty_four_cases(set_char_0+64),
 four_cases(set1),four_cases(put1):begin if o>=set1 then
@y
@ Before we typeset a character we make sure that it exists.

@<Special cases...@>=
begin if o>=set1 then
@z

@x [129] End of block of code moved outside the case statement.
  end;
@y
  end
@z

@x [132] Eliminate the |final_end| and |exit| labels.
label final_end, exit;
@y
@z
@x
vf_input:=true; return;
final_end: vf_input:=false;
exit: end;
@y
vf_input:=true;
end;
@z
@x
label final_end, exit;
@y
@z
@x
organize:=vf_input; return;
final_end: organize:=false;
exit: end;
@y
organize:=vf_input;
end;
@z

@x [134] Eliminate the |final_end| and |exit| labels.
label final_end,exit;
@y
@z
@x
do_map:=true; return;
final_end: do_map:=false;
exit:end;
@y
do_map:=true;
end;
@z

@x [135] No final newline unless verbose.
print_ln('.');@/
@y
if verbose then print_ln('.');@/
@z

@x [135] System-dependent changes.
This section should be replaced, if necessary, by changes to the program
that are necessary to make \.{VFtoVP} work at a particular installation.
It is usually best to design your change file so that all changes to
previous sections preserve the section numbering; then everybody's version
will be consistent with the printed program. More extensive changes,
which introduce new sections, can be inserted here; then only the index
itself will get a new section number.
@^system dependencies@>
@y
Parse a Unix-style command line.

@d argument_is (#) == (strcmp (long_options[option_index].name, #) = 0)

@<Define |parse_arguments|@> =
procedure parse_arguments;
const n_options = 4; {Pascal won't count array lengths for us.}
var @!long_options: array[0..n_options] of getopt_struct;
    @!getopt_return_val: integer;
    @!option_index: c_int_type;
    @!current_option: 0..n_options;
begin
  @<Initialize the option variables@>;
  @<Define the option table@>;
  repeat
    getopt_return_val := getopt_long_only (argc, argv, '', long_options,
                                           address_of (option_index));
    if getopt_return_val = -1 then begin
      {End of arguments; we exit the loop below.} ;
    end else if getopt_return_val = "?" then begin
      usage ('ovf2ovp');

    end else if argument_is ('help') then begin
      usage_help (OVF2OVP_HELP, nil);

    end else if argument_is ('version') then begin
      print_version_and_exit
        (banner, nil, 'J. Plaice, Y. Haralambous, D.E. Knuth', nil);

    end else if argument_is ('charcode-format') then begin
      if strcmp (optarg, 'ascii') = 0 then
        charcode_format := charcode_ascii
      else if strcmp (optarg, 'hex') = 0 then
        charcode_format := charcode_hex
      else
        write_ln (stderr, 'Bad character code format', optarg, '.');

    end; {Else it was a flag; |getopt| has already done the assignment.}
  until getopt_return_val = -1;

  {Now |optind| is the index of first non-option on the command line.
   We must have one two three remaining arguments.}
  if (optind + 1 <> argc) and (optind + 2 <> argc)
     and (optind + 3 <> argc) then begin
    write_ln (stderr, 'ovf2ovp: Need one to three file arguments.');
    usage ('ovf2ovp');
  end;

  vf_name := cmdline (optind);
  if optind + 2 <= argc then begin
    tfm_name := cmdline (optind + 1); {The user specified the TFM name.}
  end else begin
    {User did not specify TFM name; default it from the VF name.}
    tfm_name := basename_change_suffix (vf_name, '.ovf', '.ofm');
  end;
end;

@ Here are the options we allow.  The first is one of the standard GNU options.
@.-help@>

@<Define the option...@> =
current_option := 0;
long_options[current_option].name := 'help';
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
incr (current_option);

@ Another of the standard options.
@.-version@>

@<Define the option...@> =
long_options[current_option].name := 'version';
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
incr (current_option);

@ Print progress information?
@.-verbose@>

@<Define the option...@> =
long_options[current_option].name := 'verbose';
long_options[current_option].has_arg := 0;
long_options[current_option].flag := address_of (verbose);
long_options[current_option].val := 1;
incr (current_option);

@ The global variable |verbose| determines whether or not we print
progress information.

@<Glob...@> =
@!verbose: c_int_type;

@ It starts off |false|.

@<Initialize the option...@> =
verbose := false;

@ Here is an option to change how we output character codes.
@.-charcode-format@>

@<Define the option...@> =
long_options[current_option].name := 'charcode-format';
long_options[current_option].has_arg := 1;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
incr (current_option);

@ We use an ``enumerated'' type to store the information.

@<Type...@> =
@!charcode_format_type = charcode_ascii..charcode_default;

@
@<Const...@> =
@!charcode_ascii = 0;
@!charcode_hex = 1;
@!charcode_default = 2;

@
@<Global...@> =
@!charcode_format: charcode_format_type;

@ It starts off as the default, which is hex for OFM2OPL.

@<Initialize the option...@> =
charcode_format := charcode_default;

@ An element with all zeros always ends the list.

@<Define the option...@> =
long_options[current_option].name := 0;
long_options[current_option].has_arg := 0;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;

@ Global filenames.

@<Global...@> =
@!tfm_name:c_string;
@!vf_name, @!vpl_name:const_c_string;
@z
