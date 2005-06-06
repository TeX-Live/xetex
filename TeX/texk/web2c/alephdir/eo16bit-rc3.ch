% This is an \Aleph change file; it brings \eTeX register numbers
% from 32767 ($2^{15}-1$) to 65535 ($2^{16}-1), to bring it on line
% with \Omega
% Noticeably, since Omega has 16-bit quarterwords, it is possible
% to do so without changing much.
% Notice that we do this for marks register: everything else is
% brought to level with Omega in eomem.ch
%----------------------------------------
@x Section 53a onwards l.4600
@ \eTeX\ (in extended mode) supports 32768 (i.e., $2^{15}$) count,
dimen, skip, muskip, box, and token registers.  As in \TeX\ the first
256 registers of each kind are realized as arrays in the table of
equivalents; the additional registers are realized as tree structures
built from variable-size nodes with individual registers existing only
when needed.  Default values are used for nonexistent registers:  zero
for count and dimen values, |zero_glue| for glue (skip and muskip)
values, void for boxes, and |null| for token lists (and current marks
discussed below).

Similarly there are 32768 mark classes; the command \.{\\marks}|n|
creates a mark node for a given mark class |0<=n<=32767| (where
\.{\\marks0} is synonymous to \.{\\mark}).  The page builder (actually
the |fire_up| routine) and the |vsplit| routine maintain the current
values of |top_mark|, |first_mark|, |bot_mark|, |split_first_mark|, and
|split_bot_mark| for each mark class.  They are accessed as
\.{\\topmarks}|n| etc., and \.{\\topmarks0} is again synonymous to
\.{\\topmark}.  As in \TeX\ the five current marks for mark class zero
are realized as |cur_mark| array.  The additional current marks are
again realized as tree structure with individual mark classes existing
only when needed.

@<Generate all \eTeX...@>=
primitive("marks",mark,marks_code);
@!@:marks_}{\.{\\marks} primitive@>
primitive("topmarks",top_bot_mark,top_mark_code+marks_code);
@!@:top_marks_}{\.{\\topmarks} primitive@>
primitive("firstmarks",top_bot_mark,first_mark_code+marks_code);
@!@:first_marks_}{\.{\\firstmarks} primitive@>
primitive("botmarks",top_bot_mark,bot_mark_code+marks_code);
@!@:bot_marks_}{\.{\\botmarks} primitive@>
primitive("splitfirstmarks",top_bot_mark,split_first_mark_code+marks_code);
@!@:split_first_marks_}{\.{\\splitfirstmarks} primitive@>
primitive("splitbotmarks",top_bot_mark,split_bot_mark_code+marks_code);
@!@:split_bot_marks_}{\.{\\splitbotmarks} primitive@>

@ The |scan_register_num| procedure scans a register number that must
not exceed 255 in compatibility mode resp.\ 32767 in extended mode.

@<Declare \eTeX\ procedures for ex...@>=
procedure@?scan_register_num; forward;@t\2@>

@ @<Declare procedures that scan restricted classes of integers@>=
procedure scan_register_num;
begin scan_int;
if (cur_val<0)or(cur_val>max_reg_num) then
  begin print_err("Bad register code");
@.Bad register code@>
  help2(max_reg_help_line)("I changed this one to zero.");
  int_error(cur_val); cur_val:=0;
  end;
end;

@ @<Initialize variables for \eTeX\ comp...@>=
max_reg_num:=255;
max_reg_help_line:="A register number must be between 0 and 255.";

@ @<Initialize variables for \eTeX\ ext...@>=
max_reg_num:=32767;
max_reg_help_line:="A register number must be between 0 and 32767.";
@y
@ \eOmega\ (in extended mode) supports 65536 (i.e., $2^{16}$) count,
dimen, skip, muskip, box, and token registers. They are
implemented as in \TeX.

There are 32768 mark classes; the command \.{\\marks}|n|
creates a mark node for a given mark class |0<=n<=32767| (where
\.{\\marks0} is synonymous to \.{\\mark}).  The page builder (actually
the |fire_up| routine) and the |vsplit| routine maintain the current
values of |top_mark|, |first_mark|, |bot_mark|, |split_first_mark|, and
|split_bot_mark| for each mark class.  They are accessed as
\.{\\topmarks}|n| etc., and \.{\\topmarks0} is again synonymous to
\.{\\topmark}.  As in \TeX\ the five current marks for mark class zero
are realized as |cur_mark| array.  The additional current marks are
again realized as tree structure with individual mark classes existing
only when needed.

@<Generate all \eTeX...@>=
primitive("marks",mark,marks_code);
@!@:marks_}{\.{\\marks} primitive@>
primitive("topmarks",top_bot_mark,top_mark_code+marks_code);
@!@:top_marks_}{\.{\\topmarks} primitive@>
primitive("firstmarks",top_bot_mark,first_mark_code+marks_code);
@!@:first_marks_}{\.{\\firstmarks} primitive@>
primitive("botmarks",top_bot_mark,bot_mark_code+marks_code);
@!@:bot_marks_}{\.{\\botmarks} primitive@>
primitive("splitfirstmarks",top_bot_mark,split_first_mark_code+marks_code);
@!@:split_first_marks_}{\.{\\splitfirstmarks} primitive@>
primitive("splitbotmarks",top_bot_mark,split_bot_mark_code+marks_code);
@!@:split_bot_marks_}{\.{\\splitbotmarks} primitive@>

@ The |scan_register_num| procedure scans a (token) register number.

@<Declare \eTeX\ procedures for ex...@>=
procedure@?scan_register_num; forward;@t\2@>

@ @<Declare procedures that scan restricted classes of integers@>=
procedure scan_register_num;
begin scan_int;
if (cur_val<0)or(cur_val>max_reg_num) then
  begin print_err("Bad register code");
@.Bad register code@>
  help2(max_reg_help_line)("I changed this one to zero.");
  int_error(cur_val); cur_val:=0;
  end;
end;

@ @<Initialize variables for \eTeX\ comp...@>=
max_reg_num:=255;
max_reg_help_line:="A register number must be between 0 and 255.";

@ @<Initialize variables for \eTeX\ ext...@>=
max_reg_num:=32767;
max_reg_help_line:="A register number must be between 0 and 32727.";
@z
%----------------------------------------
@x l.4607
@ @<Glob...@>=
@!max_reg_num: halfword; {largest allowed register number}
@!max_reg_help_line: str_number; {first line of help message}

@ There are seven almost identical doubly linked trees, one for the
sparse array of the up to 32512 additional registers of each kind and
one for the sparse array of the up to 32767 additional mark classes.
The root of each such tree, if it exists, is an index node containing 16
pointers to subtrees for 4096 consecutive array elements.  Similar index
nodes are the starting points for all nonempty subtrees for 4096, 256,
and 16 consecutive array elements.  These four levels of index nodes are
followed by a fifth level with nodes for the individual array elements.

Each index node is nine words long.  The pointers to the 16 possible
subtrees or are kept in the |info| and |link| fields of the last eight
words.  (It would be both elegant and efficient to declare them as
array, unfortunately \PASCAL\ doesn't allow this.)

The fields in the first word of each index node and in the nodes for the
array elements are closely related.  The |link| field points to the next
lower index node and the |sa_index| field contains four bits (one
hexadecimal digit) of the register number or mark class.  For the lowest
index node the |link| field is |null| and the |sa_index| field indicates
the type of quantity (|int_avl|, |dimen_val|, |glue_val|, |mu_val|,
|box_val|, |tok_val|, or |mark_val|).  The |sa_used| field in the index
nodes counts how many of the 16 pointers are non-null.
@y
@ @<Glob...@>=
@!max_reg_num: halfword; {largest allowed register number}
@!max_reg_help_line: str_number; {first line of help message}

@ There is one doubly linked tree, to handle the sparse array of the up
to 32727 additional mark classes.
The root of the tree, if it exists, is an index node containing 16
pointers to subtrees for 4096 consecutive array elements.  Similar index
nodes are the starting points for all nonempty subtrees for 4096, 256,
and 16 consecutive array elements.  These four levels of index nodes are
followed by a fifth level with nodes for the individual array elements.

Each index node is nine words long.  The pointers to the 16 possible
subtrees are kept in the |info| and |link| fields of the last eight
words.  (It would be both elegant and efficient to declare them as
array, unfortunately \PASCAL\ doesn't allow this.)

The fields in the first word of each index node and in the nodes for the
array elements are closely related.  The |link| field points to the next
lower index node and the |sa_index| field contains eight bits (two
hexadecimal digits) of the mark class.  For the lowest
index node the |link| field is |null| and the |sa_index| field indicates
the type of quantity (which is always |mark_val|). The |sa_used| field in the
index nodes counts how many of the 16 pointers are non-null.
@z
%----------------------------------------
@x l.4630
The |sa_index| field in the nodes for array elements contains the four
bits plus 16 times the type.  Therefore such a node represents a count
or dimen register if and only if |sa_index<dimen_val_limit|; it
represents a skip or muskip register if and only if
|dimen_val_limit<=sa_index<mu_val_limit|; it represents a box register
if and only if |mu_val_limit<=sa_index<box_val_limit|; it represents a
token list register if and only if
|box_val_limit<=sa_index<tok_val_limit|; finally it represents a mark
class if and only if |tok_val_limit<=sa_index|.

The |new_index| procedure creates an index node (returned in |cur_ptr|)
having given contents of the |sa_index| and |link| fields.

@d box_val==4 {the additional box registers}
@d mark_val=6 {the additional mark classes}
@#
@d dimen_val_limit=@"20 {$2^4\cdot(|dimen_val|+1)$}
@d mu_val_limit=@"40 {$2^4\cdot(|mu_val|+1)$}
@d box_val_limit=@"50 {$2^4\cdot(|box_val|+1)$}
@d tok_val_limit=@"60 {$2^4\cdot(|tok_val|+1)$}
@y
The |sa_index| field in the nodes for array elements contains the eight
bits plus 256 times the type. The type field is actually ignored,
since we only use this hoop-jumpings for marks registers (the code
will probably get a rewrite to get in line with the rest of the
(e-)Omega stuf, anyway, so \dots)

The |new_index| procedure creates an index node (returned in |cur_ptr|)
having given contents of the |sa_index| and |link| fields.

@d mark_val=7 {the additional mark classes}
@z
%----------------------------------------
@x
@ The roots of the seven trees for the additional registers and mark
classes are kept in the |sa_root| array.  The first six locations must
be dumped and undumped; the last one is also known as |sa_mark|.

@d sa_mark==sa_root[mark_val] {root for mark classes}

@<Glob...@>=
@!sa_root:array[int_val..mark_val] of pointer; {roots of sparse arrays}
@!cur_ptr:pointer; {value returned by |new_index| and |find_sa_element|}
@!sa_null:memory_word; {two |null| pointers}

@ @<Set init...@>=
sa_mark:=null; sa_null.hh.lh:=null; sa_null.hh.rh:=null;

@ @<Initialize table...@>=
for i:=int_val to tok_val do sa_root[i]:=null;
@y
@ The root of the tree for the additional mark classes is kept
in |sa_mark|.

@<Glob...@>=
@!sa_mark:pointer; {pointer to sparse array of marks}
@!cur_ptr:pointer; {value returned by |new_index| and |find_sa_element|}
@!sa_null:memory_word; {two |null| pointers}

@ @<Set init...@>=
sa_mark:=null; sa_null.hh.lh:=null; sa_null.hh.rh:=null;
@z
%----------------------------------------
@x l.4681
@ Given a type |t| and a sixteen-bit number |n|, the |find_sa_element|
procedure returns (in |cur_ptr|) a pointer to the node for the
corresponding array element, or |null| when no such element exists.  The
third parameter |w| is set |true| if the element must exist, e.g.,
because it is about to be modified.  The procedure has two main
branches:  one follows the existing tree structure, the other (only used
when |w| is |true|) creates the missing nodes.

We use macros to extract the four-bit pieces from a sixteen-bit register
number or mark class and to fetch or store one of the 16 pointers from
an index node.

@d if_cur_ptr_is_null_then_return_or_goto(#)== {some tree element is missing}
  begin if cur_ptr=null then
    if w then goto #@+else return;
  end
@#
@d hex_dig1(#)==# div 4096 {the fourth lowest hexadecimal digit}
@d hex_dig2(#)==(# div 256) mod 16 {the third lowest hexadecimal digit}
@d hex_dig3(#)==(# div 16) mod 16 {the second lowest hexadecimal digit}
@d hex_dig4(#)==# mod 16 {the lowest hexadecimal digit}
@#
@d get_sa_ptr==if odd(i) then cur_ptr:=link(q+(i div 2)+1)
  else cur_ptr:=info(q+(i div 2)+1)
    {set |cur_ptr| to the pointer indexed by |i| from index node |q|}
@d put_sa_ptr(#)==if odd(i) then link(q+(i div 2)+1):=#
  else info(q+(i div 2)+1):=#
    {store the pointer indexed by |i| in index node |q|}
@d add_sa_ptr==begin put_sa_ptr(cur_ptr); incr(sa_used(q));
  end {add |cur_ptr| as the pointer indexed by |i| in index node |q|}
@d delete_sa_ptr==begin put_sa_ptr(null); decr(sa_used(q));
  end {delete the pointer indexed by |i| in index node |q|}

@<Declare \eTeX\ procedures for ex...@>=
procedure find_sa_element(@!t:small_number;@!n:halfword;@!w:boolean);
  {sets |cur_val| to sparse array element location or |null|}
label not_found,not_found1,not_found2,not_found3,not_found4,exit;
var q:pointer; {for list manipulations}
@!i:small_number; {a four bit index}
begin cur_ptr:=sa_root[t];
if_cur_ptr_is_null_then_return_or_goto(not_found);@/
q:=cur_ptr; i:=hex_dig1(n); get_sa_ptr;
if_cur_ptr_is_null_then_return_or_goto(not_found1);@/
q:=cur_ptr; i:=hex_dig2(n); get_sa_ptr;
if_cur_ptr_is_null_then_return_or_goto(not_found2);@/
q:=cur_ptr; i:=hex_dig3(n); get_sa_ptr;
if_cur_ptr_is_null_then_return_or_goto(not_found3);@/
q:=cur_ptr; i:=hex_dig4(n); get_sa_ptr;
if (cur_ptr=null)and w then goto not_found4;
return;
not_found: new_index(t,null); {create first level index node}
sa_root[t]:=cur_ptr; q:=cur_ptr; i:=hex_dig1(n);
not_found1: new_index(i,q); {create second level index node}
add_sa_ptr; q:=cur_ptr; i:=hex_dig2(n);
not_found2: new_index(i,q); {create third level index node}
add_sa_ptr; q:=cur_ptr; i:=hex_dig3(n);
not_found3: new_index(i,q); {create fourth level index node}
add_sa_ptr; q:=cur_ptr; i:=hex_dig4(n);
not_found4: @<Create a new array element of type |t| with index |i|@>;
link(cur_ptr):=q; add_sa_ptr;
exit:end;

@ The array elements for registers are subject to grouping and have an
|sa_lev| field (quite analogous to |eq_level|) instead of |sa_used|.
Since saved values as well as shorthand definitions (created by e.g.,
\.{\\countdef}) refer to the location of the respective array element,
we need a reference count that is kept in the |sa_ref| field.  An array
element can be deleted (together with all references to it) when its
|sa_ref| value is |null| and its value is the default value.
@^reference counts@>

Skip, muskip, box, and token registers use two word nodes, their values
are stored in the |sa_ptr| field.
Count and dimen registers use three word nodes, their
values are stored in the |sa_int| resp.\ |sa_dim| field in the third
word; the |sa_ptr| field is used under the name |sa_num| to store
the register number.  Mark classes use four word nodes.  The last three
words contain the five types of current marks

@d sa_lev==sa_used {grouping level for the current value}
@d pointer_node_size=2 {size of an element with a pointer value}
@d sa_type(#)==(sa_index(#) div 16) {type part of combined type/index}
@d sa_ref(#)==info(#+1) {reference count of a sparse array element}
@d sa_ptr(#)==link(#+1) {a pointer value}
@#
@d word_node_size=3 {size of an element with a word value}
@d sa_num==sa_ptr {the register number}
@d sa_int(#)==mem[#+2].int {an integer}
@d sa_dim(#)==mem[#+2].sc {a dimension (a somewhat esotheric distinction)}
@#
@d mark_class_node_size=4 {size of an element for a mark class}
@#
@d fetch_box(#)== {fetch |box(cur_val)|}
  if cur_val<256 then #:=box(cur_val)
  else  begin find_sa_element(box_val,cur_val,false);
    if cur_ptr=null then #:=null@+else #:=sa_ptr(cur_ptr);
    end

@<Create a new array element...@>=
if t=mark_val then {a mark class}
  begin cur_ptr:=get_node(mark_class_node_size);
  mem[cur_ptr+1]:=sa_null; mem[cur_ptr+2]:=sa_null; mem[cur_ptr+3]:=sa_null;
  end
else  begin if t<=dimen_val then {a count or dimen register}
    begin cur_ptr:=get_node(word_node_size); sa_int(cur_ptr):=0;
    sa_num(cur_ptr):=n;
    end
  else  begin cur_ptr:=get_node(pointer_node_size);
    if t<=mu_val then {a skip or muskip register}
      begin sa_ptr(cur_ptr):=zero_glue; add_glue_ref(zero_glue);
      end
    else sa_ptr(cur_ptr):=null; {a box or token list register}
    end;
  sa_ref(cur_ptr):=null; {all registers have a reference count}
  end;
sa_index(cur_ptr):=16*t+i; sa_lev(cur_ptr):=level_one

@ The |delete_sa_ref| procedure is called when a pointer to an array
element representing a register is being removed; this means that the
reference count should be decreased by one.  If the reduced reference
count is |null| and the register has been (globally) assigned its
default value the array element should disappear, possibly together with
some index nodes.  This procedure will never be used for mark class
nodes.
@^reference counts@>

@d add_sa_ref(#)==incr(sa_ref(#)) {increase reference count}
@#
@d change_box(#)== {change |box(cur_val)|, the |eq_level| stays the same}
  if cur_val<256 then set_equiv(box_base+cur_val,#)@+else set_sa_box(#)
@#

{ FIXME: needs debugging (sparse arrays) }
@d set_sa_box(#)==begin find_sa_element(box_val,cur_val,false);
  if cur_ptr<>0 then
    begin
      set_equiv(sa_ptr(cur_ptr),#);
      add_sa_ref(cur_ptr);
      delete_sa_ref(cur_ptr);
    end;
  end

@<Declare \eTeX\ procedures for tr...@>=
procedure delete_sa_ref(@!q:pointer); {reduce reference count}
label exit;
var p:pointer; {for list manipulations}
@!i:small_number; {a four bit index}
@!s:small_number; {size of a node}
begin decr(sa_ref(q));
if sa_ref(q)<>null then return;
if sa_index(q)<dimen_val_limit then
 if sa_int(q)=0 then s:=word_node_size
 else return
else  begin if sa_index(q)<mu_val_limit then
    if sa_ptr(q)=zero_glue then delete_glue_ref(zero_glue)
    else return
  else if sa_ptr(q)<>null then return;
  s:=pointer_node_size;
  end;
repeat i:=hex_dig4(sa_index(q)); p:=q; q:=link(p); free_node(p,s);
if q=null then {the whole tree has been freed}
  begin sa_root[i]:=null; return;
  end;
delete_sa_ptr; s:=index_node_size; {node |q| is an index node}
until sa_used(q)>0;
exit:end;

@ The |print_sa_num| procedure prints the register number corresponding
to an array element.

@<Basic print...@>=
procedure print_sa_num(@!q:pointer); {print register number}
var @!n:halfword; {the register number}
begin if sa_index(q)<dimen_val_limit then n:=sa_num(q) {the easy case}
else  begin n:=hex_dig4(sa_index(q)); q:=link(q); n:=n+16*sa_index(q);
  q:=link(q); n:=n+256*(sa_index(q)+16*sa_index(link(q)));
  end;
print_int(n);
end;

@ Here is a procedure that displays the contents of an array element
symbolically.  It is used under similar circumstances as is
|restore_trace| (together with |show_eqtb|) for the quantities kept in
the |eqtb| array.

@<Declare \eTeX\ procedures for tr...@>=
@!stat procedure show_sa(@!p:pointer;@!s:str_number);
var t:small_number; {the type of element}
begin begin_diagnostic; print_char("{"); print(s); print_char(" ");
if p=null then print_char("?") {this can't happen}
else  begin t:=sa_type(p);
  if t<box_val then print_cmd_chr(register,p)
  else if t=box_val then
    begin print_esc("box"); print_sa_num(p);
    end
  else if t=tok_val then print_cmd_chr(toks_register,p)
  else print_char("?"); {this can't happen either}
  print_char("=");
  if t=int_val then print_int(sa_int(p))
  else if t=dimen_val then
    begin print_scaled(sa_dim(p)); print("pt");
    end
  else  begin p:=sa_ptr(p);
    if t=glue_val then print_spec(p,"pt")
    else if t=mu_val then print_spec(p,"mu")
    else if t=box_val then
      if p=null then print("void")
      else  begin depth_threshold:=0; breadth_max:=1; show_node_list(p);
        end
    else if t=tok_val then
      begin if p<>null then show_token_list(link(p),null,32);
      end
    else print_char("?"); {this can't happen either}
    end;
  end;
print_char("}"); end_diagnostic(false);
end;
tats
@y
@ Given a type |t| (which is always |mark_val|) and a sixteen-bit number |n|,
the |find_sa_element| procedure returns (in |cur_ptr|) a pointer to the node
for the corresponding array element, or |null| when no such element exists.
The third parameter |w| is set |true| if the element must exist, e.g., because
it is about to be modified.  The procedure has two main branches:  one follows
the existing tree structure, the other (only used when |w| is |true|) creates
the missing nodes.

We use macros to extract the four-bit pieces from a sixteen-bit register
number or mark class and to fetch or store one of the 16 pointers from
an index node.

@d if_cur_ptr_is_null_then_return_or_goto(#)== {some tree element is missing}
  begin if cur_ptr=null then
    if w then goto #@+else return;
  end
@#
{FIXME: needs debugging}
@d hex_dig1(#)==# div 4096 {the fourth lowest hexadecimal digit}
@d hex_dig2(#)==(# div 256) mod 16 {the third lowest hexadecimal digit}
@d hex_dig3(#)==(# div 16) mod 16 {the second lowest hexadecimal digit}
@d hex_dig4(#)==# mod 16 {the lowest hexadecimal digit}
@#
@d get_sa_ptr==if odd(i) then cur_ptr:=link(q+(i div 2)+1)
  else cur_ptr:=info(q+(i div 2)+1)
    {set |cur_ptr| to the pointer indexed by |i| from index node |q|}
@d put_sa_ptr(#)==if odd(i) then link(q+(i div 2)+1):=#
  else info(q+(i div 2)+1):=#
    {store the pointer indexed by |i| in index node |q|}
@d add_sa_ptr==begin put_sa_ptr(cur_ptr); incr(sa_used(q));
  end {add |cur_ptr| as the pointer indexed by |i| in index node |q|}
@d delete_sa_ptr==begin put_sa_ptr(null); decr(sa_used(q));
  end {delete the pointer indexed by |i| in index node |q|}

@<Declare \eTeX\ procedures for ex...@>=
procedure find_sa_element(@!t:small_number;@!n:halfword;@!w:boolean);
  {sets |cur_val| to sparse array element location or |null|}
label not_found,not_found1,not_found2,not_found3,not_found4,exit;
var q:pointer; {for list manipulations}
@!i:small_number; {a four bit index}
begin
if t<>mark_val then begin
  confusion("sparse arrays, finding")
  cur_ptr:=null;
  return;
end;
cur_ptr:=sa_mark;
if_cur_ptr_is_null_then_return_or_goto(not_found);@/
q:=cur_ptr; i:=hex_dig1(n); get_sa_ptr;
if_cur_ptr_is_null_then_return_or_goto(not_found1);@/
q:=cur_ptr; i:=hex_dig2(n); get_sa_ptr;
if_cur_ptr_is_null_then_return_or_goto(not_found2);@/
q:=cur_ptr; i:=hex_dig3(n); get_sa_ptr;
if_cur_ptr_is_null_then_return_or_goto(not_found3);@/
q:=cur_ptr; i:=hex_dig4(n); get_sa_ptr;
if (cur_ptr=null)and w then goto not_found4;
return;
not_found: new_index(t,null); {create first level index node}
sa_mark:=cur_ptr; q:=cur_ptr; i:=hex_dig1(n);
not_found1: new_index(i,q); {create second level index node}
add_sa_ptr; q:=cur_ptr; i:=hex_dig2(n);
not_found2: new_index(i,q); {create third level index node}
add_sa_ptr; q:=cur_ptr; i:=hex_dig3(n);
not_found3: new_index(i,q); {create fourth level index node}
add_sa_ptr; q:=cur_ptr; i:=hex_dig4(n);
not_found4: @<Create a new array element of type |t| with index |i|@>;
link(cur_ptr):=q; add_sa_ptr;
exit:end;

@ Mark classes use four word nodes.  The last three
words contain the five types of current marks

@d sa_lev==sa_used {grouping level for the current value}
@d pointer_node_size=2 {size of an element with a pointer value}
@d sa_type(#)==(sa_index(#) div 256) {type part of combined type/index}
@d sa_ref(#)==info(#+1) {reference count of a sparse array element}
@d sa_ptr(#)==link(#+1) {a pointer value}
@#
@d sa_num==sa_ptr {the register number}
@d sa_int(#)==mem[#+2].int {an integer}
@d sa_dim(#)==mem[#+2].sc {a dimension (a somewhat esotheric distinction)}
@#
@d mark_class_node_size=4 {size of an element for a mark class}
@#

@<Create a new array element...@>=
{|if t=mark_val then|} {a mark class} {check not needed}
  begin cur_ptr:=get_node(mark_class_node_size);
  mem[cur_ptr+1]:=sa_null; mem[cur_ptr+2]:=sa_null; mem[cur_ptr+3]:=sa_null;
  end;
{|else begin|}
{|  confusion("sparse arrays, creating");|}
{|end|}
sa_index(cur_ptr):=256*t+i; sa_lev(cur_ptr):=level_one

@z
%----------------------------------------
% TODO: l.5029+: will the simply be removed?
% (i.e.: cases which cannot happen (anymore)?)
%----------------------------------------
@x l.5078
@ The command code |register| is used for `\.{\\count}', `\.{\\dimen}',
etc., as well as for references to sparse array elements defined by
`\.{\\countdef}', etc.

@<Cases of |register| for |print_cmd_chr|@>=
begin if (chr_code<mem_bot)or(chr_code>lo_mem_stat_max) then
  cmd:=sa_type(chr_code)
else  begin cmd:=chr_code-mem_bot; chr_code:=null;
  end;
if cmd=int_val then print_esc("count")
else if cmd=dimen_val then print_esc("dimen")
else if cmd=glue_val then print_esc("skip")
else print_esc("muskip");
if chr_code<>null then print_sa_num(chr_code);
end

@ Similarly the command code |toks_register| is used for `\.{\\toks}' as
well as for references to sparse array elements defined by
`\.{\\toksdef}'.

@<Cases of |toks_register| for |print_cmd_chr|@>=
begin print_esc("toks");
if chr_code<>mem_bot then print_sa_num(chr_code);
end

@ When a shorthand definition for an element of one of the sparse arrays
is destroyed, we must reduce the reference count.

@<Cases for |eq_destroy|@>=
toks_register,register:
  if (equiv_field(w)<mem_bot)or(equiv_field(w)>lo_mem_stat_max) then
    delete_sa_ref(equiv_field(w));

@ The task to maintain (change, save, and restore) register values is
essentially the same when the register is realized as sparse array
element or entry in |eqtb|.  The global variable |sa_chain| is the head
of a linked list of entries saved at the topmost level |sa_level|; the
lists for lowel levels are kept in special save stack entries.

@<Glob...@>=
@!sa_chain: pointer; {chain of saved sparse array entries}
@!sa_level: quarterword; {group level for |sa_chain|}

@ @<Set init...@>=
sa_chain:=null; sa_level:=level_zero;

@ The individual saved items are kept in pointer or word nodes similar
to those used for the array elements: a word node with value zero is,
however, saved as pointer node with the otherwise impossible |sa_index|
value |tok_val_limit|.

@d sa_loc==sa_ref {location of saved item}

@<Declare \eTeX\ procedures for tr...@>=
procedure sa_save(@!p:pointer); {saves value of |p|}
var q:pointer; {the new save node}
@!i:quarterword; {index field of node}
begin if cur_level<>sa_level then
  begin check_full_save_stack; save_type(save_ptr):=restore_sa;
  save_level(save_ptr):=sa_level; save_index(save_ptr):=sa_chain;
  incr(save_ptr); sa_chain:=null; sa_level:=cur_level;
  end;
i:=sa_index(p);
if i<dimen_val_limit then
  begin if sa_int(p)=0 then
    begin q:=get_node(pointer_node_size); i:=tok_val_limit;
    end
  else  begin q:=get_node(word_node_size); sa_int(q):=sa_int(p);
    end;
  sa_ptr(q):=null;
  end
else  begin q:=get_node(pointer_node_size); sa_ptr(q):=sa_ptr(p);
  end;
sa_loc(q):=p; sa_index(q):=i; sa_lev(q):=sa_lev(p);
link(q):=sa_chain; sa_chain:=q; add_sa_ref(p);
end;

@ @<Declare \eTeX\ procedures for tr...@>=
procedure sa_destroy(@!p:pointer); {destroy value of |p|}
begin if sa_index(p)<mu_val_limit then delete_glue_ref(sa_ptr(p))
else if sa_ptr(p)<>null then
  if sa_index(p)<box_val_limit then flush_node_list(sa_ptr(p))
  else delete_token_ref(sa_ptr(p));
end;

@ The procedure |sa_def| assigns a new value to sparse array elements,
and saves the former value if appropriate.  This procedure is used only
for skip, muskip, box, and token list registers.  The counterpart of
|sa_def| for count and dimen registers is called |sa_w_def|.

@d sa_define(#)==if e then
    if global then gsa_def(#)@+else sa_def(#)
  else define
@#
@d sa_def_box== {assign |cur_box| to |box(cur_val)|}
  begin find_sa_element(box_val,cur_val,true);
  if global then gsa_def(cur_ptr,cur_box)@+else sa_def(cur_ptr,cur_box);
  end
@#
@d sa_word_define(#)==if e then
    if global then gsa_w_def(#)@+else sa_w_def(#)
  else word_define(#)

@<Declare \eTeX\ procedures for tr...@>=
procedure sa_def(@!p:pointer;@!e:halfword);
  {new data for sparse array elements}
begin add_sa_ref(p);
if sa_ptr(p)=e then
  begin @!stat if tracing_assigns>0 then show_sa(p,"reassigning");@+tats@;@/
  sa_destroy(p);
  end
else  begin @!stat if tracing_assigns>0 then show_sa(p,"changing");@+tats@;@/
  if sa_lev(p)=cur_level then sa_destroy(p)@+else sa_save(p);
  sa_lev(p):=cur_level; sa_ptr(p):=e;
  @!stat if tracing_assigns>0 then show_sa(p,"into");@+tats@;@/
  end;
delete_sa_ref(p);
end;
@#
procedure sa_w_def(@!p:pointer;@!w:integer);
begin add_sa_ref(p);
if sa_int(p)=w then
  begin @!stat if tracing_assigns>0 then show_sa(p,"reassigning");@+tats@;@/
  end
else  begin @!stat if tracing_assigns>0 then show_sa(p,"changing");@+tats@;@/
  if sa_lev(p)<>cur_level then sa_save(p);
  sa_lev(p):=cur_level; sa_int(p):=w;
  @!stat if tracing_assigns>0 then show_sa(p,"into");@+tats@;@/
  end;
delete_sa_ref(p);
end;

@ The |sa_def| and |sa_w_def| routines take care of local definitions.
@^global definitions@>
Global definitions are done in almost the same way, but there is no need
to save old values, and the new value is associated with |level_one|.

@<Declare \eTeX\ procedures for tr...@>=
procedure gsa_def(@!p:pointer;@!e:halfword); {global |sa_def|}
begin add_sa_ref(p);
@!stat if tracing_assigns>0 then show_sa(p,"globally changing");@+tats@;@/
sa_destroy(p); sa_lev(p):=level_one; sa_ptr(p):=e;
@!stat if tracing_assigns>0 then show_sa(p,"into");@+tats@;@/
delete_sa_ref(p);
end;
@#
procedure gsa_w_def(@!p:pointer;@!w:integer); {global |sa_w_def|}
begin add_sa_ref(p);
@!stat if tracing_assigns>0 then show_sa(p,"globally changing");@+tats@;@/
sa_lev(p):=level_one; sa_int(p):=w;
@!stat if tracing_assigns>0 then show_sa(p,"into");@+tats@;@/
delete_sa_ref(p);
end;

@ The |sa_restore| procedure restores the sparse array entries pointed
at by |sa_chain|

@<Declare \eTeX\ procedures for tr...@>=
procedure sa_restore;
var p:pointer; {sparse array element}
begin repeat p:=sa_loc(sa_chain);
if sa_lev(p)=level_one then
  begin if sa_index(p)>=dimen_val_limit then sa_destroy(sa_chain);
  @!stat if tracing_restores>0 then show_sa(p,"retaining");@+tats@;@/
  end
else  begin if sa_index(p)<dimen_val_limit then
    if sa_index(sa_chain)<dimen_val_limit then sa_int(p):=sa_int(sa_chain)
    else sa_int(p):=0
  else  begin sa_destroy(p); sa_ptr(p):=sa_ptr(sa_chain);
    end;
  sa_lev(p):=sa_lev(sa_chain);
  @!stat if tracing_restores>0 then show_sa(p,"restoring");@+tats@;@/
  end;
delete_sa_ref(p);
p:=sa_chain; sa_chain:=link(p);
if sa_index(p)<dimen_val_limit then free_node(p,word_node_size)
else free_node(p,pointer_node_size);
until sa_chain=null;
end;
@y
@ @<Cases of |register| for |print_cmd_chr|@>=
begin cmd:=chr_code-mem_bot;
if cmd=int_val then print_esc("count")
else if cmd=dimen_val then print_esc("dimen")
else if cmd=glue_val then print_esc("skip")
else print_esc("muskip");
end

@ @<Cases of |toks_register| for |print_cmd_chr|@>=
print_esc("toks")

@ @<Cases for |eq_destroy|@>=
{empty section}

@z
