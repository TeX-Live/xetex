#ifndef OTFTOTFM_METRICS_HH
#define OTFTOTFM_METRICS_HH
#include <efont/otfgsub.hh>
#include <efont/otfgpos.hh>
namespace Efont { class CharstringProgram; }
class DvipsEncoding;
class GlyphFilter;

struct Setting {
    enum { NONE, FONT, SHOW, KERN, MOVE, RULE, DEAD };
    int op;
    int x;
    int y;
    Setting(int op_in, int x_in = 0, int y_in = 0)
	: op(op_in), x(x_in), y(y_in) { }
    bool valid_op() const		{ return op >= FONT && op <= RULE; }
};

class Metrics { public:

    typedef int Code;
    typedef Efont::OpenType::Glyph Glyph;
    enum { VIRTUAL_GLYPH = 0x10000 };

    typedef Efont::OpenType::Substitution Substitution;
    typedef Efont::OpenType::Positioning Positioning;

    Metrics(Efont::CharstringProgram *, int nglyphs);
    ~Metrics();

    void check() const;
    
    Glyph boundary_glyph() const	{ return _boundary_glyph; }
    Glyph emptyslot_glyph() const	{ return _emptyslot_glyph; }

    String coding_scheme() const		{ return _coding_scheme; }
    void set_coding_scheme(const String &s)	{ _coding_scheme = s; }

    int design_units() const			{ return _design_units; }
    void set_design_units(int du)		{ _design_units = du; }

    int n_mapped_fonts() const			{ return _mapped_fonts.size();}
    Efont::CharstringProgram *mapped_font(int i) const { return _mapped_fonts[i]; }
    const String &mapped_font_name(int i) const { return _mapped_font_names[i]; }
    int add_mapped_font(Efont::CharstringProgram *, const String &);

    inline int encoding_size() const		{ return _encoding.size(); }
    inline bool valid_code(Code) const;
    inline bool nonvirtual_code(Code) const;
    PermString code_name(Code) const;
    inline const char *code_str(Code) const;

    inline Glyph glyph(Code) const;
    inline uint32_t unicode(Code) const;
    inline Code encoding(Glyph) const;
    Code unicode_encoding(uint32_t) const;
    Code force_encoding(Glyph, int lookup_source = -1);
    void encode(Code, uint32_t uni, Glyph);
    void encode_virtual(Code, PermString, uint32_t uni, const Vector<Setting> &);

    void add_altselector_code(Code, int altselector_type);
    bool altselectors() const		{ return _altselectors.size() > 0; }
    
    inline Code base_code(Code) const;
    inline Glyph base_glyph(Code) const;
    void base_glyphs(Vector<Glyph> &) const;

    void add_ligature(Code in1, Code in2, Code out);
    Code pair_code(Code, Code, int lookup_source = -1);
    void add_kern(Code in1, Code in2, int kern);
    void add_single_positioning(Code, int pdx, int pdy, int adx);
    
    enum { CODE_ALL = 0x7FFFFFFF };
    void remove_ligatures(Code in1, Code in2);
    void remove_kerns(Code in1, Code in2);
    int reencode_right_ligkern(Code old_in2, Code new_in2);
    
    int apply(const Vector<Substitution>&, bool allow_single, int lookup, const GlyphFilter&, const Vector<PermString>& glyph_names);
    void apply_alternates(const Vector<Substitution>&, int lookup, const GlyphFilter&, const Vector<PermString>& glyph_names);
    int apply(const Vector<Positioning>&);

    void cut_encoding(int size);
    void shrink_encoding(int size, const DvipsEncoding &, ErrorHandler *);
    void make_base(int size);

    bool need_virtual(int size) const;
    enum SettingMode { SET_NONE = 0, SET_KEEP = 1, SET_INTERMEDIATE = 3 };
    bool setting(Code, Vector<Setting> &, SettingMode = SET_NONE) const;
    int ligatures(Code in1, Vector<Code> &in2, Vector<Code> &out, Vector<int> &context) const;
    int kerns(Code in1, Vector<Code> &in2, Vector<int> &kern) const;
    int kern(Code in1, Code in2) const;

    void unparse() const;

    struct Ligature {
	Code in2;
	Code out;
	Ligature(Code in2_, Code out_) : in2(in2_), out(out_) { }
    };

    struct Kern {
	Code in2;
	int kern;
	Kern(Code in2_, int kern_) : in2(in2_), kern(kern_) { }
    };

    struct VirtualChar {
	PermString name;
	Vector<Setting> setting;
    };
    
    struct Ligature3 {
	Code in1;
	Code in2;
	Code out;
	Ligature3(Code in1_, Code in2_, Code out_) : in1(in1_), in2(in2_), out(out_) { }
    };
    
  private:

    struct Char {
	Glyph glyph;
	Code base_code;
	uint32_t unicode;
	Vector<Ligature> ligatures;
	Vector<Kern> kerns;
	VirtualChar *virtual_char;
	int pdx;
	int pdy;
	int adx;
	Code built_in1;
	Code built_in2;
	int lookup_source;
	enum { BUILT = 1, INTERMEDIATE = 2, CONTEXT_ONLY = 4, LIVE = 8, BASE_LIVE = 16 };
	int flags;
	
	Char()			: glyph(0), base_code(-1), virtual_char(0), pdx(0), pdy(0), adx(0), built_in1(-1), built_in2(-1), lookup_source(-1), flags(0) { }
	void clear();
	void swap(Char &);
	bool visible() const		{ return glyph != 0; }
	bool visible_base() const	{ return glyph != 0 && glyph != VIRTUAL_GLYPH; }
	bool flag(int f) const		{ return (flags & f) != 0; }
	bool context_setting(Code in1, Code in2) const;
    };

    Vector<Char> _encoding;
    mutable Vector<int> _emap;

    Glyph _boundary_glyph;
    Glyph _emptyslot_glyph;

    Vector<Kern> _altselectors;

    String _coding_scheme;
    int _design_units;

    bool _liveness_marked : 1;

    Vector<Efont::CharstringProgram *> _mapped_fonts;
    Vector<String> _mapped_font_names;
    
    Metrics(const Metrics &);	// does not exist
    Metrics &operator=(const Metrics &); // does not exist

    inline void assign_emap(Glyph, Code);
    Code hard_encoding(Glyph) const;

    Ligature *ligature_obj(Code, Code);
    Kern *kern_obj(Code, Code);
    inline void new_ligature(Code, Code, Code);
    inline void repoint_ligature(Code, Ligature *, Code);

    friend bool operator<(const Ligature3 &, const Ligature3 &);
    void all_ligatures(Vector<Ligature3> &) const;
    void mark_liveness(int size, const Vector<Ligature3> &);
    void reencode(const Vector<Code> &);

    void apply_ligature(const Vector<Code> &, const Substitution *, int lookup);

};


inline bool
Metrics::valid_code(Code code) const
{
    return code >= 0 && code < _encoding.size();
}

inline bool
Metrics::nonvirtual_code(Code code) const
{
    return code >= 0 && code < _encoding.size() && !_encoding[code].virtual_char;
}

inline Metrics::Glyph
Metrics::glyph(Code code) const
{
    if (code < 0 || code >= _encoding.size())
	return 0;
    else
	return _encoding[code].glyph;
}

inline uint32_t
Metrics::unicode(Code code) const
{
    if (code < 0 || code >= _encoding.size())
	return 0;
    else
	return _encoding[code].unicode;
}

inline Metrics::Glyph
Metrics::base_glyph(Code code) const
{
    if (code < 0 || code >= _encoding.size() || _encoding[code].base_code < 0)
	return 0;
    else
	return _encoding[ _encoding[code].base_code ].glyph;
}

inline Metrics::Code
Metrics::base_code(Code code) const
{
    if (code < 0 || code >= _encoding.size())
	return 0;
    else
	return _encoding[code].base_code;
}

inline Metrics::Code
Metrics::encoding(Glyph g) const
{
    if (g >= 0 && g < _emap.size() && _emap.at_u(g) >= -1)
	return _emap.at_u(g);
    else
	return hard_encoding(g);
}

inline void
Metrics::assign_emap(Glyph g, Code code)
{
    if (g >= _emap.size())
	_emap.resize(g + 1, -1);
    _emap[g] = (_emap[g] == -1 || _emap[g] == code ? code : -2);
}

inline const char *
Metrics::code_str(Code code) const
{
    return code_name(code).c_str();
}

#endif
