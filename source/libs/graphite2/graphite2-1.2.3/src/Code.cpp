/*  GRAPHITE2 LICENSING

    Copyright 2010, SIL International
    All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should also have received a copy of the GNU Lesser General Public
    License along with this library in the file named "LICENSE".
    If not, write to the Free Software Foundation, 51 Franklin Street, 
    Suite 500, Boston, MA 02110-1335, USA or visit their web page on the 
    internet at http://www.fsf.org/licenses/lgpl.html.

Alternatively, the contents of this file may be used under the terms of the
Mozilla Public License (http://mozilla.org/MPL) or the GNU General Public
License, as published by the Free Software Foundation, either version 2
of the License or (at your option) any later version.
*/
// This class represents loaded graphite stack machine code.  It performs 
// basic sanity checks, on the incoming code to prevent more obvious problems
// from crashing graphite.
// Author: Tim Eves

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include "graphite2/Segment.h"
#include "inc/Code.h"
#include "inc/Face.h"
#include "inc/GlyphFace.h"
#include "inc/GlyphCache.h"
#include "inc/Machine.h"
#include "inc/Rule.h"
#include "inc/Silf.h"

#include <cstdio>

#ifdef NDEBUG
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#endif


using namespace graphite2;
using namespace vm;

namespace {

inline bool is_return(const instr i) {
    const opcode_t * opmap = Machine::getOpcodeTable();
    const instr pop_ret  = *opmap[POP_RET].impl,
                ret_zero = *opmap[RET_ZERO].impl,
                ret_true = *opmap[RET_TRUE].impl;
    return i == pop_ret || i == ret_zero || i == ret_true;
}

struct context
{
    context(uint8 ref=0) : codeRef(ref) {flags.changed=false; flags.referenced=false; flags.inserted=false;}
    struct { 
        uint8   changed:1,
                referenced:1,
                inserted:1;
    } flags;
    uint8       codeRef;
};

} // end namespace


class Machine::Code::decoder
{
public:
    struct limits;
    struct analysis
    {
        uint8     slotref;
        context   contexts[256];
        byte      max_ref;
        
        analysis() : slotref(0), max_ref(0) {};
        void set_ref(int index) throw();
        void set_changed(int index) throw();

    };
    
    decoder(const limits & lims, Code &code) throw();
    
    bool        load(const byte * bc_begin, const byte * bc_end);
    void        apply_analysis(instr * const code, instr * code_end);
    byte        max_ref() { return _analysis.max_ref; }
    int         pre_context() const { return _pre_context; }
    
private:
    opcode      fetch_opcode(const byte * bc);
    void        analyse_opcode(const opcode, const int8 * const dp) throw();
    bool        emit_opcode(opcode opc, const byte * & bc);
    bool 		validate_opcode(const opcode opc, const byte * const bc);
    bool        valid_upto(const uint16 limit, const uint16 x) const throw();
    void        failure(const status_t s) const throw() { _code.failure(s); }
    
    Code              & _code;
    int                 _pre_context;
    uint16              _rule_length;
    instr             * _instr;
    byte              * _data;
    const limits      & _max;
    analysis            _analysis;
};


struct Machine::Code::decoder::limits
{
  const byte * const bytecode;
  const uint8        pre_context;
  const uint16       rule_length,
                     classes,
                     glyf_attrs,
                     features;
  const byte         attrid[gr_slatMax];
};
   
inline Machine::Code::decoder::decoder(const limits & lims, Code &code) throw()
: _code(code),
  _pre_context(code._constraint ? 0 : lims.pre_context), 
  _rule_length(code._constraint ? 1 : lims.rule_length), 
  _instr(code._code), _data(code._data), _max(lims)
{ }
    


Machine::Code::Code(bool is_constraint, const byte * bytecode_begin, const byte * const bytecode_end,
           uint8 pre_context, uint16 rule_length, const Silf & silf, const Face & face)
 :  _code(0), _data(0), _data_size(0), _instr_count(0), _max_ref(0), _status(loaded),
    _constraint(is_constraint), _modify(false), _delete(false), _own(true)
{
#ifdef GRAPHITE2_TELEMETRY
    telemetry::category _code_cat(face.tele.code);
#endif
    assert(bytecode_begin != 0);
    if (bytecode_begin == bytecode_end)
    {
      ::new (this) Code();
      return;
    }
    assert(bytecode_end > bytecode_begin);
    const opcode_t *    op_to_fn = Machine::getOpcodeTable();
    
    // Allocate code and dat target buffers, these sizes are a worst case 
    // estimate.  Once we know their real sizes the we'll shrink them.
    _code = static_cast<instr *>(malloc((bytecode_end - bytecode_begin)
                                             * sizeof(instr)));
    _data = static_cast<byte *>(malloc((bytecode_end - bytecode_begin)
                                             * sizeof(byte)));
    
    if (!_code || !_data) {
        failure(alloc_failed);
        return;
    }
    
    const decoder::limits lims = {
        bytecode_end,
        pre_context,
        rule_length,
        silf.numClasses(),
        face.glyphs().numAttrs(),
        face.numFeatures(), 
        {1,1,1,1,1,1,1,1, 
         1,1,1,1,1,1,1,255,
         1,1,1,1,1,1,1,1, 
         1,1,1,1,1,1,0,0, 
         0,0,0,0,0,0,0,0, 
         0,0,0,0,0,0,0,0, 
         0,0,0,0,0,0,0, silf.numUser()}
    };
    
    decoder dec(lims, *this);
    if(!dec.load(bytecode_begin, bytecode_end))
       return;
    
    // Is this an empty program?
    if (_instr_count == 0)
    {
      release_buffers();
      ::new (this) Code();
      return;
    }
    
    // When we reach the end check we've terminated it correctly
    if (!is_return(_code[_instr_count-1])) {
        failure(missing_return);
        return;
    }

    assert((_constraint && immutable()) || !_constraint);
    dec.apply_analysis(_code, _code + _instr_count);
    _max_ref = dec.max_ref();
    
    // Now we know exactly how much code and data the program really needs
    // realloc the buffers to exactly the right size so we don't waste any 
    // memory.
    assert((bytecode_end - bytecode_begin) >= std::ptrdiff_t(_instr_count));
    assert((bytecode_end - bytecode_begin) >= std::ptrdiff_t(_data_size));
    _code = static_cast<instr *>(realloc(_code, (_instr_count+1)*sizeof(instr)));
    _data = static_cast<byte *>(realloc(_data, _data_size*sizeof(byte)));

    if (!_code)
        failure(alloc_failed);

    // Make this RET_ZERO, we should never reach this but just in case ...
    _code[_instr_count] = op_to_fn[RET_ZERO].impl[_constraint];

#ifdef GRAPHITE2_TELEMETRY
    telemetry::count_bytes(_data_size + (_instr_count+1)*sizeof(instr));
#endif
}

Machine::Code::~Code() throw ()
{
    if (_own)
        release_buffers();
}


bool Machine::Code::decoder::load(const byte * bc, const byte * bc_end)
{
    while (bc < bc_end)
    {
        const opcode opc = fetch_opcode(bc++);
        if (opc == vm::MAX_OPCODE)
            return false;
        
        analyse_opcode(opc, reinterpret_cast<const int8 *>(bc));
        
        if (!emit_opcode(opc, bc))
            return false;
    }
    
    return bool(_code);
}

// Validation check and fixups.
//

opcode Machine::Code::decoder::fetch_opcode(const byte * bc)
{
    const opcode opc = opcode(*bc++);

    // Do some basic sanity checks based on what we know about the opcode
    if (!validate_opcode(opc, bc))	return MAX_OPCODE;

    // And check it's arguments as far as possible
    switch (opc)
    {
        case NOP :
        case PUSH_BYTE :
        case PUSH_BYTEU :
        case PUSH_SHORT :
        case PUSH_SHORTU :
        case PUSH_LONG :
        case ADD :
        case SUB :
        case MUL :
        case DIV :
        case MIN_ :
        case MAX_ :
        case NEG :
        case TRUNC8 :
        case TRUNC16 :
        case COND :
        case AND :
        case OR :
        case NOT :
        case EQUAL :
        case NOT_EQ :
        case LESS :
        case GTR :
        case LESS_EQ :
        case GTR_EQ :
            break;
        case NEXT :
        case NEXT_N :           // runtime checked
        case COPY_NEXT :
            ++_pre_context;
            break;
        case PUT_GLYPH_8BIT_OBS :
            valid_upto(_max.classes, bc[0]);
            break;
        case PUT_SUBS_8BIT_OBS :
            valid_upto(_rule_length, _pre_context + int8(bc[0]));
            valid_upto(_max.classes, bc[1]);
            valid_upto(_max.classes, bc[2]);
            break;
        case PUT_COPY :
            valid_upto(_rule_length, _pre_context + int8(bc[0]));
            break;
        case INSERT :
            --_pre_context;
            break;
        case DELETE :
            break;
        case ASSOC :
            for (uint8 num = bc[0]; num; --num)
                valid_upto(_rule_length, _pre_context + int8(bc[num]));
            break;
        case CNTXT_ITEM :
            valid_upto(_max.rule_length, _max.pre_context + int8(bc[0]));
            if (bc + 2 + bc[1] >= _max.bytecode)  failure(jump_past_end);
            if (_pre_context != 0)                failure(nested_context_item);
            break;
        case ATTR_SET :
        case ATTR_ADD :
        case ATTR_SUB :
        case ATTR_SET_SLOT :
        	valid_upto(gr_slatMax, bc[0]);
            break;
        case IATTR_SET_SLOT :
            if (valid_upto(gr_slatMax, bc[0]))
            	valid_upto(_max.attrid[bc[0]], bc[1]);
            break;
        case PUSH_SLOT_ATTR :
            valid_upto(gr_slatMax, bc[0]);
            valid_upto(_rule_length, _pre_context + int8(bc[1]));
            break;
        case PUSH_GLYPH_ATTR_OBS :
            valid_upto(_max.glyf_attrs, bc[0]);
            valid_upto(_rule_length, _pre_context + int8(bc[1]));
            break;
        case PUSH_GLYPH_METRIC :
            valid_upto(kgmetDescent, bc[0]);
            valid_upto(_rule_length, _pre_context + int8(bc[1]));
            // level: dp[2] no check necessary
            break;
        case PUSH_FEAT :
            valid_upto(_max.features, bc[0]);
            valid_upto(_rule_length, _pre_context + int8(bc[1]));
            break;
        case PUSH_ATT_TO_GATTR_OBS :
            valid_upto(_max.glyf_attrs, bc[0]);
            valid_upto(_rule_length, _pre_context + int8(bc[1]));
            break;
        case PUSH_ATT_TO_GLYPH_METRIC :
            valid_upto(kgmetDescent, bc[0]);
            valid_upto(_rule_length, _pre_context + int8(bc[1]));
            // level: dp[2] no check necessary
            break;
        case PUSH_ISLOT_ATTR :
            if (valid_upto(gr_slatMax, bc[0]))
            {
            	valid_upto(_rule_length, _pre_context + int8(bc[1]));
            	valid_upto(_max.attrid[bc[0]], bc[2]);
            }
            break;
        case PUSH_IGLYPH_ATTR :// not implemented
        case POP_RET :
        case RET_ZERO :
        case RET_TRUE :
            break;
        case IATTR_SET :
        case IATTR_ADD :
        case IATTR_SUB :
            if (valid_upto(gr_slatMax, bc[0]))
            	valid_upto(_max.attrid[bc[0]], bc[1]);
            break;
        case PUSH_PROC_STATE :  // dummy: dp[0] no check necessary
        case PUSH_VERSION :
            break;
        case PUT_SUBS :
            valid_upto(_rule_length, _pre_context + int8(bc[0]));
            valid_upto(_max.classes, uint16(bc[1]<< 8) | bc[2]);
            valid_upto(_max.classes, uint16(bc[3]<< 8) | bc[4]);
            break;
        case PUT_SUBS2 :        // not implemented
        case PUT_SUBS3 :        // not implemented
            break;
        case PUT_GLYPH :
            valid_upto(_max.classes, uint16(bc[0]<< 8) | bc[1]);
            break;
        case PUSH_GLYPH_ATTR :
        case PUSH_ATT_TO_GLYPH_ATTR :
            valid_upto(_max.glyf_attrs, uint16(bc[0]<< 8) | bc[1]);
            valid_upto(_rule_length, _pre_context + int8(bc[2]));
            break;
        default:
            failure(invalid_opcode);
            break;
    }

    return bool(_code) ? opc : MAX_OPCODE;
}


void Machine::Code::decoder::analyse_opcode(const opcode opc, const int8  * arg) throw()
{
  if (_code._constraint) return;
  
  switch (opc)
  {
    case DELETE :
      _code._delete = true;
      break;
    case PUT_GLYPH_8BIT_OBS :
    case PUT_GLYPH :
      _code._modify = true;
      _analysis.set_changed(_analysis.slotref);
      break;
    case NEXT :
    case COPY_NEXT :
      if (!_analysis.contexts[_analysis.slotref].flags.inserted)
        ++_analysis.slotref;
      _analysis.contexts[_analysis.slotref] = context(_code._instr_count+1);
      if (_analysis.slotref > _analysis.max_ref) _analysis.max_ref = _analysis.slotref;
      break;
    case INSERT :
      _analysis.contexts[_analysis.slotref].flags.inserted = true;
      _code._modify = true;
      break;
    case PUT_SUBS_8BIT_OBS :    // slotref on 1st parameter
    case PUT_SUBS : 
      _code._modify = true;
      _analysis.set_changed(_analysis.slotref);
      // no break
    case PUT_COPY :
    {
      if (arg[0] != 0) { _analysis.set_changed(_analysis.slotref); _code._modify = true; }
      if (arg[0] <= 0 && -arg[0] <= _analysis.slotref - _analysis.contexts[_analysis.slotref].flags.inserted)
        _analysis.set_ref(_analysis.slotref + arg[0] - _analysis.contexts[_analysis.slotref].flags.inserted);
      else if (_analysis.slotref + arg[0] > _analysis.max_ref) _analysis.max_ref = _analysis.slotref + arg[0];
      break;
    }
    case PUSH_ATT_TO_GATTR_OBS : // slotref on 2nd parameter
        if (_code._constraint) return;
        // no break
    case PUSH_GLYPH_ATTR_OBS :
    case PUSH_SLOT_ATTR :
    case PUSH_GLYPH_METRIC :
    case PUSH_ATT_TO_GLYPH_METRIC :
    case PUSH_ISLOT_ATTR :
    case PUSH_FEAT :
      if (arg[1] <= 0 && -arg[1] <= _analysis.slotref - _analysis.contexts[_analysis.slotref].flags.inserted)
        _analysis.set_ref(_analysis.slotref + arg[1] - _analysis.contexts[_analysis.slotref].flags.inserted);
      else if (_analysis.slotref + arg[1] > _analysis.max_ref) _analysis.max_ref = _analysis.slotref + arg[1];
      break;
    case PUSH_ATT_TO_GLYPH_ATTR :
        if (_code._constraint) return;
        // no break
    case PUSH_GLYPH_ATTR :
      if (arg[2] <= 0 && -arg[2] <= _analysis.slotref - _analysis.contexts[_analysis.slotref].flags.inserted)
        _analysis.set_ref(_analysis.slotref + arg[2] - _analysis.contexts[_analysis.slotref].flags.inserted);
      else if (_analysis.slotref + arg[2] > _analysis.max_ref) _analysis.max_ref = _analysis.slotref + arg[2];
      break;
    case ASSOC :                // slotrefs in varargs
      break;
    default:
        break;
  }
}


bool Machine::Code::decoder::emit_opcode(opcode opc, const byte * & bc)
{
    const opcode_t * op_to_fn = Machine::getOpcodeTable();
    const opcode_t & op       = op_to_fn[opc];
    if (op.impl[_code._constraint] == 0)
    {
        failure(unimplemented_opcode_used);
        return false;
    }

    const size_t     param_sz = op.param_sz == VARARGS ? bc[0] + 1 : op.param_sz;

    // Add this instruction
    *_instr++ = op.impl[_code._constraint]; 
    ++_code._instr_count;

    // Grab the parameters
    if (param_sz) {
        memcpy(_data, bc, param_sz * sizeof(byte));
        bc               += param_sz;
        _data            += param_sz;
        _code._data_size += param_sz;
    }
    
    // recursively decode a context item so we can split the skip into 
    // instruction and data portions.
    if (opc == CNTXT_ITEM)
    {
        assert(_pre_context == 0);
        _pre_context = _max.pre_context + int8(_data[-2]);
        _rule_length = _max.rule_length;

        const size_t ctxt_start = _code._instr_count;
        byte & instr_skip = _data[-1];
        byte & data_skip  = *_data++;
        ++_code._data_size;

        if (load(bc, bc + instr_skip))
        {
            bc += instr_skip;
            data_skip  = instr_skip - (_code._instr_count - ctxt_start);
            instr_skip = _code._instr_count - ctxt_start;

            _rule_length = 1;
            _pre_context = 0;
        }
    }
    
    return bool(_code);
}


void Machine::Code::decoder::apply_analysis(instr * const code, instr * code_end)
{
    // insert TEMP_COPY commands for slots that need them (that change and are referenced later)
    int tempcount = 0;
    if (_code._constraint) return;

    const instr temp_copy = Machine::getOpcodeTable()[TEMP_COPY].impl[0];
    for (const context * c = _analysis.contexts, * const ce = c + _analysis.slotref; c != ce; ++c)
    {
        if (!c->flags.referenced || !c->flags.changed) continue;
        
        instr * const tip = code + c->codeRef + tempcount;        
        memmove(tip+1, tip, (code_end - tip) * sizeof(instr));
        *tip = temp_copy;
        ++code_end;
        ++tempcount;
    }
    
    _code._instr_count = code_end - code;
}


inline
bool Machine::Code::decoder::validate_opcode(const opcode opc, const byte * const bc)
{
	if (opc >= MAX_OPCODE)
	{
		failure(invalid_opcode);
		return false;
	}
	const opcode_t & op = Machine::getOpcodeTable()[opc];
	const size_t param_sz = op.param_sz == VARARGS ? bc[0] + 1 : op.param_sz;
	if (bc + param_sz > _max.bytecode)
	{
		failure(arguments_exhausted);
		return false;
	}
	return true;
}


bool Machine::Code::decoder::valid_upto(const uint16 limit, const uint16 x) const throw()
{
	const bool t = x < limit;
    if (!t)	failure(out_of_range_data);
    return t;
}


inline 
void Machine::Code::failure(const status_t s) throw() {
    release_buffers();
    _status = s;
}


inline
void Machine::Code::decoder::analysis::set_ref(const int index) throw() {
    contexts[index].flags.referenced = true;
    if (index > max_ref) max_ref = index;
}


inline
void Machine::Code::decoder::analysis::set_changed(const int index) throw() {
    contexts[index].flags.changed = true;
    if (index > max_ref) max_ref = index;
}


void Machine::Code::release_buffers() throw()
{
    free(_code);
    free(_data);
    _code = 0;
    _data = 0;
    _own  = false;
}


int32 Machine::Code::run(Machine & m, slotref * & map) const
{
    assert(_own);
    assert(*this);          // Check we are actually runnable

    if (m.slotMap().size() <= size_t(_max_ref + m.slotMap().context()))
    {
        m._status = Machine::slot_offset_out_bounds;
//        return (m.slotMap().end() - map);
        return 1;
    }

    return  m.run(_code, _data, map);
}

