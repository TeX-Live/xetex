// -*- related-file-name: "../include/efont/otf.hh" -*-

/* otf.{cc,hh} -- OpenType font basics
 *
 * Copyright (c) 2002-2004 Eddie Kohler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version. This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/otf.hh>
#include <lcdf/error.hh>
#include <lcdf/straccum.hh>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <efont/otfdata.hh>	// for ntohl()

#ifndef static_assert
#define static_assert(c) switch (c) case 0: case (c):
#endif

#define USHORT_AT(d)		(ntohs(*(const uint16_t *)(d)))
#define ULONG_AT(d)		(ntohl(*(const uint32_t *)(d)))
#define ULONG_AT2(d)		((USHORT_AT((d)) << 16) | USHORT_AT((d)+2))

namespace Efont { namespace OpenType {

Vector<PermString> debug_glyph_names;

Font::Font(const String &s, ErrorHandler *errh)
    : _str(s)
{
    _str.align(4);
    _error = parse_header(errh ? errh : ErrorHandler::silent_handler());
}

int
Font::parse_header(ErrorHandler *errh)
{
    // HEADER FORMAT:
    // Fixed	sfnt version
    // USHORT	numTables
    // USHORT	searchRange
    // USHORT	entrySelector
    // USHORT	rangeShift
    int len = length();
    const uint8_t *data = this->data();
    if (HEADER_SIZE > len)
	return errh->error("OTF too small for header"), -EFAULT;
    if ((data[0] != 'O' || data[1] != 'T' || data[2] != 'T' || data[3] != 'O')
	&& (data[0] != '\000' || data[1] != '\001'))
	return errh->error("not an OpenType font (bad magic number)"), -ERANGE;
    int ntables = USHORT_AT(data + 4);
    if (ntables == 0)
	return errh->error("OTF contains no tables"), -EINVAL;
    if (HEADER_SIZE + TABLE_DIR_ENTRY_SIZE * ntables > len)
	return errh->error("OTF table directory out of range"), -EFAULT;

    // TABLE DIRECTORY ENTRY FORMAT:
    // ULONG	tag
    // ULONG	checksum
    // ULONG	offset
    // ULONG	length
    uint32_t last_tag = 0U;
    for (int i = 0; i < ntables; i++) {
	int loc = HEADER_SIZE + TABLE_DIR_ENTRY_SIZE * i;
	uint32_t tag = ULONG_AT(data + loc);
	uint32_t offset = ULONG_AT(data + loc + 8);
	uint32_t length = ULONG_AT(data + loc + 12);
	if (tag <= last_tag)
	    return errh->error("tags out of order"), -EINVAL;
	if (offset + length > (uint32_t) len)
	    return errh->error("OTF data for '%s' out of range", Tag(tag).text().c_str()), -EFAULT;
	last_tag = tag;
    }
    
    return 0;
}

String
Font::table(Tag tag) const
{
    if (error() < 0)
	return String();
    const uint8_t *entry = tag.table_entry(data() + HEADER_SIZE, USHORT_AT(data() + 4), TABLE_DIR_ENTRY_SIZE);
    if (entry)
	return _str.substring(ULONG_AT(entry + 8), ULONG_AT(entry + 12));
    else
	return String();
}



/**************************
 * Tag                    *
 *                        *
 **************************/

Tag::Tag(const char *s)
    : _tag(0)
{
    if (!s)
	s = "";
    for (int i = 0; i < 4; i++)
	if (*s == 0)
	    _tag = (_tag << 8) | 0x20;
	else if (*s < 32 || *s > 126) { // don't care if s is signed
	    _tag = 0;
	    return;
	} else
	    _tag = (_tag << 8) | *s++;
    if (*s)
	_tag = 0;
}

Tag::Tag(const String &s)
    : _tag(0)
{
    if (s.length() <= 4) {
	const char *ss = s.data();
	for (int i = 0; i < s.length(); i++, ss++)
	    if (*ss < 32 || *ss > 126) {
		_tag = 0;
		return;
	    } else
		_tag = (_tag << 8) | *ss;
	for (int i = s.length(); i < 4; i++)
	    _tag = (_tag << 8) | 0x20;
    }
}

bool
Tag::valid() const
{
    uint32_t tag = _tag;
    for (int i = 0; i < 4; i++, tag >>= 8)
	if ((tag & 255) < 32 || (tag & 255) > 126)
	    return false;
    return true;
}

String
Tag::text() const
{
    StringAccum sa;
    uint32_t tag = _tag;
    for (int i = 0; i < 4; i++, tag = (tag << 8) | 0x20)
	if (tag != 0x20202020) {
	    uint8_t c = (tag >> 24) & 255;
	    if (c < 32 || c > 126)
		sa.snprintf(6, "\\%03o", c);
	    else
		sa << c;
	}
    return sa.take_string();
}

String
Tag::langsys_text(Tag script, Tag langsys)
{
    if (!langsys.null())
	return script.text() + "." + langsys.text();
    else
	return script.text();
}

const uint8_t *
Tag::table_entry(const uint8_t *table, int n, int entry_size) const
{
    assert(((uintptr_t)table & 1) == 0);
    int l = 0;
    int r = n - 1;
    while (l <= r) {
	int m = (l + r) / 2;
	const uint8_t *entry = table + m * entry_size;
	uint32_t m_tag = ULONG_AT2(entry);
	if (_tag < m_tag)
	    r = m - 1;
	else if (_tag == m_tag)
	    return entry;
	else
	    l = m + 1;
    }
    return 0;
}



/**************************
 * ScriptList             *
 *                        *
 **************************/

int
ScriptList::assign(const String &str, ErrorHandler *errh)
{
    _str = str;
    _str.align(4);
    int result = check_header(errh ? errh : ErrorHandler::silent_handler());
    if (result < 0)
	_str = String();
    return result;
}

int
ScriptList::check_header(ErrorHandler *errh)
{
    // HEADER FORMAT:
    // USHORT	scriptCount
    // 6bytes	scriptRecord[]
    int scriptCount;
    if (_str.length() < SCRIPTLIST_HEADERSIZE
	|| (scriptCount = USHORT_AT(_str.udata()),
	    _str.length() < SCRIPTLIST_HEADERSIZE + scriptCount*SCRIPT_RECSIZE))
	return errh->error("OTF ScriptList too short");

    // XXX check that scripts are sorted

    return 0;
}

int
ScriptList::script_offset(Tag script) const
{
    if (_str.length() == 0)
	return -1;
    const uint8_t *data = _str.udata();
    if (const uint8_t *entry = script.table_entry(data + SCRIPTLIST_HEADERSIZE, USHORT_AT(data), SCRIPT_RECSIZE))
	return USHORT_AT(entry + 4);
    else
	return 0;
}

int
ScriptList::check_script(Tag tag, int script_off, ErrorHandler *errh) const
{
    const uint8_t *data = _str.udata();
    int langSysCount;
    if (_str.length() < script_off + SCRIPT_HEADERSIZE
	|| (langSysCount = USHORT_AT(data + script_off + 2),
	    (_str.length() < script_off + SCRIPT_HEADERSIZE + langSysCount*LANGSYS_RECSIZE)))
	return (errh ? errh->error("OTF Script table for '%s' out of range", tag.text().c_str()) : -1);
    // XXX check that langsys are sorted
    return 0;
}

int
ScriptList::langsys_offset(Tag script, Tag langsys, ErrorHandler *errh) const
{
    int script_off = script_offset(script);
    if (script_off == 0) {
	script = Tag("DFLT");
	script_off = script_offset(script);
    }
    if (script_off <= 0)
	return script_off;

    // check script bounds
    if (check_script(script, script_off, errh) < 0)
	return -1;

    // search script table
    const uint8_t *data = _str.udata();
    int langSysCount = USHORT_AT(data + script_off + 2);
    if (const uint8_t *entry = langsys.table_entry(data + script_off + SCRIPT_HEADERSIZE, langSysCount, LANGSYS_RECSIZE))
	return script_off + USHORT_AT(entry + 4);

    // return default
    int defaultLangSys = USHORT_AT(data + script_off);
    if (defaultLangSys != 0)
	return script_off + defaultLangSys;
    else
	return 0;
}

int
ScriptList::language_systems(Vector<Tag> &script, Vector<Tag> &langsys, ErrorHandler *errh) const
{
    script.clear();
    langsys.clear();
    
    const uint8_t *data = _str.udata();
    int nscripts = USHORT_AT(data);
    for (int i = 0; i < nscripts; i++) {
	Tag script_tag(ULONG_AT2(data + SCRIPTLIST_HEADERSIZE + i*SCRIPT_RECSIZE));
	int script_off = USHORT_AT(data + SCRIPTLIST_HEADERSIZE + i*SCRIPT_RECSIZE + 4);
	if (check_script(script_tag, script_off, errh) < 0)
	    return -1;
	const uint8_t *script_table = data + script_off;
	if (USHORT_AT(script_table) != 0) // default LangSys
	    script.push_back(script_tag), langsys.push_back(Tag());
	int nlangsys = USHORT_AT(script_table + 2);
	for (int j = 0; j < nlangsys; j++) {
	    Tag langsys_tag(ULONG_AT2(script_table + SCRIPT_HEADERSIZE + j*LANGSYS_RECSIZE));
	    script.push_back(script_tag), langsys.push_back(langsys_tag);
	}
    }
    
    return 0;
}

int
ScriptList::features(Tag script, Tag langsys, int &required_fid, Vector<int> &fids, ErrorHandler *errh, bool clear_fids) const
{
    required_fid = -1;
    if (clear_fids)
	fids.clear();
    
    int offset = langsys_offset(script, langsys);
    if (offset <= 0)
	return offset;

    // check langsys bounds
    const uint8_t *data = _str.udata();
    int featureCount;
    if (_str.length() < offset + LANGSYS_HEADERSIZE
	|| (featureCount = USHORT_AT(data + offset + 4),
	    (_str.length() < offset + LANGSYS_HEADERSIZE + featureCount*FEATURE_RECSIZE)))
	return (errh ? errh->error("OTF LangSys table for '%s/%s' out of range", script.text().c_str(), langsys.text().c_str()) : -1);

    // search langsys table
    int f = USHORT_AT(data + offset + 2);
    if (f != 0xFFFF)
	required_fid = f;
    data += offset + 6;
    for (int i = 0; i < featureCount; i++, data += FEATURE_RECSIZE)
	fids.push_back(USHORT_AT(data));
    
    return 0;
}



/**************************
 * FeatureList            *
 *                        *
 **************************/

int
FeatureList::assign(const String &str, ErrorHandler *errh)
{
    _str = str;
    _str.align(2);
    int result = check_header(errh ? errh : ErrorHandler::silent_handler());
    if (result < 0)
	_str = String();
    return result;
}

int
FeatureList::check_header(ErrorHandler *errh)
{
    int featureCount;
    if (_str.length() < FEATURELIST_HEADERSIZE
	|| (featureCount = USHORT_AT(_str.udata()),
	    _str.length() < FEATURELIST_HEADERSIZE + featureCount*FEATURE_RECSIZE))
	return errh->error("OTF FeatureList too short");
    return 0;
}

Tag
FeatureList::tag(int fid) const
{
    if (_str.length() == 0)
	return Tag();
    const uint8_t *data = _str.udata();
    int nfeatures = USHORT_AT(data);
    if (fid >= 0 && fid < nfeatures)
	return Tag(ULONG_AT2(data + FEATURELIST_HEADERSIZE + fid*FEATURE_RECSIZE));
    else
	return Tag();
}

String
FeatureList::params(int fid, int length, ErrorHandler *errh, bool old_style_offset) const
{
    if (_str.length() == 0 || length < 0)
	return String();
    if (errh == 0)
	errh = ErrorHandler::silent_handler();

    const uint8_t *data = _str.udata();
    int len = _str.length();
    int nfeatures = USHORT_AT(data);
    if (fid < 0 || fid >= nfeatures)
	return errh->error("OTF feature ID '%d' out of range", fid), String();
    int foff = USHORT_AT(data + FEATURELIST_HEADERSIZE + fid*FEATURE_RECSIZE + 4);
    if (len < foff + FEATURE_HEADERSIZE)
	return errh->error("OTF LookupList for feature ID '%d' too short", fid), String();
    int poff = USHORT_AT(data + foff);
    if (poff == 0)
	return String();
    if (!old_style_offset)
	poff += foff;
    if (len < poff + length)
	return errh->error("OTF feature parameters for feature ID '%d' out of range", fid), String();
    else
	return _str.substring(poff, length);
}

int
FeatureList::find(Tag tag, const Vector<int> &fids) const
{
    if (fids.size() == 0 || _str.length() == 0)
	return -1;

    const uint8_t *data = _str.udata();
    int nfeatures = USHORT_AT(data);
    for (const int *fidp = fids.begin(); fidp != fids.end(); fidp++)
	if (*fidp >= 0 && *fidp < nfeatures) {
	    uint32_t ftag = ULONG_AT2(data + FEATURELIST_HEADERSIZE + (*fidp)*FEATURE_RECSIZE);
	    if (ftag == tag.value())
		return *fidp;
	}

    return -1;
}

void
FeatureList::filter(Vector<int> &fids, const Vector<Tag> &sorted_ftags) const
{
    if (_str.length() == 0)
	fids.clear();
    else {
	std::sort(fids.begin(), fids.end()); // sort fids
	
	int i = 0, j = 0;
	while (i < fids.size() && fids[i] < 0)
	    fids[i++] = 0x7FFFFFFF;

	// XXX check that feature list is in alphabetical order
	
	const uint8_t *data = _str.udata();
	int nfeatures = USHORT_AT(data);
	while (i < fids.size() && j < sorted_ftags.size() && fids[i] < nfeatures) {
	    uint32_t ftag = ULONG_AT2(data + FEATURELIST_HEADERSIZE + fids[i]*FEATURE_RECSIZE);
	    if (ftag < sorted_ftags[j].value()) { // not an interesting feature
		// replace featureID with a large number, remove later
		fids[i] = 0x7FFFFFFF;
		i++;
	    } else if (ftag == sorted_ftags[j].value()) // interesting feature
		i++;
	    else		// an interesting feature is not available
		j++;
	}

	fids.resize(i);		// remove remaining uninteresting features
	std::sort(fids.begin(), fids.end()); // resort, to move bad ones last
	while (fids.size() && fids.back() == 0x7FFFFFFF)
	    fids.pop_back();
    }
}

int
FeatureList::lookups(int fid, Vector<int> &results, ErrorHandler *errh, bool clear_results) const
{
    if (clear_results)
	results.clear();
    if (_str.length() == 0)
	return -1;
    if (errh == 0)
	errh = ErrorHandler::silent_handler();

    const uint8_t *data = _str.udata();
    int len = _str.length();
    int nfeatures = USHORT_AT(data);
    if (fid < 0 || fid >= nfeatures)
	return errh->error("OTF feature ID '%d' out of range", fid);
    int foff = USHORT_AT(data + FEATURELIST_HEADERSIZE + fid*FEATURE_RECSIZE + 4);
    int lookupCount;
    if (len < foff + FEATURE_HEADERSIZE
	|| (lookupCount = USHORT_AT(data + foff + 2),
	    len < foff + FEATURE_HEADERSIZE + lookupCount*LOOKUPLIST_RECSIZE))
	return errh->error("OTF LookupList for feature ID '%d' too short", fid);
    const uint8_t *ldata = data + foff + FEATURE_HEADERSIZE;
    for (int j = 0; j < lookupCount; j++, ldata += LOOKUPLIST_RECSIZE)
	results.push_back(USHORT_AT(ldata));

    return 0;
}

int
FeatureList::lookups(const Vector<int> &fids, Vector<int> &results, ErrorHandler *errh) const
{
    results.clear();
    if (_str.length() == 0)
	return -1;

    for (int i = 0; i < fids.size(); i++)
	if (lookups(fids[i], results, errh, false) < 0)
	    return -1;

    // sort results and remove duplicates
    std::sort(results.begin(), results.end());
    int *unique_result = std::unique(results.begin(), results.end());
    results.resize(unique_result - results.begin());
    return 0;
}

int
FeatureList::lookups(const Vector<int> &required_fids, const Vector<int> &fids, const Vector<Tag> &sorted_ftags, Vector<int> &results, ErrorHandler *errh) const
{
    Vector<int> fidsx(fids);
    filter(fidsx, sorted_ftags);
    for (int i = 0; i < required_fids.size(); i++)
	fidsx.push_back(required_fids[i]);
    return lookups(fidsx, results, errh);
}

int
FeatureList::lookups(int required_fid, const Vector<int> &fids, const Vector<Tag> &sorted_ftags, Vector<int> &results, ErrorHandler *errh) const
{
    Vector<int> fidsx(fids);
    filter(fidsx, sorted_ftags);
    if (required_fid >= 0)
	fidsx.push_back(required_fid);
    return lookups(fidsx, results, errh);
}

int
FeatureList::lookups(const ScriptList &script_list, Tag script, Tag langsys, const Vector<Tag> &sorted_ftags, Vector<int> &results, ErrorHandler *errh) const
{
    int required_fid;
    Vector<int> fids;
    int result = script_list.features(script, langsys, required_fid, fids, errh);
    if (result >= 0) {
	filter(fids, sorted_ftags);
	if (required_fid >= 0)
	    fids.push_back(required_fid);
	result = lookups(fids, results, errh);
    }
    return result;
}


/**************************
 * Coverage               *
 *                        *
 **************************/

Coverage::Coverage() throw ()
{
}

Coverage::Coverage(Glyph first, Glyph last) throw ()
{
    if (first <= last) {
	_str = String("\000\002\000\001\000\000\000\000\000\000", 10);
	uint8_t *data = _str.mutable_udata();
	data[4] = first >> 8;
	data[5] = first & 255;
	data[6] = last >> 8;
	data[7] = last & 255;
	_str.align(2);
    }
}

Coverage::Coverage(const String &str, ErrorHandler *errh, bool do_check) throw ()
    : _str(str)
{
    _str.align(2);
    if (do_check) {
	if (check(errh ? errh : ErrorHandler::silent_handler()) < 0)
	    _str = String();
    } else {			// check()'s shorten-string side effect
	const uint8_t *data = _str.udata();
	int count = USHORT_AT(data + 2);
	if (data[1] == T_LIST)
	    _str = _str.substring(0, HEADERSIZE + count*LIST_RECSIZE);
	else
	    _str = _str.substring(0, HEADERSIZE + count*RANGES_RECSIZE);
    }
}

int
Coverage::check(ErrorHandler *errh)
    // side effect: shorten string to cover coverage
{
    // HEADER FORMAT:
    // USHORT	coverageFormat
    // USHORT	glyphCount
    const uint8_t *data = _str.udata();
    if (_str.length() < HEADERSIZE)
	return errh->error("OTF coverage table too small for header");
    int coverageFormat = USHORT_AT(data);
    int count = USHORT_AT(data + 2);

    int len;
    switch (coverageFormat) {
      case T_LIST:
	len = HEADERSIZE + count*LIST_RECSIZE;
	if (_str.length() < len)
	    return errh->error("OTF coverage table too short (format 1)");
	// XXX don't check sorting
	break;
      case T_RANGES:
	len = HEADERSIZE + count*RANGES_RECSIZE;
	if (_str.length() < len)
	    return errh->error("OTF coverage table too short (format 2)");
	// XXX don't check sorting
	// XXX don't check startCoverageIndexes
	break;
      default:
	return errh->error("OTF coverage table has unknown format %d", coverageFormat);
    }

    _str = _str.substring(0, len);
    return 0;
}

int
Coverage::size() const throw ()
{
    if (_str.length() == 0)
	return -1;
    const uint8_t *data = _str.udata();
    if (data[1] == T_LIST)
	return (_str.length() - HEADERSIZE) / LIST_RECSIZE;
    else if (data[1] == T_RANGES) {
	data += _str.length() - RANGES_RECSIZE;
	return USHORT_AT(data + 4) + USHORT_AT(data + 2) - USHORT_AT(data) + 1;
    } else
	return -1;
}

int
Coverage::coverage_index(Glyph g) const throw ()
{
    if (_str.length() == 0)
	return -1;
    
    const uint8_t *data = _str.udata();
    int count = USHORT_AT(data + 2);
    if (data[1] == T_LIST) {
	int l = 0, r = count - 1;
	data += HEADERSIZE;
	while (l <= r) {
	    int m = (l + r) >> 1;
	    int mval = USHORT_AT(data + m * LIST_RECSIZE);
	    if (g < mval)
		r = m - 1;
	    else if (g == mval)
		return m;
	    else
		l = m + 1;
	}
	return -1;
    } else if (data[1] == T_RANGES) {
	int l = 0, r = count - 1;
	data += HEADERSIZE;
	while (l <= r) {
	    int m = (l + r) >> 1;
	    const uint8_t *rec = data + m * RANGES_RECSIZE;
	    if (g < USHORT_AT(rec))
		r = m - 1;
	    else if (g <= USHORT_AT(rec + 2))
		return USHORT_AT(rec + 4) + g - USHORT_AT(rec);
	    else
		l = m + 1;
	}
	return -1;
    } else
	return -1;
}

Glyph
Coverage::operator[](int cindex) const throw ()
{
    if (_str.length() == 0 || cindex < 0)
	return 0;
    
    const uint8_t *data = _str.udata();
    int count = USHORT_AT(data + 2);
    if (data[1] == T_LIST)
	return (cindex < count ? USHORT_AT(data + cindex * LIST_RECSIZE) : 0);
    else if (data[1] == T_RANGES) {
	int l = 0, r = count - 1;
	data += HEADERSIZE;
	while (l <= r) {
	    int m = (l + r) >> 1;
	    const uint8_t *rec = data + m * RANGES_RECSIZE;
	    int start_cindex = USHORT_AT(rec + 4);
	    if (cindex < start_cindex)
		r = m - 1;
	    else if (cindex < start_cindex + USHORT_AT(rec + 2) - USHORT_AT(rec))
		return USHORT_AT(rec) + cindex - start_cindex;
	    else
		l = m + 1;
	}
	return 0;
    } else
	return 0;
}

void
Coverage::unparse(StringAccum &sa) const throw ()
{
    const uint8_t *data = _str.udata();
    if (_str.length() == 0)
	sa << "@*#!";
    else if (data[1] == T_LIST) {
	int count = USHORT_AT(data + 2);
	for (int i = 0; i < count; i++) {
	    if (i) sa << ',';
	    sa << USHORT_AT(data + HEADERSIZE + i*LIST_RECSIZE);
	}
    } else {
	for (int pos = HEADERSIZE; pos < _str.length(); pos += RANGES_RECSIZE) {
	    Glyph start = USHORT_AT(data + pos);
	    Glyph end = USHORT_AT(data + pos + 2);
	    if (pos > HEADERSIZE) sa << ',';
	    sa << start;
	    if (end != start) sa << '.' << '.' << end;
	}
    }
}

String
Coverage::unparse() const throw ()
{
    StringAccum sa;
    unparse(sa);
    return sa.take_string();
}

Coverage
operator&(const Coverage &a, const Coverage &b)
{
    StringAccum sa;
    sa << '\000' << '\001' << '\000' << '\000';
    Coverage::iterator ai = a.begin(), bi = b.begin();
    while (ai && bi) {
	if (*ai < *bi)
	    ai.forward_to(*bi);
	else if (*ai == *bi) {
	    uint16_t x = *ai;
	    sa << (char)(x >> 8) << (char)(x & 0xFF);
	    ai++, bi++;
	} else
	    bi.forward_to(*ai);
    }
    int n = (sa.length() - 4) / 2;
    sa[2] = (n >> 8);
    sa[3] = (n & 0xFF);
    return Coverage(sa.take_string(), 0, false);
}

bool
operator<=(const Coverage &a, const Coverage &b)
{
    Coverage::iterator ai = a.begin(), bi = b.begin();
    while (ai && bi) {
	if (*ai != *bi && !bi.forward_to(*ai))
	    return false;
	ai++, bi++;
    }
    return bi || !ai;
}


/************************
 * Coverage::iterator   *
 *                      *
 ************************/

Coverage::iterator::iterator(const String &str, int pos)
    : _str(str), _pos(pos), _value(0)
{
    // XXX assume _str has been checked

    // shrink _str to fit the coverage table
    const uint8_t *data = _str.udata();
    if (_str.length()) {
	int n = USHORT_AT(data + 2);
	switch (USHORT_AT(data)) {
	  case T_LIST:
	    _str = _str.substring(0, HEADERSIZE + n*LIST_RECSIZE);
	    break;
	  case T_RANGES:
	    _str = _str.substring(0, HEADERSIZE + n*RANGES_RECSIZE);
	    break;
	  default:
	    _str = String();
	    break;
	}
    }
    if (_pos >= _str.length())
	_pos = _str.length();
    else {
	// now, move _pos into the coverage table
	_pos = HEADERSIZE;
	_value = (_pos >= _str.length() ? 0 : USHORT_AT(data + _pos));
    }
}

int
Coverage::iterator::coverage_index() const
{
    const uint8_t *data = _str.udata();
    assert(_pos < _str.length());
    if (data[1] == T_LIST)
	return (_pos - HEADERSIZE) / LIST_RECSIZE;
    else
	return USHORT_AT(data + _pos + 4) + _value - USHORT_AT(data + _pos);
}

void
Coverage::iterator::operator++(int)
{
    const uint8_t *data = _str.udata();
    int len = _str.length();
    if (_pos >= len
	|| (data[1] == T_RANGES && ++_value <= USHORT_AT(data + _pos + 2)))
	return;
    _pos += (data[1] == T_LIST ? LIST_RECSIZE : RANGES_RECSIZE);
    _value = (_pos >= len ? 0 : USHORT_AT(data + _pos));
}

bool
Coverage::iterator::forward_to(Glyph find)
{
    // XXX really should check that this works
    
    if (find <= _value)
	return find == _value;
    else if (_pos >= _str.length())
	return false;

    const uint8_t *data = _str.udata();
    if (data[1] == T_LIST) {
	// check for "common" case: next element
	_pos += LIST_RECSIZE;
	if (_pos >= _str.length())
	    return false;
	else if (find <= USHORT_AT(data + _pos)) {
	    _value = USHORT_AT(data + _pos);
	    return find == _value;
	}
	
	// otherwise, binary search over remaining area
	int l = ((_pos - HEADERSIZE) / LIST_RECSIZE) + 1;
	int r = (_str.length() - HEADERSIZE) / LIST_RECSIZE - 1;
	data += HEADERSIZE;
	while (l <= r) {
	    int m = (l + r) >> 1;
	    Glyph g = USHORT_AT(data + m * LIST_RECSIZE);
	    if (find < g)
		r = m - 1;
	    else if (find == g)
		l = m, r = m - 1;
	    else
		l = m + 1;
	}
	_pos = HEADERSIZE + l * LIST_RECSIZE;
	_value = (_pos >= _str.length() ? 0 : USHORT_AT(data - HEADERSIZE + _pos));
    } else {		// data[1] == T_RANGES
	// check for "common" case: this or next element
	if (find <= USHORT_AT(data + _pos + 2)) {
	    assert(find >= USHORT_AT(data + _pos));
	    _value = find;
	    return true;
	}
	_pos += RANGES_RECSIZE;
	if (_pos >= _str.length())
	    return false;
	else if (find <= USHORT_AT(data + _pos + 2)) {
	    _value = (find >= USHORT_AT(data + _pos) ? find : USHORT_AT(data + _pos));
	    return find == _value;
	}

	// otherwise, binary search over remaining area
	int l = ((_pos - HEADERSIZE) / RANGES_RECSIZE) + 1;
	int r = (_str.length() - HEADERSIZE) / RANGES_RECSIZE - 1;
	data += HEADERSIZE;
	while (l <= r) {
	    int m = (l + r) >> 1;
	    if (find < USHORT_AT(data + m * RANGES_RECSIZE))
		r = m - 1;
	    else if (find <= USHORT_AT(data + m * RANGES_RECSIZE + 2)) {
		_pos = HEADERSIZE + m * RANGES_RECSIZE;
		_value = find;
		return true;
	    } else
		l = m + 1;
	}
	_pos = HEADERSIZE + l * LIST_RECSIZE;
	_value = (_pos >= _str.length() ? 0 : USHORT_AT(data - HEADERSIZE + _pos));
    }

    return find == _value;
}


/**************************
 * GlyphSet               *
 *                        *
 **************************/

GlyphSet::GlyphSet()
{
    memset(_v, 0, sizeof(_v));
}

GlyphSet::GlyphSet(const GlyphSet &o)
{
    for (int i = 0; i < VLEN; i++)
	if (o._v[i]) {
	    _v[i] = new uint32_t[VULEN];
	    memcpy(_v[i], o._v[i], sizeof(uint32_t) * VULEN);
	} else
	    _v[i] = 0;
}

GlyphSet::~GlyphSet()
{
    for (int i = 0; i < VLEN; i++)
	delete[] _v[i];
}

int
GlyphSet::change(Glyph g, bool value)
{
    if ((unsigned)g > MAXGLYPH)
	return -1;
    uint32_t *&u = _v[g >> SHIFT];
    if (!u) {
	u = new uint32_t[VULEN];
	memset(u, 0, sizeof(uint32_t) * VULEN);
    }
    uint32_t mask = (1 << (g & 0x1F));
    if (value)
	u[(g & MASK) >> 5] |= mask;
    else
	u[(g & MASK) >> 5] &= ~mask;
    return 0;
}

GlyphSet &
GlyphSet::operator=(const GlyphSet &o)
{
    if (&o != this) {
	for (int i = 0; i < VLEN; i++)
	    if (o._v[i]) {
		if (!_v[i])
		    _v[i] = new uint32_t[VULEN];
		memcpy(_v[i], o._v[i], sizeof(uint32_t) * VULEN);
	    } else if (_v[i])
		memset(_v[i], 0, sizeof(uint32_t) * VULEN);
    }
    return *this;
}


/**************************
 * ClassDef               *
 *                        *
 **************************/

ClassDef::ClassDef(const String &str, ErrorHandler *errh) throw ()
    : _str(str)
{
    _str.align(2);
    if (check(errh ? errh : ErrorHandler::silent_handler()) < 0)
	_str = String();
}

int
ClassDef::check(ErrorHandler *errh)
{
    // HEADER FORMAT:
    // USHORT	coverageFormat
    // USHORT	glyphCount
    const uint8_t *data = _str.udata();
    if (_str.length() < 6)	// NB: prevents empty format-2 tables
	return errh->error("OTF class def table too small for header");
    int classFormat = USHORT_AT(data);

    int len;
    if (classFormat == T_LIST) {
	int count = USHORT_AT(data + 4);
	len = LIST_HEADERSIZE + count*LIST_RECSIZE;
	// XXX don't check sorting
    } else if (classFormat == T_RANGES) {
	int count = USHORT_AT(data + 2);
	len = RANGES_HEADERSIZE + count*RANGES_RECSIZE;
	// XXX don't check sorting
    } else
	return errh->error("OTF class def table has unknown format %d", classFormat);

    if (len > _str.length())
	return errh->error("OTF class def table too short");
    else {
	_str = _str.substring(0, len);
	return 0;
    }
}

int
ClassDef::lookup(Glyph g) const throw ()
{
    if (_str.length() == 0)
	return -1;
    
    const uint8_t *data = _str.udata();
    int coverageFormat = USHORT_AT(data);
    
    if (coverageFormat == T_LIST) {
	Glyph start = USHORT_AT(data + 2);
	int count = USHORT_AT(data + 4);
	if (g < start || g >= start + count)
	    return 0;
	else
	    return USHORT_AT(data + LIST_HEADERSIZE + (g - start) * LIST_RECSIZE);
    } else if (coverageFormat == T_RANGES) {
	int l = 0, r = USHORT_AT(data + 2) - 1;
	data += RANGES_HEADERSIZE;
	while (l <= r) {
	    int m = (l + r) >> 1;
	    const uint8_t *rec = data + m * RANGES_RECSIZE;
	    if (g < USHORT_AT(rec))
		r = m - 1;
	    else if (g <= USHORT_AT(rec + 2))
		return USHORT_AT(rec + 4);
	    else
		l = m + 1;
	}
	return 0;
    } else
	return 0;
}

void
ClassDef::unparse(StringAccum &sa) const throw ()
{
    const uint8_t *data = _str.udata();
    if (_str.length() == 0)
	sa << "@*#!";
    else if (data[1] == T_LIST) {
	Glyph start = USHORT_AT(data + 2);
	int count = USHORT_AT(data + 4);
	for (int i = 0; i < count; i++) {
	    if (i) sa << ',';
	    sa << start + i << '=' << USHORT_AT(data + LIST_HEADERSIZE + i*LIST_RECSIZE);
	}
    } else {
	for (int pos = RANGES_HEADERSIZE; pos < _str.length(); pos += RANGES_RECSIZE) {
	    Glyph start = USHORT_AT(data + pos);
	    Glyph end = USHORT_AT(data + pos + 2);
	    if (pos > RANGES_HEADERSIZE) sa << ',';
	    sa << start;
	    if (end != start) sa << '.' << '.' << end;
	    sa << '=' << USHORT_AT(data + pos + 4);
	}
    }
}

String
ClassDef::unparse() const throw ()
{
    StringAccum sa;
    unparse(sa);
    return sa.take_string();
}


/******************************
 * ClassDef::class_iterator   *
 *                            *
 ******************************/

ClassDef::class_iterator::class_iterator(const String &str, int pos, int the_class, const Coverage::iterator &coviter)
    : _str(str), _pos(pos), _class(the_class), _coviter(coviter)
{
    // XXX assume _str has been checked

    // cannot handle the_class == 0 when coviter doesn't exist
    if (_class == 0 && !_coviter)
	throw Error("cannot iterate over ClassDef class 0");

    // shrink _str to fit the coverage table, and create a fake coverage
    // iterator if necessary
    const uint8_t *data = _str.udata();
    if (_str.length()) {
	switch (USHORT_AT(data)) {
	  case T_LIST: {
	      Glyph start = USHORT_AT(data + 2);
	      int nglyphs = USHORT_AT(data + 4);
	      _str = _str.substring(0, LIST_HEADERSIZE + nglyphs*LIST_RECSIZE);
	      if (!_coviter)
		  _coviter = Coverage(start, start + nglyphs - 1).begin();
	      if (_class)
		  _coviter.forward_to(start);
	      break;
	  }
	  case T_RANGES: {
	      Glyph start = USHORT_AT(data + RANGES_HEADERSIZE);
	      int nranges = USHORT_AT(data + 2);
	      _str = _str.substring(0, RANGES_HEADERSIZE + nranges*RANGES_RECSIZE);
	      if (!_coviter) {
		  Glyph end = USHORT_AT(data + RANGES_HEADERSIZE + 2 + (nranges - 1)*RANGES_RECSIZE);
		  _coviter = Coverage(start, end).begin();
	      }
	      if (_class)
		  _coviter.forward_to(start);
	      break;
	  }
	  default:
	    _str = String();
	    break;
	}
    }

    // move to the first relevant glyph
    if (_pos >= _str.length())
	_pos = _str.length();
    else {
	_pos = 0;
	(*this)++;
    }
}

void
ClassDef::class_iterator::increment_class0()
{
    const uint8_t *data = _str.udata();
    int len = _str.length();
    bool is_list = (data[1] == T_LIST);
    
    if (_pos != 0)
	_coviter++;
    else
	_pos = FIRST_POS;

    if (_pos == FIRST_POS && _coviter) {
	if (*_coviter < USHORT_AT(data + (is_list ? 2 : RANGES_HEADERSIZE)))
	    return;
	_pos = (is_list ? LIST_HEADERSIZE : RANGES_HEADERSIZE);
    }

    while (_pos > 0 && _pos < len && _coviter) {
	Glyph g = *_coviter;
	if (is_list) {
	    _pos = LIST_HEADERSIZE + LIST_RECSIZE*(g - USHORT_AT(data + 2));
	    if (_pos >= len)
		break;
	    else if (USHORT_AT(data + _pos) == 0) // _class == 0
		return;
	    _coviter++;
	} else {
	    if (g < USHORT_AT(data + _pos)) // in a zero range
		return;
	    else if (g > USHORT_AT(data + _pos + 2))
		_pos += RANGES_RECSIZE;
	    else if (USHORT_AT(data + _pos + 4) == 0) // _class == 0
		return;
	    else
		_coviter.forward_to(USHORT_AT(data + _pos + 2) + 1);
	}
    }

    if (_coviter)
	_pos = LAST_POS;
    else
	_pos = len;
}

void
ClassDef::class_iterator::operator++(int)
{
    if (_class == 0) {
	increment_class0();
	return;
    }
    
    const uint8_t *data = _str.udata();
    int len = _str.length();
    bool is_list = (data[1] == T_LIST);

    if (_pos != 0)
	_coviter++;
    else
	_pos = (is_list ? LIST_HEADERSIZE : RANGES_HEADERSIZE);

    while (_pos < len && _coviter) {
	Glyph g = *_coviter;
	if (is_list) {
	    _pos = LIST_HEADERSIZE + LIST_RECSIZE*(g - USHORT_AT(data + 2));
	    if (_pos >= len || USHORT_AT(data + _pos) == _class)
		return;
	    _coviter++;
	} else {
	    while (_pos < len && (g > USHORT_AT(data + _pos + 2)
				  || USHORT_AT(data + _pos + 4) != _class))
		_pos += RANGES_RECSIZE;
	    // now, _pos >= len, or g <= rec.end && class == rec.class
	    if (_pos >= len || g >= USHORT_AT(data + _pos))
		return;
	    _coviter.forward_to(USHORT_AT(data + _pos));
	}
    }

    _pos = len;
}


}}


// template instantiations
#include <lcdf/vector.cc>
