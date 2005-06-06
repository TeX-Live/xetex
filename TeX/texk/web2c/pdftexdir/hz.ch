% WEB change file containing HZ extensions for pdfTeX 
%
% Copyright (c) 1996-2002 Han Th\^e\llap{\raise 0.5ex\hbox{\'{}}} Th\`anh, <thanh@pdftex.org>
%
% This file is part of pdfTeX.
%
% pdfTeX is free software; you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation; either version 2 of the License, or
% (at your option) any later version.
%
% pdfTeX is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with pdfTeX; if not, write to the Free Software
% Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
%
% $Id: //depot/Build/source/TeX/texk/web2c/pdftexdir/hz.ch#2 $

@x [155] - margin kerning
@d acc_kern=2 {|subtype| of kern nodes from accents}
@y
@d acc_kern=2 {|subtype| of kern nodes from accents}
@d margin_kern_node = 40
@d margin_kern_node_size = 3
@d margin_char(#) == info(# + 2)
@d left_side == 0
@d right_side == 1
@d lp_code_base == 2
@d rp_code_base == 3
@d ef_code_base == 4
@z

@x [183] - margin kerning
  kern_node: @<Display kern |p|@>;
@y
  margin_kern_node: begin
    print_esc("kern");
    print_scaled(width(p));
    if subtype(p) = left_side then
        print(" (left margin)")
    else
        print(" (right margin)");
    end;
  kern_node: @<Display kern |p|@>;
@z

@x [202] - margin kerning
    kern_node,math_node,penalty_node: do_nothing;
@y
    margin_kern_node: begin
        free_avail(margin_char(p));
        free_node(p, margin_kern_node_size);
        goto done;
      end;
    kern_node,math_node,penalty_node: do_nothing;
@z

@x [206] - margin kerning
kern_node,math_node,penalty_node: begin r:=get_node(small_node_size);
  words:=small_node_size;
  end;
@y
kern_node,math_node,penalty_node: begin r:=get_node(small_node_size);
  words:=small_node_size;
  end;
margin_kern_node: begin
    r := get_node(margin_kern_node_size);
    fast_get_avail(margin_char(r));
    font(margin_char(r)) := font(margin_char(p));
    character(margin_char(r)) := character(margin_char(p));
    words := small_node_size;
  end;
@z

@x [236]
@d pdf_int_pars=pdftex_first_integer_code + 10 {total number of \pdfTeX's integer parameters}
@y
@d pdf_adjust_spacing_code   = pdftex_first_integer_code + 10 {level of spacing adjusting}
@d pdf_protrude_chars_code   = pdftex_first_integer_code + 11 {protrude chars at left/right edge of paragraphs}
@d pdf_int_pars=pdftex_first_integer_code + 12 {total number of \pdfTeX's integer parameters}
@z

@x [236]
@d error_context_lines==int_par(error_context_lines_code)
@y
@d error_context_lines==int_par(error_context_lines_code)
@#
@d pdf_adjust_spacing   == int_par(pdf_adjust_spacing_code)
@d pdf_protrude_chars   == int_par(pdf_protrude_chars_code)
@z

@x [237]
error_context_lines_code:print_esc("errorcontextlines");
@y
error_context_lines_code:print_esc("errorcontextlines");
@#
pdf_adjust_spacing_code:   print_esc("pdfadjustspacing");
pdf_protrude_chars_code:   print_esc("pdfprotrudechars");
@z

@x [238]
primitive("errorcontextlines",assign_int,int_base+error_context_lines_code);@/
@!@:error_context_lines_}{\.{\\errorcontextlines} primitive@>
@y
primitive("errorcontextlines",assign_int,int_base+error_context_lines_code);@/
@!@:error_context_lines_}{\.{\\errorcontextlines} primitive@>
primitive("pdfadjustspacing",assign_int,int_base+pdf_adjust_spacing_code);@/
@!@:pdf_adjust_spacing_}{\.{\\pdfadjustspacing} primitive@>
primitive("pdfprotrudechars",assign_int,int_base+pdf_protrude_chars_code);@/
@!@:pdf_protrude_chars_}{\.{\\pdfprotrudechars} primitive@>
@z

@x [413] - font expansion
var m:halfword; {|chr_code| part of the operand token}
@y
var m:halfword; {|chr_code| part of the operand token}
    n, k: integer; {accumulators}
@z

@x [426] - font expansion, margin kerning
begin scan_font_ident;
if m=0 then scanned_result(hyphen_char[cur_val])(int_val)
else scanned_result(skew_char[cur_val])(int_val);
@y
begin scan_font_ident;
if m=0 then scanned_result(hyphen_char[cur_val])(int_val)
else if m=1 then scanned_result(skew_char[cur_val])(int_val)
else begin
    n := cur_val;
    scan_char_num;
    k := cur_val;
    case m of 
    lp_code_base: scanned_result(get_lp_code(n, k))(int_val);
    rp_code_base: scanned_result(get_rp_code(n, k))(int_val);
    ef_code_base: scanned_result(get_ef_code(n, k))(int_val);
    end;
end;
@z

@x [622] - margin kerning
glue_node: @<Move right or output leaders@>;
@y
glue_node: @<Move right or output leaders@>;
margin_kern_node,
@z

@x [32e] - font expansion
@!tmp_f: internal_font_number; {for use with |pdf_init_font|}

@y
@!tmp_f: internal_font_number; {for use with |pdf_init_font|}
@ Here come some subroutines to deal with expanded fonts for HZ-algorithm.

@p
function init_font_base(v: integer): integer;
var i, j: integer;
begin
    i := pdf_get_mem(256);
    for j := 0 to 255 do
        pdf_mem[i + j] := v;
    init_font_base := i;
end;

procedure set_lp_code(f: internal_font_number; c: eight_bits; i: integer);
begin
    if pdf_font_lp_base[f] = 0 then
        pdf_font_lp_base[f] := init_font_base(0);
    pdf_mem[pdf_font_lp_base[f] + c] := fix_int(i, -1000, 1000);
end;

procedure set_rp_code(f: internal_font_number; c: eight_bits; i: integer);
begin
    if pdf_font_rp_base[f] = 0 then
        pdf_font_rp_base[f] := init_font_base(0);
    pdf_mem[pdf_font_rp_base[f] + c] := fix_int(i, -1000, 1000);
end;

procedure set_ef_code(f: internal_font_number; c: eight_bits; i: integer);
begin
    if pdf_font_ef_base[f] = 0 then
        pdf_font_ef_base[f] := init_font_base(1000);
    pdf_mem[pdf_font_ef_base[f] + c] := fix_int(i, 0, 1000);
end;

function expand_font_name(f: internal_font_number; e: integer): str_number;
var old_setting:0..max_selector; {holds |selector| setting}
begin
    old_setting:=selector; selector:=new_string;
    print(font_name[f]);
    if e > 0 then
        print("+"); {minus sign will be printed by |print_int|}
    print_int(e);
    selector:=old_setting;
    expand_font_name := make_string;
end;

function auto_expand_font(f: internal_font_number; e: integer): internal_font_number;
{creates an expanded font from the base font; doesn't load expanded tfm at all}
var k: internal_font_number;
    nw, nk, ni, i, j: integer;
begin
    k := font_ptr + 1;
    incr(font_ptr);
    if (font_ptr >= font_max) then
        overflow("maximum internal font number (font_max)", font_max);
    font_name[k] := expand_font_name(f, e);
    font_area[k] := font_area[f];
    hyphen_char[k] := hyphen_char[f];
    skew_char[k] := skew_char[f];
    font_bchar[k] := font_bchar[f];
    font_false_bchar[k] := font_false_bchar[f];
    font_bc[k] := font_bc[f];
    font_ec[k] := font_ec[f];
    font_size[k] := font_size[f];
    font_dsize[k] := font_dsize[f];
    font_params[k] := font_params[f];
    font_glue[k] := font_glue[f];
    bchar_label[k] := bchar_label[f];

    char_base[k] := char_base[f];
    height_base[k] := height_base[f];
    depth_base[k] := depth_base[f];
    lig_kern_base[k] := lig_kern_base[f];
    exten_base[k] := exten_base[f];
    param_base[k] := param_base[f];
    
    nw := height_base[f] - width_base[f];
    ni := lig_kern_base[f] - italic_base[f];
    nk := exten_base[f] - (kern_base[f] + kern_base_offset);
    if (fmem_ptr + nw + ni + nk >= font_mem_size) then
        overflow("number of words of font memory (font_mem_size)", font_mem_size);
    width_base[k] := fmem_ptr;
    italic_base[k] := width_base[k] + nw;
    kern_base[k] := italic_base[k] + ni - kern_base_offset;
    fmem_ptr := fmem_ptr + nw + ni + nk;

    for i := 0 to nw - 1 do
        font_info[width_base[k] + i].sc := 
           round_xn_over_d(font_info[width_base[f] + i].sc, 1000 + e, 1000);
    for i := 0 to ni - 1 do
        font_info[italic_base[k] + i].sc := 
           round_xn_over_d(font_info[italic_base[f] + i].sc, 1000 + e, 1000);
    for i := 0 to nk - 1 do
        font_info[kern_base[k] + kern_base_offset + i].sc := 
           round_xn_over_d(font_info[kern_base[f] + kern_base_offset + i].sc, 1000 + e, 1000);
    
    auto_expand_font := k;
end;

procedure set_expand_param(k, f: internal_font_number; e: integer);
var i, j: integer;
begin
    pdf_font_expand_ratio[k] := e;
    pdf_font_step[k] := pdf_font_step[f];
    pdf_font_auto_expand[k] := pdf_font_auto_expand[f];
    pdf_font_blink[k] := f;
    pdf_font_lp_base[k] := pdf_font_lp_base[f];
    pdf_font_rp_base[k] := pdf_font_rp_base[f];
    pdf_font_ef_base[k] := pdf_font_ef_base[f];
end;

function tfm_lookup(s: str_number; fs: scaled): internal_font_number;
{looks up for a TFM with name |s| loaded at |fs| size; if found then flushes |s|}
var k: internal_font_number;
begin
    if fs <> 0 then begin
        for k := font_base + 1 to font_ptr do 
            if str_eq_str(font_name[k], s) and (font_size[k] = fs) then begin
                flush_str(s);
                tfm_lookup := k;
                return;
            end;
    end
    else begin
        for k := font_base + 1 to font_ptr do 
            if str_eq_str(font_name[k], s) then begin
                flush_str(s);
                tfm_lookup := k;
                return;
            end;
    end;
    tfm_lookup := null_font;
end;

function load_expand_font(f: internal_font_number; e: integer): internal_font_number;
{loads font |f| expanded by |e| thousandths into font memory; |e| is nonzero
and is a multiple of |pdf_font_step[f]|}
label found;
var s: str_number; {font name}
    k: internal_font_number;
begin
    s := expand_font_name(f, e);
    k := tfm_lookup(s, font_size[f]);
    if k = null_font then begin
        if pdf_font_auto_expand[f] then 
            k := auto_expand_font(f, e)
        else
            k := read_font_info(null_cs, s, "", font_size[f]);
    end;
    set_expand_param(k, f, e);
    load_expand_font := k;
end;

function auto_expand_vf(f: internal_font_number): boolean;
{check for a virtual auto-expanded font}
var save_f, bf, lf, k: internal_font_number;
    e: integer;
begin
    auto_expand_vf := false;
    if (not pdf_font_auto_expand[f]) or (pdf_font_blink[f] = null_font) then 
        return; {not an auto-expanded font}
    bf := pdf_font_blink[f];
    if pdf_font_type[bf] = new_font_type then {we must process the base font first}
    begin
        save_f := tmp_f;
        tmp_f := bf;
        do_vf; 
        tmp_f := save_f;
    end;

    if pdf_font_type[bf] <> virtual_font_type then 
        return; {not a virtual font}

    e := pdf_font_expand_ratio[f];
    for k := 0 to vf_local_font_num[bf] - 1 do begin
        lf := vf_default_font[bf] + k;
        vf_e_fnts[vf_nf] := vf_e_fnts[lf];
        vf_i_fnts[vf_nf] := auto_expand_font(vf_i_fnts[lf], e);
        set_expand_param(vf_i_fnts[vf_nf], vf_i_fnts[lf], e);
        incr(vf_nf);
    end;
    vf_packet_base[f] := vf_packet_base[bf];
    vf_local_font_num[f] := vf_local_font_num[bf];
    vf_default_font[f] := vf_nf - vf_local_font_num[f];

    pdf_font_type[f] := virtual_font_type;
    auto_expand_vf := true;
end;

function fix_expand_value(f: internal_font_number; e: integer): integer;
{return the multiple of |pdf_font_step[f]| that is nearest to |e|}
var step: integer;
    max_expand: integer;
    neg: boolean;
begin
    fix_expand_value := 0;
    if e = 0 then 
        return;
    if e < 0 then begin
        e := -e;
        neg := true;
        max_expand := -pdf_font_expand_ratio[pdf_font_shrink[f]];
    end
    else begin
        neg := false;
        max_expand := pdf_font_expand_ratio[pdf_font_stretch[f]];
    end;
    if e > max_expand then
        e :=  max_expand
    else begin
        step := pdf_font_step[f];
        if e mod step > 0 then
            e := step*round_xn_over_d(e, 1, step);
    end;
    if neg then
        e := -e;
    fix_expand_value := e;
end;

function get_expand_font(f: internal_font_number; e: integer): internal_font_number;
{look up and create if not found an expanded version of |f|; |f| is an
expandable font; |e| is nonzero and is a multiple of |pdf_font_step[f]|}
var k: internal_font_number;
begin
    k := pdf_font_elink[f];
    while k <> null_font do begin
        if pdf_font_expand_ratio[k] = e then begin
            get_expand_font := k;
            return;
        end;
        k := pdf_font_elink[k];
    end;
    k := load_expand_font(f, e);
    pdf_font_elink[k] := pdf_font_elink[f];
    pdf_font_elink[f] := k;
    get_expand_font := k;
end;

function expand_font(f: internal_font_number; e: integer): internal_font_number;
{looks up for font |f| expanded by |e| thousandths; if not found then
creates it}
var max_expand: integer;
begin
    expand_font := f;
    if e = 0 then
        return;
    e := fix_expand_value(f, e);
    if e = 0 then
        return;
    if pdf_font_elink[f] = null_font then
        pdf_error("font expansion", "uninitialized pdf_font_elink");
    expand_font := get_expand_font(f, e);
end;

procedure read_expand_font; {read font expansion spec and load expanded font}
var font_shrink, font_stretch, font_step: integer;
    f: internal_font_number;
begin
    scan_font_ident;
    f := cur_val;
    if f = null_font then
        pdf_error("font", "invalid font identifier");
    scan_optional_equals;
    scan_int;
    font_stretch := fix_int(cur_val, 0, 1000);
    scan_int;
    font_shrink := fix_int(cur_val, 0, 1000);
    scan_int;
    font_step := fix_int(cur_val, 0, 1000);
    if font_step = 0 then
        pdf_error("expand", "invalid step of font expansion");
    font_stretch := font_stretch - font_stretch mod font_step;
    if font_stretch < 0 then
        font_stretch := 0;
    font_shrink := font_shrink - font_shrink mod font_step;
    if font_shrink < 0 then
        font_shrink := 0;
    pdf_font_step[f] := font_step;
    if scan_keyword("autoexpand") then
        pdf_font_auto_expand[f] := true;
    if (font_stretch = 0) and (font_shrink = 0) then
        pdf_error("expand", "invalid limit of font expansion");
    if font_stretch > 0 then
        pdf_font_stretch[f] := get_expand_font(f, font_stretch);
    if font_shrink > 0 then
        pdf_font_shrink[f] := get_expand_font(f, -font_shrink);
end;
@z

@x [32f] - margin kerning
glue_node: @<(\pdfTeX) Move right or output leaders@>;
@y
glue_node: @<(\pdfTeX) Move right or output leaders@>;
margin_kern_node,
@z

@x [649] - font expansion
@ Here now is |hpack|, which contains few if any surprises.

@p function hpack(@!p:pointer;@!w:scaled;@!m:small_number):pointer;
@y 
@ @<Glob...@>=
@!pdf_font_blink: ^internal_font_number; {link to base font}
@!pdf_font_elink: ^internal_font_number; {link to expanded fonts}
@!pdf_font_stretch: ^integer; {limit of stretching}
@!pdf_font_shrink: ^integer; {limit of shrinking}
@!pdf_font_step: ^integer;  {amount of one step}
@!pdf_font_expand_ratio: ^integer; {current expansion ratio}
@!pdf_font_auto_expand: ^boolean;
@!pdf_font_lp_base: ^integer;
@!pdf_font_rp_base: ^integer;
@!pdf_font_ef_base: ^integer;
@!font_expand_ratio: integer;
@!last_leftmost_char: pointer;
@!last_rightmost_char: pointer;

@ @d cal_margin_kern_var(#) ==
begin
    character(cp) := character(#);
    font(cp) := font(#);
    do_subst_font(cp, 1000);
    if font(cp) <> font(#) then
        margin_kern_stretch := margin_kern_stretch + left_pw(#) - left_pw(cp);
    font(cp) := font(#);
    do_subst_font(cp, -1000);
    if font(cp) <> font(#) then
        margin_kern_shrink := margin_kern_shrink + left_pw(cp) - left_pw(#);
end
    
@<Calculate variations of marginal kerns@>=
begin
    lp := last_leftmost_char;
    rp := last_rightmost_char;
    fast_get_avail(cp);
    if lp <> null then
        cal_margin_kern_var(lp);
    if rp <> null then
        cal_margin_kern_var(rp);
    free_avail(cp);
end

@ Here is |hpack|, which is place where we do font substituting when
font expansion is being used. We define some constants used when calling
|hpack| to deal with font expansion.

@d cal_expand_ratio    == 2 {calculate amount for font expansion after breaking
                             paragraph into lines}
@d subst_ex_font       == 3 {substitute fonts}

@d substituted = 3 {|subtype| of kern nodes that should be substituted}

@d left_pw(#) == char_pw(#, left_side)
@d right_pw(#) == char_pw(#, right_side)

@p
function check_expand_pars(f: internal_font_number): boolean;
var k: internal_font_number;
begin
    check_expand_pars := false;
    if (pdf_font_step[f] = 0) or ((pdf_font_stretch[f] = null_font) and 
                                  (pdf_font_shrink[f] = null_font)) then
        return;
    if cur_font_step < 0 then
        cur_font_step := pdf_font_step[f]
    else if cur_font_step <> pdf_font_step[f] then
        pdf_error("font expansion", "using fonts with different step of expansion in one paragraph is not allowed");
    k := pdf_font_stretch[f];
    if k <> null_font then begin
        if max_stretch_ratio < 0 then
            max_stretch_ratio := pdf_font_expand_ratio[k]
        else if max_stretch_ratio <> pdf_font_expand_ratio[k] then
            pdf_error("font expansion", "using fonts with different limit of expansion in one paragraph is not allowed");
    end;
    k := pdf_font_shrink[f];
    if k <> null_font then begin
        if max_shrink_ratio < 0 then
            max_shrink_ratio := pdf_font_expand_ratio[k]
        else if max_shrink_ratio <> pdf_font_expand_ratio[k] then
            pdf_error("font expansion", "using fonts with different limit of expansion in one paragraph is not allowed");
    end;
    check_expand_pars := true;
end;

function char_stretch(f: internal_font_number; c: eight_bits): scaled;
var k: internal_font_number;
    dw: scaled;
    ef: integer;
begin
    char_stretch := 0;
    k := pdf_font_stretch[f];
    ef := get_ef_code(f, c);
    if (k <> null_font) and (ef > 0) then begin
        dw := char_width(k)(char_info(k)(c)) - char_width(f)(char_info(f)(c));
        if dw > 0 then
            char_stretch := round_xn_over_d(dw, ef, 1000);
    end;
end;

function char_shrink(f: internal_font_number; c: eight_bits): scaled;
var k: internal_font_number;
    dw: scaled;
    ef: integer;
begin
    char_shrink := 0;
    k := pdf_font_shrink[f];
    ef := get_ef_code(f, c);
    if (k <> null_font) and (ef > 0) then begin
        dw := char_width(f)(char_info(f)(c)) - char_width(k)(char_info(k)(c));
        if dw > 0 then
            char_shrink := round_xn_over_d(dw, ef, 1000);
    end;
end;

function get_kern(f: internal_font_number; lc, rc: eight_bits): scaled;
label continue;
var i: four_quarters;
    j: four_quarters;
    k: font_index;
    p: pointer;
    s: integer;
begin
    get_kern := 0;
    i := char_info(f)(lc);
    if char_tag(i) <> lig_tag then
        return;
    k := lig_kern_start(f)(i);
    j := font_info[k].qqqq;
    if skip_byte(j) <= stop_flag then
        goto continue + 1;
    k := lig_kern_restart(f)(j);
continue:
    j := font_info[k].qqqq;
continue + 1:
    if (next_char(j) = rc) and (skip_byte(j) <= stop_flag) and 
       (op_byte(j) >= kern_flag)
    then begin
        get_kern := char_kern(f)(j);
        return;
    end;
    if skip_byte(j) = qi(0) then
        incr(k)
    else begin
        if skip_byte(j) >= stop_flag then
            return;
        k := k + qo(skip_byte(j)) + 1;
    end;
    goto continue;
end;

function kern_stretch(p: pointer): scaled;
var l, r: pointer;
    d: scaled;
begin
    kern_stretch := 0;
    if (prev_char_p = null) or (link(prev_char_p) <> p) or (link(p) = null)
    then
        return;
    l := prev_char_p;
    r := link(p);
    if type(l) = ligature_node then
        l := lig_char(l);
    if type(r) = ligature_node then
        r := lig_char(r);
    if not (is_char_node(l) and is_char_node(r) and 
            (font(l) = font(r)) and 
            (pdf_font_stretch[font(l)] <> null_font))
    then
        return;
    d := get_kern(pdf_font_stretch[font(l)], character(l), character(r));
    kern_stretch := round_xn_over_d(d - width(p), 
                                    get_ef_code(font(l), character(l)), 1000);
end;

function kern_shrink(p: pointer): scaled;
var l, r: pointer;
    d: scaled;
begin
    kern_shrink := 0;
    if (prev_char_p = null) or (link(prev_char_p) <> p) or (link(p) = null)
    then
        return;
    l := prev_char_p;
    r := link(p);
    if type(l) = ligature_node then
        l := lig_char(l);
    if type(r) = ligature_node then
        r := lig_char(r);
    if not (is_char_node(l) and is_char_node(r) and 
            (font(l) = font(r)) and 
            (pdf_font_shrink[font(l)] <> null_font))
    then
        return;
    d := get_kern(pdf_font_shrink[font(l)], character(l), character(r));
    kern_shrink := round_xn_over_d(width(p) - d, 
                                    get_ef_code(font(l), character(l)), 1000);
end;

procedure do_subst_font(p: pointer; ex_ratio: integer);
var f, k: internal_font_number;
    r: pointer;
    ef: integer;
begin
    if not is_char_node(p) and (type(p) = disc_node) then begin
        r := pre_break(p);
        while r <> null do begin
            if is_char_node(r) or (type(r) = ligature_node) then
                do_subst_font(r, ex_ratio);
            r := link(r);
        end;
        r := post_break(p);
        while r <> null do begin
            if is_char_node(r) or (type(r) = ligature_node) then
                do_subst_font(r, ex_ratio);
            r := link(r);
        end;
        return;
    end;
    if is_char_node(p) then
        r := p
    else if type(p) = ligature_node then
        r := lig_char(p)
    else begin
        {|short_display_n(p, 5);|}
        pdf_error("font expansion", "invalid node type");
    end;
    f := font(r);
    ef := get_ef_code(f, character(r));
    if ef = 0 then
        return;
    if (pdf_font_stretch[f] <> null_font) and (ex_ratio > 0) then
        k := expand_font(f, divide_scaled(ex_ratio*
                                pdf_font_expand_ratio[pdf_font_stretch[f]]*ef,
                                1000000, 0))
    else if (pdf_font_shrink[f] <> null_font) and (ex_ratio < 0) then
        k := expand_font(f, -divide_scaled(ex_ratio*
                                pdf_font_expand_ratio[pdf_font_shrink[f]]*ef,
                                1000000, 0))
    else
        k := f;
    if k <> f then begin
        font(r) := k;
        if not is_char_node(p) then begin
            r := lig_ptr(p);
            while r <> null do begin
                font(r) := k;
                r := link(r);
            end;
        end;
    end;
end;

function char_pw(p: pointer; side: small_number): scaled;
var f: internal_font_number;
    c: integer;
begin
    char_pw := 0;
    if side = left_side then
        last_leftmost_char := null
    else
        last_rightmost_char := null;
    if p = null then
        return;
    if not is_char_node(p) then begin
        if type(p) = ligature_node then
            p := lig_char(p)
        else 
            return;
    end;
    f := font(p);
    if side = left_side then begin
        c := get_lp_code(f, character(p));
        last_leftmost_char := p;
    end
    else begin
        c := get_rp_code(f, character(p));
        last_rightmost_char := p;
    end;
    if c = 0 then
        return;
    char_pw := 
        round_xn_over_d(quad(f), c, 1000);
end;

function new_margin_kern(w: scaled; p: pointer; side: small_number): pointer;
var k: pointer;
begin
    k := get_node(margin_kern_node_size);
    type(k) := margin_kern_node;
    subtype(k) := side;
    width(k) := w;
    if p = null then
        pdf_error("margin kerning", "invalid pointer to marginal char node");
    fast_get_avail(margin_char(k));
    character(margin_char(k)) := character(p);
    font(margin_char(k)) := font(p);
    new_margin_kern := k;
end;

function hpack(@!p:pointer;@!w:scaled;@!m:small_number):pointer;
@z

@x [649] - font expansion
begin last_badness:=0; r:=get_node(box_node_size); type(r):=hlist_node;
subtype(r):=min_quarterword; shift_amount(r):=0;
q:=r+list_offset; link(q):=p;@/
@y
font_stretch: scaled;
font_shrink: scaled;
k: scaled;
begin last_badness:=0; r:=get_node(box_node_size); type(r):=hlist_node;
subtype(r):=min_quarterword; shift_amount(r):=0;
q:=r+list_offset; link(q):=p;@/
if m = cal_expand_ratio then begin
    prev_char_p := null;
    font_stretch := 0;
    font_shrink := 0;
    font_expand_ratio := 0;
end;
@z

@x [649] - font expansion
hpack:=r;
@y
if (m = cal_expand_ratio) and (font_expand_ratio <> 0) then begin
    font_expand_ratio := fix_int(font_expand_ratio, -1000, 1000);
    q := list_ptr(r);
    free_node(r, box_node_size);
    r := hpack(q, w, subst_ex_font);
end;
hpack:=r;
@z

@x [651] - font expansion
  kern_node: x:=x+width(p);
@y
  margin_kern_node: begin
    if m = cal_expand_ratio then begin
        f := font(margin_char(p));
        do_subst_font(margin_char(p), 1000);
        if f <> font(margin_char(p)) then
            font_stretch := font_stretch - width(p) - 
                char_pw(margin_char(p), subtype(p));
        font(margin_char(p)) := f;
        do_subst_font(margin_char(p), -1000);
        if f <> font(margin_char(p)) then
            font_shrink := font_shrink - width(p) -
                char_pw(margin_char(p), subtype(p));
        font(margin_char(p)) := f;
      end
    else if m = subst_ex_font then begin
            do_subst_font(margin_char(p), font_expand_ratio);
            width(p) := -char_pw(margin_char(p), subtype(p));
      end;
    x := x + width(p);
    end;
  kern_node: begin
    if (m = cal_expand_ratio) and (subtype(p) = normal) then begin
        k := kern_stretch(p);
        if k <> 0 then begin
            subtype(p) := substituted;
            font_stretch := font_stretch + k;
        end;
        k := kern_shrink(p);
        if k <> 0 then begin
            subtype(p) := substituted;
            font_shrink := font_shrink + k;
        end;
      end
    else if (m = subst_ex_font) and (subtype(p) = substituted) then begin
        if type(link(p)) = ligature_node then
            width(p) := get_kern(font(prev_char_p),
                                 character(prev_char_p),
                                 character(lig_char(link(p))))
        else 
            width(p) := get_kern(font(prev_char_p),
                                 character(prev_char_p),
                                 character(link(p)))
      end;
    x := x + width(p);
    end;
@z

@x [651] - font expansion
  ligature_node: @<Make node |p| look like a |char_node|
    and |goto reswitch|@>;
@y
  ligature_node: begin
      if m = subst_ex_font then
          do_subst_font(p, font_expand_ratio);
      @<Make node |p| look like a |char_node| and |goto reswitch|@>;
    end;
  disc_node:
      if m = subst_ex_font then
          do_subst_font(p, font_expand_ratio);
@z

@x [654] - font expansion
begin f:=font(p); i:=char_info(f)(character(p)); hd:=height_depth(i);
@y
begin
if m >= cal_expand_ratio then begin
    prev_char_p := p;
    case m of
    cal_expand_ratio: begin
        f := font(p);
        add_char_stretch(font_stretch)(character(p));
        add_char_shrink(font_shrink)(character(p));
    end;
    subst_ex_font:
        do_subst_font(p, font_expand_ratio);
    endcases;
end;
f:=font(p); i:=char_info(f)(character(p)); hd:=height_depth(i);
@z

@x [658] - font expansion
@ @<Determine horizontal glue stretch setting...@>=
begin @<Determine the stretch order@>;
@y
@ If |hpack| is called with |m=cal_expand_ratio| we calculate
|font_expand_ratio| and return without checking for overfull or underfull box.

@<Determine horizontal glue stretch setting...@>=
begin @<Determine the stretch order@>;
if (m = cal_expand_ratio) and (o = normal) and (font_stretch > 0) then begin
    font_expand_ratio := divide_scaled(x, font_stretch, 3);
    return;
end;
@z

@x [664] - font expansion
@ @<Determine horizontal glue shrink setting...@>=
begin @<Determine the shrink order@>;
@y
@ @<Determine horizontal glue shrink setting...@>=
begin @<Determine the shrink order@>;
if (m = cal_expand_ratio) and (o = normal) and (font_shrink > 0) then begin
    font_expand_ratio := divide_scaled(x, font_shrink, 3);
    return;
end;
@z

@x [822] - font expansion
@d delta_node_size=7 {number of words in a delta node}
@y
@d delta_node_size=9 {number of words in a delta node}
@z

@x [823] - font expansion, margin kerning, avoiding overfull boxes
@<Glo...@>=
@!active_width:array[1..6] of scaled;
  {distance from first active node to~|cur_p|}
@!cur_active_width:array[1..6] of scaled; {distance from current active node}
@!background:array[1..6] of scaled; {length of an ``empty'' line}
@!break_width:array[1..6] of scaled; {length being computed after current break}
@y
@d do_seven_eight(#) == if pdf_adjust_spacing > 1 then begin #(7);#(8); end
@d do_all_eight(#) == do_all_six(#); do_seven_eight(#)
@d do_one_seven_eight(#) == #(1); do_seven_eight(#)

@d total_font_stretch == cur_active_width[7] 
@d total_font_shrink == cur_active_width[8] 

@d save_active_width(#) == prev_active_width[#] := active_width[#]
@d restore_active_width(#) == active_width[#] := prev_active_width[#]

@<Glo...@>=
@!active_width:array[1..8] of scaled;
  {distance from first active node to~|cur_p|}
@!cur_active_width:array[1..8] of scaled; {distance from current active node}
@!background:array[1..8] of scaled; {length of an ``empty'' line}
@!break_width:array[1..8] of scaled; {length being computed after current break}
@#
@!auto_breaking: boolean; {make |auto_breaking| accessible out of |line_break|}
@!prev_p: pointer; {make |prev_p| accessible out of |line_break|}
@!first_p: pointer; {to access the first node of the paragraph}
@!prev_char_p: pointer; {pointer to the previous char of an implicit kern}
@!next_char_p: pointer; {pointer to the next char of an implicit kern}
@# 
@!try_prev_break: boolean; {force break at the previous legal breakpoint?}
@!prev_legal: pointer; {the previous legal breakpoint}
@!prev_prev_legal: pointer; {to save |prev_p| corresponding to |prev_legal|}
@!prev_auto_breaking: boolean; {to save |auto_breaking| corresponding to |prev_legal|}
@!prev_active_width: array[1..8] of scaled; {to save |active_width| corresponding to |prev_legal|}
@!rejected_cur_p: pointer; {the last |cur_p| that has been rejected}
@!before_rejected_cur_p: boolean; {|cur_p| is still before |rejected_cur_p|?}
@#
@!max_stretch_ratio: integer; {maximal stretch ratio of expanded fonts}
@!max_shrink_ratio: integer; {maximal shrink ratio of expanded fonts}
@!cur_font_step: integer; {the current step of expanded fonts}
@z

@x [827] - font expansion
background[6]:=shrink(q)+shrink(r);
@y
background[6]:=shrink(q)+shrink(r);
if pdf_adjust_spacing > 1 then begin
    background[7] := 0;
    background[8] := 0;
    max_stretch_ratio := -1;
    max_shrink_ratio := -1;
    cur_font_step := -1;
    prev_char_p := null;
end;
@z

@x [829] - margin kerning
@<Declare subprocedures for |line_break|@>=
procedure try_break(@!pi:integer;@!break_type:small_number);
@y
@d discardable(#) == not(
    is_char_node(#) or 
    non_discardable(#) or
    ((type(#) = kern_node) and (subtype(#) <> explicit)) or
    (type(#) = margin_kern_node)
)

@<Declare subprocedures for |line_break|@>=
function prev_rightmost(s, e: pointer): pointer;
var p: pointer;
begin
    prev_rightmost := null;
    p := s;
    if p = null then
        return;
    while link(p) <> e do begin
        p := link(p);
        if p = null then
            return;
    end;
    prev_rightmost := p;
end;

function total_pw(q, p: pointer): scaled;
label reswitch;
var l, r, s: pointer;
    n: integer;
begin
    if break_node(q) = null then
        l := first_p
    else
        l := cur_break(break_node(q));
    r := prev_rightmost(prev_p, p); { get |link(r)=p| }
    if r <> null then begin
        if (type(r) = disc_node) and 
           (type(p) = disc_node) and 
           (pre_break(p) = null) then  
        { I cannot remember when this case happens but I encountered it once }
        begin { find the predecessor of |r| }
            if r = prev_p then 
            { |link(prev_p)=p| and |prev_p| is also a |disc_node| }
            begin
                { start from the leftmost node }
                r  := prev_rightmost(l, p);
            end
            else
                r := prev_rightmost(prev_p, p);
        end
        else if (p <> null) and 
                (type(p) = disc_node) and 
                (pre_break(p) <> null) then 
        { a |disc_node| with non-empty |pre_break|, protrude the last char }
        begin
            r := pre_break(p);
            while link(r) <> null do
                r := link(r);
        end;
    end;
reswitch:
    while (l <> null) and discardable(l) do
        l := link(l);
    if (l <> null) and (type(l) = disc_node) then begin
    {|
        short_display_n(l, 2);
        print_ln;
        breadth_max := 10;
        depth_threshold := 2;
        show_node_list(l);
        print_ln;
    |}
        if post_break(l) <> null then
            l := post_break(l)
        else begin
            n := replace_count(l);
            l := link(l);
            while n > 0 do begin 
                if link(l) <> null then 
                    l := link(l);
                decr(n);
            end;
        end;
        goto reswitch;
    end;
    total_pw := left_pw(l) + right_pw(r);
end;

procedure try_break(@!pi:integer;@!break_type:small_number);
@z

@x
var r:pointer; {runs through the active list}
@y
var r:pointer; {runs through the active list}
@!margin_kern_stretch: scaled;
@!margin_kern_shrink: scaled;
@!lp, rp, cp: pointer;
@z


@x [829] - font expansion
do_all_six(copy_to_cur_active);
@y
do_all_eight(copy_to_cur_active);
@z

@x [832] - font expansion
  begin do_all_six(update_width);
@y
  begin do_all_eight(update_width);
@z

@x [837] - font expansion
begin no_break_yet:=false; do_all_six(set_break_width_to_background);
@y
begin no_break_yet:=false; do_all_eight(set_break_width_to_background);
@z

@x [839] - font expansion
@<Glob...@>=
@!disc_width:scaled; {the length of discretionary material preceding a break}
@y
@d reset_disc_width(#) == disc_width[#] := 0

@d add_disc_width_to_break_width(#) ==  
    break_width[#] := break_width[#] + disc_width[#]

@d add_disc_width_to_active_width(#) ==  
    active_width[#] := active_width[#] + disc_width[#]

@d sub_disc_width_from_active_width(#) ==  
    active_width[#] := active_width[#] - disc_width[#]

@d add_char_stretch_end(#) == char_stretch(f, #)
@d add_char_stretch(#) == # := # + add_char_stretch_end

@d add_char_shrink_end(#) == char_shrink(f, #)
@d add_char_shrink(#) == # := # + add_char_shrink_end

@d sub_char_stretch_end(#) == char_stretch(f, #)
@d sub_char_stretch(#) == # := # - sub_char_stretch_end

@d sub_char_shrink_end(#) == char_shrink(f, #)
@d sub_char_shrink(#) == # := # - sub_char_shrink_end

@d add_kern_stretch_end(#) == kern_stretch(#)
@d add_kern_stretch(#) == # := # + add_kern_stretch_end

@d add_kern_shrink_end(#) == kern_shrink(#)
@d add_kern_shrink(#) == # := # + add_kern_shrink_end

@d sub_kern_stretch_end(#) == kern_stretch(#)
@d sub_kern_stretch(#) == # := # - sub_kern_stretch_end

@d sub_kern_shrink_end(#) == kern_shrink(#)
@d sub_kern_shrink(#) == # := # - sub_kern_shrink_end

@<Glob...@>=
@!disc_width: array[1..8] of scaled; {the length of discretionary material preceding a break}
@z

@x [840] - font expansion
break_width[1]:=break_width[1]+disc_width;
@y
do_one_seven_eight(add_disc_width_to_break_width);
@z

@x [841] - font expansion
  break_width[1]:=break_width[1]-char_width(f)(char_info(f)(character(v)));
@y
  break_width[1]:=break_width[1]-char_width(f)(char_info(f)(character(v)));
  if (pdf_adjust_spacing > 1) and check_expand_pars(f) then begin
      prev_char_p := v;
      sub_char_stretch(break_width[7])(character(v));
      sub_char_shrink(break_width[8])(character(v));
  end;
@z

@x [841] - font expansion
    break_width[1]:=@|break_width[1]-
      char_width(f)(char_info(f)(character(lig_char(v))));
@y
    break_width[1]:=@|break_width[1]-
      char_width(f)(char_info(f)(character(lig_char(v))));
    if (pdf_adjust_spacing > 1) and check_expand_pars(f) then begin
        prev_char_p := v;
        sub_char_stretch(break_width[7])(character(lig_char(v)));
        sub_char_shrink(break_width[8])(character(lig_char(v)));
    end;
@z

@x [841] - font expansion
  hlist_node,vlist_node,rule_node,kern_node:
    break_width[1]:=break_width[1]-width(v);
@y
  hlist_node,vlist_node,rule_node,kern_node: begin
    break_width[1]:=break_width[1]-width(v);
    if (type(v) = kern_node) and
       (pdf_adjust_spacing > 1) and (subtype(v) = normal)
    then begin
        sub_kern_stretch(break_width[7])(v);
        sub_kern_shrink(break_width[8])(v);
    end;
  end;
@z

@x [842] - font expansion
  break_width[1]:=@|break_width[1]+char_width(f)(char_info(f)(character(s)));
@y
  break_width[1]:=@|break_width[1]+char_width(f)(char_info(f)(character(s)));
  if (pdf_adjust_spacing > 1) and check_expand_pars(f) then begin
      prev_char_p := s;
      add_char_stretch(break_width[7])(character(s));
      add_char_shrink(break_width[8])(character(s));
  end;
@z

@x [842] - font expansion
    break_width[1]:=break_width[1]+
      char_width(f)(char_info(f)(character(lig_char(s))));
@y
    break_width[1]:=break_width[1]+
      char_width(f)(char_info(f)(character(lig_char(s))));
    if (pdf_adjust_spacing > 1) and check_expand_pars(f) then begin
        prev_char_p := s;
        add_char_stretch(break_width[7])(character(lig_char(s)));
        add_char_shrink(break_width[8])(character(lig_char(s)));
    end;
@z

@x [842] - font expansion
  hlist_node,vlist_node,rule_node,kern_node:
    break_width[1]:=break_width[1]+width(s);
@y
  hlist_node,vlist_node,rule_node,kern_node: begin
    break_width[1]:=break_width[1]+width(s);
    if (type(s) = kern_node) and
       (pdf_adjust_spacing > 1) and (subtype(s) = normal)
    then begin
        add_kern_stretch(break_width[7])(s);
        add_kern_shrink(break_width[8])(s);
    end;
  end;
@z

@x [843] - font expansion
  begin do_all_six(convert_to_break_width);
@y
  begin do_all_eight(convert_to_break_width);
@z

@x [843] - font expansion
  begin do_all_six(store_break_width);
@y
  begin do_all_eight(store_break_width);
@z

@x [843] - font expansion
  do_all_six(new_delta_to_break_width);
@y
  do_all_eight(new_delta_to_break_width);
@z

@x [844] - font expansion
  do_all_six(new_delta_from_break_width);
@y
  do_all_eight(new_delta_from_break_width);
@z

@x [851] - font expansion, margin kerning
shortfall:=line_width-cur_active_width[1]; {we're this much too short}
@y
shortfall:=line_width-cur_active_width[1]; {we're this much too short}

{|
if pdf_output > 2 then begin
print_ln;
if (r <> null) and (break_node(r) <> null) then
    short_display_n(cur_break(break_node(r)), 5);
print_ln;
short_display_n(cur_p, 5);
print_ln;
end;
|}

if pdf_protrude_chars > 1 then
    shortfall := shortfall + total_pw(r, cur_p);
if (pdf_adjust_spacing > 1) and (shortfall <> 0) then begin
    margin_kern_stretch := 0;
    margin_kern_shrink := 0;
    if pdf_protrude_chars > 1 then 
        @<Calculate variations of marginal kerns@>;
    if (shortfall > 0) and ((total_font_stretch + margin_kern_stretch) > 0) 
    then begin
        if (total_font_stretch + margin_kern_stretch) > shortfall then
            shortfall := ((total_font_stretch + margin_kern_stretch) div 
                          (max_stretch_ratio div cur_font_step)) div 2
        else
            shortfall := shortfall - (total_font_stretch + margin_kern_stretch);
    end
    else if (shortfall < 0) and ((total_font_shrink + margin_kern_shrink) > 0) 
    then begin
        if (total_font_shrink + margin_kern_shrink) > -shortfall then
            shortfall := -((total_font_shrink + margin_kern_shrink) div 
                           (max_shrink_ratio div cur_font_step)) div 2
        else
            shortfall := shortfall + (total_font_shrink + margin_kern_shrink);
    end;
end;
@z

@x [860] - font expansion
    begin do_all_six(downdate_width);
@y
    begin do_all_eight(downdate_width);
@z

@x [860] - font expansion
    begin do_all_six(update_width);
    do_all_six(combine_two_deltas);
@y
    begin do_all_eight(update_width);
    do_all_eight(combine_two_deltas);
@z

@x [861] - font expansion
  begin do_all_six(update_active);
  do_all_six(copy_to_cur_active);
@y
  begin do_all_eight(update_active);
  do_all_eight(copy_to_cur_active);
@z

@x [862] - margin kerning, avoiding overfull boxes
@!auto_breaking:boolean; {is node |cur_p| outside a formula?}
@!prev_p:pointer; {helps to determine when glue nodes are breakpoints}
@y
@z

@x [863] - margin kerning, avoiding overfull boxes
  while (cur_p<>null)and(link(active)<>last_active) do
@y
  prev_char_p := null;
  prev_legal := null;
  rejected_cur_p := null;
  try_prev_break := false;
  before_rejected_cur_p := false;
  first_p := cur_p; {to access the first node of paragraph as the first active
                     node has |break_node=null|}
  while (cur_p<>null)and(link(active)<>last_active) do
@z

@x [864] - font expansion
do_all_six(store_background);@/
@y
do_all_eight(store_background);@/
@z

@x [666] - font expansion
  else act_width:=act_width+width(cur_p);
@y
  else begin
    act_width:=act_width+width(cur_p);
    if (pdf_adjust_spacing > 1) and (subtype(cur_p) = normal) then begin
        add_kern_stretch(active_width[7])(cur_p);
        add_kern_shrink(active_width[8])(cur_p);
    end;
  end;
@z

@x [866] - font expansion
  act_width:=act_width+char_width(f)(char_info(f)(character(lig_char(cur_p))));
@y
  act_width:=act_width+char_width(f)(char_info(f)(character(lig_char(cur_p))));
  if (pdf_adjust_spacing > 1) and check_expand_pars(f) then begin
      prev_char_p := cur_p;
      add_char_stretch(active_width[7])(character(lig_char(cur_p)));
      add_char_shrink(active_width[8])(character(lig_char(cur_p)));
  end;
@z

@x [867] - font expansion
act_width:=act_width+char_width(f)(char_info(f)(character(cur_p)));
@y
act_width:=act_width+char_width(f)(char_info(f)(character(cur_p)));
if (pdf_adjust_spacing > 1) and check_expand_pars(f) then begin
    prev_char_p := cur_p;
    add_char_stretch(active_width[7])(character(cur_p));
    add_char_shrink(active_width[8])(character(cur_p));
end;
@z

@x [869] - font expansion
begin s:=pre_break(cur_p); disc_width:=0;
@y
begin s:=pre_break(cur_p);
do_one_seven_eight(reset_disc_width);
@z

@x [869] - font expansion
  act_width:=act_width+disc_width;
  try_break(hyphen_penalty,hyphenated);
  act_width:=act_width-disc_width;
@y
  do_one_seven_eight(add_disc_width_to_active_width);
  try_break(hyphen_penalty,hyphenated);
  do_one_seven_eight(sub_disc_width_from_active_width);
@z

@x [870] - font expansion
  disc_width:=disc_width+char_width(f)(char_info(f)(character(s)));
@y
  disc_width[1]:=disc_width[1]+char_width(f)(char_info(f)(character(s)));
  if (pdf_adjust_spacing > 1) and check_expand_pars(f) then begin
      prev_char_p := s;
      add_char_stretch(disc_width[7])(character(s));
      add_char_shrink(disc_width[8])(character(s));
  end;
@z

@x [870] - font expansion
    disc_width:=disc_width+
      char_width(f)(char_info(f)(character(lig_char(s))));
@y
    disc_width[1]:=disc_width[1]+
      char_width(f)(char_info(f)(character(lig_char(s))));
    if (pdf_adjust_spacing > 1) and check_expand_pars(f) then begin
        prev_char_p := s;
        add_char_stretch(disc_width[7])(character(lig_char(s)));
        add_char_shrink(disc_width[8])(character(lig_char(s)));
    end;
@z

@x [870] - font expansion
  hlist_node,vlist_node,rule_node,kern_node:
    disc_width:=disc_width+width(s);
@y
  hlist_node,vlist_node,rule_node,kern_node: begin
    disc_width[1]:=disc_width[1]+width(s);
    if (type(s) = kern_node) and
       (pdf_adjust_spacing > 1) and (subtype(s) = normal)
    then begin
        add_kern_stretch(disc_width[7])(s);
        add_kern_shrink(disc_width[8])(s);
    end;
  end;
@z

@x [871] - font expansion
  act_width:=act_width+char_width(f)(char_info(f)(character(s)));
@y
  act_width:=act_width+char_width(f)(char_info(f)(character(s)));
  if (pdf_adjust_spacing > 1) and check_expand_pars(f) then begin
      prev_char_p := s;
      add_char_stretch(active_width[7])(character(s));
      add_char_shrink(active_width[8])(character(s));
  end;
@z

@x [871] - font expansion
    act_width:=act_width+
      char_width(f)(char_info(f)(character(lig_char(s))));
@y
    act_width:=act_width+
      char_width(f)(char_info(f)(character(lig_char(s))));
    if (pdf_adjust_spacing > 1) and check_expand_pars(f) then begin
        prev_char_p := s;
        add_char_stretch(active_width[7])(character(lig_char(s)));
        add_char_shrink(active_width[8])(character(lig_char(s)));
    end;
@z

@x [671] - font expansion
  hlist_node,vlist_node,rule_node,kern_node:
    act_width:=act_width+width(s);
@y
  hlist_node,vlist_node,rule_node,kern_node: begin
    act_width:=act_width+width(s);
    if (type(s) = kern_node) and
       (pdf_adjust_spacing > 1) and (subtype(s) = normal)
    then begin
        add_kern_stretch(active_width[7])(s);
        add_kern_shrink(active_width[8])(s);
    end;
  end;
@z

@x [877] - margin kerning
var q,@!r,@!s:pointer; {temporary registers for list manipulation}
@y
var q,@!r,@!s:pointer; {temporary registers for list manipulation}
    p, k: pointer; 
    w: scaled;
@z

@x [881] - margin kerning
@<Put the \(r)\.{\\rightskip} glue after node |q|@>;
done:
@y
@<Put the \(r)\.{\\rightskip} glue after node |q|@>;
done:
if pdf_protrude_chars > 0 then begin
    p := prev_rightmost(temp_head, q);
    while (p <> null) and discardable(p) do begin
        p := prev_rightmost(temp_head, p);
    end;
    w := right_pw(p);
    if p <> null then begin
        while link(p) <> q do
            p := link(p);
        if w <> 0 then begin
            k := new_margin_kern(-w, last_rightmost_char, right_side);
            link(p) := k;
            link(k) := q;
        end;
    end;
end;
@z

@x [887] - margin kerning
if left_skip<>zero_glue then
@y
if pdf_protrude_chars > 0 then begin
    p := q;
    while (p <> null) and discardable(p) do
        p := link(p);
    w := left_pw(p);
    if w <> 0 then begin
        k := new_margin_kern(-w, last_leftmost_char, left_side);
        link(k) := q;
        q := k;
    end;
end;
if left_skip<>zero_glue then
@z

@x [889] - font expansion, pre vadjust
just_box:=hpack(q,cur_width,exactly);
@y
if pdf_adjust_spacing > 0 then
    just_box := hpack(q, cur_width, cal_expand_ratio)
else
    just_box := hpack(q, cur_width, exactly);
@z

@x [1100] - margin kerning
var p:pointer; {the box}
@y
var p:pointer; {the box}
    r: pointer; {to remove marging kern nodes}
@z

@x [1100] - margin kerning
while link(tail)<>null do tail:=link(tail);
@y
if c = copy_code then begin
    while link(tail)<>null do tail:=link(tail);
end
else while link(tail) <> null do begin
    r := link(tail);
    if not is_char_node(r) and (type(r) = margin_kern_node) then begin
        link(tail) := link(r);
        free_avail(margin_char(r));
        free_node(r, margin_kern_node_size);
    end;
    tail:=link(tail);
end;
@z

@x [1147] - margin kerning
ligature_node:@<Make node |p| look like a |char_node|...@>;
@y
ligature_node:@<Make node |p| look like a |char_node|...@>;
margin_kern_node: d:=width(p);
@z

@x [1253] - font expansion
assign_font_int: begin n:=cur_chr; scan_font_ident; f:=cur_val;
  scan_optional_equals; scan_int;
  if n=0 then hyphen_char[f]:=cur_val@+else skew_char[f]:=cur_val;
  end;
@y
assign_font_int: begin n:=cur_chr; scan_font_ident; f:=cur_val;
  if n < lp_code_base then begin
    scan_optional_equals; scan_int;
    if n=0 then hyphen_char[f]:=cur_val@+else skew_char[f]:=cur_val;
  end
  else begin
    scan_char_num; p := cur_val;
    scan_optional_equals; scan_int;
    case n of 
    lp_code_base: set_lp_code(f, p, cur_val);
    rp_code_base: set_rp_code(f, p, cur_val);
    ef_code_base: set_ef_code(f, p, cur_val);
    end;
  end;
end;
@z

@x [1254] - font expansion
primitive("skewchar",assign_font_int,1);
@!@:skew_char_}{\.{\\skewchar} primitive@>
@y
primitive("skewchar",assign_font_int,1);
@!@:skew_char_}{\.{\\skewchar} primitive@>
primitive("lpcode",assign_font_int,lp_code_base);
@!@:lp_code_}{\.{\\lpcode} primitive@>
primitive("rpcode",assign_font_int,rp_code_base);
@!@:rp_code_}{\.{\\rpcode} primitive@>
primitive("efcode",assign_font_int,ef_code_base);
@!@:ef_code_}{\.{\\efcode} primitive@>
@z

@x [1255] - font expansion
assign_font_int: if chr_code=0 then print_esc("hyphenchar")
  else print_esc("skewchar");
@y
assign_font_int: case chr_code of
0: print_esc("hyphenchar");
1: print_esc("skewchar");
lp_code_base: print_esc("lpcode");
rp_code_base: print_esc("rpcode");
ef_code_base: print_esc("efcode");
endcases;
@z

@x [1344]
@d pdf_literal_node            == pdftex_first_extension_code + 0
@y
@d pdf_literal_node            == pdftex_first_extension_code + 0
@d pdf_font_expand_code        = pdftex_last_extension_code + 1
@z

@x [1344]
primitive("setlanguage",extension,set_language_code);@/
@!@:set_language_}{\.{\\setlanguage} primitive@>
@y
primitive("setlanguage",extension,set_language_code);@/
@!@:set_language_}{\.{\\setlanguage} primitive@>
primitive("pdffontexpand",extension,pdf_font_expand_code);@/
@!@:pdf_font_expand_}{\.{\\pdffontexpand} primitive@>
@z

@x [1346]
  immediate_code:print_esc("immediate");
  set_language_code:print_esc("setlanguage");
@y
  immediate_code:print_esc("immediate");
  set_language_code:print_esc("setlanguage");
  pdf_font_expand_code: print_esc("pdffontexpand");
@z

@x [1348]
set_language_code:@<Implement \.{\\setlanguage}@>;
@y
set_language_code:@<Implement \.{\\setlanguage}@>;
pdf_font_expand_code: @<Implement \.{\\pdffontexpand}@>;
@z

@x [1354]
@<Implement \.{\\special}@>=
begin new_whatsit(special_node,write_node_size); write_stream(tail):=null;
p:=scan_toks(false,true); write_tokens(tail):=def_ref;
end
@y
@<Implement \.{\\special}@>=
begin new_whatsit(special_node,write_node_size); write_stream(tail):=null;
p:=scan_toks(false,true); write_tokens(tail):=def_ref;
end

@ @<Implement \.{\\pdffontexpand}@>= 
    read_expand_font
@z
