% still more web2c stuff for the e-TeX part of Aleph
% this will be merged into the others soon

@x [51.1332] l.24203 (ca.) texarray
  line_stack:=xmalloc_array (integer, max_in_open);
@y
  line_stack:=xmalloc_array (integer, max_in_open);
  eof_seen:=xmalloc_array (boolean, max_in_open);
  grp_stack:=xmalloc_array (save_pointer, max_in_open);
  if_stack:=xmalloc_array (pointer, max_in_open);
@z

@x [51.1337] l.24371 (ca.) texarray
  trie_root:=0; trie_c[0]:=si(0); trie_ptr:=0;
@y
  trie_root:=0; trie_c[0]:=si(0); trie_ptr:=0;
  hyph_root:=0; hyph_start:=0;
@z

@x [53a.???] l.??? texarray
@!eof_seen : array[1..max_in_open] of boolean; {has eof been seen?}
@y
@!eof_seen : ^boolean; {has eof been seen?}
@z

@x [53a.???] l.??? texarray
@!grp_stack : array[0..max_in_open] of save_pointer; {initial |cur_boundary|}
@!if_stack : array[0..max_in_open] of pointer; {initial |cond_ptr|}
@y
@!grp_stack : ^save_pointer; {initial |cur_boundary|}
@!if_stack : ^pointer; {initial |cond_ptr|}
@z

@x [53a.???] l.??? texarray
hyph_root:=0; hyph_start:=0;
@y
@z

% @x [54/web2c.???] l.??? needed earlier
% replacement, but always existing character |font_bc[f]|.
% @^inner loop@>
% 
% @<Declare additional functions for ML\TeX@>=
% function effective_char(@!err_p:boolean;
% @y
% replacement, but always existing character |font_bc[f]|.
% @^inner loop@>
% 
% @<Declare \eTeX\ procedures for sc...@>=
% function effective_char(@!err_p:boolean;
% @z

