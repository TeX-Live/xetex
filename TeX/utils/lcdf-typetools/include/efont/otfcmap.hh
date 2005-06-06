// -*- related-file-name: "../../libefont/otfcmap.cc" -*-
#ifndef EFONT_OTFCMAP_HH
#define EFONT_OTFCMAP_HH
#include <efont/otf.hh>
#include <lcdf/error.hh>
namespace Efont { namespace OpenType {

class Cmap { public:

    Cmap(const String &, ErrorHandler * = 0);
    // default destructor

    bool ok() const			{ return _error >= 0; }
    int error() const			{ return _error; }

    inline Glyph map_uni(uint32_t c) const;
    int map_uni(const Vector<uint32_t> &in, Vector<Glyph> &out) const;

  private:

    String _str;
    int _error;
    int _ntables;
    int _first_unicode_table;
    mutable Vector<int> _table_error;

    enum { HEADER_SIZE = 4, ENCODING_SIZE = 8,
	   HIBYTE_SUBHEADERS = 524 };
    enum Format { F_BYTE = 0, F_HIBYTE = 2, F_SEGMENTED = 4, F_TRIMMED = 6,
		  F_HIBYTE32 = 8, F_TRIMMED32 = 10, F_SEGMENTED32 = 12 };
    
    int parse_header(ErrorHandler *);
    int first_unicode_table() const	{ return _first_unicode_table; }
    int first_table(int platform, int encoding) const;
    int check_table(int t, ErrorHandler * = 0) const;
    Glyph map_table(int t, uint32_t, ErrorHandler * = 0) const;
    void dump_table(int t, Vector<uint32_t> &cs, Vector<uint32_t> &gs, ErrorHandler * = 0) const;
    
};


inline Glyph Cmap::map_uni(uint32_t c) const
{
    return map_table(first_unicode_table(), c, ErrorHandler::default_handler());
}

}}
#endif
