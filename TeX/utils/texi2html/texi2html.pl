#! @PERL@
'di ';
'ig 00 ';
#+##############################################################################
#
# texi2html: Program to transform Texinfo documents to HTML
#
#    Copyright (C) 1999, 2000  Free Software Foundation, Inc.
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#-##############################################################################

# This requires perl version 5 or higher
require 5.0;

# Perl pragma to restrict unsafe constructs
use strict;
# for POSIX::setlocale
require 5.004;
# used in case of tests, to revert to "C" locale.
use POSIX qw(setlocale LC_ALL LC_CTYPE);
# used to find a relative path back to the current working directory
use File::Spec;
#
# According to
# larry.jones@sdrc.com (Larry Jones)
# this pragma is not present in perl5.004_02:
#
# Perl pragma to control optional warnings
# use warnings;

# Declarations. Empty lines separate the different classes of variables:


#++##############################################################################
#
# NOTE FOR DEBUGGING THIS SCRIPT:
# You can run 'perl texi2html.pl' directly, provided you have
# the environment variable T2H_HOME set to the directory containing
# the texi2html.init file
#
#--##############################################################################

# CVS version:
# $Id: texi2html.pl,v 1.111 2004/03/22 23:22:50 pertusus Exp $

# Homepage:
my $T2H_HOMEPAGE = "http://texi2html.cvshome.org/";

# Authors (appears in comments):
my $T2H_AUTHORS = <<EOT;
Written by: Lionel Cons <Lionel.Cons\@cern.ch> (original author)
            Karl Berry  <karl\@freefriends.org>
            Olaf Bachmann <obachman\@mathematik.uni-kl.de>
            and many others.
Maintained by: Many creative people <dev\@texi2html.cvshome.org>
Send bugs and suggestions to <users\@texi2html.cvshome.org>
EOT

# Version: set in configure.in
my $THISVERSION = '@PACKAGE_VERSION@';
my $THISPROG = "texi2html $THISVERSION"; # program name and version

# set by configure, prefix for the sysconfdir and so on
my $prefix = '@prefix@';
my $sysconfdir;
my $pkgdatadir;
# We need to eval as $prefix has to be expanded. However when we haven't
# run configure @sysconfdir will be expanded as an array, thus we verify
# whether configure was run or not
if ('@sysconfdir@' ne '@' . 'sysconfdir@')
{
    $sysconfdir = eval '"@sysconfdir@"';
}
else
{
    $sysconfdir = "/usr/local/etc";
}
if ('@datadir@' ne '@' . 'datadir@')
{
    $pkgdatadir = eval '"@datadir@/@PACKAGE@"';
}
else
{
    $pkgdatadir = "/usr/local/share/texi2html";
}
# The man page for this program is included at the end of this file and can be
# viewed using the command 'nroff -man texi2html'.

#+++############################################################################
#                                                                              #
# Constants                                                                    #
#                                                                              #
#---############################################################################

my $DEBUG_MENU   =  1;
my $DEBUG_INDEX =  2;
my $DEBUG_TEXI  =  4;
my $DEBUG_MACROS =  8;
my $DEBUG_FORMATS   = 16;
my $DEBUG_ELEMENTS  = 32;
my $DEBUG_USER  = 64;
my $DEBUG_L2H   = 128;

my $ERROR = "***";                 # prefix for errors
my $WARN  = "**";                  # prefix for warnings

# FIXME [^\s] doesn't seems to work in texi2html, although it works when
# called as texi2html.pl ?? With perl (revision 5.0 version 8 subversion 0)
# previously was [^\s\{\}]+
my $VARRE = '[\w\-]+';          # RE for a variable name
my $NODERE = '[^:]+';             # RE for node names

my $MAX_LEVEL = 4;
my $MIN_LEVEL = 1;

my $i18n_dir = 'i18n'; # name of the directory containing the per language files
my $conf_file_name = 'Config' ;

#+++############################################################################
#                                                                              #
# Initialization                                                               #
# Pasted content of File $(srcdir)/texi2html.init: Default initializations     #
#                                                                              #
#---############################################################################

# leave this within comments, and keep the require statement
# This way, you can directly run texi2html.pl, if $ENV{T2H_HOME}/texi2html.init
# exists.

{
package Texi2HTML::Config;

our $I = \&Texi2HTML::I18n::get_string;

sub load($) 
{
    my $file = shift;
    eval { require($file) ;};
    if ($@ ne '')
    {
        print STDERR "error loading $file: $@\n";
        return 0;
    }
    return 1;
}

# customization options variables
our $DEBUG;
our $PREFIX;
our $VERBOSE;
our $SUBDIR;
our $IDX_SUMMARY;
our $SPLIT;
our $SHORT_REF;
our @EXPAND;
our $EXPAND;
#our $TOC;
our $TOP;
our $DOCTYPE ;
our $FRAMESET_DOCTYPE ;
our $CHECK ;
our $TEST ;
our $DUMP_TEXI;
our $MACRO_EXPAND;
our $USE_GLOSSARY ;
our $INVISIBLE_MARK ;
our $USE_ISO ;
our $TOP_FILE ;
our $TOC_FILE;
our $FRAMES;
our $SHOW_MENU;
our $NUMBER_SECTIONS;
our $USE_NODES;
our $USE_UNICODE;
our $NODE_FILES;
our $NODE_NAME_IN_MENU;
our $AVOID_MENU_REDUNDANCY;
our $SECTION_NAVIGATION;
our $SHORTEXTN ;
our $EXTENSION;
our $OUT ;
our $NOVALIDATE;
our $DEF_TABLE ;
our $LANG ;
our $DO_CONTENTS;
our $DO_SCONTENTS;
our $SEPARATED_FOOTNOTES;
our $TOC_LINKS;
our $L2H ;
our $L2H_L2H ;
our $L2H_SKIP ;
our $L2H_TMP ;
our $L2H_CLEAN ;
our $L2H_FILE;
our $L2H_HTML_VERSION;
our $EXTERNAL_DIR;
our @INCLUDE_DIRS ;
our @PREPEND_DIRS ;
our $IGNORE_PREAMBLE_TEXT;
our @CSS_FILES;

# customization variables
our $ENCODING;
our $DOCUMENT_ENCODING;
our $MENU_PRE_STYLE;
our $CENTER_IMAGE;
our $EXAMPLE_INDENT_CELL;
our $SMALL_EXAMPLE_INDENT_CELL;
our $SMALL_FONT_SIZE;
our $SMALL_RULE;
our $DEFAULT_RULE;
our $MIDDLE_RULE;
our $BIG_RULE;
our $TOP_HEADING;
our $INDEX_CHAPTER;
our $SPLIT_INDEX;
our $HREF_DIR_INSTEAD_FILE;
our $AFTER_BODY_OPEN;
our $PRE_BODY_CLOSE;
our $EXTRA_HEAD;
our $VERTICAL_HEAD_NAVIGATION;
our $WORDS_IN_PAGE;
our $ICONS;
our $UNNUMBERED_SYMBOL_IN_MENU;
our $MENU_SYMBOL;
our $OPEN_QUOTE_SYMBOL;
our $CLOSE_QUOTE_SYMBOL;
our $TOC_LIST_STYLE;
our $TOC_LIST_ATTRIBUTE;
our $TOP_NODE_FILE;
our $NODE_FILE_EXTENSION;
our $BEFORE_OVERVIEW;
our $AFTER_OVERVIEW;
our $BEFORE_TOC_LINES;
our $AFTER_TOC_LINES;
our $NEW_CROSSREF_STYLE;;
our %ACTIVE_ICONS;
our %NAVIGATION_TEXT;
our %PASSIVE_ICONS;
our %BUTTONS_GOTO;
our %BUTTONS_EXAMPLE;
our @CHAPTER_BUTTONS;
our @MISC_BUTTONS;
our @SECTION_BUTTONS;
our @SECTION_FOOTER_BUTTONS;
our @NODE_FOOTER_BUTTONS;

# customization variables which may be guessed in the script
#our $ADDRESS;
our $BODYTEXT;
our $CSS_LINES;
our $DOCUMENT_DESCRIPTION;

# I18n
our $LANGUAGES;

# customizable subroutines references
our $print_section;
our $one_section;
our $end_section;
our $print_Top_header;
our $print_Top_footer;
our $print_Top;
our $print_Toc;
our $print_Overview;
our $print_Footnotes;
our $print_About;
our $print_misc_header;
our $print_misc_footer;
our $print_misc;
our $print_section_header;
our $print_section_footer;
our $print_chapter_header;
our $print_chapter_footer;
our $print_page_head;
our $print_page_foot;
our $print_head_navigation;
our $print_foot_navigation;
our $button_icon_img;
our $print_navigation;
our $about_body;
our $print_frame;
our $print_toc_frame;
our $toc_body;
our $titlepage;
our $css_lines;
our $print_redirection_page;
our $init_out;
our $finish_out;
our $node_file_name;
our $element_file_name;

our $protect_text;
our $anchor;
our $def_item;
our $def;
our $menu;
our $menu_link;
our $menu_description;
our $menu_comment;
our $simple_menu_link;
our $ref_beginning;
our $info_ref;
our $book_ref;
our $external_ref;
our $internal_ref;
our $table_item;
our $table_line;
our $row;
our $cell;
our $list_item;
our $comment;
our $def_line;
our $def_line_no_texi;
our $raw;
our $heading;
our $paragraph;
our $preformatted;
our $foot_line_and_ref;
our $foot_section;
our $address;
our $image;
our $index_entry_label;
our $index_entry;
our $index_letter;
our $print_index;
our $index_summary;
our $summary_letter;
our $complex_format;
our $cartouche;
our $sp;
our $definition_category;
our $table_list;
our $index_summary_file_entry;
our $index_summary_file_end;
our $index_summary_file_begin;
our $style;
our $format;
our $normal_text;
our $empty_line;
our $unknown;
our $unknown_style;

our $PRE_ABOUT;
our $AFTER_ABOUT;

# hash which entries might be redefined by the user
our $complex_format_map;
our %accent_map;
our %def_map;
our %format_map;
our %simple_map;
our %simple_map_pre;
our %simple_map_texi;
our %style_map;
our %style_map_pre;
our %style_map_texi;
our %paragraph_style;
our %things_map;
our %pre_map;
our %texi_map;
our %unicode_map;
our %unicode_diacritical;
our %ascii_character_map;
our %ascii_simple_map;
our %ascii_things_map;
our %perl_charset_to_html;
our %iso_symbols;
our %to_skip;
our %css_map;
our %special_list_commands;
our %accent_letters;
our %unicode_accents;
our %special_accents;

$toc_body                 = \&T2H_GPL_toc_body;
$style                    = \&T2H_GPL_style;
$format                   = \&T2H_GPL_format;
$normal_text              = \&t2h_gpl_normal_text;

sub T2H_GPL_toc_body($)
{
    my $elements_list = shift;
#    my $do_contents = shift;
#    my $do_scontents = shift;
    #return unless ($do_contents or $do_scontents or $FRAMES);
    return unless ($DO_CONTENTS or $DO_SCONTENTS or $FRAMES);
    my $current_level = 0;
    my $ul_style = $NUMBER_SECTIONS ? $TOC_LIST_ATTRIBUTE : ''; 
    foreach my $element (@$elements_list)
    {
        next if ($element->{'top'} or $element->{'index_page'});
        my $ind = '  ' x $current_level;
        my $level = $element->{'toc_level'};
        print STDERR "Bug no toc_level for ($element) $element->{'texi'}\n" if (!defined ($level));
        if ($level > $current_level)
        {
            while ($level > $current_level)
            {
                $current_level++;
                my $ln = "\n$ind<ul${ul_style}>\n";
                $ind = '  ' x $current_level;
                push(@{$Texi2HTML::TOC_LINES}, $ln);
            }
        }
        elsif ($level < $current_level)
        {
            while ($level < $current_level)
            {
                $current_level--;
                $ind = '  ' x $current_level;
                my $line = "</li>\n$ind</ul>";
                $line .=  "</li>" if ($level == $current_level);
                push(@{$Texi2HTML::TOC_LINES}, "$line\n");
                
            }
        }
        else
        {
            push(@{$Texi2HTML::TOC_LINES}, "</li>\n");
        }
        my $file = '';
        $file = $element->{'file'} if ($SPLIT);
        my $text = $element->{'text'};
        $text = $element->{'name'} unless ($NUMBER_SECTIONS);
        my $entry = "<li>" . &$anchor ($element->{'tocid'}, "$file#$element->{'id'}",$text);
        push (@{$Texi2HTML::TOC_LINES}, $ind . $entry);
        push(@{$Texi2HTML::OVERVIEW}, $entry. "</li>\n") if ($level == 1);
    }
    while (0 < $current_level)
    {
        $current_level--;
        my $ind = '  ' x $current_level;
        push(@{$Texi2HTML::TOC_LINES}, "</li>\n$ind</ul>\n");
    }
    #@{$Texi2HTML::TOC_LINES} = () unless ($do_contents);
    @{$Texi2HTML::TOC_LINES} = () unless ($DO_CONTENTS);
    if (@{$Texi2HTML::TOC_LINES})
    {
        unshift @{$Texi2HTML::TOC_LINES}, $BEFORE_TOC_LINES;
        push @{$Texi2HTML::TOC_LINES}, $AFTER_TOC_LINES;
    }
    #@{$Texi2HTML::OVERVIEW} = () unless ($do_scontents or $FRAMES);
    @{$Texi2HTML::OVERVIEW} = () unless ($DO_SCONTENTS or $FRAMES);
    if (@{$Texi2HTML::OVERVIEW})
    {
        unshift @{$Texi2HTML::OVERVIEW}, "<ul${ul_style}>\n";
        push @{$Texi2HTML::OVERVIEW}, "</ul>\n";
        unshift @{$Texi2HTML::OVERVIEW}, $BEFORE_OVERVIEW;
        push @{$Texi2HTML::OVERVIEW}, $AFTER_OVERVIEW;
    }
}

sub T2H_GPL_style($$$$$$$$$)
{                           # known style
    my $style = shift;
    my $command = shift;
    my $text = shift;
    my $args = shift;
    my $no_close = shift;
    my $no_open = shift;
    my $line_nr = shift;
    my $state = shift;
    my $style_stack = shift;

    my $do_quotes = 0;
    my $use_attribute = 0;
    my $use_begin_end = 0;
    if (ref($style) eq 'HASH')
    {
        #print STDERR "GPL_STYLE $command";
        #print STDERR " @$args\n";
        $do_quotes = $style->{'quote'};
        if ((@{$style->{'args'}} == 1) and defined($style->{'attribute'}))
        {
            $style = $style->{'attribute'};
            $use_attribute = 1;
            $text = $args->[0];
        }
        elsif (defined($style->{'function'}))
        {
            $text = &{$style->{'function'}}($command, $args, $style_stack, $state, $line_nr);
        }
    }
    else
    {
        if ($style =~ s/^\"//)
        {                       # add quotes
            $do_quotes = 1;
        }
        if ($style =~ s/^\&//)
        {                       # custom
            $style = 'Texi2HTML::Config::' . $style;
            eval "\$text = &$style(\$text, \$command, \$style_stack)";
        }
        elsif ($style ne '')
        {
            $use_attribute = 1;
        }
        else
        {                       # no style
        }
    }
    if ($use_attribute)
    {                       # good style
        my $attribute_text = '';
        if ($style =~ /^(\w+)(\s+.*)/)
        {
            $style = $1;
            $attribute_text = $2;
        }
        $text = "<${style}$attribute_text>$text</$style>" ;
    }
    if (ref($style) eq 'HASH')
    {
        if (defined($style->{'begin'}) and !$no_open)
        {
             $text = $style->{'begin'} . $text;
        }
        if (defined($style->{'end'}) and !$no_close)
        {
            $text = $text . $style->{'end'};
        }
    }
    if ($do_quotes)
    {
        $text = $OPEN_QUOTE_SYMBOL . "$text" if (!$no_open);
        $text .= $CLOSE_QUOTE_SYMBOL if (!$no_close);
    }
    return $text;
}

sub T2H_GPL_format($$$)
{
    my $tag = shift;
    my $element = shift;
    my $text = shift;
    return '' if (!defined($element) or ($text !~ /\S/));
    my $attribute_text = '';
    if ($element =~ /^(\w+)(\s+.*)/)
    {
        $element = $1;
        $attribute_text = $2;
    }
    return "<${element}$attribute_text>\n" . $text. "</$element>\n";
}

sub t2h_gpl_normal_text($)
{
    my $text = shift;
    $text =~ s/``/"/go;
    $text =~ s/''/"/go;
    $text =~ s/-(--?)/$1/go;
    return $text;
}
# @INIT@

require "$ENV{T2H_HOME}/texi2html.init" 
    if ($0 =~ /\.pl$/ &&
        -e "$ENV{T2H_HOME}/texi2html.init" && -r "$ENV{T2H_HOME}/texi2html.init");

my $translation_file = 'translations.pl'; # file containing all the translations
my $T2H_OBSOLETE_STRINGS;
require "$ENV{T2H_HOME}/$translation_file"
    if ($0 =~ /\.pl$/ &&
        -e "$ENV{T2H_HOME}/$translation_file" && -r "$ENV{T2H_HOME}/$translation_file");

# @T2H_TRANSLATIONS_FILE@
my $index_name = -1;
my @index_to_hash = ('style_map', 'style_map_pre', 'style_map_texi');
foreach my $hash (\%style_map, \%style_map_pre, \%style_map_texi)
{
    $index_name++;
    my $name = $index_to_hash[$index_name];
    foreach my $style (keys(%{$hash}))
    {
         next unless (ref($hash->{$style}) eq 'HASH');
         $hash->{$style}->{'args'} = ['normal'] if (!exists($hash->{$style}->{'args'}));
         die "Bug: args not defined for $style in $name" if (!defined($hash->{$style}->{'args'}));
#print STDERR "DEFAULT($name, $hash) add normal as arg for $style ($hash->{$style}), $hash->{$style}->{'args'}\n";
    }
}

my %special_style = (
           #'xref'      => ['keep','normal','normal','keep','normal'],
           'xref'         => { 'args' => ['keep','keep','keep','keep','keep'],
               'function' => \&main::do_xref },
           'ref'         => { 'args' => ['keep','keep','keep','keep','keep'],
               'function' => \&main::do_xref },
           'pxref'         => { 'args' => ['keep','keep','keep','keep','keep'],
               'function' => \&main::do_xref },
           'inforef'      => { 'args' => ['keep','keep','keep'], 
               'function' => \&main::do_xref },
           'image'        => { 'args' => ['keep'], 'function' => \&main::do_image },
           'anchor'       => { 'args' => ['keep'], 'function' => \&main::do_anchor_label },
           'footnote'     => { 'args' => ['keep'], 'function' => \&main::do_footnote },
);

# @image is replaced by the first arg in strings
$style_map_texi{'image'} = { 'args' => ['keep'],
       'function' => \&t2h_default_no_texi_image }
    unless (defined($style_map_texi{'image'}));

foreach my $special (keys(%special_style))
{
    $style_map{$special} = $special_style{$special}
          unless (defined($style_map{$special}));
    $style_map_pre{$special} = $special_style{$special}
          unless (defined($style_map_pre{$special}));
    $style_map_texi{$special} = { 'args' => ['keep'],
        'function' => \&t2h_remove_command }
          unless (defined($style_map_texi{$special}));
}


sub t2h_utf8_accent($$)
{
    my $accent = shift;
    my $args = shift;
                                                                                
    my $text = $args->[0];

    if ($accent eq 'dotless')
    {
        return "\x{0131}" if ($text eq 'i');
        #return "\x{}" if ($text eq 'j'); # not found !
        return $text;
    }
        
    return Unicode::Normalize::NFC($text . chr(hex($unicode_diacritical{$accent}))) 
        if (defined($unicode_diacritical{$accent}));
    return ascii_accents($text, $accent);
}

sub t2h_utf8_normal_text($)
{
    my $text = shift;
    $text =~ s/---/\x{2014}/g;
    $text =~ s/--/\x{2013}/g;
    $text =~ s/``/\x{201C}/g;
    $text =~ s/''/\x{201D}/g;
    return $text;
}

sub t2h_cross_manual_normal_text($)
{
   my $text = shift;
   $text = main::normalise_space($text);
   my $result = '';
   while ($text ne '')
   {
        if ($text =~ s/^([A-Za-z0-9]+)//o)
        {
             $result .= $1;
        }
        elsif ($text =~ s/^ //o)
        {
             $result .= '-';
        }
        elsif ($text =~ s/^(.)//o)
        {
             if (exists($ascii_character_map{$1}))
             {
                  $result .= '_' . lc($ascii_character_map{$1});
             }
             elsif ($USE_UNICODE)
             {
                  $result .= $1;
             }
             else
             {
                  $result .= '_' . '00' . lc(sprintf("%02x",ord($1)));
             }
        }
        else
        {
             print STDERR "Bug: unknown character in node (likely in infinite loop)\n";
             sleep 1;
        }    
   }
   
   return $result;
}

sub t2h_nounicode_cross_manual_accent($$)
{
    my $accent = shift;
    my $args = shift;
                                                                                
    my $text = $args->[0];

    return '_' . lc($unicode_accents{$accent}->{$text})
        if (defined($unicode_accents{$accent}->{$text}));
    return ($text . '_' . lc($unicode_diacritical{$accent})) 
        if (defined($unicode_diacritical{$accent}));
    return ascii_accents($text, $accent);
}


$USE_UNICODE = '@USE_UNICODE@';
if ($USE_UNICODE eq '@USE_UNICODE@')
{
    $USE_UNICODE = 1;
    eval {
        require Encode;
        require Unicode::Normalize; 
        Encode->import('encode');
    };
    $USE_UNICODE = 0 if ($@);
}

}

our %value;
our %user_sub;

# variables which might be redefined by the user but aren't likely to be  
# they seem to be in the main namespace
our $index_properties;
our %predefined_index;
our %valid_index;
our %sec2level;
our %code_style_map;
our %region_lines;

# Some global variables are set in the script, and used in the subroutines
# they are in the Texi2HTML namespace, thus prefixed with Texi2HTML::.
# see texi2html.init for details.

#+++############################################################################
#                                                                              #
# Initialization                                                               #
# Pasted content of File $(srcdir)/MySimple.pm: Command-line processing        #
#                                                                              #
#---############################################################################

# leave this within comments, and keep the require statement
# This way, you can directly run texi2html.pl, if $ENV{T2H_HOME}/texi2html.init
# exists.

# @MYSIMPLE@

require "$ENV{T2H_HOME}/MySimple.pm"
    if ($0 =~ /\.pl$/ &&
        -e "$ENV{T2H_HOME}/MySimple.pm" && -r "$ENV{T2H_HOME}/MySimple.pm");

#+++############################################################################
#                                                                              #
# Initialization                                                               #
# Pasted content of File $(srcdir)/T2h_i18n.pm: Internationalisation           #
#                                                                              #
#---############################################################################

# leave this within comments, and keep the require statement
# This way, you can directly run texi2html.pl, if $ENV{T2H_HOME}/T2h_i18n.pm
# exists.

# @T2H_I18N@
require "$ENV{T2H_HOME}/T2h_i18n.pm"
    if ($0 =~ /\.pl$/ &&
        -e "$ENV{T2H_HOME}/T2h_i18n.pm" && -r "$ENV{T2H_HOME}/T2h_i18n.pm");


{
package Texi2HTML::LaTeX2HTML::Config;

# latex2html variables
# These variables are not used. They are here for information only, and
# an example of config file for latex2html file is included.
my $ADDRESS;
my $ANTI_ALIAS;
my $ANTI_ALIAS_TEXT;
my $ASCII_MODE;
my $AUTO_LINK;
my $AUTO_PREFIX;
my $CHILDLINE;
my $DEBUG;
my $DESTDIR;
my $ERROR;
my $EXTERNAL_FILE;
my $EXTERNAL_IMAGES;
my $EXTERNAL_UP_LINK;
my $EXTERNAL_UP_TITLE;
my $FIGURE_SCALE_FACTOR;
my $HTML_VERSION;
my $IMAGES_ONLY;
my $INFO;
my $LINE_WIDTH;
my $LOCAL_ICONS;
my $LONG_TITLES;
my $MATH_SCALE_FACTOR;
my $MAX_LINK_DEPTH;
my $MAX_SPLIT_DEPTH;
my $NETSCAPE_HTML;
my $NOLATEX;
my $NO_FOOTNODE;
my $NO_IMAGES;
my $NO_NAVIGATION;
my $NO_SIMPLE_MATH;
my $NO_SUBDIR;
my $PAPERSIZE;
my $PREFIX;
my $PS_IMAGES;
my $REUSE;
my $SCALABLE_FONTS;
my $SHORTEXTN;
my $SHORT_INDEX;
my $SHOW_SECTION_NUMBERS;
my $SPLIT;
my $TEXDEFS;
my $TITLE;
my $TITLES_LANGUAGE;
my $TMP;
my $VERBOSE;
my $WORDS_IN_NAVIGATION_PANEL_TITLES;
my $WORDS_IN_PAGE;

# @T2H_L2H_INIT@
}

package main;

#
# pre-defined indices
#
$index_properties =
{
 'c' => { name => 'cp'},
 'f' => { name => 'fn', code => 1},
 'v' => { name => 'vr', code => 1},
 'k' => { name => 'ky', code => 1},
 'p' => { name => 'pg', code => 1},
 't' => { name => 'tp', code => 1}
};


%predefined_index = (
                     'cp', 'c',
                     'fn', 'f',
                     'vr', 'v',
                     'ky', 'k',
                     'pg', 'p',
                     'tp', 't',
	            );

#
# valid indices
#
%valid_index = (
                'c', 1,
                'f', 1,
                'v', 1,
                'k', 1,
                'p', 1,
                't', 1,
               );

#
# commands with ---, -- '' and `` preserved
# usefull with the old interface

%code_style_map = (
           'code'    => 1,
           'command' => 1,
           'env'     => 1,
           'file'    => 1,
           'kbd'     => 1,
           'option'  => 1,
           'samp'    => 1,
           'verb'    => 1,
);

our $simple_map_ref = \%Texi2HTML::Config::simple_map;
our $simple_map_pre_ref = \%Texi2HTML::Config::simple_map_pre;
our $simple_map_texi_ref = \%Texi2HTML::Config::simple_map_texi;
our $style_map_ref = \%Texi2HTML::Config::style_map;
our $style_map_pre_ref = \%Texi2HTML::Config::style_map_pre;
our $style_map_texi_ref = \%Texi2HTML::Config::style_map_texi;
our $things_map_ref = \%Texi2HTML::Config::things_map;
our $pre_map_ref = \%Texi2HTML::Config::pre_map;
our $texi_map_ref = \%Texi2HTML::Config::texi_map;

# delete from hash if we are using te new interface
foreach my $code (keys(%code_style_map))
{
    delete ($code_style_map{$code}) 
       if (ref($style_map_ref->{$code}) eq 'HASH');
}

# no paragraph in these commands
our %no_paragraph_macro = (
           'xref'         => 1,
           'ref'          => 1,
           'pxref'        => 1,
           'inforef'      => 1,
           'anchor'       => 1,
);


#foreach my $command (keys(%Texi2HTML::Config::style_map))
#{
#    next unless (ref($style_map_ref->{$command}) eq 'HASH');
#    print STDERR "CMD: $command\n";
#    die "Bug: no args for $command in style_map\n" unless defined($style_map_ref->{$command}->{'args'});
#    die "Bug: no args for $command in style_map_pre\n" unless defined($style_map_pre_ref->{$command}->{'args'});
#    die "Bug: non existence of args for $command in style_map_texi\n" unless (exists($style_map_texi_ref->{$command}->{'args'}));
#    die "Bug: no args for $command in style_map_texi\n" unless defined($style_map_texi_ref->{$command}->{'args'});
#}

#
# texinfo section names to level
#
%sec2level = (
	      'top', 0,
	      'chapter', 1,
	      'unnumbered', 1,
	      'chapheading', 1,
	      'appendix', 1,
	      'section', 2,
	      'unnumberedsec', 2,
	      'heading', 2,
	      'appendixsec', 2,
	      'subsection', 3,
	      'unnumberedsubsec', 3,
	      'subheading', 3,
	      'appendixsubsec', 3,
	      'subsubsection', 4,
	      'unnumberedsubsubsec', 4,
	      'subsubheading', 4,
	      'appendixsubsubsec', 4,
         );

# the reverse mapping. There is an entry for each sectionning command.
# The value is a ref on an array containing at each index the corresponding
# sectionning command name.
my %level2sec;
{
    my $sections = [ ];
    my $appendices = [ ];
    my $unnumbered = [ ];
    my $headings = [ ];
    foreach my $command (keys (%sec2level))
    {
        if ($command =~ /^appendix/)
        {
            $level2sec{$command} = $appendices;
        }
        elsif ($command =~ /^unnumbered/ or $command eq 'top')
        {
            $level2sec{$command} = $unnumbered;
        }
        elsif ($command =~ /section$/ or $command eq 'chapter')
        {
            $level2sec{$command} = $sections;
        }
        else
        {
            $level2sec{$command} = $headings;
        }
        $level2sec{$command}->[$sec2level{$command}] = $command;
    }
}

# this are synonyms
$sec2level{'appendixsection'} = 2;
# sec2level{'majorheading'} is also 1 and not 0
$sec2level{'majorheading'} = 1;
$sec2level{'chapheading'} = 1;
# FIXME this one could be centered...
$sec2level{'centerchap'} = 1;

# regions treated especially. The text for these regions is collected in the
# corresponding array 
%region_lines = (
          'titlepage'            => [ ],
          'documentdescription'  => [ ],
          'copying'              => [ ],
);

# a hash associating a format @thing / @end thing with the type of the format
# 'complex' 'simple' 'deff' 'list' 'menu' 'paragraph_style'
my %format_type = (); 

foreach my $simple_format (keys(%Texi2HTML::Config::format_map))
{
   $format_type{$simple_format} = 'simple';
}
foreach my $paragraph_style (keys(%Texi2HTML::Config::paragraph_style))
{
   $format_type{$paragraph_style} = 'paragraph_style';
}
foreach my $complex_format (keys(%$Texi2HTML::Config::complex_format_map))
{
   $format_type{$complex_format} = 'complex';
}
foreach my $table (('table', 'ftable', 'vtable', 'multitable'))
{
   $format_type{$table} = 'table';
}
foreach my $def_format (keys(%Texi2HTML::Config::def_map))
{
   $format_type{$def_format} = 'deff';
}
$format_type{'itemize'} = 'list';
$format_type{'enumerate'} = 'list';

$format_type{'menu'} = 'menu';

$format_type{'cartouche'} = 'cartouche';

# fake format at the bottom of the stack
$format_type{'noformat'} = '';

# fake formats are formats used internally within other formats
# we associate them with a real format, for the error messages
my %fake_format = (
     'line' => 'table',
     'term' => 'table',
     'item' => 'list or table',
     'row' => 'multitable row',
     'cell' => 'multitable cell',
     'deff_item' => 'definition command',
     'menu_comment' => 'menu',
     'menu_description' => 'menu',
     'menu_preformatted' => 'menu',
  );

foreach my $key (keys(%fake_format))
{
    $format_type{$key} = 'fake';
}

# A hash associating style @-comand with the type, 'accent', real 'style',
# 'simple' style, or 'special'.
our %style_type = (); 
foreach my $style (keys(%Texi2HTML::Config::style_map))
{
    $style_type{$style} = 'style';
}
foreach my $accent (keys(%Texi2HTML::Config::unicode_accents), 'tieaccent', 'dotless')
{
    $style_type{$accent} = 'accent';
}
foreach my $simple ('ctrl', 'w', 'url')
{
    $style_type{$simple} = 'simple';
}
foreach my $special ('footnote', 'ref', 'xref', 'pxref', 'inforef', 'anchor', 'image')
{
    $style_type{$special} = 'special';
}

# raw formats which are expanded especially
my @raw_regions = ('html', 'verbatim', 'tex', 'xml');

# special raw formats which are expanded between first and second pass
# and are replaced by specific commands. Currently used for tex. It takes
# precedence over raw_regions.
my @special_regions = ();

# regions expanded or not depending on the value of this hash
my %text_macros = (
     'iftex' => 0, 
     'ignore' => 0, 
     'menu' => 0, 
     'ifplaintext' => 0, 
     'ifinfo' => 0,
     'ifxml' => 0,
     'ifhtml' => 0, 
     'html' => 0, 
     'tex' => 0, 
     'xml' => 0,
     'titlepage' => 1, 
     'documentdescription' => 1, 
     'copying' => 1, 
     'ifnothtml' => 1, 
     'ifnottex' => 1, 
     'ifnotplaintext' => 1, 
     'ifnotinfo' => 1,
     'ifnotxml' => 1,
     'direntry' => 0,
     'verbatim' => 'raw', 
     'ifclear' => 'value', 
     'ifset' => 'value' 
     );
    
# those macros aren't considered as beginning a paragraph
my %no_line_macros = (
    'setfilename' => 1,
    'settitle' => 1,
    'macro' => 1,
    'unmacro' => 1,
    'rmacro' => 1,
    'set' => 1,
    'clear' => 1,
    'kbdinputstyle' => 1,
    'novalidate' => 1,
    'syncodeindex' => 1,
    'synindex' => 1,
    'defindex' => 1,
    'defcodeindex' => 1,
    'author' => 1,
    'documentlanguage' => 1,
    'title' => 1,
    'titlefont' => 1,
    'subtitle' => 1,
    'shorttitle' => 1,
    'shorttitlepage' => 1,
    'include' => 1,
    'verbatiminclude' => 1,
    'copying' => 1,
    'end copying' => 1,
    'dircategory' => 1,
    'tab' => 1,
    'item' => 1,
    'itemx' => 1,
    '*' => 1,
    'sp' => 1,
);

foreach my $key (keys(%Texi2HTML::Config::to_skip))
{
    $no_line_macros{$key} = 1;
}

foreach my $key (keys(%text_macros))
{
    unless ($text_macros{$key} eq 'raw')
    {
        $no_line_macros{$key} = 1;
        $no_line_macros{"end $key"} = 1;
    }
}

# The css formats are associated with complex format commands, and associated
# with the 'pre_style' key
foreach my $complex_format (keys(%$Texi2HTML::Config::complex_format_map))
{
    next if (defined($Texi2HTML::Config::complex_format_map->{$complex_format}->{'pre_style'}));
    $Texi2HTML::Config::complex_format_map->{$complex_format}->{'pre_style'} = '';
    $Texi2HTML::Config::complex_format_map->{$complex_format}->{'pre_style'} = $Texi2HTML::Config::css_map{"pre.$complex_format"} if (exists($Texi2HTML::Config::css_map{"pre.$complex_format"}));
}

#+++############################################################################
#                                                                              #
# Argument parsing, initialisation                                             #
#                                                                              #
#---############################################################################

#
# flush stdout and stderr after every write
#
select(STDERR);
$| = 1;
select(STDOUT);
$| = 1;

#FIXME my or our ?
my $I = \&Texi2HTML::I18n::get_string;

my $T2H_TODAY; # date set by pretty_date
my $T2H_USER; # user running the script
my $documentdescription; # text in @documentdescription 

# shorthand for Texi2HTML::Config::VERBOSE
my $T2H_VERBOSE;

#print STDERR "" . &$I('test i18n: \' , \a \\ %% %{unknown}a %known % %{known}  \\', { 'known' => 'a known string', 'no' => 'nope'}); exit 0;

# file:      file name to locate. It can be a file path.
# all_files: if true collect all the files with that name, otherwise stop
#            at first match.
sub locate_init_file($;$)
{
    my $file = shift;
    my $all_files = shift;
    if ($file =~ /^\//)
    {
         return $file if (-e $file and -r $file);
    }
    else
    {
         my @files;
         my @dirs = ('./');
         push @dirs, "$ENV{'HOME'}/.texi2html/" if (defined($ENV{'HOME'}));
         push @dirs, "$sysconfdir/texi2html/" if (defined($sysconfdir));
         push @dirs, "$pkgdatadir" if (defined($pkgdatadir));
         foreach my $dir (@dirs)
         {
              next unless (-d "$dir");
              if ($all_files)
              {
                  push (@files, "$dir/$file") if (-e "$dir/$file" and -r "$dir/$file");
              }
              else
              {
                  return "$dir/$file" if (-e "$dir/$file" and -r "$dir/$file");
              }
         }
         return @files if ($all_files);
    }
    return undef;
}

#
# called on -init-file
sub load_init_file
{
    # First argument is option
    shift;
    # second argument is value of options
    my $init_file = shift;
    my $file;
    if ($file = locate_init_file($init_file))
    {
        print STDERR "# reading initialization file from $file\n"
            if ($T2H_VERBOSE);
        return (Texi2HTML::Config::load($file));
    }
    else
    {
        print STDERR "$ERROR Error: can't read init file $init_file\n";
        return 0;
    }
}

my $cmd_line_lang = 0; # 1 if lang was succesfully set by the command line 
my $lang_set = 0; # 1 if lang was set

#
# called on -lang
sub set_document_language ($;$$)
{
    my $lang = shift;
    my $from_command_line = shift;
    my $line_nr = shift;
    my @files = locate_init_file("$i18n_dir/$lang", 1);
    foreach  my $file (@files)
    {
        Texi2HTML::Config::load($file);
    }
    if (Texi2HTML::I18n::set_language($lang))
    {
        print STDERR "# using '$lang' as document language\n" if ($T2H_VERBOSE);
        $Texi2HTML::Config::LANG = $lang;
        $lang_set = 1;
        $cmd_line_lang = 1 if ($from_command_line);
    }
    else
    {
        echo_error ("Language specs for '$lang' do not exists. Reverting to '$Texi2HTML::Config::LANG'", $line_nr);
    }
}

sub set_expansion($$)
{
    my $region = shift;
    my $set = shift;
    $set = 1 if (!defined($set));
    if ($set)
    {
         push (@Texi2HTML::Config::EXPAND, $region) unless (grep {$_ eq $region} @Texi2HTML::Config::EXPAND);
    }
    else
    {
         @Texi2HTML::Config::EXPAND = grep {$_ ne $region} @Texi2HTML::Config::EXPAND;
    }
}
	 
sub set_encoding($)
{
    my $encoding = shift;
    if ($Texi2HTML::Config::USE_UNICODE)
    {
         if (! Encode::resolve_alias($encoding))
         {
              echo_warn("Encoding $Texi2HTML::Config::DOCUMENT_ENCODING unknown");
              return undef;
         }
         print STDERR "# Using encoding " . Encode::resolve_alias($encoding) . "\n"
             if ($T2H_VERBOSE);
         return Encode::resolve_alias($encoding); 
    }
    else
    {
         echo_warn("Nothing specific done for encodings (due to the perl version)");
         return undef;
    }
}

our %cross_ref_texi_map = %Texi2HTML::Config::texi_map;
our %cross_ref_simple_map_texi = %Texi2HTML::Config::simple_map_texi;
our %cross_ref_style_map_texi = ();

foreach my $command (keys(%Texi2HTML::Config::style_map_texi))
{
    $cross_ref_style_map_texi{$command} = {}; 
    foreach my $key (keys (%{$Texi2HTML::Config::style_map_texi{$command}}))
    {
#print STDERR "$command, $key, $style_map_texi{$command}->{$key}\n";
         $cross_ref_style_map_texi{$command}->{$key} = 
              $Texi2HTML::Config::style_map_texi{$command}->{$key};
    }
}

$cross_ref_simple_map_texi{"\n"} = ' ';


# This function is used to construct a link name from a node name as 
# described in the proposal I posted on texinfo-pretest.
sub cross_manual_links($$)
{
    my $nodes_hash = shift;
    my $cross_reference_hash = shift;

    $simple_map_texi_ref = \%cross_ref_simple_map_texi;
    $style_map_texi_ref = \%cross_ref_style_map_texi;
    $texi_map_ref = \%cross_ref_texi_map;
    my $normal_text_kept = $Texi2HTML::Config::normal_text;
    $Texi2HTML::Config::normal_text = \&Texi2HTML::Config::t2h_cross_manual_normal_text;

    foreach my $key (keys(%$nodes_hash))
    {
        my $node = $nodes_hash->{$key};
        next if ($node->{'index_page'});
        if (!defined($node->{'texi'}))
        {
            # begin debug section 
            foreach my $key (keys(%$node))
            {
                #print STDERR "$key:$node->{$key}!!!\n";
            }
            # end debug section 
        }
        else 
        {
            if ($Texi2HTML::Config::USE_UNICODE)
            {
                 my $text = $node->{'texi'};
#print STDERR "CROSS_MANUAL $node->{'texi'}\n";
                 if (defined($Texi2HTML::Config::DOCUMENT_ENCODING) and 
                      Encode::resolve_alias($Texi2HTML::Config::DOCUMENT_ENCODING) and
                      (Encode::resolve_alias($Texi2HTML::Config::DOCUMENT_ENCODING) ne 'utf8'))
                 {
                      $text = Encode::decode($Texi2HTML::Config::DOCUMENT_ENCODING, $text);
                 }
                 $node->{'cross_manual_target'} = unicode_to_protected(Unicode::Normalize::NFC(remove_texi($text)));
            }
            else
            {
                 $node->{'cross_manual_target'} = remove_texi($node->{'texi'});
            }
#print STDERR "CROSS_MANUAL_TARGET $node->{'cross_manual_target'}\n";
            unless ($node->{'external_node'})
            {
                if (defined($cross_reference_hash->{$node->{'cross_manual_target'}}))
                {
                    echo_error("Node equivalent with `$node->{'texi'}' allready used `$cross_reference_hash->{$node->{'cross_manual_target'}}'");
                }
                else 
                {
                    $cross_reference_hash->{$node->{'cross_manual_target'}} = $node->{'texi'};
                }
            }
            #print STDERR "$node->{'texi'}: $node->{'cross_manual_target'}\n";
        }
    }

    $Texi2HTML::Config::normal_text = $normal_text_kept;
    $simple_map_texi_ref = \%Texi2HTML::Config::simple_map_texi;
    $style_map_texi_ref = \%Texi2HTML::Config::style_map_texi;
    $texi_map_ref = \%Texi2HTML::Config::texi_map;
}

sub unicode_to_protected($)
{
   my $text = shift;
   my $result = '';
   while ($text ne '')
   {
        if ($text =~ s/^([A-Za-z0-9_\-]+)//o)
        {
             $result .= $1;
        }
        elsif ($text =~ s/^(.)//o)
        {
             $result .= '_' . lc(sprintf("%04x",ord($1)));
        }
        else
        {
             print STDERR "Bug: unknown character in node (likely in infinite loop)\n";
             sleep 1;
        }    
   }
   
   return $result;
}

sub cross_manual_line($)
{
    my $text = shift;
    $simple_map_texi_ref = \%cross_ref_simple_map_texi;
    $style_map_texi_ref = \%cross_ref_style_map_texi;
    $texi_map_ref = \%cross_ref_texi_map;
    my $normal_text_kept = $Texi2HTML::Config::normal_text;
    $Texi2HTML::Config::normal_text = \&Texi2HTML::Config::t2h_cross_manual_normal_text;
    
    my $cross_ref;
    if ($Texi2HTML::Config::USE_UNICODE)
    {
         $cross_ref = unicode_to_protected(Unicode::Normalize::NFC(remove_texi($text)));
    }
    else
    {
         $cross_ref = remove_texi($text);
    }

    $Texi2HTML::Config::normal_text = $normal_text_kept;
    $simple_map_texi_ref = \%Texi2HTML::Config::simple_map_texi;
    $style_map_texi_ref = \%Texi2HTML::Config::style_map_texi;
    $texi_map_ref = \%Texi2HTML::Config::texi_map;
    return $cross_ref;
}

# T2H_OPTIONS is a hash whose keys are the (long) names of valid
# command-line options and whose values are a hash with the following keys:
# type    ==> one of !|=i|:i|=s|:s (see GetOpt::Long for more info)
# linkage ==> ref to scalar, array, or subroutine (see GetOpt::Long for more info)
# verbose ==> short description of option (displayed by -h)
# noHelp  ==> if 1 -> for "not so important options": only print description on -h 1
#                2 -> for obsolete options: only print description on -h 2
my $T2H_OPTIONS;
$T2H_OPTIONS -> {'debug'} =
{
 type => '=i',
 linkage => \$Texi2HTML::Config::DEBUG,
 verbose => 'output HTML with debuging information',
};

$T2H_OPTIONS -> {'doctype'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::DOCTYPE,
 verbose => 'document type which is output in header of HTML files',
 noHelp => 1
};

$T2H_OPTIONS -> {'frameset-doctype'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::FRAMESET_DOCTYPE,
 verbose => 'document type for HTML frameset documents',
 noHelp => 1
};

$T2H_OPTIONS -> {'test'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::TEST,
 verbose => 'use predefined information to avoid differences with reference files',
 noHelp => 1
};

$T2H_OPTIONS -> {'dump-texi'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::DUMP_TEXI,
 verbose => 'dump the output of first pass into a file with extension passfirst and exit',
 noHelp => 1
};

$T2H_OPTIONS -> {'macro-expand'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::MACRO_EXPAND,
 verbose => 'output macro expanded source in <file>',
};

$T2H_OPTIONS -> {'expand'} =
{
 type => '=s',
 linkage => sub {set_expansion($_[1], 1);},
 verbose => 'Expand info|tex|none section of texinfo source',
 noHelp => 1,
};

$T2H_OPTIONS -> {'no-expand'} =
{
 type => '=s',
 linkage => sub {set_expansion ($_[1], 0);},
 verbose => 'Don\'t expand the given section of texinfo source',
};

$T2H_OPTIONS -> {'noexpand'} = 
{
 type => '=s',
 linkage => $T2H_OPTIONS->{'no-expand'}->{'linkage'},
 verbose => $T2H_OPTIONS->{'no-expand'}->{'verbose'},
 noHelp  => 1,
};

$T2H_OPTIONS -> {'ifhtml'} =
{
 type => '!',
 linkage => sub { set_expansion('html', $_[1]); },
 verbose => "expand ifhtml and html sections",
};

$T2H_OPTIONS -> {'ifinfo'} =
{
 type => '!',
 linkage => sub { set_expansion('info', $_[1]); },
 verbose => "expand ifinfo",
};

$T2H_OPTIONS -> {'ifxml'} =
{
 type => '!',
 linkage => sub { set_expansion('xml', $_[1]); },
 verbose => "expand ifxml and xml sections",
};

$T2H_OPTIONS -> {'iftex'} =
{
 type => '!',
 linkage => sub { set_expansion('tex', $_[1]); },
 verbose => "expand iftex and tex sections",
};

$T2H_OPTIONS -> {'ifplaintext'} =
{
 type => '!',
 linkage => sub { set_expansion('plaintext', $_[1]); },
 verbose => "expand ifplaintext sections",
};

#$T2H_OPTIONS -> {'no-ifhtml'} =
#{
# type => '!',
# linkage => sub { set_expansion('html', (! $_[1])); },
# verbose => "don't expand ifhtml and html sections",
# noHelp => 1,
#};

#$T2H_OPTIONS -> {'no-ifinfo'} =
#{
# type => '!',
# linkage => sub { set_expansion('info', (! $_[1])); },
# verbose => "don't expand ifinfo",
# noHelp => 1,
#};

#$T2H_OPTIONS -> {'no-ifxml'} =
#{
# type => '!',
# linkage => sub { set_expansion('xml', (! $_[1])); },
# verbose => "don't expand ifxml and xml sections",
# noHelp => 1,
#};

#$T2H_OPTIONS -> {'no-iftex'} =
#{
# type => '!',
# linkage => sub { set_expansion('tex', (! $_[1])); },
# verbose => "don't expand iftex and tex sections",
# noHelp => 1,
#};

#$T2H_OPTIONS -> {'no-ifplaintext'} =
#{
# type => '!',
# linkage => sub { set_expansion('plaintext', (! $_[1])); },
# verbose => "don't expand ifplaintext sections",
# noHelp => 1,
#};

$T2H_OPTIONS -> {'invisible'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::INVISIBLE_MARK,
 verbose => 'use text in invisble anchor',
 noHelp  => 1,
};

$T2H_OPTIONS -> {'iso'} =
{
 type => 'iso',
 linkage => \$Texi2HTML::Config::USE_ISO,
 verbose => 'if set, ISO8859 characters are used for special symbols (like copyright, etc)',
 noHelp => 1,
};

$T2H_OPTIONS -> {'I'} =
{
 type => '=s',
 linkage => \@Texi2HTML::Config::INCLUDE_DIRS,
 verbose => 'append $s to the @include search path',
};

$T2H_OPTIONS -> {'P'} =
{
 type => '=s',
 linkage => sub {unshift (@Texi2HTML::Config::PREPEND_DIRS, $_[1]);},
 verbose => 'prepend $s to the @include search path',
};

$T2H_OPTIONS -> {'top-file'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::TOP_FILE,
 verbose => 'use $s as top file, instead of <docname>.html',
};


$T2H_OPTIONS -> {'toc-file'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::TOC_FILE,
 verbose => 'use $s as ToC file, instead of <docname>_toc.html',
};

$T2H_OPTIONS -> {'frames'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::FRAMES,
 verbose => 'output files which use HTML 4.0 frames (experimental)',
 noHelp => 1,
};

$T2H_OPTIONS -> {'menu'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::SHOW_MENU,
 verbose => 'output Texinfo menus',
};

#$T2H_OPTIONS -> {'no-menu'} =
#{
# type => '!',
# linkage => sub { $Texi2HTML::Config::SHOW_MENU = (! $_[1]);},
# verbose => "don't output Texinfo menus",
# noHelp => 1,
#};

$T2H_OPTIONS -> {'number'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::NUMBER_SECTIONS,
 verbose => 'use numbered sections',
};

#$T2H_OPTIONS -> {'no-number'} =
#{
# type => '!',
# linkage => sub { $Texi2HTML::Config::NUMBER_SECTIONS = (! $_[1]);}, 
# verbose => 'sections not numbered',
# noHelp => 1,
#};

$T2H_OPTIONS -> {'use-nodes'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::USE_NODES,
 verbose => 'use nodes for sectionning',
};

$T2H_OPTIONS -> {'node-files'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::NODE_FILES,
 verbose => 'produce one file per node for cross references'
};

$T2H_OPTIONS -> {'separated-footnotes'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::SEPARATED_FOOTNOTES,
 verbose => 'footnotes on a separated page',
 noHelp => 1,
};

$T2H_OPTIONS -> {'toc-links'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::TOC_LINKS,
 verbose => 'create links from headings to toc entries'
};

$T2H_OPTIONS -> {'split'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::SPLIT,
 verbose => 'split document on section|chapter|node else no splitting',
};

$T2H_OPTIONS -> {'sec-nav'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::SECTION_NAVIGATION,
 verbose => 'output navigation panels for each section',
};

$T2H_OPTIONS -> {'subdir'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::SUBDIR,
 verbose => 'put files in directory $s, not $cwd',
 noHelp => 1,
};

$T2H_OPTIONS -> {'short-ext'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::SHORTEXTN,
 verbose => 'use "htm" extension for output HTML files',
};

$T2H_OPTIONS -> {'prefix'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::PREFIX,
 verbose => 'use as prefix for output files, instead of <docname>',
};

$T2H_OPTIONS -> {'output'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::OUT,
 verbose => 'output goes to $s (directory if split)',
};

$T2H_OPTIONS -> {'no-validate'} = 
{
 type => '!',
 linkage => \$Texi2HTML::Config::NOVALIDATE,
 verbose => 'suppress node cross-reference validation',
};

$T2H_OPTIONS -> {'short-ref'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::SHORT_REF,
 verbose => 'if set, references are without section numbers',
};

$T2H_OPTIONS -> {'idx-sum'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::IDX_SUMMARY,
 verbose => 'if set, also output index summary',
 noHelp  => 1,
};

$T2H_OPTIONS -> {'def-table'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::DEF_TABLE,
 verbose => 'if set, \@def.. are converted using tables.',
 noHelp  => 1,
};

$T2H_OPTIONS -> {'Verbose'} =
{
 type => '!',
 linkage=> \$Texi2HTML::Config::VERBOSE,
 verbose => 'print progress info to stdout',
};

$T2H_OPTIONS -> {'lang'} =
{
 type => '=s',
 linkage => sub {set_document_language($_[1], 1)},
 verbose => 'use $s as document language (ISO 639 encoding)',
};

$T2H_OPTIONS -> {'ignore-preamble-text'} =
{
  type => '!',
  linkage => \$Texi2HTML::Config::IGNORE_PREAMBLE_TEXT,
  verbose => 'if set, ignore the text before @node and sectionning commands',
  noHelp => 1,
};

$T2H_OPTIONS -> {'html-xref-prefix'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::EXTERNAL_DIR,
 verbose => '$s is the base dir for external manual references',
 noHelp => 1,
};

$T2H_OPTIONS -> {'l2h'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::L2H,
 verbose => 'if set, uses latex2html for @math and @tex',
};

$T2H_OPTIONS -> {'l2h-l2h'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::L2H_L2H,
 verbose => 'program to use for latex2html translation',
 noHelp => 1,
};

$T2H_OPTIONS -> {'l2h-skip'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::L2H_SKIP,
 verbose => 'if set, tries to reuse previously latex2html output',
 noHelp => 1,
};

$T2H_OPTIONS -> {'l2h-tmp'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::L2H_TMP,
 verbose => 'if set, uses $s as temporary latex2html directory',
 noHelp => 1,
};

$T2H_OPTIONS -> {'l2h-file'} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::L2H_FILE,
 verbose => 'if set, uses $s as latex2html init file',
 noHelp => 1,
};


$T2H_OPTIONS -> {'l2h-clean'} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::L2H_CLEAN,
 verbose => 'if set, do not keep intermediate latex2html files for later reuse',
 noHelp => 1,
};

$T2H_OPTIONS -> {'D'} =
{
 type => '=s',
 linkage => sub {$value{$_[1]} = 1;},
 verbose => 'equivalent to Texinfo "@set $s 1"',
 noHelp => 1,
};

$T2H_OPTIONS -> {'U'} =
{
 type => '=s',
 linkage => sub {delete $value{$_[1]};},
 verbose => 'equivalent to Texinfo "@clear $s"',
 noHelp => 1,
};

$T2H_OPTIONS -> {'init-file'} =
{
 type => '=s',
 linkage => \&load_init_file,
 verbose => 'load init file $s'
};

$T2H_OPTIONS -> {'css-include'} =
{
 type => '=s',
 linkage => \@Texi2HTML::Config::CSS_FILES,
 verbose => 'use css file $s'
};

##
## obsolete cmd line options
##
my $T2H_OBSOLETE_OPTIONS;

$T2H_OBSOLETE_OPTIONS -> {'out-file'} =
{
 type => '=s',
 linkage => sub {$Texi2HTML::Config::OUT = $_[1]; $Texi2HTML::Config::SPLIT = '';},
 verbose => 'if set, all HTML output goes into file $s, obsoleted by "-output" with different semantics',
 noHelp => 2
};

$T2H_OBSOLETE_OPTIONS -> {init_file} =
{
 type => '=s',
 linkage => \&load_init_file,
 verbose => 'obsolete, use "-init-file" instead',
 noHelp => 2
};

$T2H_OBSOLETE_OPTIONS -> {l2h_clean} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::L2H_CLEAN,
 verbose => 'obsolete, use "-l2h-clean" instead',
 noHelp => 2,
};

$T2H_OBSOLETE_OPTIONS -> {l2h_l2h} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::L2H_L2H,
 verbose => 'obsolete, use "-l2h-l2h" instead',
 noHelp => 2
};

$T2H_OBSOLETE_OPTIONS -> {l2h_skip} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::L2H_SKIP,
 verbose => 'obsolete, use "-l2h-skip" instead',
 noHelp => 2
};

$T2H_OBSOLETE_OPTIONS -> {l2h_tmp} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::L2H_TMP,
 verbose => 'obsolete, use "-l2h-tmp" instead',
 noHelp => 2
};

$T2H_OBSOLETE_OPTIONS -> {out_file} =
{
 type => '=s',
 linkage => sub {$Texi2HTML::Config::OUT = $_[1]; $Texi2HTML::Config::SPLIT = '';},
 verbose => 'obsolete, use "-out-file" instead',
 noHelp => 2
};

$T2H_OBSOLETE_OPTIONS -> {short_ref} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::SHORT_REF,
 verbose => 'obsolete, use "-short-ref" instead',
 noHelp => 2
};

$T2H_OBSOLETE_OPTIONS -> {idx_sum} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::IDX_SUMMARY,
 verbose => 'obsolete, use "-idx-sum" instead',
 noHelp  => 2
};

$T2H_OBSOLETE_OPTIONS -> {def_table} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::DEF_TABLE,
 verbose => 'obsolete, use "-def-table" instead',
 noHelp  => 2
};

$T2H_OBSOLETE_OPTIONS -> {short_ext} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::SHORTEXTN,
 verbose => 'obsolete, use "-short-ext" instead',
 noHelp  => 2
};

$T2H_OBSOLETE_OPTIONS -> {sec_nav} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::SECTION_NAVIGATION,
 verbose => 'obsolete, use "-sec-nav" instead',
 noHelp  => 2
};

$T2H_OBSOLETE_OPTIONS -> {top_file} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::TOP_FILE,
 verbose => 'obsolete, use "-top-file" instead',
 noHelp  => 2
};

$T2H_OBSOLETE_OPTIONS -> {toc_file} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::TOC_FILE,
 verbose => 'obsolete, use "-toc-file" instead',
 noHelp  => 2
};

$T2H_OBSOLETE_OPTIONS -> {glossary} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::USE_GLOSSARY,
# verbose => "if set, uses section named `Footnotes' for glossary",
 verbose => "this does nothing",
 noHelp  => 2,
};

$T2H_OBSOLETE_OPTIONS -> {dump_texi} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::DUMP_TEXI,
 verbose => 'obsolete, use "-dump-texi" instead',
 noHelp => 1
};

$T2H_OBSOLETE_OPTIONS -> {frameset_doctype} =
{
 type => '=s',
 linkage => \$Texi2HTML::Config::FRAMESET_DOCTYPE,
 verbose => 'obsolete, use "-frameset-doctype" instead',
 noHelp => 2
};

$T2H_OBSOLETE_OPTIONS -> {'no-section_navigation'} =
{
 type => '!',
 linkage => sub {$Texi2HTML::Config::SECTION_NAVIGATION = 0;},
 verbose => 'obsolete, use -nosec_nav',
 noHelp => 2,
};
my $use_acc; # not used
$T2H_OBSOLETE_OPTIONS -> {use_acc} =
{
 type => '!',
 linkage => \$use_acc,
 verbose => 'obsolete, set to true unconditionnaly',
 noHelp => 2
};
$T2H_OBSOLETE_OPTIONS -> {expandinfo} =
{
 type => '!',
 linkage => sub {push @Texi2HTML::Config::EXPAND, 'info';},
 verbose => 'obsolete, use "-expand info" instead',
 noHelp => 2,
};
$T2H_OBSOLETE_OPTIONS -> {expandtex} =
{
 type => '!',
 linkage => sub {push @Texi2HTML::Config::EXPAND, 'tex';},
 verbose => 'obsolete, use "-expand tex" instead',
 noHelp => 2,
};
$T2H_OBSOLETE_OPTIONS -> {monolithic} =
{
 type => '!',
 linkage => sub {$Texi2HTML::Config::SPLIT = '';},
 verbose => 'obsolete, use "-split no" instead',
 noHelp => 2
};
$T2H_OBSOLETE_OPTIONS -> {split_node} =
{
 type => '!',
 linkage => sub{$Texi2HTML::Config::SPLIT = 'section';},
 verbose => 'obsolete, use "-split section" instead',
 noHelp => 2,
};
$T2H_OBSOLETE_OPTIONS -> {split_chapter} =
{
 type => '!',
 linkage => sub{$Texi2HTML::Config::SPLIT = 'chapter';},
 verbose => 'obsolete, use "-split chapter" instead',
 noHelp => 2,
};
$T2H_OBSOLETE_OPTIONS -> {no_verbose} =
{
 type => '!',
 linkage => sub {$Texi2HTML::Config::VERBOSE = 0;},
 verbose => 'obsolete, use -noverbose instead',
 noHelp => 2,
};
$T2H_OBSOLETE_OPTIONS -> {output_file} =
{
 type => '=s',
 linkage => sub {$Texi2HTML::Config::OUT = $_[1]; $Texi2HTML::Config::SPLIT = '';},
 verbose => 'obsolete, use -out_file instead',
 noHelp => 2
};

$T2H_OBSOLETE_OPTIONS -> {section_navigation} =
{
 type => '!',
 linkage => \$Texi2HTML::Config::SECTION_NAVIGATION,
 verbose => 'obsolete, use -sec_nav instead',
 noHelp => 2,
};

$T2H_OBSOLETE_OPTIONS -> {verbose} =
{
 type => '!',
 linkage=> \$Texi2HTML::Config::VERBOSE,
 verbose => 'obsolete, use -Verbose instead',
 noHelp => 2
};

# read initialzation from $sysconfdir/texi2htmlrc or $HOME/.texi2htmlrc
# obsolete.
my @rc_files = ();
push @rc_files, "$sysconfdir/texi2htmlrc" if defined($sysconfdir);
push @rc_files, "$ENV{'HOME'}/.texi2htmlrc" if (defined($ENV{HOME}));
foreach my $i (@rc_files)
{
    if (-e $i and -r $i)
    {
        print STDERR "# reading initialization file from $i\n"
	    if ($T2H_VERBOSE);
        print STDERR "Reading config from $i is obsolete, use texi2html/$conf_file_name instead\n";
        Texi2HTML::Config::load($i);
    }
}

# read initialization files
foreach my $file (locate_init_file($conf_file_name, 1))
{
	print STDERR "# reading initialization file from $file\n" if ($T2H_VERBOSE);
    Texi2HTML::Config::load($file);
}
    
#
# %value hold texinfo variables, see also -D, -U, @set and @clear.
# we predefine html (the output format) and texi2html (the translator)
%value = 
      ( 
          'html' => 1,
          'texi2html' => $THISVERSION,
      );                       

#+++############################################################################
#                                                                              #
# parse command-line options
#                                                                              #
#---############################################################################


my $T2H_USAGE_TEXT = <<EOT;
Usage: texi2html  [OPTIONS] TEXINFO-FILE
Translates Texinfo source documentation to HTML.
EOT
my $T2H_FAILURE_TEXT = <<EOT;
Try 'texi2html --help' for usage instructions.
EOT

my $options = new Getopt::MySimple;
# some older version of GetOpt::Long don't have
# Getopt::Long::Configure("pass_through")
eval {Getopt::Long::Configure("pass_through");};
my $Configure_failed = $@ && <<EOT;
**WARNING: Parsing of obsolete command-line options could have failed.
           Consider to use only documented command-line options (run
           'texi2html --help 2' for a complete list) or upgrade to perl
           version 5.005 or higher.
EOT
# FIXME getOptions is called 2 times, and thus adds 2 times the default 
# help and version and after that warns.
if (! $options->getOptions($T2H_OPTIONS, $T2H_USAGE_TEXT, "$THISVERSION\n"))
{
    print STDERR "$Configure_failed" if $Configure_failed;
    die $T2H_FAILURE_TEXT;
}
if (@ARGV > 1)
{
    eval {Getopt::Long::Configure("no_pass_through");};
    if (! $options->getOptions($T2H_OBSOLETE_OPTIONS, $T2H_USAGE_TEXT, "$THISVERSION\n"))
    {
        print STDERR "$Configure_failed" if $Configure_failed;
        die $T2H_FAILURE_TEXT;
    }
}
# $T2H_DEBUG and $T2H_VERBOSE are shorthands
my $T2H_DEBUG = $Texi2HTML::Config::DEBUG;
$T2H_VERBOSE = $Texi2HTML::Config::VERBOSE;

#+++############################################################################
#                                                                              #
# evaluation of cmd line options
#                                                                              #
#---############################################################################

# retro compatibility for $Texi2HTML::Config::EXPAND
push (@Texi2HTML::Config::EXPAND, $Texi2HTML::Config::EXPAND) if ($Texi2HTML::Config::EXPAND);

# correct %text_macros and @special_regions based on command line and init
# variables
$text_macros{'menu'} = 1 if ($Texi2HTML::Config::SHOW_MENU);

push @special_regions, 'tex' if ($Texi2HTML::Config::L2H);

foreach my $expanded (@Texi2HTML::Config::EXPAND)
{
    $text_macros{"if$expanded"} = 1 if (exists($text_macros{"if$expanded"}));
    next unless (exists($text_macros{$expanded}));
    if (grep {$_ eq $expanded} @special_regions)
    {
         $text_macros{$expanded} = 'special'; 
    }
    elsif (grep {$_ eq $expanded} @raw_regions)
    {
         $text_macros{$expanded} = 'raw'; 
    }
    else
    {
         $text_macros{$expanded} = 1; 
    }
}

# handle ifnot regions
foreach my $region (keys (%text_macros))
{
    next if ($region =~ /^ifnot/);
    if ($text_macros{$region} and $region =~ /^if(\w+)$/)
    {
        $text_macros{"ifnot$1"} = 0;
    }
}

if ($T2H_VERBOSE)
{
    print STDERR "# Expanded: ";
    foreach my $text_macro (keys(%text_macros))
    {
        print STDERR "$text_macro " if ($text_macros{$text_macro});
    }
    print STDERR "\n";
}

# This is kept in that file although it is html formatting as it seems to 
# be rather obsolete
$Texi2HTML::Config::INVISIBLE_MARK = '<img src="invisible.xbm" alt="">' if $Texi2HTML::Config::INVISIBLE_MARK eq 'xbm';

$T2H_DEBUG |= $DEBUG_TEXI if ($Texi2HTML::Config::DUMP_TEXI);

# Construct hashes used for cross references generation
# Do it now as the user may have changed $USE_UNICODE

foreach my $key (keys(%Texi2HTML::Config::unicode_map))
{
    if ($Texi2HTML::Config::unicode_map{$key} ne '')
    {
        if ($Texi2HTML::Config::USE_UNICODE)
        {
             $cross_ref_texi_map{$key} = chr(hex($Texi2HTML::Config::unicode_map{$key}));
        }
        else
        {
             $cross_ref_texi_map{$key} = '_' . lc($Texi2HTML::Config::unicode_map{$key});
        }
    }
}

foreach my $key (keys(%cross_ref_style_map_texi))
{
    if (($Texi2HTML::Config::unicode_accents{$key} or ($key eq 'tieaccent') or ($key eq 'dotless')) 
        and (ref($cross_ref_style_map_texi{$key}) eq 'HASH'))
    {
        if ($Texi2HTML::Config::USE_UNICODE)
        {
             $cross_ref_style_map_texi{$key}->{'function'} = \&Texi2HTML::Config::t2h_utf8_accent;
        }
        else
        {
             $cross_ref_style_map_texi{$key}->{'function'} = \&Texi2HTML::Config::t2h_nounicode_cross_manual_accent;
        }
    }
}

#
# file name buisness
#

# this is directly pasted over from latex2html
sub getcwd
{
    local($_) = `pwd`;

    die "'pwd' failed (out of memory?)\n"
        unless length;
    chop;
    $_;
}


my $docu_dir;            # directory of the document
my $docu_name;           # basename of the document
my $docu_rdir;           # directory for the output
#my $docu_ext = "html";   # extension
my $docu_ext = $Texi2HTML::Config::EXTENSION;   # extension
my $docu_toc;            # document's table of contents
my $docu_stoc;           # document's short toc
my $docu_foot;           # document's footnotes
my $docu_about;          # about this document
my $docu_top;            # document top
my $docu_doc;            # document (or document top of split)

die "Need exactly one file to translate\n$T2H_FAILURE_TEXT" unless @ARGV == 1;
my $docu = shift(@ARGV);
if ($docu =~ /(.*\/)/)
{
    chop($docu_dir = $1);
    $docu_name = $docu;
    $docu_name =~ s/.*\///;
}
else
{
    $docu_dir = '.';
    $docu_name = $docu;
}
unshift(@Texi2HTML::Config::INCLUDE_DIRS, $docu_dir);
unshift(@Texi2HTML::Config::INCLUDE_DIRS, @Texi2HTML::Config::PREPEND_DIRS);
$docu_name =~ s/\.te?x(i|info)?$//;
$docu_name = $Texi2HTML::Config::PREFIX if ($Texi2HTML::Config::PREFIX);

# resulting files splitting
if ($Texi2HTML::Config::SPLIT =~ /section/i)
{
    $Texi2HTML::Config::SPLIT = 'section';
}
elsif ($Texi2HTML::Config::SPLIT =~ /node/i)
{
    $Texi2HTML::Config::SPLIT = 'node';
}
elsif ($Texi2HTML::Config::SPLIT =~ /chapter/i)
{
    $Texi2HTML::Config::SPLIT = 'chapter';
}
else
{
    $Texi2HTML::Config::SPLIT = '';
}

# Something like backward compatibility
if ($Texi2HTML::Config::SPLIT and $Texi2HTML::Config::SUBDIR)
{
    $Texi2HTML::Config::OUT = $Texi2HTML::Config::SUBDIR;
}

# subdir

die "output to STDOUT and split or frames incompatible\n" 
    if (($Texi2HTML::Config::SPLIT or $Texi2HTML::Config::FRAMES) and ($Texi2HTML::Config::OUT eq '-'));

if ($Texi2HTML::Config::SPLIT and ($Texi2HTML::Config::OUT eq ''))
{
    $Texi2HTML::Config::OUT = $docu_name;
}

if ($Texi2HTML::Config::SPLIT and ($Texi2HTML::Config::OUT eq '.'))
{# This is to avoid trouble with latex2html
    $Texi2HTML::Config::OUT = '';
}

$docu_rdir = '';

if ($Texi2HTML::Config::SPLIT and ($Texi2HTML::Config::OUT ne ''))
{
    $Texi2HTML::Config::OUT =~ s|/*$||;
    $docu_rdir = "$Texi2HTML::Config::OUT/"; 
    unless (-d $Texi2HTML::Config::OUT)
    {
        if ( mkdir($Texi2HTML::Config::OUT, oct(755)))
        {
            print STDERR "# created directory $Texi2HTML::Config::OUT\n" if ($T2H_VERBOSE);
        }
        else
        {
            die "$ERROR can't create directory $Texi2HTML::Config::OUT\n";
        }
    }
    print STDERR "# putting result files into directory $docu_rdir\n" if ($T2H_VERBOSE);
}
elsif (! $Texi2HTML::Config::SPLIT and ($Texi2HTML::Config::OUT ne ''))
{
    if ($Texi2HTML::Config::OUT =~ m|(.*)/|)
    {# there is a leading directories
        $docu_rdir = "$1/";
        unless (-d $docu_rdir)
        {
            if ( mkdir($docu_rdir, oct(755)))
            {
                 print STDERR "# created directory $docu_rdir\n" if ($T2H_VERBOSE);
            }
            else
            {
                die "$ERROR can't create directory $docu_rdir\n";
            }
        }
        print STDERR "# putting result files into directory $docu_rdir\n" if ($T2H_VERBOSE);
    }
    else
    {
        print STDERR "# putting result files into current directory \n" if ($T2H_VERBOSE);
        $docu_rdir = '';
    }
}

# We don't use "./" as $docu_rdir when $docu_rdir is the current directory
# because it is problematic for latex2html. To test writability with -w, 
# however we need a real directory.
my $result_rdir = $docu_rdir;
$result_rdir = "." if ($docu_rdir eq '');
unless (-w $result_rdir)
{
    $docu_rdir = 'current directory' if ($docu_rdir eq '');
    die "$ERROR $docu_rdir not writable\n";
}

# relative path leading to the working directory from the document directory
my $path_to_working_dir = $docu_rdir;
if ($docu_rdir ne '')
{
    my $cwd = getcwd;
    my $docu_path = $docu_rdir;
    $docu_path = $cwd . '/' . $docu_path unless ($docu_path =~ /^\//);
    my @result = ();
    foreach my $element (split /\//, File::Spec->canonpath($docu_path))
    {
        if ($element eq '')
        {
            push @result, '';
        }
        elsif ($element eq '..')
        {
            if (@result and ($result[-1] eq ''))
            {
                print STDERR "Too much .. in absolute file name\n";
            }
            elsif (@result and ($result[-1] ne '..'))
            {
                pop @result;
            }
            else
            {
                push @result, $element;
            }
        }
        else
        {
            push @result, $element;
        }
    }
    $path_to_working_dir = File::Spec->abs2rel($cwd, join ('/', @result));
    $path_to_working_dir =~ s|.*/||;
    $path_to_working_dir .= '/' unless($path_to_working_dir eq '');
}

# extension
if ($Texi2HTML::Config::SHORTEXTN)
{
    $docu_ext = "htm";
}
if ($Texi2HTML::Config::TOP_FILE =~ s/\..*$//)
{
    $Texi2HTML::Config::TOP_FILE .= ".$docu_ext";
}

$docu_doc = "$docu_name.$docu_ext"; # document's contents
if ($Texi2HTML::Config::SPLIT)
{
    # if Texi2HTML::Config::NODE_FILES is true and a node is called ${docu_name}_toc
    # ${docu_name}_ovr... there may be trouble with the old naming scheme in
    # very rare circumstances. This won't be fixed, the new scheme will be used
    # soon.
    $docu_toc   = $Texi2HTML::Config::TOC_FILE || "${docu_name}_toc.$docu_ext";
    $docu_stoc  = "${docu_name}_ovr.$docu_ext";
    $docu_foot  = "${docu_name}_fot.$docu_ext";
    $docu_about = "${docu_name}_abt.$docu_ext";
    $docu_top   = $Texi2HTML::Config::TOP_FILE || $docu_doc;
}
else
{
    if ($Texi2HTML::Config::OUT)
    {
        $docu_doc = $Texi2HTML::Config::OUT;
        $docu_doc =~ s|.*/||;
    }
    $docu_toc = $docu_foot = $docu_stoc = $docu_about = $docu_top = $docu_doc;
}

# For use in init files
$Texi2HTML::Config::TOP_FILE = $docu_top;
$Texi2HTML::Config::TOC_FILE = $docu_toc;

my $docu_doc_file = "$docu_rdir$docu_doc"; 
my $docu_toc_file  = "$docu_rdir$docu_toc";
my $docu_stoc_file = "$docu_rdir$docu_stoc";
my $docu_foot_file = "$docu_rdir$docu_foot";
my $docu_about_file = "$docu_rdir$docu_about";
my $docu_top_file  = "$docu_rdir$docu_top";

my $docu_frame_file =     "$docu_rdir${docu_name}_frame.$docu_ext";
my $docu_toc_frame_file = "$docu_rdir${docu_name}_toc_frame.$docu_ext";

#
# _foo: internal variables to track @foo
#
foreach my $key ('_author', '_title', '_subtitle', '_shorttitlepage',
	 '_settitle', '_setfilename', '_shorttitle', '_titlefont')
{
    $value{$key} = '';            # prevent -w warnings
}
my $index;                         # ref on a hash for the index entries
my %indices = ();                  # hash of indices names containing 
                                   #[ $Pages, $Entries ] (page indices and 
                                   # raw index entries)
my @index_labels = ();             # array corresponding with @?index commands
                                   # constructed during pass_texi, used to
                                   # put labels in pass_text
#
# initial counters
#
my $foot_num = 0;
my $relative_foot_num = 0;
my $idx_num = 0;
my $sec_num = 0;
my $anchor_num = 0;

#
# can I use ISO8859 characters? (HTML+)
#
if ($Texi2HTML::Config::USE_ISO)
{
    foreach my $thing (keys(%Texi2HTML::Config::iso_symbols))
    {
         $things_map_ref->{$thing} = $Texi2HTML::Config::iso_symbols{$thing};
         $pre_map_ref->{$thing} = $Texi2HTML::Config::iso_symbols{$thing};
    }
}

# process a css file
sub process_css_file ($$)
{
    my $fh =shift;
    my $file = shift;
    my $in_rules = 0;
    my $in_comment = 0;
    my $in_import = 0;
    my $in_string = 0;
    my $rules = [];
    my $imports = [];
    while (<$fh>)
    {
	    #print STDERR "Line: $_";
        if ($in_rules)
        {
            push @$rules, $_;
            next;
        }
        my $text = '';
        while (1)
        { 
		#sleep 1;
		#print STDERR "${text}!in_comment $in_comment in_rules $in_rules in_import $in_import in_string $in_string: $_";
             if ($in_comment)
             {
                 if (s/^(.*?\*\/)//)
                 {
                     $text .= $1;
                     $in_comment = 0;
                 }
                 else
                 {
                     push @$imports, $text . $_;
                     last;
                 }
             }
             elsif (!$in_string and s/^\///)
             { # what do '\' do here ?
                 if (s/^\*//)
                 {
                     $text .= '/*';
                     $in_comment = 1;
                 }
                 else
                 {
                     push (@$imports, $text. "\n") if ($text ne '');
                     push (@$rules, '/' . $_);
                     $in_rules = 1;
                     last;
                 }
             }
             elsif (!$in_string and $in_import and s/^([\"\'])//)
             { # strings outside of import start rules
                 $text .= "$1";
                 $in_string = quotemeta("$1");
             }
             elsif ($in_string and s/^(\\$in_string)//)
             {
                 $text .= $1;
             }
             elsif ($in_string and s/^($in_string)//)
             {
                 $text .= $1;
                 $in_string = 0;
             }
             elsif ((! $in_string and !$in_import) and (s/^([\\]?\@import)$// or s/^([\\]?\@import\s+)//))
             {
                 $text .= $1;
                 $in_import = 1;
             }
             elsif (!$in_string and $in_import and s/^\;//)
             {
                 $text .= ';';
                 $in_import = 0;
             }
             elsif (($in_import or $in_string) and s/^(.)//)
             {
                  $text .= $1;
             }
             elsif (!$in_import and s/^([^\s])//)
             { 
                  push (@$imports, $text. "\n") if ($text ne '');
                  push (@$rules, $1 . $_);
                  $in_rules = 1;
                  last;
             }
             elsif (s/^(\s)//)
             {
                  $text .= $1;
             }
             elsif ($_ eq '')
             {
                  push (@$imports, $text);
                  last;
             }
        } 
    }
    warn "$WARN string not closed in css file $file\n" if ($in_string);
    warn "$WARN comment not closed in css file $file\n" if ($in_comment);
    warn "$WARN \@import not finished in css file $file\n"  if ($in_import and !$in_comment and !$in_string);
    return ($imports, $rules);
}

my @css_import_lines;
my @css_rule_lines;

# process css files
foreach my $file (@Texi2HTML::Config::CSS_FILES)
{
    my $css_file_fh;
    my $css_file;
    if ($file eq '-')
    {
        $css_file_fh = \*STDIN;
        $css_file = '-';
    }
    else
    {
         $css_file = locate_init_file ($file);
         unless (defined($css_file))
         {
             warn "css file $file not found\n";
             next;
         }
         unless (open (CSSFILE, "$css_file"))
         {
             warn "Cannot open ${css_file}: $!";
             next;
        }
        $css_file_fh = \*CSSFILE;
    }
    my ($import_lines, $rules_lines);
    ($import_lines, $rules_lines) = process_css_file ($css_file_fh, $css_file);
    push @css_import_lines, @$import_lines;
    push @css_rule_lines, @$rules_lines;
}

if ($T2H_DEBUG & $DEBUG_USER)
{
    if (@css_import_lines)
    {
        print STDERR "# css import lines\n";
        foreach my $line (@css_import_lines)
        {
            print STDERR "$line";
        }
    }
    if (@css_rule_lines)
    {
        print STDERR "# css rule lines\n";
        foreach my $line (@css_rule_lines)
        {
            print STDERR "$line";
        }
    }
}

#
# read texi2html extensions (if any)
# FIXME isn't that obsolete ? (obsoleted by -init-file)
my $extensions = 'texi2html.ext';  # extensions in working directory
if (-f $extensions)
{
    print STDERR "# reading extensions from $extensions\n" if $T2H_VERBOSE;
    require($extensions);
}
my $progdir;
($progdir = $0) =~ s/[^\/]+$//;
if ($progdir && ($progdir ne './'))
{
    $extensions = "${progdir}texi2html.ext"; # extensions in texi2html directory
    if (-f $extensions)
    {
	print STDERR "# reading extensions from $extensions\n" if $T2H_VERBOSE;
	require($extensions);
    }
}

print STDERR "# reading from $docu\n" if $T2H_VERBOSE;

{

package Texi2HTML::LaTeX2HTML;

#########################################################################
#
# latex2html stuff
#
# latex2html conversions consist of three stages:
# 1) ToLatex: Put "latex" code into a latex file
# 2) ToHtml: Use latex2html to generate corresponding html code and images
# 3) FromHtml: Extract generated code and images from latex2html run
#

# init l2h defaults for files and names

# variable which shouldn't be global FIXME
use vars qw(
            %l2h_img
           );
my ($l2h_name, $l2h_latex_file, $l2h_cache_file, $l2h_html_file, $l2h_prefix);

# holds the status of latex2html operations. If 0 it means that there was 
# an error
my $status = 0;
my $debug;
my $docu_rdir;

#if ($Texi2HTML::Config::L2H)
sub init($$$)
{
    my $docu_name = shift;
    $docu_rdir = shift;
    $debug = shift;
    $l2h_name =  "${docu_name}_l2h";
    $l2h_latex_file = "$docu_rdir${l2h_name}.tex";
    $l2h_cache_file = "${docu_rdir}l2h_cache.pm";
    # destination dir -- generated images are put there, should be the same
    # as dir of enclosing html document --
    $l2h_html_file = "$docu_rdir${l2h_name}.html";
    $l2h_prefix = "${l2h_name}_";
    $status = init_to_latex();
}

##########################
#
# First stage: Generation of Latex file
# Initialize with: l2h_InitToLatex
# Add content with: l2h_ToLatex($text) --> HTML placeholder comment
# Finish with: l2h_FinishToLatex
#

my $l2h_latex_preamble = <<EOT;
% This document was automatically generated by the l2h extenstion of texi2html
% DO NOT EDIT !!!
\\documentclass{article}
\\usepackage{html}
\\begin{document}
EOT

my $l2h_latex_closing = <<EOT;
\\end{document}
EOT

my %l2h_to_latex = ();
my @l2h_to_latex = ();
my $l2h_latex_count = 0;     # number of latex texts really stored
my $l2h_to_latex_count = 0;  # total number of latex texts processed
my $l2h_cached_count = 0;    # number of cached latex text
my %l2h_cache = ();          
#$Texi2HTML::Config::L2H = l2h_InitToLatex() if ($Texi2HTML::Config::L2H);

# return used latex 1, if l2h could be initalized properly, 0 otherwise
#sub l2h_InitToLatex
sub init_to_latex()
{
    unless ($Texi2HTML::Config::L2H_SKIP)
    {
        unless (open(L2H_LATEX, ">$l2h_latex_file"))
        {
            warn "$ERROR Error l2h: Can't open latex file '$l2h_latex_file' for writing\n";
            return 0;
        }
        print STDERR "# l2h: use ${l2h_latex_file} as latex file\n" if ($T2H_VERBOSE);
        print L2H_LATEX $l2h_latex_preamble;
    }
    # open database for caching
    #l2h_InitCache();
    init_cache();
    return  1;
}


# print text (1st arg) into latex file (if not already there), return
# @tex_$number which can be later on replaced by the latex2html
# generated text
#sub l2h_ToLatex
sub to_latex
{
    my($text) = @_;
    my($count);
    $l2h_to_latex_count++;
    $text =~ s/(\s*)$//;
    # try whether we can cache it
    #my $cached_text = l2h_FromCache($text);
    my $cached_text = from_cache($text);
    if ($cached_text)
    {
        $l2h_cached_count++;
        return $cached_text;
    }
    # try whether we have text already on things to do
    unless ($count = $l2h_to_latex{$text})
    {
        $count = $l2h_latex_count;
        $l2h_latex_count++;
        $l2h_to_latex{$text} = $count;
        $l2h_to_latex[$count] = $text;
        unless ($Texi2HTML::Config::L2H_SKIP)
        {
            print L2H_LATEX "\\begin{rawhtml}\n";
            print L2H_LATEX "<!-- l2h_begin ${l2h_name} ${count} -->\n";
            print L2H_LATEX "\\end{rawhtml}\n";

            print L2H_LATEX "$text\n";

            print L2H_LATEX "\\begin{rawhtml}\n";
            print L2H_LATEX "<!-- l2h_end ${l2h_name} ${count} -->\n";
            print L2H_LATEX "\\end{rawhtml}\n";
        }
    }
    return "\@tex_${count} ";
}

# print closing into latex file and close it
#sub l2h_FinishToLatex
sub finish_to_latex()
{
    my ($reused);
    $reused = $l2h_to_latex_count - $l2h_latex_count - $l2h_cached_count;
    unless ($Texi2HTML::Config::L2H_SKIP)
    {
        print L2H_LATEX $l2h_latex_closing;
        close(L2H_LATEX);
    }
    print STDERR "# l2h: finished to latex ($l2h_cached_count cached, $reused reused, $l2h_latex_count contents)\n" if ($T2H_VERBOSE);
    unless ($l2h_latex_count)
    {
        #l2h_Finish();
        finish();
        return 0;
    }
    return 1;
}

###################################
# Second stage: Use latex2html to generate corresponding html code and images
#
# l2h_ToHtml([$l2h_latex_file, [$l2h_html_dir]]):
#   Call latex2html on $l2h_latex_file
#   Put images (prefixed with $l2h_name."_") and html file(s) in $l2h_html_dir
#   Return 1, on success
#          0, otherwise
#
#sub l2h_ToHtml
sub to_html()
{
    my ($call, $dotbug);
    if ($Texi2HTML::Config::L2H_SKIP)
    {
        print STDERR "# l2h: skipping latex2html run\n" if ($T2H_VERBOSE);
        return 1;
    }
    # Check for dot in directory where dvips will work
    if ($Texi2HTML::Config::L2H_TMP)
    {
        if ($Texi2HTML::Config::L2H_TMP =~ /\./)
        {
            warn "$ERROR Warning l2h: l2h_tmp dir contains a dot. Use /tmp, instead\n";
            $dotbug = 1;
        }
    }
    else
    {
        if (main::getcwd() =~ /\./)
        {
            warn "$ERROR Warning l2h: current dir contains a dot. Use /tmp as l2h_tmp dir \n";
            $dotbug = 1;
        }
    }
    # fix it, if necessary and hope that it works
    $Texi2HTML::Config::L2H_TMP = "/tmp" if ($dotbug);

    $call = $Texi2HTML::Config::L2H_L2H;
    # use init file, if specified
    my $init_file = main::locate_init_file($Texi2HTML::Config::L2H_FILE);
    $call = $call . " -init_file " . $init_file if ($init_file);
    # set output dir
    $call .=  ($docu_rdir ? " -dir $docu_rdir" : " -no_subdir");
    # use l2h_tmp, if specified
    $call = $call . " -tmp $Texi2HTML::Config::L2H_TMP" if ($Texi2HTML::Config::L2H_TMP);
    # use a given html version if specified
    $call = $call . " -html_version $Texi2HTML::Config::L2H_HTML_VERSION" if ($Texi2HTML::Config::L2H_HTML_VERSION);
    # options we want to be sure of
    $call = $call ." -address 0 -info 0 -split 0 -no_navigation -no_auto_link";
    $call = $call ." -prefix ${l2h_prefix} $l2h_latex_file";

    print STDERR "# l2h: executing '$call'\n" if ($Texi2HTML::Config::VERBOSE);
    if (system($call))
    {
        warn "l2h ***Error: '${call}' did not succeed\n";
        return 0;
    }
    else
    {
        print STDERR "# l2h: latex2html finished successfully\n" if ($Texi2HTML::Config::VERBOSE);
        return 1;
    }
}

##########################
# Third stage: Extract generated contents from latex2html run
# Initialize with: l2h_InitFromHtml
#   open $l2h_html_file for reading
#   reads in contents into array indexed by numbers
#   return 1,  on success -- 0, otherwise
# Extract Html code with: l2h_FromHtml($text)
#   replaces in $text all previosuly inserted comments by generated html code
#   returns (possibly changed) $text
# Finish with: l2h_FinishFromHtml
#   closes $l2h_html_dir/$l2h_name.".$docu_ext"

my $l2h_extract_error = 0;
my $l2h_range_error = 0;
my @l2h_from_html;

#sub l2h_InitFromHtml()
sub init_from_html()
{
    local(%l2h_img);
    my ($count, $h_line);

    if (! open(L2H_HTML, "<${l2h_html_file}"))
    {
        print STDERR "$ERROR Error l2h: Can't open ${l2h_html_file} for reading\n";
        return 0;
    }
    print STDERR "# l2h: use ${l2h_html_file} as html file\n" if ($T2H_VERBOSE);

    my $l2h_html_count = 0;
    while ($h_line = <L2H_HTML>)
    {
        if ($h_line =~ /^<!-- l2h_begin $l2h_name ([0-9]+) -->/)
        {
            $count = $1;
            my $h_content = "";
            while ($h_line = <L2H_HTML>)
            {
                if ($h_line =~ /^<!-- l2h_end $l2h_name $count -->/)
                {
                    chomp $h_content;
                    chomp $h_content;
                    $l2h_html_count++;
                    #$h_content = l2h_ToCache($count, $h_content);
                    $h_content = to_cache($count, $h_content);
                    $l2h_from_html[$count] = $h_content;
                    $h_content = '';
                    last;
                }
                $h_content = $h_content.$h_line;
            }
            if ($h_content)
            {
                print STDERR "$ERROR Warning l2h: l2h_end $l2h_name $count not found\n"
                    if ($Texi2HTML::Config::VERBOSE);
                close(L2H_HTML);
                return 0;
            }
        }
    }
    print STDERR "# l2h: Got $l2h_html_count of $l2h_latex_count html contents\n"
        if ($Texi2HTML::Config::VERBOSE);

    close(L2H_HTML);
    return 1;
}

sub latex2html()
{
    return unless($status);
    return unless ($status = finish_to_latex());
    return unless ($status = to_html());
    return unless ($status = init_from_html());
}

# FIXME used ??
#sub l2h_FromHtml($)
sub from_html($)
{
    my($text) = @_;
    my($done, $to_do, $count);
    $to_do = $text;
    $done = '';
    while ($to_do =~ /([^\000]*)<!-- l2h_replace $l2h_name ([0-9]+) -->([^\000]*)/)
    {
        $to_do = $1;
        $count = $2;
        $done = $3.$done;
        $done = "<!-- l2h_end $l2h_name $count -->".$done
            #if ($T2H_DEBUG & $DEBUG_L2H);
            if ($debug);

        #$done = l2h_ExtractFromHtml($count) . $done;
        $done = extract_from_html($count) . $done;

        $done = "<!-- l2h_begin $l2h_name $count -->".$done
            #if ($T2H_DEBUG & $DEBUG_L2H);
            if ($debug);
    }
    return $to_do.$done;
}

sub do_tex($)
{
    my $count = shift;
    my $result = '';
    $result = "<!-- l2h_begin $l2h_name $count -->"
            #if ($T2H_DEBUG & $DEBUG_L2H);
            if ($debug);
    $result .= extract_from_html($count);
    $result .= "<!-- l2h_end $l2h_name $count -->"
            #if ($T2H_DEBUG & $DEBUG_L2H);
            if ($debug);
    return $result;
}

#sub l2h_ExtractFromHtml($)
sub extract_from_html($)
{
    my $count = shift;
    return $l2h_from_html[$count] if ($l2h_from_html[$count]);
    if ($count >= 0 && $count < $l2h_latex_count)
    {
        # now we are in trouble
        my $line;
        $l2h_extract_error++;
        print STDERR "$ERROR l2h: can't extract content $count from html\n"
            if ($T2H_VERBOSE);
        # try simple (ordinary) substition (without l2h)
        #my $l_l2h = $Texi2HTML::Config::L2H;
        $Texi2HTML::Config::L2H = 0;
        my $l_l2h = $status;
        $status = 0;
        $line = $l2h_to_latex{$count};
        $line = main::substitute_text({}, $line);
        $line = "<!-- l2h: ". __LINE__ . " use texi2html -->" . $line
            #if ($T2H_DEBUG & $DEBUG_L2H);
            if ($debug);
        #$Texi2HTML::Config::L2H = $l_l2h;
        $status = $l_l2h;
        return $line;
    }
    else
    {
        # now we have been incorrectly called
        $l2h_range_error++;
        print STDERR "$ERROR l2h: Request of $count content which is out of valide range [0,$l2h_latex_count)\n";
        return "<!-- l2h: ". __LINE__ . " out of range count $count -->"
            #if ($T2H_DEBUG & $DEBUG_L2H);
            if ($debug);
        return "<!-- l2h: out of range count $count -->";
    }
}

#sub l2h_FinishFromHtml()
sub finish_from_html()
{
    if ($Texi2HTML::Config::VERBOSE)
    {
        if ($l2h_extract_error + $l2h_range_error)
        {
            print STDERR "# l2h: finished from html ($l2h_extract_error extract and $l2h_range_error errors)\n";
        }
        else
        {
            print STDERR "# l2h: finished from html (no errors)\n";
        }
    }
}

#sub l2h_Finish()
sub finish()
{
    return unless($status);
    finish_from_html();
    #l2h_StoreCache();
    store_cache();
    if ($Texi2HTML::Config::L2H_CLEAN)
    {
        local ($_);
        print STDERR "# l2h: removing temporary files generated by l2h extension\n"
            if $Texi2HTML::Config::VERBOSE;
        while (<"$docu_rdir$l2h_name"*>)
        {
            unlink $_;
        }
    }
    print STDERR "# l2h: Finished\n" if $Texi2HTML::Config::VERBOSE;
    return 1;
}

##############################
# stuff for l2h caching
#

# I tried doing this with a dbm data base, but it did not store all
# keys/values. Hence, I did as latex2html does it
#sub l2h_InitCache
sub init_cache
{
    if (-r "$l2h_cache_file")
    {
        my $rdo = do "$l2h_cache_file";
        warn("$ERROR l2h Error: could not load $docu_rdir$l2h_cache_file: $@\n")
            unless ($rdo);
    }
}

#sub l2h_StoreCache
sub store_cache
{
    return unless $l2h_latex_count;
    my ($key, $value);
    open(FH, ">$l2h_cache_file") || return warn"$ERROR l2h Error: could not open $docu_rdir$l2h_cache_file for writing: $!\n";
    while (($key, $value) = each %l2h_cache)
    {
        # escape stuff
        $key =~ s|/|\\/|g;
        $key =~ s|\\\\/|\\/|g;
        # weird, a \ at the end of the key results in an error
        # maybe this also broke the dbm database stuff
        $key =~ s|\\$|\\\\|;
        $value =~ s/\|/\\\|/go;
        $value =~ s/\\\\\|/\\\|/go;
        $value =~ s|\\\\|\\\\\\\\|g;
        print FH "\n\$l2h_cache_key = q/$key/;\n";
        print FH "\$l2h_cache{\$l2h_cache_key} = q|$value|;\n";
    }
    print FH "1;";
    close(FH);
}

# return cached html, if it exists for text, and if all pictures
# are there, as well
#sub l2h_FromCache
sub from_cache
{
    my $text = shift;
    my $cached = $l2h_cache{$text};
    if ($cached)
    {
        while ($cached =~ m/SRC="(.*?)"/g)
        {
            unless (-e "$docu_rdir$1")
            {
                return undef;
            }
        }
        return $cached;
    }
    return undef;
}

# insert generated html into cache, move away images,
# return transformed html
my $maximage = 1;
#sub l2h_ToCache($$)
sub to_cache($$)
{
    my $count = shift;
    my $content = shift;
    my @images = ($content =~ /SRC="(.*?)"/g);
    my ($src, $dest);

    for $src (@images)
    {
        $dest = $l2h_img{$src};
        unless ($dest)
        {
            my $ext;
            if ($src =~ /.*\.(.*)$/ && $1 ne $docu_ext)
            {
                $ext = $1;
            }
            else
            {
                warn "$ERROR: L2h image $src has invalid extension\n";
                next;
            }
            while (-e "$docu_rdir${docu_name}_$maximage.$ext")
            {
                $maximage++;
            }
            $dest = "${docu_name}_$maximage.$ext";
            system("cp -f $docu_rdir$src $docu_rdir$dest");
            $l2h_img{$src} = $dest;
            #unlink "$docu_rdir$src" unless ($T2H_DEBUG & $DEBUG_L2H);
            unlink "$docu_rdir$src" unless ($debug);
        }
        $content =~ s/$src/$dest/g;
    }
    $l2h_cache{$l2h_to_latex[$count]} = $content;
    return $content;
}

}

#+++###########################################################################
#                                                                             #
# Pass texi: read source, handle variable, ignored text,                      #
#                                                                             #
#---###########################################################################

my @fhs = ();			# hold the file handles to read
my $input_spool;		# spooled lines to read
my @lines = ();             # whole document
my @lines_numbers = ();     # line number, originating file associated with 
                            # whole document 
my $macros;                 # macros. reference on a hash
my %info_enclose;           # macros defined with definfoenclose
my $texi_line_number = { 'file_name' => '', 'line_nr' => 0, 'macro' => '' };

sub initialise_state_texi($)
{
    my $state = shift;
    $state->{'texi'} = 1;           # for substitute_text and close_stack: 
                                    # 1 if pass_texi/scan_texi is to be used
}

my @first_lines = ();

sub pass_texi()
{
    my $first_lines = 1;        # is it the first lines
    my $state = {};
                                # holds the informations about the context
                                # to pass it down to the functions
    initialise_state_texi($state);
    my @stack;
    my $text;
 INPUT_LINE: while (defined($_ = next_line($texi_line_number))) 
    {
        #
        # remove the lines preceding \input or an @-command
        # 
        if ($first_lines)
        {
            if (/^\\input/)
            {
                push @first_lines, $_;
                $first_lines = 0;
                next;
            }
            if (/^\s*\@/)
            {
                $first_lines = 0;
            }
            else
            {
                push @first_lines, $_;
                next;
            }
        }
	#print STDERR "line_nr $texi_line_number->{'line_nr'} :$_";
        my $chomped_line = $_;
        if (scan_texi ($_, \$text, \@stack, $state, $texi_line_number) and chomp($chomped_line))
        {
        #print STDERR "scan_texi line_nr $texi_line_number->{'line_nr'}\n";
            push (@lines_numbers, { 'file_name' => $texi_line_number->{'file_name'},
                  'line_nr' => $texi_line_number->{'line_nr'},
                  'macro' => $texi_line_number->{'macro'} });
        }
        
        #dump_stack (\$text, \@stack, $state);
        if ($state->{'bye'})
        {
            #dump_stack(\$text, \@stack, $state);
            close_stack(\$text, \@stack, $state, $texi_line_number);
        }
        next if (@stack);
        $_ = $text;
        $text = '';
        next if !defined($_);
        push @lines, split_lines ($_);
        last if ($state->{'bye'});
    }
    if (@stack or $state->{'macro'} or $state->{'ignored'} or $state->{'macro_name'} or $state->{'raw'})
    {
	    #dump_stack(\$text, \@stack, $state);
        close_stack(\$text, \@stack, $state, $texi_line_number);
        push @lines, split_lines ($text);
    }
    print STDERR "# end of pass texi\n" if $T2H_VERBOSE;
}

# return the line after removing things according to to_skip map.
sub skip_texi($$$)
{
    my $line = shift;
    my $macro = shift;
    my $state = shift;
    my $text;
    
    if ($macro eq 'bye')
    {
        $state->{'bye'} = 1;
        $line = "";
        $text = "\@$macro\n";
    }
    elsif ($macro eq 'end')
    {
        if ($line =~ /^\s+(\w+)/o and $Texi2HTML::Config::to_skip{"end $1"})
        {
            $line =~ s/^(\s+\w+\s*)//o;
            $text = "\@$macro" . $1;
        }
        
    }
    elsif ($Texi2HTML::Config::to_skip{$macro} eq 'arg')
    {
        $line =~ s/(\s+\S*)//o;
        $text = "\@$macro" . $1;
    }
    elsif ($Texi2HTML::Config::to_skip{$macro} eq 'line')
    {
        $text = "\@$macro" . $line;
        $line = '';
        #chomp $line;
    }
    elsif ($Texi2HTML::Config::to_skip{$macro} eq 'whitespace')
    {
        $line =~ s/(\s*)//o;
        $text = "\@$macro" . $1;
    }
    elsif ($Texi2HTML::Config::to_skip{$macro} eq 'space')
    {
        $line =~ s/([ \t]*)//o;
        $text = "\@$macro" . $1;
    }
    else
    {
        $text = "\@$macro";
    }
    #$line = undef if (!defined($line) or $line eq '');
    $line = '' if (!defined($line));
    return ($line, $text);
}

#+++###########################################################################
#                                                                             #
# Pass structure: parse document structure                                    #
#                                                                             #
#---###########################################################################

# This is a virtual element for things appearing before @node and 
# sectionning commands
my $element_before_anything =
{ 
    'before_anything' => 1,
    'place' => [],
    'texi' => 'VIRTUAL ELEMENT BEFORE ANYTHING',
};

sub initialise_state_structure($)
{
    my $state = shift;
    $state->{'structure'} = 1;      # for substitute_text and close_stack: 
                                    # 1 if pass_structure/scan_structure is 
                                    # to be used
    $state->{'menu'} = 0;           # number of opened menus
    $state->{'detailmenu'} = 0;     # number of opened detailed menus      
    $state->{'level'} = 0;          # current sectionning level
    $state->{'table_stack'} = [ "no table" ]; # a stack of opened tables/lists
    delete ($state->{'region_lines'}) unless (defined($state->{'region_lines'}));
}

my @doc_lines = ();         # whole document
my @doc_numbers = ();       # whole document line numbers and file names
my @nodes_list = ();        # nodes in document reading order
                            # each member is a reference on a hash
my @sections_list = ();     # sections in reading order
                            # each member is a reference on a hash
my @elements_list = ();     # sectionning elements (nodes and sections)
                            # in reading order. Each member is a reference
                            # on a hash which also appears in %nodes,
                            # @sections_list @nodes_list, @all_elements
my @all_elements;           # all the elements in document order
my %nodes = ();             # nodes hash. The key is the texi node name
my %cross_reference_nodes = ();  # normalized node names
my %sections = ();          # sections hash. The key is the section number
                            # headings are there, although they are not elements
my $element_top;            # Top element
my $node_top;               # Top node
my $node_first;             # First node
my $element_index;          # element with first index
my $element_chapter_index;  # chapter with first index
my $element_first;          # first element
my $element_last;           # last element

# This is a virtual element used to have the right hrefs for index entries
# and anchors in footnotes
my $footnote_element = 
{ 
    'id' => 'SEC_Foot',
    'file' => $docu_foot,
    'footnote' => 1,
    'element' => 1,
    'place' => [],
};

#my $do_contents;            # do table of contents if true
#my $do_scontents;           # do short table of contents if true
my $novalidate = $Texi2HTML::Config::NOVALIDATE; # @novalidate appeared

sub pass_structure()
{
    my $state = {};
                                # holds the informations about the context
                                # to pass it down to the functions
    initialise_state_structure($state);
    $state->{'element'} = $element_before_anything;
    $state->{'place'} = $element_before_anything->{'place'};
    my @stack;
    my $text;
    my $line_nr;

    while (@lines)
    {
        $_ = shift @lines;
        my $chomped_line = $_;
        if (!chomp($chomped_line) and @lines)
        {
             $lines[0] = $_ . $lines[0];
             next;
        }
        $line_nr = shift (@lines_numbers);
        #print STDERR "PASS_STRUCTURE: $_";
        if (!$state->{'raw'} and !$state->{'special'} and !$state->{'verb'})
        {
            my $tag = '';
            if (/^\s*\@(\w+)\b/)
            {
                $tag = $1;
            }

            #
            # analyze the tag
            #
            if ($tag and $tag eq 'node' or defined($sec2level{$tag}) or $tag eq 'printindex')
            {
                $_ = substitute_texi_line($_); #usefull if there is an anchor ???
                if (@stack and $tag eq 'node' or defined($sec2level{$tag}))
                {
                    close_stack(\$text, \@stack, $state, $line_nr);
                    if (exists($state->{'region_lines'}))
                    {
                        push @{$region_lines{$state->{'region_lines'}->{'format'}}}, split_lines ($text);
                    }
                    else
                    {
                        push @doc_lines, split_lines ($text);
                    }
                    $text = '';
                }
                if ($tag eq 'node')
                {
                    my $node_ref;
                    my $auto_directions;
                    $auto_directions = 1 unless (/,/o);
                    my ($node, $node_next, $node_prev, $node_up) = split(/,/, $_);
                    $node =~ s/^\@node\s+// if ($node);
                    if ($node)
                    {
                        $node = normalise_space($node);
                        if (exists($nodes{$node}) and defined($nodes{$node})
                             and $nodes{$node}->{'seen'})
                        {
                            echo_error ("Duplicate node found: $node", $line_nr);
                            next;
                        }
                        else
                        {
                            if (exists($nodes{$node}) and defined($nodes{$node}))
                            { # node appeared in a menu
                                $node_ref = $nodes{$node};
                            }
                            else
                            {
                                my $first;
                                $first = 1 if (!defined($node_ref));
                                $node_ref = {};
                                $node_first = $node_ref if ($first);
                                $nodes{$node} = $node_ref;
                            }
                            $node_ref->{'node'} = 1;
                            $node_ref->{'tag'} = 'node';
                            $node_ref->{'tag_level'} = 'node';
                            $node_ref->{'texi'} = $node;
                            $node_ref->{'seen'} = 1;
                            $node_ref->{'automatic_directions'} = $auto_directions;
                            $node_ref->{'place'} = [];
                            $node_ref->{'current_place'} = [];
                            merge_element_before_anything($node_ref);
                            $node_ref->{'index_names'} = [];
                            $state->{'place'} = $node_ref->{'current_place'};
                            $state->{'element'} = $node_ref;
                            $state->{'after_element'} = 1;
                            $state->{'node_ref'} = $node_ref;
                            # makeinfo treats differently case variants of
                            # top in nodes and anchors and in refs commands and 
                            # refs from nodes. 
                            if ($node =~ /^top$/i)
                            {
                                if (!defined($node_top))
                                {
                                    $node_top = $node_ref;
                                    $node_top->{'texi'} = 'Top';
                                    delete $nodes{$node};
                                    $nodes{$node_top->{'texi'}} = $node_ref;
                                }
                                else
                                { # All the refs are going to point to the first Top
                                    echo_warn ("Top node allready exists", $line_nr);
                                    #warn "$WARN Top node allready exists\n";
                                }
                            }
                            unless (@nodes_list)
                            {
                                $node_ref->{'first'} = 1;
                            }
                            push (@nodes_list, $node_ref);
                            push @elements_list, $node_ref;
                        }
                    }
                    else
                    {
                        echo_error ("Node is undefined: $_ (eg. \@node NODE-NAME, NEXT, PREVIOUS, UP)", $line_nr);
                        next;
                    }
                
                    if ($node_next)
                    {
                        $node_ref->{'node_next'} = normalise_node($node_next);
                    }
                    if ($node_prev)
                    {
                        $node_ref->{'node_prev'} = normalise_node($node_prev);
                    }
                    if ($node_up)
                    { 
                        $node_ref->{'node_up'} = normalise_node($node_up);
                    }
                }
                elsif (defined($sec2level{$tag}))
                {
                    if (/^\@$tag\s*(.*)$/)
                    {
                        my $name = normalise_space($1);
                        $name = '' if (!defined($name));
                        my $level = $sec2level{$tag};
                        $state->{'after_element'} = 1;
                        my ($docid, $num);
                        if($tag ne 'top')
                        {
                            $sec_num++;
                            $num = $sec_num;
                            $docid = "SEC$sec_num";
                        }
                        else
                        {
                            $num = 0;
                            $docid = "SEC_Top";
                        }
                        if ($tag !~ /heading/)
                        {
                            my $section_ref = { 'texi' => $name, 
                               'level' => $level,
                               'tag' => $tag,
                               'sec_num' => $num,
                               'section' => 1, 
                               'id' => $docid,
                               'index_names' => [],
                               'current_place' => [],
                               'place' => []
                            };
             
                            if ($tag eq 'top')
                            {
                                $section_ref->{'top'} = 1;
                                $section_ref->{'number'} = '';
                                $sections{0} = $section_ref;
                                $element_top = $section_ref;
                            }
                            $sections{$num} = $section_ref;
                            merge_element_before_anything($section_ref);
                            if ($state->{'node_ref'} and !exists($state->{'node_ref'}->{'with_section'}))
                            {
                                my $node_ref = $state->{'node_ref'};
                                $section_ref->{'node_ref'} = $node_ref;
                                $section_ref->{'titlefont'} = $node_ref->{'titlefont'};
                                $node_ref->{'with_section'} = $section_ref;
                                $node_ref->{'top'} = 1 if ($tag eq 'top');
                            }
                            if (! $name and $level)
                            {
                               echo_warn ("$tag without name", $line_nr);
                            }
                            push @sections_list, $section_ref;
                            push @elements_list, $section_ref;
                            $state->{'section_ref'} = $section_ref;
                            $state->{'element'} = $section_ref;
                            $state->{'place'} = $section_ref->{'current_place'};
                            my $node_ref = "NO NODE";
                            my $node_texi ='';
                            if ($state->{'node_ref'})
                            {
                                $node_ref = $state->{'node_ref'};
                                $node_texi = $state->{'node_ref'}->{'texi'};
                            }
                            print STDERR "# pass_structure node($node_ref)$node_texi, tag \@$tag($level) ref $section_ref, num,id $num,$docid\n   $name\n"
                               if $T2H_DEBUG & $DEBUG_ELEMENTS;
                        }
                        else 
                        {
                            my $section_ref = { 'texi' => $name, 
                                'level' => $level,
                                'heading' => 1,
                                'tag' => $tag,
                                'tag_level' => $tag,
                                'sec_num' => $sec_num, 
                                'id' => $docid,
                                'number' => '' };
                            $state->{'element'} = $section_ref;
                            push @{$state->{'place'}}, $section_ref;
                            $sections{$sec_num} = $section_ref;
                        }
                    }
                }
                elsif (/^\@printindex\s+(\w+)/)
                {
                    unless (@elements_list)
                    {
                        echo_warn ("Printindex before document beginning: \@printindex $1", $line_nr);
                        next;
                    }
                    $state->{'after_element'} = 0;
                    # $element_index is the first element with index
                    $element_index = $elements_list[-1] unless (defined($element_index));
                    # associate the index to the element such that the page
                    # number is right
                    my $placed_elements = [];
                    push @{$elements_list[-1]->{'index_names'}}, { 'name' => $1, 'place' => $placed_elements };
                    $state->{'place'} = $placed_elements;
                }
                if (exists($state->{'region_lines'}))
                {
                    push @{$region_lines{$state->{'region_lines'}->{'format'}}}, $_;
                }
                else
                {
                    push @doc_lines, $_;
                    push @doc_numbers, $line_nr;
                }
                next;
            }
        }
        if (scan_structure ($_, \$text, \@stack, $state, $line_nr) and !(exists($state->{'region_lines'})))
        {
            push (@doc_numbers, $line_nr);
        }
        next if (@stack);
        $_ = $text;
        $text = '';
        next if (!defined($_));
        if ($state->{'region_lines'})
        {
            push @{$region_lines{$state->{'region_lines'}->{'format'}}}, split_lines ($_);
        }
        else
        {
            push @doc_lines, split_lines ($_);
        }
    }
    if (@stack)
    {
        close_stack(\$text, \@stack, $state, $line_nr);
        push @doc_lines, split_lines ($text) if ($text and (!exists($state->{'region_lines'})));
    }
    echo_warn ("At end of document, $state->{'region_lines'}->{'number'} $state->{'region_lines'}->{'format'} not closed") if (exists($state->{'region_lines'}));
    print STDERR "# end of pass structure\n" if $T2H_VERBOSE;
}

# split line at end of line and put each resulting line in an array
sub split_lines($)
{
   my $line = shift;
   my @result = ();
   my $i = 0;
   while ($line)
   {
       $result[$i] = '';
       $line =~ s/^(.*)//;
       $result[$i] .= $1;
       $result[$i] .= "\n" if ($line =~ s/^\n//);
       #print STDERR "$i: $result[$i]";
       $i++;
   }
   return @result;
}

# return the line after removing things according to to_skip map.
# if the skipped macro has an effect it is done here
sub skip($$$)
{
    my $line = shift;
    my $macro = shift;
    my $state = shift;

    if ($macro eq 'lowersections')
    {
        my ($sec, $level);
        while (($sec, $level) = each %sec2level)
        {
            $sec2level{$sec} = $level + 1;
        }
        $state->{'level'}--;
    }
    elsif ($macro eq 'raisesections')
    {
        my ($sec, $level);
        while (($sec, $level) = each %sec2level)
        {
            $sec2level{$sec} = $level - 1;
        }
        $state->{'level'}++;
    }
    elsif ($macro eq 'contents')
    {
        #$do_contents = 1;
        $Texi2HTML::Config::DO_CONTENTS = 1;
    }
    elsif ($macro eq 'detailmenu')
    {
        $state->{'detailmenu'}++;
    }
    elsif (($macro eq 'summarycontents') or ($macro eq 'shortcontents'))
    {
        #$do_scontents = 1;
        $Texi2HTML::Config::DO_SCONTENTS = 1;
    }
    elsif ($macro eq 'novalidate')
    {
        $novalidate = 1;
    }
    
    if ($macro eq 'end')
    {
        if ($line =~ /^\s+(\w+)/o and $Texi2HTML::Config::to_skip{"end $1"})
        {
            $state->{'detailmenu'}-- if ($1 eq 'detailmenu' and $state->{'detailmenu'});
            $line =~ s/^\s+(\w+)\s*//o;
        }
    }
    elsif ($Texi2HTML::Config::to_skip{$macro} eq 'arg')
    {
        $line =~ s/\s+(\S*)//o;
    }
    elsif ($Texi2HTML::Config::to_skip{$macro} eq 'line')
    {
        $line = '';
        #chomp $line;
    }
    elsif ($Texi2HTML::Config::to_skip{$macro} eq 'space')
    {
        $line =~ s/[ \t]*//o;
    } 
    elsif ($Texi2HTML::Config::to_skip{$macro} eq 'whitespace')
    {
        $line =~ s/\s*//o;
    } 
    return $line if ($line);
    return undef;
}

# merge the things appearing before the first @node or sectionning command
# (held by element_before_anything) with the current element if not allready 
# done 
sub merge_element_before_anything($)
{
    my $element = shift;
    if (exists($element_before_anything->{'place'}))
    {
        $element->{'current_place'} = $element_before_anything->{'place'};
        $element->{'titlefont'} = $element_before_anything->{'titlefont'};
        delete $element_before_anything->{'place'};
        foreach my $placed_thing (@{$element->{'current_place'}})
        {
            $placed_thing->{'element'} = $element if (exists($placed_thing->{'element'}));
        }
    }
}

# find menu_prev, menu_up... for a node in menu
sub menu_entry_texi($$$)
{
    my $node = shift;
    my $state = shift;
    my $line_nr = shift;
    my $node_menu_ref = {};
    if (exists($nodes{$node}))
    {
        $node_menu_ref = $nodes{$node};
    }
    else
    {
        $nodes{$node} = $node_menu_ref;
        $node_menu_ref->{'texi'} = $node;
        $node_menu_ref->{'external_node'} = 1 if ($node =~ /\(.+\)/ or $novalidate);
    }
    $node_menu_ref->{'menu_node'} = 1;
    if ($state->{'node_ref'})
    {
        $node_menu_ref->{'menu_up'} = $state->{'node_ref'};
        $node_menu_ref->{'menu_up_hash'}->{$state->{'node_ref'}->{'texi'}} = 1;
    }
    else
    {
        echo_warn ("menu entry without previous node: $node", $line_nr) unless ($node =~ /\(.+\)/);
        #warn "$WARN menu entry without previous node: $node\n" unless ($node =~ /\(.+\)/);
    }
    return if ($state->{'detailmenu'});
    if ($state->{'prev_menu_node'})
    {
        $node_menu_ref->{'menu_prev'} = $state->{'prev_menu_node'};
        $state->{'prev_menu_node'}->{'menu_next'} = $node_menu_ref;
    }
    elsif ($state->{'node_ref'})
    {
        $state->{'node_ref'}->{'menu_child'} = $node_menu_ref;
    }
    $state->{'prev_menu_node'} = $node_menu_ref;
}

my %files = ();   # keys are files. This is used to avoid reusing an allready
                  # used file name
my %empty_indices = (); # value is true for an index name key if the index 
                        # is empty
my %printed_indices = (); # value is true for an index name not empty and
                          # printed
		  
# find next, prev, up, back, forward, fastback, fastforward
# find element id and file
# split index pages
# associate placed items (items which have links to them) with the right 
# file and id
# associate nodes with sections
sub rearrange_elements()
{
    @all_elements = @elements_list;
    
    print STDERR "# find sections levels and toplevel\n"
        if ($T2H_DEBUG & $DEBUG_ELEMENTS);
    
    my $toplevel = 4;
    # correct level if raisesections or lowersections overflowed
    # and find toplevel
    foreach my $element (values(%sections))
    {
        my $level = $element->{'level'};
        if ($level > $MAX_LEVEL)
        {
             $element->{'level'} = $MAX_LEVEL;
        }
        elsif ($level < $MIN_LEVEL and !$element->{'top'})
        {
             $element->{'level'} = $MIN_LEVEL;
        }
        else
        {
             $element->{'level'} = $level;
        }
        $element->{'toc_level'} = $element->{'level'};
        # This is for top
        $element->{'toc_level'} = $MIN_LEVEL if ($element->{'level'} < $MIN_LEVEL);
        # find the new tag corresponding with the level of the section
        $element->{'tag_level'} = $level2sec{$element->{'tag'}}->[$element->{'level'}] if ($element->{'tag'} !~ /heading/);
        $toplevel = $element->{'level'} if (($element->{'level'} < $toplevel) and ($element->{'level'} > 0 and ($element->{'tag'} !~ /heading/)));
        print STDERR "# section level $level: $element->{'texi'}\n" if ($T2H_DEBUG & $DEBUG_ELEMENTS);
    }
    
    print STDERR "# find sections structure, construct section numbers (toplevel=$toplevel)\n"
        if ($T2H_DEBUG & $DEBUG_ELEMENTS);
	
    my $in_appendix = 0;
    # these arrays heve an element per sectionning level. 
    my @previous_numbers = ();   # holds the number of the previous sections
                                 # at the same and upper levels
    my @previous_sections = ();  # holds the ref of the previous sections
    
    foreach my $section (@sections_list)
    {
        next if ($section->{'top'});
        print STDERR "Bug level undef for ($section) $section->{'texi'}\n" if (!defined($section->{'level'}));
        $section->{'toplevel'} = 1 if ($section->{'level'} == $toplevel);
        # undef things under that section level
        for (my $level = $section->{'level'} + 1; $level < $MAX_LEVEL + 1 ; $level++)
        {
            $previous_numbers[$level] = undef;
            $previous_sections[$level] = undef;
        }
        my $number_set;
        # find number at the current level
        if ($section->{'tag'} =~ /appendix/ and !$in_appendix)
        {
            $previous_numbers[$toplevel] = 'A';
            $in_appendix = 1;
            $number_set = 1 if ($section->{'level'} == $toplevel);
        }
        if (!defined($previous_numbers[$section->{'level'}]) and !$number_set)
        {
            if ($section->{'tag'} =~ /unnumbered/)
            {
                 $previous_numbers[$section->{'level'}] = undef;
            }
            else
            {
                $previous_numbers[$section->{'level'}] = 1;
            }
        }
        elsif ($section->{'tag'} !~ /unnumbered/ and !$number_set)
        {
            $previous_numbers[$section->{'level'}]++;
        }
        # construct the section number
        $section->{'number'} = '';

        unless ($section->{'tag'} =~ /unnumbered/)
        { 
            my $level = $section->{'level'};
            while ($level > $toplevel)
            {
                my $number = $previous_numbers[$level];
                $number = 0 if (!defined($number));
                if ($section->{'number'})
                {
                    $section->{'number'} = "$number.$section->{'number'}";
                }
                else
                {
                    $section->{'number'} = $number;
                }    
                $level--;
            }
            my $toplevel_number = $previous_numbers[$toplevel];
            $toplevel_number = 0 if (!defined($toplevel_number));
            $section->{'number'} = "$toplevel_number.$section->{'number'}";
        }
        # find the previous section
        if (defined($previous_sections[$section->{'level'}]))
        {
            my $prev_section = $previous_sections[$section->{'level'}];
            $section->{'section_prev'} = $prev_section;
            $prev_section->{'next'} = $section;
            $prev_section->{'element_next'} = $section;
        }
        # find the up section
        if ($section->{'level'} == $toplevel)
        {
            $section->{'up'} = undef;
        }
        else
        {
            my $level = $section->{'level'} - 1;
            while (!defined($previous_sections[$level]) and ($level >= 0))
            {
                 $level--;
            }
            if ($level >= 0)
            {
                $section->{'up'} = $previous_sections[$level];
                # 'child' is the first child
                $section->{'up'}->{'child'} = $section unless ($section->{'section_prev'});
            }
            else
            {
                 $section->{'up'} = undef;
            }
        }
        $previous_sections[$section->{'level'}] = $section;
        # element_up is used for reparenting in case an index page 
        # splitted a section. This is used in order to preserve the up which
        # points to the up section. See below at index pages generation.
        $section->{'element_up'} = $section->{'up'};

        my $up = "NO_UP";
        $up = $section->{'up'} if (defined($section->{'up'}));
        print STDERR "# numbering section ($section->{'level'}): $section->{'number'}: (up: $up) $section->{'texi'}\n"
            if ($T2H_DEBUG & $DEBUG_ELEMENTS);
    }

    my @node_directions = ('node_prev', 'node_next', 'node_up');
    # handle nodes 
    # the node_prev... are texinfo strings, find the associated node references
    print STDERR "# Resolve nodes directions\n" if ($T2H_DEBUG & $DEBUG_ELEMENTS);
    foreach my $node (@nodes_list)
    {
        foreach my $direction (@node_directions)
        {
            if ($node->{$direction} and !ref($node->{$direction}))
            {
                if ($nodes{$node->{$direction}} and $nodes{$node->{$direction}}->{'seen'})
                {
                     $node->{$direction} = $nodes{$node->{$direction}};
                }
                elsif (($node->{$direction} =~ /^\(.*\)/) or $novalidate)
                { # ref to an external node
                    if (exists($nodes{$node->{$direction}}))
                    {
                        $node->{$direction} = $nodes{$node->{$direction}};
                    }
                    else
                    {
                        # FIXME if {'seen'} this is a node appearing in the
                        # document and a node like `(file)node'. What to 
                        # do now ?
                        my $node_ref = { 'texi' => $node->{$direction},
                            'external_node' => 1 };
                        $nodes{$node->{$direction}} = $node_ref;
                        $node->{$direction} = $node_ref;
                    }
                }
                else
                {
                     echo_warn ("$direction `$node->{$direction}' for `$node->{'texi'}' not found");
                     delete $node->{$direction};
                }
            }
        }
    }

    # find section preceding and following top 
    my $section_before_top;   # section preceding the top node
    my $section_after_top;       # section following the top node
    if ($node_top)
    {
        my $previous_is_top = 0;
        foreach my $element (@all_elements)
        {
            if ($element eq $node_top)
            {
                $previous_is_top = 1;
                next;
            }
            if ($previous_is_top)
            {
                if ($element->{'section'})
                {
                    $section_after_top = $element;
                    last;
                }
                next;
            }
            $section_before_top = $element if ($element->{'section'});
        }
    }
    print STDERR "# section before Top: $section_before_top->{'texi'}\n" 
        if ($section_before_top and ($T2H_DEBUG & $DEBUG_ELEMENTS));
    print STDERR "# section after Top: $section_after_top->{'texi'}\n" 
         if ($section_after_top and ($T2H_DEBUG & $DEBUG_ELEMENTS));
    
    print STDERR "# Build the elements list\n" if ($T2H_DEBUG & $DEBUG_ELEMENTS);
    if (!$Texi2HTML::Config::USE_NODES)
    {
        #the only sectionning elements are sections
        @elements_list = @sections_list;
        # if there is no section we use nodes...
        if (!@elements_list)
        {
            print STDERR "# no section\n" if ($T2H_DEBUG & $DEBUG_ELEMENTS);
            @elements_list = @all_elements;
        }
        elsif (!$element_top and $node_top and !$node_top->{'with_section'})
        { # special case for the top node if it isn't associated with 
          # a section. The top node element is inserted between the 
          # $section_before_top and the $section_after_top
            $node_top->{'as_section'} = 1;
            $node_top->{'section_ref'} = $node_top;
            my @old_element_lists = @elements_list;
            @elements_list = ();
            while (@old_element_lists)
            {
                my $section = shift @old_element_lists;
                if ($section_before_top and ($section eq $section_before_top))
                {
                    push @elements_list, $section;
                    push @elements_list, $node_top;
                    last;
                }
                elsif ($section_after_top and ($section eq $section_after_top))
                {
                    push @elements_list, $node_top;
                    push @elements_list, $section;
                    last;
                }
                push @elements_list, $section;
            }
            push @elements_list, @old_element_lists;
        }
        
        foreach my $element (@elements_list)
        {
            print STDERR "# new section element $element->{'texi'}\n"
                if ($T2H_DEBUG & $DEBUG_ELEMENTS);
        }
    }
    else
    {
        # elements are sections if possible, and node if no section associated
        my @elements = ();
        while (@elements_list)
        {
            my $element = shift @elements_list;
            if ($element->{'node'})
            {
                if (!defined($element->{'with_section'}))
                {
                    $element->{'toc_level'} = $MIN_LEVEL if (!defined($element->{'toc_level'}));
                    print STDERR "# new node element ($element) $element->{'texi'}\n"
                        if ($T2H_DEBUG & $DEBUG_ELEMENTS);
                    push @elements, $element;
                }
            }
            else
            {
                print STDERR "# new section element ($element) $element->{'texi'}\n"
                    if ($T2H_DEBUG & $DEBUG_ELEMENTS);
                push @elements, $element;
            }
        }
        @elements_list = @elements;
    }
    foreach my $element (@elements_list)
    {
        $element->{'element'} = 1;
    }
    
    # nodes are attached to the section preceding them if not allready 
    # associated with a section
    print STDERR "# Find the section associated with each node\n"
        if ($T2H_DEBUG & $DEBUG_ELEMENTS);
    my $current_section = $sections_list[0];
    $current_section = $node_top if ($node_top and $node_top->{'as_section'} and !$section_before_top);
    my $current;
    foreach my $element (@all_elements)
    {
        if ($element->{'node'} and !$element->{'as_section'})
        {   
            if ($element->{'with_section'})
            { # the node is associated with a section
                $element->{'section_ref'} = $element->{'with_section'};
                push @{$element->{'section_ref'}->{'nodes'}}, $element;
            }
            elsif (defined($current_section))
            {
                $current_section = $section_after_top 
                    if ($current_section->{'node'} and $section_after_top);
                $element->{'in_top'} = 1 if ($current_section->{'top'});
                $element->{'section_ref'} = $current_section;
                # nodes are considered sub elements for the purprose of 
                # reparenting and their element_next and element_prev
                # are next and prev node associated with the same section
                $element->{'element_up'} = $current_section;
                $element->{'toc_level'} = $current_section->{'toc_level'};
                if (defined($current))
                {
                    $current->{'element_next'} = $element;
                    $element->{'element_prev'} = $current;
                }
                $current = $element;
                push @{$element->{'section_ref'}->{'nodes'}}, $element;
            }
            else
            {
                $element->{'toc_level'} = $MIN_LEVEL;
            }
        }
        else
        {
            $current = undef;
            $current_section = $element;
            if ($element->{'node'})
            { # Top node
                $element->{'toc_level'} = $MIN_LEVEL;
                push @{$element->{'section_ref'}->{'nodes'}}, $element;
            }
        }
    }
    print STDERR "# Complete nodes next prev and up based on menus and sections\n"
        if ($T2H_DEBUG & $DEBUG_ELEMENTS);
    foreach my $node (@nodes_list)
    {
        if (!$node->{'first'} and !$node->{'top'} and !$node->{'menu_up'} and ($node->{'texi'} !~ /^top$/i) and $Texi2HTML::Config::SHOW_MENU)
        {
            warn "$WARN `$node->{'texi'}' doesn't appear in menus\n";
        }

        # use values deduced from menus to complete missing up, next, prev
        # or from sectionning commands if automatic sectionning
        if ($node->{'node_up'})
        {
            $node->{'up'} = $node->{'node_up'};
        }
        elsif ($node->{'automatic_directions'} and $node->{'section_ref'} and defined($node->{'section_ref'}->{'up'}))
        {
            $node->{'up'} = get_node($node->{'section_ref'}->{'up'});
        }
        elsif ($node->{'menu_up'})
        {
            $node->{'up'} = $node->{'menu_up'};
        }

        if ($node->{'up'} and !$node->{'up'}->{'external_node'})
        {
            # We detect when the up node has no menu entry for that node, as
            # there may be infinite loops when finding following node (see below)
            unless (defined($node->{'menu_up_hash'}) and ($node->{'menu_up_hash'}->{$node->{'up'}->{'texi'}}))
            {
                print STDERR "$WARN `$node->{'up'}->{'texi'}' is up for `$node->{'texi'}', but has no menu entry for this node\n" if ($Texi2HTML::Config::SHOW_MENU);
                push @{$node->{'up_not_in_menu'}}, $node->{'up'}->{'texi'};
            }
        }

        # Find next node
        if ($node->{'node_next'})
        {
            $node->{'next'} = $node->{'node_next'};
        }
        elsif ($node->{'texi'} eq 'Top')
        { # special case as said in the texinfo manual
            $node->{'next'} = $node->{'menu_child'} if ($node->{'menu_child'});
        }
        elsif ($node->{'automatic_directions'})
        {
            if (defined($node->{'section_ref'}))
            {
                my $next;
                my $section = $node->{'section_ref'};
                if (defined($section->{'next'}))
                {
                    $next = get_node($section->{'next'})
                }
                else 
                {
                    while (defined($section->{'up'}) and !defined($section->{'next'}))
                    {
                        $section = $section->{'up'};
                    }
                    if (defined($section->{'next'}))
                    {
                        $next = get_node($section->{'next'});
                    }
                }
                $node->{'next'} = $next;
            }
        }
        if (!defined($node->{'next'}) and $node->{'menu_next'})
        {
            $node->{'next'} = $node->{'menu_next'};
        }
        # Find prev node
        if ($node->{'node_prev'})
        {
            $node->{'prev'} = $node->{'node_prev'};
        }
        elsif ($node->{'automatic_directions'})
        {
            if (defined($node->{'section_ref'}))
            {
                my $section = $node->{'section_ref'};
                if (defined($section->{'section_prev'}))
                {
                    $node->{'prev'} = get_node($section->{'section_prev'});
                }
                elsif (defined($section->{'up'}))
                {
                    $node->{'prev'} = get_node($section->{'up'});
                }
            }
        }
        # next we try menus. makeinfo don't do that
        if (!defined($node->{'prev'}) and $node->{'menu_prev'}) 
        {
            $node->{'prev'} = $node->{'menu_prev'};
        }
        # the prev node is the parent node
        elsif (!defined($node->{'prev'}) and $node->{'menu_up'})
        {
            $node->{'prev'} = $node->{'menu_up'};
        }
    
        # the following node is the node following in node reading order
        # it is thus first the child, else the next, else the next following
        # the up
        if ($node->{'menu_child'})
        {
            $node->{'following'} = $node->{'menu_child'};
        }
        elsif ($node->{'automatic_directions'} and defined($node->{'section_ref'}) and defined($node->{'section_ref'}->{'child'}))
        {
            $node->{'following'} = get_node ($node->{'section_ref'}->{'child'});
        }
        elsif (defined($node->{'next'}))
        {
            $node->{'following'} = $node->{'next'};
        }
	else
        {
            my $up = $node->{'up'};
            # in order to avoid infinite recursion in case the up node is the 
            # node itself we use the up node as following when there isn't 
            # a correct menu structure, here and also below.
            $node->{'following'} = $up if (defined($up) and grep {$_ eq $up->{'texi'}} @{$node->{'up_not_in_menu'}});
            while ((!defined($node->{'following'})) and (defined($up)))
            {
                if (($node_top) and ($up eq $node_top))
                { # if we are at Top, Top is following 
                    $node->{'following'} = $node_top;
                    $up = undef;
                }
                if (defined($up->{'next'}))
                {
                    $node->{'following'} = $up->{'next'};
                }
                elsif (defined($up->{'up'}))
                {
                    if (! grep { $_ eq $up->{'up'}->{'texi'} } @{$node->{'up_not_in_menu'}}) 
                    { 
                        $up = $up->{'up'};
                    }
                    else
                    { # in that case we can go into a infinite loop
                        $node->{'following'} = $up->{'up'};
                    }
                }
                else
                {
                    $up = undef;
                }
            }
        }
    }
    
    # find first and last elements before we split indices
    # FIXME Is it right for the last element ? Or should it be the last
    # with indices taken into account ?
    $element_first = $elements_list[0];
    print STDERR "# element first: $element_first->{'texi'}\n" if ($T2H_DEBUG & $DEBUG_ELEMENTS); 
    print STDERR "# top node: $node_top->{'texi'}\n" if (defined($node_top) and
        ($T2H_DEBUG & $DEBUG_ELEMENTS));
    # If there is no @top section no top node the first node is the top element
    $element_top = $node_top if (!defined($element_top) and defined($node_top));
    $element_top = $element_first unless (defined($element_top));
    $element_top->{'top'} = 1 if ($element_top->{'node'});
    $element_last = $elements_list[-1];
    print STDERR "# element top: $element_top->{'texi'}\n" if ($element_top and
        ($T2H_DEBUG & $DEBUG_ELEMENTS));
    
    print STDERR "# find forward and back\n" if ($T2H_DEBUG & $DEBUG_ELEMENTS);
    my $prev;
    foreach my $element (@elements_list)
    {
        # complete the up for toplevel elements
        if ($element->{'toplevel'} and !defined($element->{'up'}) and $element ne $element_top)
        {
            $element->{'up'} = $element_top;
        }
        # The childs are element which should be reparented in cas a chapter 
        # is split by an index
        push @{$element->{'element_up'}->{'childs'}}, $element if (defined($element->{'element_up'}));
        if ($prev)
        {
            $element->{'back'} = $prev;
            $prev->{'forward'} = $element;
            $prev = $element;
        }
        else
        {
            $prev = $element;
        }
        # If the element is not a node, then all the node directions are copied
        # if there is an associated node
        if (defined($element->{'node_ref'}))
        {
            $element->{'nodenext'} = $element->{'node_ref'}->{'next'};
            $element->{'nodeprev'} = $element->{'node_ref'}->{'prev'};
            $element->{'menu_next'} = $element->{'node_ref'}->{'menu_next'};
            $element->{'menu_prev'} = $element->{'node_ref'}->{'menu_prev'};
            $element->{'menu_child'} = $element->{'node_ref'}->{'menu_child'};
            $element->{'menu_up'} = $element->{'node_ref'}->{'menu_up'};
            $element->{'nodeup'} = $element->{'node_ref'}->{'up'};
            $element->{'following'} = $element->{'node_ref'}->{'following'};
        }
        elsif (! $element->{'node'})
        { # the section has no node associated. Find the node directions using 
          # sections
            if (defined($element->{'next'}))
            {
                 $element->{'nodenext'} = get_node($element->{'next'});
            }
            if (defined($element->{'section_prev'}))
            {
                 $element->{'nodeprev'} = get_node($element->{'section_prev'});
            }
            if (defined($element->{'up'}))
            {
                 $element->{'nodeup'} = get_node($element->{'up'});
            }
            if ($element->{'child'})
            {
                $element->{'following'} = get_node($element->{'child'});
            }
            elsif ($element->{'next'})
            {
                $element->{'following'} = get_node($element->{'next'});
            }
            elsif ($element->{'up'})
            {
                my $up = $element;
                while ($up->{'up'} and !$element->{'following'})
                {
                    $up = $up->{'up'};
                    if ($up->{'next_section'})
                    {
                        $element->{'following'} = get_node ($up->{'next_section'});
                    }
                }
            }
        }
        if ($element->{'node'})
        {
             $element->{'nodeup'} = $element->{'up'};
             $element->{'nodeprev'} = $element->{'prev'};
             $element->{'nodenext'} = $element->{'next'};
        }
    }

    my @new_elements = ();
    print STDERR "# preparing indices\n" if ($T2H_DEBUG & $DEBUG_ELEMENTS);

    while(@elements_list)
    {
        my $element = shift @elements_list;
        # @checked_elements are the elements included in the $element (including
        # itself) and are searched for indices
        my @checked_elements = ();
        if (!$element->{'node'} or $element->{'as_section'})
        {
            if (!$Texi2HTML::Config::USE_NODES)
            {
                foreach my $node (@{$element->{'nodes'}})
                {
                    # we update the element index, first element with index
                    # if it is a node
                    $element_index = $element if ($element_index and ($node eq $element_index));
                    push @checked_elements, $node;
                    # we push the section itself after the corresponding node
                    if (defined($element->{'node_ref'}) and ($node eq $element->{'node_ref'}))
                    {
                        push @checked_elements, $element;
                    }
                }
                if (!defined($element->{'node_ref'}) and !$element->{'node'})
                {
                    push @checked_elements, $element;
                }
                $element->{'nodes'} = []; # We reset the element nodes list
                # as the nodes may be associated below to another element if 
                # the element is split accross several other elements/pages
            }
            else
            {
                if ($element->{'node_ref'})
                {
                    push @checked_elements, $element->{'node_ref'};
                    $element_index = $element if ($element_index and ($element->{'node_ref'} eq $element_index));
                }
                push @checked_elements, $element;
                $element->{'nodes'} = [];
            }
        }
        else
        {
            push @checked_elements, $element;
        }
        my $checked_nodes = '';
        foreach my $checked (@checked_elements)
        {
            $checked_nodes .= "$checked->{'texi'}, ";
        }
        print STDERR "# Elements checked for $element->{'texi'}: $checked_nodes\n" if ($T2H_DEBUG & $DEBUG_ELEMENTS);
        # current_element is the last element holding text
        my $current_element = { 'holder' => 1, 'texi' => 'HOLDER', 
            'place' => [], 'indices' => [] };
        # back is sed to find back and forward
        my $back = $element->{'back'} if defined($element->{'back'});
        # forward is sed to find forward of the last inserted element
        my $forward = $element->{'forward'};
        my $element_next = $element->{'element_next'};
        my $index_num = 0;
        my @waiting_elements = (); # elements (nodes) not used for sectionning 
                                 # waiting to be associated with an element
        foreach my $checked_element(@checked_elements)
        {
	    if ($checked_element->{'element'})
            { # this is the element, we must add it
                push @new_elements, $checked_element;
                if ($current_element->{'holder'})
                { # no previous element added
                    push @{$checked_element->{'place'}}, @{$current_element->{'place'}};
                    foreach my $index(@{$current_element->{'indices'}})
                    {
                        push @{$checked_element->{'indices'}}, [ { 'element' => $checked_element, 'page' => $index->[0]->{'page'}, 'name' => $index->[0]->{'name'} } ] ;
                    }
                }
                else
                {  
                    if ($checked_element->{'toplevel'})
                    # there was an index_page added, this index_page is toplevel.
                    # it begun a new chapter. The element next for this 
                    # index page (current_element) is the checked_element
                    {
                        $current_element->{'element_next'} = $checked_element;
                    }
                    $current_element->{'next'} = $checked_element;
                    $current_element->{'following'} = $checked_element;
                    $checked_element->{'prev'} = $current_element;
                }
                $current_element = $checked_element;
                $checked_element->{'back'} = $back;
                $back->{'forward'} = $checked_element if (defined($back));
                $back = $checked_element;
                push @{$checked_element->{'nodes'}}, @waiting_elements;
                my $waiting_element;
                while (@waiting_elements)
                {
                    $waiting_element = shift @waiting_elements;
                    $waiting_element->{'section_ref'} = $checked_element;
                }
            }
            elsif ($current_element->{'holder'})
            {
                push @waiting_elements, $checked_element;
            }
            else
            {
                push @{$current_element->{'nodes'}}, $checked_element;
                $checked_element->{'section_ref'} = $current_element;
            }
            push @{$current_element->{'place'}}, @{$checked_element->{'current_place'}};
            foreach my $index (@{$checked_element->{'index_names'}})
            {
                print STDERR "# Index in `$checked_element->{'texi'}': $index->{'name'}. Current is `$current_element->{'texi'}'\n"
                    if ($T2H_DEBUG & $DEBUG_INDEX);
                my ($Pages, $Entries) = get_index($index->{'name'});
                if (defined($Pages))
                {
                    my @pages = @$Pages;
                    my $first_page = shift @pages;
                    # begin debug section
                    my $back_texi = 'NO_BACK';
                    $back_texi = $back->{'texi'} if (defined($back));
                    print STDERR "# New index first page (back `$back_texi', current `$current_element->{'texi'}')\n" if ($T2H_DEBUG & $DEBUG_INDEX);
                    # end debug section
                    push @{$current_element->{'indices'}}, [ {'element' => $current_element, 'page' => $first_page, 'name' => $index->{'name'} } ];
                    if (@pages)
                    {
                        if ($current_element->{'holder'})
                        { # the current element isn't a real element. 
                          # We add the real element 
                          # we are in a node of a section but the element
                          # is splitted by the index, thus we must add 
                          # a new element which will contain the text 
                          # between the beginning of the element and the index
                            push @new_elements, $checked_element;
                            print STDERR "# Add element `$element->{'texi'}' before index page\n" 
                                if ($T2H_DEBUG & $DEBUG_INDEX);
                            $checked_element->{'element'} = 1;
                            $checked_element->{'level'} = $element->{'level'};
                            $checked_element->{'toc_level'} = $element->{'toc_level'};
                            $checked_element->{'toplevel'} = $element->{'toplevel'};
                            $checked_element->{'up'} = $element->{'up'};
                            $checked_element->{'element_added'} = 1;
                            delete $checked_element->{'with_section'};
                            if ($checked_element->{'toplevel'})
                            {
                                $element->{'element_prev'}->{'element_next'} = $checked_element if (exists($element->{'element_prev'}));
                            }
                            push @{$checked_element->{'place'}}, @{$current_element->{'place'}};
                            foreach my $index(@{$current_element->{'indices'}})
                            {
                                push @{$checked_element->{'indices'}}, [ { 'element' => $checked_element, 'page' => $index->[0]->{'page'}, 'name' => $index->[0]->{'name'} } ] ;
                            }
                            push @{$checked_element->{'nodes'}}, @waiting_elements;
                            my $waiting_element;
                            while (@waiting_elements)
                            {
                                $waiting_element = shift @waiting_elements;
                                $waiting_element->{'section_ref'} = $checked_element;
                            }
                            $checked_element->{'back'} = $back;
                            $back->{'forward'} = $checked_element if (defined($back));
                            $current_element = $checked_element;
                            $back = $checked_element;
                        }
                        my $index_page;
                        while(@pages)
                        {
                            print STDERR "# New page (back `$back->{'texi'}', current `$current_element->{'texi'}')\n" if ($T2H_DEBUG & $DEBUG_INDEX);
                            $index_num++;
                            my $page = shift @pages;
                            $index_page = { 'index_page' => 1,
                             'texi' => "$element->{'texi'} index $index->{'name'} page $index_num",
                             'level' => $element->{'level'},
                             'tag' => $element->{'tag'},
                             'tag_level' => $element->{'tag_level'},
                             'toplevel' => $element->{'toplevel'},
                             'up' => $element->{'up'},
                             'element_up' => $element->{'element_up'},
                             'element_next' => $element_next,
                             'element_ref' => $element,
                             'back' => $back,
                             'prev' => $back,
                             'next' => $current_element->{'next'},
                             'following' => $current_element->{'following'},
                             'nodeup' => $current_element->{'nodeup'},
                             'nodenext' => $current_element->{'nodenext'},
                             'nodeprev' => $back,
                             'place' => [],
                             'page' => $page
                            };
                            $index_page->{'node'} = 1 if ($element->{'node'});
                            while ($nodes{$index_page->{'texi'}})
                            {
                                $nodes{$index_page->{'texi'}} .= ' ';
                            }
                            $nodes{$index_page->{'texi'}} = $index_page;
                            push @{$current_element->{'indices'}->[-1]}, {'element' => $index_page, 'page' => $page, 'name' => $index->{'name'} };
                            push @new_elements, $index_page;
                            $back->{'forward'} = $index_page;
                            $back->{'next'} = $index_page;
                            $back->{'nodenext'} = $index_page;
                            $back->{'element_next'} = $index_page unless ($back->{'top'});
                            $back->{'following'} = $index_page;
                            $back = $index_page;
                            $index_page->{'toplevel'} = 1 if ($element->{'top'});
                        }
                        $current_element = $index_page;
                    }
                }
                else
                {
                    print STDERR "# Empty index: $index->{'name'}\n" 
                        if ($T2H_DEBUG & $DEBUG_INDEX);
                    $empty_indices{$index->{'name'}} = 1;
                }
                push @{$current_element->{'place'}}, @{$index->{'place'}};
            }
        }
        if ($forward and ($current_element ne $element))
        {
            $current_element->{'forward'} = $forward;
            $forward->{'back'} = $current_element;
        }
        next if ($current_element eq $element or !$current_element->{'toplevel'});
        # reparent the elements below $element, following element
        # and following parent of element to the last index page
        print STDERR "# Reparent `$element->{'texi'}':\n" if ($T2H_DEBUG & $DEBUG_INDEX);
        my @reparented_elements = ();
        @reparented_elements = (@{$element->{'childs'}}) if (defined($element->{'childs'}));
        push @reparented_elements, $element->{'element_next'} if (defined($element->{'element_next'}));
        foreach my $reparented(@reparented_elements)
        {
            next if ($reparented->{'toplevel'});
            $reparented->{'element_up'} = $current_element;
	    print STDERR "   reparented: $reparented->{'texi'}\n"
                    if ($T2H_DEBUG & $DEBUG_INDEX);
        }
    }
    @elements_list = @new_elements;
    
    print STDERR "# find fastback and fastforward\n" 
       if ($T2H_DEBUG & $DEBUG_ELEMENTS);
    foreach my $element (@elements_list)
    {
        my $up = get_top($element);
        next unless (defined($up));
        $element_chapter_index = $up if ($element_index and ($element_index eq $element));
	#print STDERR "$element->{'texi'} (top: $element->{'top'}, toplevel: $element->{'toplevel'}, $element->{'element_up'}, $element->{'element_up'}->{'texi'}): up: $up, $up->{'texi'}\n";
        # fastforward is the next element on same level than the upper parent
        # element
        $element->{'fastforward'} = $up->{'element_next'} if (exists ($up->{'element_next'}));
        # if the element isn't at the highest level, fastback is the 
        # highest parent element
        if ($up and ($up ne $element))
        {
            $element->{'fastback'} = $up;
        }
        elsif ($element->{'toplevel'})
        {
            # the element is a top level element, we adjust the next
            # toplevel element fastback
            $element->{'fastforward'}->{'fastback'} = $element if ($element->{'fastforward'});
        }
    }
    my $index_nr = 0;
    # convert directions in direction with first letter in all caps, to be
    # consistent with the convention used in the .init file.
    # find id for nodes and indices
    foreach my $element (@elements_list)
    {
        $element->{'this'} = $element;
        foreach my $direction (('Up', 'Forward', 'Back', 'Next', 
            'Prev', 'FastForward', 'FastBack', 'This', 'NodeUp', 
            'NodePrev', 'NodeNext', 'Following' ))
        {
            my $direction_no_caps = $direction;
            $direction_no_caps =~ tr/A-Z/a-z/;
            $element->{$direction} = $element->{$direction_no_caps};
        }
        if ($element->{'index_page'})
        {
            $element->{'id'} = "INDEX" . $index_nr;
            $index_nr++;
        }
    }
    my $node_nr = 1;
    foreach my $node (@nodes_list)
    {
        $node->{'id'} = 'NOD' . $node_nr;
        $node_nr++;
        # debug check
        print STDERR "Bug: level defined for node `$node->{'texi'}'\n" if (defined($node->{'level'}) and !$node->{'element_added'});
    }

    # Find cross manual links as explained on the texinfo mailing list
    cross_manual_links(\%nodes, \%cross_reference_nodes);

    # Find node file names
    if ($Texi2HTML::Config::NODE_FILES)
    {
        my $top;
        if ($node_top)
        {
            $top = $node_top;
        }
        elsif ($element_top->{'node_ref'})
        {
            $top = $element_top->{'node_ref'};
        }
        else
        {
            $top = $node_first;
        }
        if ($top)
        {
            my $file = "$Texi2HTML::Config::TOP_NODE_FILE.$Texi2HTML::Config::NODE_FILE_EXTENSION";
            $top->{'file'} = $file if ($Texi2HTML::Config::SPLIT eq 'node');
            $top->{'node_file'} = $file;
        }
        foreach my $key (keys(%nodes))
        {
            my $node = $nodes{$key};
            next if ($node->{'external_node'} or $node->{'index_page'});
            if (defined($Texi2HTML::Config::node_file_name))
            {
                 ($node->{'file'}, $node->{'node_file'}) =
                      &$Texi2HTML::Config::node_file_name ($node);
            }
            else
            {
                 next if (defined($node->{'file'}));
                 my $name = remove_texi($node->{'texi'});
                 $name =~ s/[^\w\.\-]/-/g;
                 my $file = "${name}.$Texi2HTML::Config::NODE_FILE_EXTENSION";
                 $node->{'file'} = $file if (($Texi2HTML::Config::SPLIT eq 'node') and ($Texi2HTML::Config::USE_NODES or $node->{'with_section'}));
                 $node->{'node_file'} = $file;
            }
        }
    }
    # find document nr and document file for sections and nodes. 
    # Split according to Texi2HTML::Config::SPLIT.
    # find file and id for placed elements (anchors, index entries, headings)
    if ($Texi2HTML::Config::SPLIT)
    {
        my $cut_section = $toplevel;
        my $doc_nr = -1;
        if ($Texi2HTML::Config::SPLIT eq 'section')
        {
            $cut_section = 2 if ($toplevel <= 2);
        }
        my $top_doc_nr;
        my $prev_nr;
        foreach my $element (@elements_list)
        {
            print STDERR "# Splitting ($Texi2HTML::Config::SPLIT) $element->{'texi'}\n" if ($T2H_DEBUG & $DEBUG_ELEMENTS);
            $doc_nr++ if (
               ($Texi2HTML::Config::SPLIT eq 'node') or
               (
                 (!$element->{'node'} or $element->{'element_added'}) and ($element->{'level'} <= $cut_section)
               )
              );
            $doc_nr = 0 if ($doc_nr < 0); # happens if first elements are nodes
            $element->{'doc_nr'} = $doc_nr;
            if ($Texi2HTML::Config::NODE_FILES and ($Texi2HTML::Config::SPLIT eq 'node'))
            {
                my $node = get_node($element);
                if ($node and $node->{'file'})
                {
                    $element->{'file'} = $node->{'file'};
                }
                unless ($element->{'file'})
                {
                    $element->{'file'} = "${docu_name}_$doc_nr.$docu_ext";
                    $element->{'doc_nr'} = $doc_nr;
                }
            }
            else
            {
                $element->{'file'} = "${docu_name}_$doc_nr.$docu_ext";
                my $is_top = 0;
                if (defined($top_doc_nr))
                {
                    if ($doc_nr eq $top_doc_nr)
                    {
                        $element->{'file'} = "$docu_top";
                        if ($element->{'level'} # this is an element below @top.
                                               # It starts a new file.
                          or ($element->{'node'} and ($element ne $node_top) and (!defined($element->{'section_ref'}) or $element->{'section_ref'} ne $element_top))
                          )# this is a node not associated with top
                        {
                            $doc_nr++;
                            $element->{'doc_nr'} = $doc_nr;
                            $element->{'file'} = "${docu_name}_$doc_nr.$docu_ext";
                        }
                    }
                }
                elsif ($element eq $element_top or (defined($element->{'section_ref'}) and $element->{'section_ref'} eq $element_top) or (defined($element->{'node_ref'}) and !$element->{'node_ref'}->{'element_added'} and $element->{'node_ref'} eq $element_top))
                { # the top element
                    $is_top = 1;
                    $element->{'file'} = "$docu_top";
                    # if there is a previous element, we force it to be in 
                    # another file than top
                    $doc_nr++ if (defined($prev_nr) and $doc_nr == $prev_nr);
                    $top_doc_nr = $doc_nr;
                    $element->{'doc_nr'} = $doc_nr;
                }
                if (defined($Texi2HTML::Config::element_file_name))
                {
                     $element->{'file'} = 
                         &$Texi2HTML::Config::element_file_name ($element, $is_top, $docu_name);
                }
            }
            add_file($element->{'file'});
            $prev_nr = $doc_nr;
            foreach my $place(@{$element->{'place'}})
            {
                $place->{'file'} = $element->{'file'};
                $place->{'id'} = $element->{'id'} unless defined($place->{'id'});
            }
            if ($element->{'nodes'})
            {
                foreach my $node (@{$element->{'nodes'}})
                {
                    $node->{'doc_nr'} = $element->{'doc_nr'};
                    $node->{'file'} = $element->{'file'};
                }
            }
            elsif ($element->{'node_ref'} and !$element->{'node_ref'}->{'element_added'})
            {
                $element->{'node_ref'}->{'doc_nr'} = $element->{'doc_nr'} ;
                $element->{'node_ref'}->{'file'} = $element->{'file'};
            }
        }
    }
    else
    {
        foreach my $element(@elements_list)
        {
            #die "$ERROR monolithic file and a node have the same file name $docu_doc\n" if ($Texi2HTML::Config::NODE_FILES and $files{$docu_doc});
            $element->{'file'} =  "$docu_doc";
            $element->{'doc_nr'} = 0;
            foreach my $place(@{$element->{'place'}})
            {
                $place->{'file'} = "$element->{'file'}";
                $place->{'id'} = $element->{'id'} unless defined($place->{'id'});
            }
        }
        foreach my $node(@nodes_list)
        {
            $node->{'file'} =  "$docu_doc";
            $node->{'doc_nr'} = 0;
        }
    }
    # correct the id and file for the things placed in footnotes
    foreach my $place(@{$footnote_element->{'place'}})
    {
        $place->{'file'} = $footnote_element->{'file'};
        $place->{'id'} = $footnote_element->{'id'} unless defined($place->{'id'});
    }
    foreach my $file (keys(%files))
    {
        last unless ($T2H_DEBUG & $DEBUG_ELEMENTS);
        print STDERR "$file: $files{$file}->{'counter'}\n";
    }
    foreach my $element ((@elements_list, $footnote_element))
    {
        last unless ($T2H_DEBUG & $DEBUG_ELEMENTS);
        my $is_toplevel = 'not top';
        $is_toplevel = 'top' if ($element->{'toplevel'});
        print STDERR "$element ";
        if ($element->{'index_page'})
        {
            print STDERR "index($element->{'id'}, $is_toplevel, doc_nr $element->{'doc_nr'}($element->{'file'})): $element->{'texi'}\n";
        }
        elsif ($element->{'node'})
        {
            my $added = '';
            $added = 'added, ' if ($element->{'element_added'});
            print STDERR "node($element->{'id'}, toc_level $element->{'toc_level'}, $is_toplevel, ${added}doc_nr $element->{'doc_nr'}($element->{'file'})) $element->{'texi'}:\n";
            print STDERR "  section_ref: $element->{'section_ref'}->{'texi'}\n" if (defined($element->{'section_ref'}));
        }
        elsif ($element->{'footnote'})
        {
            print STDERR "footnotes($element->{'id'}, file $element->{'file'})\n";
        }
        else 
        {
            my $number = "UNNUMBERED";
            $number = $element->{'number'} if ($element->{'number'});
            print STDERR "$number ($element->{'id'}, $is_toplevel, level $element->{'level'}-$element->{'toc_level'}, doc_nr $element->{'doc_nr'}($element->{'file'})) $element->{'texi'}:\n";
            print STDERR "  node_ref: $element->{'node_ref'}->{'texi'}\n" if (defined($element->{'node_ref'}));
        }

if (!$element->{'footnote'})
{
    die if (!defined($files{$element->{'file'}})) ;
    print STDERR "$element->{'file'}: $files{$element->{'file'}}, counter $files{$element->{'file'}}->{'counter'}\n";
}
        print STDERR "  TOP($toplevel) " if ($element->{'top'});
        print STDERR "  u: $element->{'up'}->{'texi'}\n" if (defined($element->{'up'}));
        print STDERR "  ch: $element->{'child'}->{'texi'}\n" if (defined($element->{'child'}));
        print STDERR "  fb: $element->{'fastback'}->{'texi'}\n" if (defined($element->{'fastback'}));
        print STDERR "  b: $element->{'back'}->{'texi'}\n" if (defined($element->{'back'}));
        print STDERR "  p: $element->{'prev'}->{'texi'}\n" if (defined($element->{'prev'}));
        print STDERR "  n: $element->{'next'}->{'texi'}\n" if (defined($element->{'next'}));
        print STDERR "  n_u: $element->{'nodeup'}->{'texi'}\n" if (defined($element->{'nodeup'}));
        print STDERR "  f: $element->{'forward'}->{'texi'}\n" if (defined($element->{'forward'}));
        print STDERR "  follow: $element->{'following'}->{'texi'}\n" if (defined($element->{'following'}));
	print STDERR "  m_p: $element->{'menu_prev'}->{'texi'}\n" if (defined($element->{'menu_prev'}));
	print STDERR "  m_n: $element->{'menu_next'}->{'texi'}\n" if (defined($element->{'menu_next'}));
	print STDERR "  m_u: $element->{'menu_up'}->{'texi'}\n" if (defined($element->{'menu_up'}));
	print STDERR "  m_ch: $element->{'menu_child'}->{'texi'}\n" if (defined($element->{'menu_child'}));
	print STDERR "  u_e: $element->{'element_up'}->{'texi'}\n" if (defined($element->{'element_up'}));
	print STDERR "  n_e: $element->{'element_next'}->{'texi'}\n" if (defined($element->{'element_next'}));
        print STDERR "  ff: $element->{'fastforward'}->{'texi'}\n" if (defined($element->{'fastforward'}));
        if (defined($element->{'menu_up_hash'}))
        {
            print STDERR "  parent nodes:\n";
            foreach my $menu_up (keys%{$element->{'menu_up_hash'}})
            {
                print STDERR "   $menu_up ($element->{'menu_up_hash'}->{$menu_up})\n";
            }
        }
        if (defined($element->{'nodes'}))
        {
            print STDERR "  nodes: $element->{'nodes'} (@{$element->{'nodes'}})\n";
            foreach my $node (@{$element->{'nodes'}})
            {
                my $beginning = "   ";
                $beginning = "  *" if ($node->{'with_section'});
                my $file = $node->{'file'};
                $file = "file undef" if (! defined($node->{'file'}));
                print STDERR "${beginning}$node->{'texi'} $file\n";
            }
        }
        print STDERR "  places: $element->{'place'}\n";
        foreach my $place(@{$element->{'place'}})
        {
            if ($place->{'entry'})
            {
                print STDERR "    index($place): $place->{'entry'}\n";
            }
            elsif ($place->{'anchor'})
            {
                print STDERR "    anchor: $place->{'texi'}\n";
            }
            else
            {
                print STDERR "    heading: $place->{'texi'}\n";
            }
        }
        if ($element->{'indices'})
        {
            print STDERR "  indices: $element->{'indices'}\n";
            foreach my $index(@{$element->{'indices'}})
            {
                print STDERR "    $index: ";
                foreach my $page (@$index)
                {
                    print STDERR "'$page->{'element'}->{'texi'}'($page->{'name'}): $page->{'page'} ";
                }
                print STDERR "\n";
            }
        }
    }
}

sub add_file($)
{
    my  $file = shift;
    if ($files{$file})
    {
         $files{$file}->{'counter'}++;
    }
    else
    {
         $files{$file} = { 
           #'type' => 'section', 
           'counter' => 1
         };
    }
}

# find parent element which is a top element, or a node within the top section
sub get_top($)
{
   my $element = shift;
   my $up = $element;
   while (!$up->{'toplevel'} and !$up->{'top'})
   {
       $up = $up->{'element_up'};
       if (!defined($up))
       {
           # If there is no section, it is normal not to have toplevel element,
           # and it is also the case if there is a low level element before
           # a top level element
           print STDERR "$WARN no toplevel for $element->{'texi'} (could be normal)\n" if (@sections_list);
           return undef;
       }
   }
   return $up;
}

sub get_node($)
{
    my $element = shift;
    return undef if (!defined($element));
    return $element if ($element->{'node'});
    return $element->{'node_ref'} if ($element->{'node_ref'} and !$element->{'node_ref'}->{'element_added'});
    return $element;
}
# get the html names from the texi for all elements
sub do_names()
{
    # for nodes and anchors we haven't any state defined
    # This seems right, however, as we don't want @refs or @footnotes
    # or @anchors within nodes, section commands or anchors.
    foreach my $node (%nodes)
    {
        next if ($nodes{$node}->{'index_page'}); # some nodes are index pages.
        $nodes{$node}->{'text'} = substitute_line ($nodes{$node}->{'texi'});
        $nodes{$node}->{'name'} = $nodes{$node}->{'text'};
        $nodes{$node}->{'no_texi'} = &$Texi2HTML::Config::protect_text(remove_texi($nodes{$node}->{'texi'}));
        if ($nodes{$node}->{'external_node'} and !$nodes{$node}->{'seen'})
        {
            $nodes{$node}->{'file'} = do_external_ref($node);
        }
    }
    foreach my $number (keys(%sections))
    {
        my $section = $sections{$number};
        $section->{'name'} = substitute_line ($section->{'texi'});
        $section->{'text'} = $section->{'number'} . " " . $section->{'name'};
        $section->{'text'} =~ s/^\s*//;
        $section->{'no_texi'} = &$Texi2HTML::Config::protect_text($section->{'number'} . " " .remove_texi($section->{'texi'}));
        $section->{'no_texi'} =~ s/^\s*//;
    }
    my $tocnr = 1;
    foreach my $element (@elements_list)
    {
        if (!$element->{'top'} and !$element->{'index_page'})
        {
            $element->{'tocid'} = 'TOC' . $tocnr;
            $tocnr++;
        }
        next if (defined($element->{'text'}));
        if ($element->{'index_page'})
        {
            my $page = $element->{'page'};
            my $sec_name = $element->{'element_ref'}->{'text'};
            $element->{'text'} = ($page->{First} ne $page->{Last} ?
                "$sec_name: $page->{First} -- $page->{Last}" :
                "$sec_name: $page->{First}");
            $sec_name = $element->{'element_ref'}->{'no_texi'};
	    #$sec_name = $element->{'element_ref'}->{'number'} . " " .$sec_name if (defined($element->{'element_ref'}->{'number'}));
	    #$sec_name =~ s/^\s*//;
            $element->{'no_texi'} = &$Texi2HTML::Config::protect_text($page->{First} ne $page->{Last} ?
                "$sec_name: $page->{First} -- $page->{Last}" :
                "$sec_name: $page->{First}");
        }
    }
}

@{$Texi2HTML::TOC_LINES} = ();            # table of contents
@{$Texi2HTML::OVERVIEW} = ();           # short table of contents



#+++############################################################################
#                                                                              #
# Stuff related to Index generation                                            #
#                                                                              #
#---############################################################################

# FIXME what to do with index entries appearing in @copying 
# @documentdescription and @titlepage
sub enter_index_entry($$$$$$)
{
    my $prefix = shift;
    my $line_nr = shift;
    my $key = shift;
    my $place = shift;
    my $element = shift;
    my $use_section_id = shift;
    unless (exists ($index_properties->{$prefix}))
    {
        echo_error ("Undefined index command: ${prefix}index", $line_nr);
        #warn "$ERROR Undefined index command: ${prefix}index\n";
        return 0;
    }
    if (!exists($element->{'tag'}) and !$element->{'footnote'})
    {
        echo_warn ("Index entry before document: \@${prefix}index $key", $line_nr); 
    }
    $key =~ s/\s+$//;
    $key =~ s/^\s*//;
    my $entry = $key;
    # The $key is mostly usefull for alphabetical sorting
    $key = remove_texi($key);
    return if ($key =~ /^\s*$/);
    while (exists $index->{$prefix}->{$key})
    {
        $key .= ' ';
    }
    my $id = '';
    unless ($use_section_id)
    {
        $id = 'IDX' . ++$idx_num;
    }
    $index->{$prefix}->{$key}->{'entry'} = $entry;
    $index->{$prefix}->{$key}->{'element'} = $element;
    $index->{$prefix}->{$key}->{'label'} = $id;
    $index->{$prefix}->{$key}->{'prefix'} = $prefix;
    push @$place, $index->{$prefix}->{$key};
    print STDERR "# enter ${prefix}index '$key' with id $id ($index->{$prefix}->{$key})\n"
        if ($T2H_DEBUG & $DEBUG_INDEX);
    push @index_labels, $index->{$prefix}->{$key};
    return $index->{$prefix}->{$key};
}

# returns prefix of @?index command associated with 2 letters prefix name
# for example returns 'c' for 'cp'
sub index_name2prefix
{
    my $name = shift;
    my $prefix;

    for $prefix (keys %$index_properties)
    {
        return $prefix if ($index_properties->{$prefix}->{'name'} eq $name);
    }
    return undef;
}

# get all the entries (for all the prefixes) in the $normal and $code 
# references, formatted with @code{code } if it is a $code entry.
sub get_index_entries($$)
{
    my $normal = shift;
    my $code = shift;
    my $entries = {};
    foreach my $prefix (keys %$normal)
    {
        for my $key (keys %{$index->{$prefix}})
        {
            $entries->{$key} = $index->{$prefix}->{$key};
        }
    }

    if (defined($code))
    {
        foreach my $prefix (keys %$code)
        {
            unless (exists $normal->{$prefix})
            {
                foreach my $key (keys %{$index->{$prefix}})
                {
                    $entries->{$key} = $index->{$prefix}->{$key};
                    # use @code for code style index entry
                    $entries->{$key}->{'entry'} = "\@code{$entries->{$key}->{entry}}";
                }
            }
        }
    }
    return $entries;
}

# sort according to cmp if both $a and $b are alphabetical or non alphabetical, 
# otherwise the alphabetical is ranked first
sub by_alpha
{
    if ($a =~ /^[A-Za-z]/)
    {
        if ($b =~ /^[A-Za-z]/)
        {
            return lc($a) cmp lc($b);
        }
        else
        {
            return 1;
        }
    }
    elsif ($b =~ /^[A-Za-z]/)
    {
        return -1;
    }
    else
    {
        return lc($a) cmp lc($b);
    }
}

# returns an array of index entries pages splitted by letters
# each page has the following members:
# {First}            first letter on that page
# {Last}             last letter on that page
# {Letters}          ref on an array with all the letters for that page
# {EntriesByLetter}  ref on a hash. Each key is a letter, with value
#                    a ref on arrays of index entries begining with this letter
sub get_index_pages($)
{
    my $entries = shift;
    my (@Letters);
    my ($EntriesByLetter, $Pages, $page) = ({}, [], {});
    my @keys = sort by_alpha keys %$entries;

    # each index entry is placed according to its first letter in
    # EntriesByLetter
    for my $key (@keys)
    {
        push @{$EntriesByLetter->{uc(substr($key,0, 1))}} , $entries->{$key};
    }
    @Letters = sort by_alpha keys %$EntriesByLetter;
    $Texi2HTML::Config::SPLIT_INDEX = 0 unless $Texi2HTML::Config::SPLIT;

    if ($Texi2HTML::Config::SPLIT_INDEX and $Texi2HTML::Config::SPLIT_INDEX =~ /^\d+$/)
    {
        my $i = 0;
        my ($prev_letter);
        for my $letter (@Letters)
        {
            if ($i > $Texi2HTML::Config::SPLIT_INDEX)
            {
                $page->{Last} = $prev_letter;
                push @$Pages, $page;
                $i=0;
            }
	    if ($i == 0)
	    {
		$page = {};
		$page->{Letters} = [];
		$page->{EntriesByLetter} = {};
		$page->{First} = $letter;
	    }
            push @{$page->{Letters}}, $letter;
            $page->{EntriesByLetter}->{$letter} = [@{$EntriesByLetter->{$letter}}];
            $i += scalar(@{$EntriesByLetter->{$letter}});
            $prev_letter = $letter;
        }
        $page->{Last} = $Letters[$#Letters];
        push @$Pages, $page;
    }
    else
    {
        warn "$WARN Bad Texi2HTML::Config::SPLIT_INDEX: $Texi2HTML::Config::SPLIT_INDEX\n" if ($Texi2HTML::Config::SPLIT_INDEX);
        $page->{First} = $Letters[0];
        $page->{Last} = $Letters[$#Letters];
        $page->{Letters} = \@Letters;
        $page->{EntriesByLetter} = $EntriesByLetter;
        push @$Pages, $page;
        return $Pages;
    }
    return $Pages;
}

sub get_index($;$)
{
    my $name = shift;
    my $line_nr = shift;
    return (@{$indices{$name}}) if ($indices{$name});
    my $prefix = index_name2prefix($name);
    unless ($prefix)
    {
        echo_error ("Bad index name: $name", $line_nr);
        #warn "$ERROR Bad index name: $name\n";
        return;
    }
    if ($index_properties->{$prefix}->{code})
    {
        $index_properties->{$prefix}->{from_code}->{$prefix} = 1;
    }
    else
    {
        $index_properties->{$prefix}->{from}->{$prefix}= 1;
    }

    my $Entries = get_index_entries($index_properties->{$prefix}->{from},
                                  $index_properties->{$prefix}->{from_code});
    return unless %$Entries;
    my $Pages = get_index_pages($Entries);
    $indices{$name} = [ $Pages, $Entries ];
    return ($Pages, $Entries);
}

my @foot_lines = ();           # footnotes
my $copying_comment = '';      # comment constructed from text between
                               # @copying and @end copying with licence
my $from_encoding;             # texinfo file encoding
my $to_encoding;               # out file encoding

sub initialise_state($)
{
    my $state = shift;
    $state->{'preformatted'} = 0 unless exists($state->{'preformatted'}); 
    $state->{'code_style'} = 0 unless exists($state->{'code_style'}); 
    $state->{'keep_texi'} = 0 unless exists($state->{'keep_texi'});
    $state->{'keep_nr'} = 0 unless exists($state->{'keep_nr'});
    $state->{'format_stack'} = [ {'format' => "noformat"} ] unless exists($state->{'format_stack'});
    $state->{'paragraph_style'} = [ '' ] unless exists($state->{'paragraph_style'}); 
    $state->{'preformatted_stack'} = [ '' ] unless exists($state->{'preformatted_stack'}); 
    $state->{'menu'} = 0 unless exists($state->{'menu'}); 
    $state->{'style_stack'} = [] unless exists($state->{'style_stack'});
    # if there is no $state->{'element'} the first element is used
    $state->{'element'} = $elements_list[0] unless (exists($state->{'element'}) and !$state->{'element'}->{'before_anything'});
}

sub pass_text()
{
    my %state;
    initialise_state (\%state);
    my @stack;
    my $text;
    my $doc_nr;
    my $in_doc = 0;
    my $element;
    my @text =();
    my @section_lines = ();
    my @head_lines = ();
    my $one_section = 1 if (@elements_list == 1);

    if (@elements_list == 0)
    {
        warn "$WARN empty document\n";
        exit (0);
    }

    # We set titlefont only if the titlefont appeared in the top element
    if (defined($element_top->{'titlefont'}))
    {
         $element_top->{'has_heading'} = 1;
         $value{'_titlefont'} = $element_top->{'titlefont'};
    }
    
    # prepare %Texi2HTML::THISDOC
    # FIXME 0 should be valid
    $Texi2HTML::THISDOC{'fulltitle'} = substitute_line($value{'_title'}) || substitute_line($value{'_settitle'}) || substitute_line($value{'_shorttitlepage'}) || substitute_line($value{'_titlefont'});
    $Texi2HTML::THISDOC{'title'} = substitute_line($value{'_settitle'}) || $Texi2HTML::THISDOC{'fulltitle'};
    $Texi2HTML::THISDOC{'shorttitle'} =  substitute_line($value{'_shorttitle'});

    # find Top name
    my $element_top_text = '';
    if ($element_top and $element_top->{'text'} and (!$node_top or ($element_top ne $node_top)))
    {
        $element_top_text = $element_top->{'text'};
    }
    my $top_name = $Texi2HTML::Config::TOP_HEADING || $element_top_text || $Texi2HTML::THISDOC{'title'} || $Texi2HTML::THISDOC{'shorttitle'} || &$I('Top');

    $Texi2HTML::THISDOC{'fulltitle'} = $Texi2HTML::THISDOC{'fulltitle'} || &$I('Untitled Document') ;
    $Texi2HTML::THISDOC{'title'} = $Texi2HTML::THISDOC{'settitle'} || $Texi2HTML::THISDOC{'fulltitle'};
    $Texi2HTML::THISDOC{'author'} = substitute_line($value{'_author'});
    $Texi2HTML::THISDOC{'titlefont'} = substitute_line($value{'_titlefont'});
    $Texi2HTML::THISDOC{'subtitle'} = substitute_line($value{'_subtitle'});

    $Texi2HTML::THISDOC{'title_texi'} = $value{'_title'} || $value{'_settitle'} || $value{'_shorttitlepage'} || $value{'_titlefont'};
    $Texi2HTML::THISDOC{'title_no_texi'} = &$Texi2HTML::Config::protect_text(remove_texi($value{'_title'})) || &$Texi2HTML::Config::protect_text(remove_texi($value{'_settitle'})) || &$Texi2HTML::Config::protect_text(remove_texi($value{'_shorttitlepage'})) || &$Texi2HTML::Config::protect_text(remove_texi($value{'_titlefont'}));
    $Texi2HTML::THISDOC{'shorttitle_no_texi'} =  &$Texi2HTML::Config::protect_text(remove_texi($value{'_shorttitle'}));

    my $top_no_texi = '';
    if ($element_top and $element_top->{'no_texi'}  and (!$node_top or ($element_top ne $node_top)))
    {
        $top_no_texi = $element_top->{'no_texi'};
    }

    $top_no_texi = $Texi2HTML::Config::TOP_HEADING || $top_no_texi || $Texi2HTML::THISDOC{'title_no_texi'} || $Texi2HTML::THISDOC{'shorttitle_no_texi'} || &$I('Top');
    $Texi2HTML::THISDOC{'title_no_texi'} = $Texi2HTML::THISDOC{'title_no_texi'} || &$I('Untitled Document');

    for my $key (keys %Texi2HTML::THISDOC)
    {
        next if (ref($Texi2HTML::THISDOC{$key}));
        $Texi2HTML::THISDOC{$key} =~ s/\s*$//;
    }
    $Texi2HTML::THISDOC{'program'} = $THISPROG;
    $Texi2HTML::THISDOC{'program_homepage'} = $T2H_HOMEPAGE;
    $Texi2HTML::THISDOC{'program_authors'} = $T2H_AUTHORS;
    $Texi2HTML::THISDOC{'user'} = $T2H_USER;
    $Texi2HTML::THISDOC{'today'} = $T2H_TODAY;
#    $Texi2HTML::THISDOC{'documentdescription'} = $documentdescription;
    $Texi2HTML::THISDOC{'copying'} = $copying_comment;
    $Texi2HTML::THISDOC{'toc_file'} = ''; 
    $Texi2HTML::THISDOC{'toc_file'} = $docu_toc if ($Texi2HTML::Config::SPLIT); 
    $Texi2HTML::THISDOC{'file_base_name'} = $docu_name;
    $Texi2HTML::THISDOC{'destination_directory'} = $docu_rdir;
    $Texi2HTML::THISDOC{'authors'} = [] if (!defined($Texi2HTML::THISDOC{'authors'}));
    $Texi2HTML::THISDOC{'subtitles'} = [] if (!defined($Texi2HTML::THISDOC{'subtitles'}));
    $Texi2HTML::THISDOC{'titles'} = [] if (!defined($Texi2HTML::THISDOC{'titles'}));
    foreach my $element (('authors', 'subtitles', 'titles'))
    {
        my $i;
        for ($i = 0; $i < $#{$Texi2HTML::THISDOC{$element}} + 1; $i++) 
        {
            chomp ($Texi2HTML::THISDOC{$element}->[$i]);
            $Texi2HTML::THISDOC{$element}->[$i] = substitute_line($Texi2HTML::THISDOC{$element}->[$i]);
            #print STDERR "$element:$i: $Texi2HTML::THISDOC{$element}->[$i]\n";
        }
    }
    # prepare TOC, OVERVIEW
    if ($Texi2HTML::Config::SPLIT)
    {
        $Texi2HTML::HREF{'Contents'} = $docu_toc.'#SEC_Contents' if @{$Texi2HTML::TOC_LINES};
        $Texi2HTML::HREF{'Overview'} = $docu_stoc.'#SEC_Overview' if @{$Texi2HTML::OVERVIEW};
        $Texi2HTML::HREF{'Footnotes'} = $docu_foot. '#SEC_Foot';
        $Texi2HTML::HREF{'About'} = $docu_about . '#SEC_About' unless $one_section;
    }
    else
    {
        $Texi2HTML::HREF{'Contents'} = '#SEC_Contents' if @{$Texi2HTML::TOC_LINES};
        $Texi2HTML::HREF{'Overview'} = '#SEC_Overview' if @{$Texi2HTML::OVERVIEW};
        $Texi2HTML::HREF{'Footnotes'} = '#SEC_Foot';
        $Texi2HTML::HREF{'About'} = '#SEC_About' unless $one_section;
    }
    
    %Texi2HTML::NAME =
        (
         'First',   $element_first->{'text'},
         'Last',    $element_last->{'text'},
         'About',    &$I('About This Document'),
         'Contents', &$I('Table of Contents'),
         'Overview', &$I('Short Table of Contents'),
         'Top',      $top_name,
         'Footnotes', &$I('Footnotes'),
        );
    $Texi2HTML::NAME{'Index'} = $element_chapter_index->{'text'} if (defined($element_chapter_index));
    $Texi2HTML::NAME{'Index'} = $Texi2HTML::Config::INDEX_CHAPTER if ($Texi2HTML::Config::INDEX_CHAPTER ne '');
    
    %Texi2HTML::NO_TEXI =
        (
         'First',   $element_first->{'no_texi'},
         'Last',    $element_last->{'no_texi'},
         'About',    &$I('About This Document'),
         'Contents', &$I('Table of Contents'),
         'Overview', &$I('Short Table of Contents'),
         'Top',      $top_no_texi,
         'Footnotes', &$I('Footnotes'),
        );
    $Texi2HTML::NO_TEXI{'Index'} = $element_chapter_index->{'no_texi'} if (defined($element_chapter_index));
    $Texi2HTML::TITLEPAGE = '';
    $Texi2HTML::TITLEPAGE = substitute_text({}, @{$region_lines{'titlepage'}})
        if (@{$region_lines{'titlepage'}});
    &$Texi2HTML::Config::titlepage();

    $to_encoding = &$Texi2HTML::Config::init_out();

    ############################################################################
    # print frame and frame toc file
    #
    if ( $Texi2HTML::Config::FRAMES )
    {
        #open(FILE, "> $docu_frame_file")
        #    || die "$ERROR: Can't open $docu_frame_file for writing: $!\n";
        my $FH = open_out($docu_frame_file);
        print STDERR "# Creating frame in $docu_frame_file ...\n" if $T2H_VERBOSE;
        &$Texi2HTML::Config::print_frame($FH, $docu_toc_frame_file, $docu_top_file);
        close_out($FH, $docu_frame_file);

        #open(FILE, "> $docu_toc_frame_file")
        #    || die "$ERROR: Can't open $docu_toc_frame_file for writing: $!\n";
        $FH = open_out($docu_toc_frame_file);
        print STDERR "# Creating toc frame in $docu_frame_file ...\n" if $T2H_VERBOSE;
        #&$Texi2HTML::Config::print_toc_frame(\*FILE, $Texi2HTML::OVERVIEW);
        &$Texi2HTML::Config::print_toc_frame($FH, $Texi2HTML::OVERVIEW);
        #close(FILE);
        close_out($FH, $docu_toc_frame_file);
    }

    ############################################################################
    #
    #

    my $FH;
    my $index_pages;
    my $index_pages_nr;
    my $index_nr = 0;
    my $line_nr;
    my $first_section = 0; # 1 if it is the first section of a page
    while (@doc_lines)
    {
        unless ($index_pages)
        { # not in a index split over sections
            $_ = shift @doc_lines;
            my $chomped_line = $_;
            if (!chomp($chomped_line) and @doc_lines)
            { # if the line has no end of line it is concatenated with the next
                 $doc_lines[0] = $_ . $doc_lines[0];
                 next;
            }
            $line_nr = shift (@doc_numbers);
            #print STDERR "$line_nr->{'file_name'}($line_nr->{'macro'},$line_nr->{'line_nr'}) $_" if ($line_nr);
        }
        #print STDERR "PASS_TEXT: $_";
        #dump_stack(\$text, \@stack, \%state);
        if (!$state{'raw'} and !$state{'verb'})
        {
            my $tag = '';
            $tag = $1 if (/^\@(\w+)/ and !$index_pages);

            if (($tag eq 'node') or defined($sec2level{$tag}) or $index_pages)
            {
                if (@stack)
                {
                    close_stack(\$text, \@stack, \%state, $line_nr);
                    push @section_lines, $text;
                    $text = '';
                }
                $sec_num++ if ($sec2level{$tag});
                my $new_element;
                my $current_element;
                if ($tag =~ /heading/)
                {# handle headings, they are not in element lists
                    $current_element = $sections{$sec_num};
                    #print STDERR "HEADING $_";
                    if (! $element)
                    {
                        $new_element = shift @elements_list;
                        $element->{'has_heading'} = 1 if ($new_element->{'top'});
                    }
                    else
                    {
                        if ($element->{'top'})
                        {
                            $element->{'has_heading'} = 1;
                        }
                        push (@section_lines, &$Texi2HTML::Config::anchor($current_element->{'id'}) . "\n");
                        push @section_lines, &$Texi2HTML::Config::heading($current_element);
                        next;
                    }
                }
                elsif (!$index_pages)
                {# handle node and structuring elements
                    $current_element = shift (@all_elements);
                    #begin debug section
                    if ($current_element->{'node'})
                    {
                         print STDERR 'NODE ' . "$current_element->{'texi'}($current_element->{'file'})" if ($T2H_DEBUG & $DEBUG_ELEMENTS);
                         print STDERR "($current_element->{'section_ref'}->{'texi'})" if ($current_element->{'section_ref'} and ($T2H_DEBUG & $DEBUG_ELEMENTS));
                    }
                    else
                    {
                         print STDERR 'SECTION ' . $current_element->{'texi'} if ($T2H_DEBUG & $DEBUG_ELEMENTS);
                    }
                    print STDERR ": $_" if ($T2H_DEBUG & $DEBUG_ELEMENTS);
                    #end debug section

                    # The element begins a new section if there is no previous
                    # or it is an element and not the current one or the 
                    # associated section (in case of node) is not the current one
                    if (!$element 
                      or ($current_element->{'element'} and ($current_element ne $element))
                      or ($current_element->{'section_ref'} and ($current_element->{'section_ref'} ne $element)))
                    {
                         $new_element = shift @elements_list;
                    }
                    # begin debugging section
                    my $section_element = $new_element;
                    $section_element = $element unless ($section_element);
                    if (!$current_element->{'node'} and !$current_element->{'index_page'} and ($section_element ne $current_element))
                    {
                         print STDERR "NODE: $element->{'texi'}\n" if ($element->{'node'});
                         warn "elements_list and all_elements not in sync (elements $section_element->{'texi'}, all $current_element->{'texi'}): $_";
                    }
                    # end debugging section
                }
                else
                { # this is a new index section
                    $new_element = $index_pages->[$index_pages_nr]->{'element'};
                    $current_element = $index_pages->[$index_pages_nr]->{'element'};
                    print STDERR "New index page '$new_element->{'texi'}' nr: $index_pages_nr\n" if ($T2H_DEBUG & $DEBUG_ELEMENTS);
                    my $list_element = shift @elements_list;
                    die "element in index_pages $new_element->{'texi'} and in list $list_element->{'texi'} differs\n" unless ($list_element eq $new_element);
                }
                if ($new_element)
                {
                    $index_nr = 0;
                    my $old = 'NO_OLD';
                    $old = $element->{'texi'} if (defined($element));
                    print STDERR "NEW: $new_element->{'texi'}, OLD: $old\n" if ($T2H_DEBUG & $DEBUG_ELEMENTS);
# FIXME this should be done differently now that there could be elements
# associated with the same file
                    if ($element and ($new_element->{'doc_nr'} != $element->{'doc_nr'}) and @foot_lines and !$Texi2HTML::Config::SEPARATED_FOOTNOTES)
                    { # note that this can only happen if $Texi2HTML::Config::SPLIT
                        &$Texi2HTML::Config::foot_section (\@foot_lines);
                        push @section_lines, @foot_lines;
                        @foot_lines = ();
                        $relative_foot_num = 0;
                    }
                    # print the element that just finished
                    $Texi2HTML::THIS_SECTION = \@section_lines;
                    $Texi2HTML::THIS_HEADER = \@head_lines;
                    if ($element)
                    {
                        #$FH = finish_element($FH, $element, $new_element, $first_section);
                        finish_element($FH, $element, $new_element, $first_section);
                        $first_section = 0;
                        @section_lines = ();
                        @head_lines = ();
                    }
                    else
                    {
                        print STDERR "# Writing elements:" if ($T2H_VERBOSE);
                        if ($Texi2HTML::Config::IGNORE_PREAMBLE_TEXT)
                        {
                             @section_lines = ();
                             @head_lines = ();
                        }
                        # remove empty line at the beginning of @section_lines
                        shift @section_lines while (@section_lines and ($section_lines[0] =~ /^\s*$/));
                    }
                    # begin new element
                    my $previous_file;
                    $previous_file = $element->{'file'} if (defined($element));
                    $element = $new_element;
                    $state{'element'} = $element;
                    $Texi2HTML::THIS_ELEMENT = $element;
                    #print STDERR "Doing hrefs for $element->{'texi'} First ";
                    $Texi2HTML::HREF{'First'} = href($element_first, $element->{'file'});
                    #print STDERR "Last ";
                    $Texi2HTML::HREF{'Last'} = href($element_last, $element->{'file'});
                    #print STDERR "Index ";
                    $Texi2HTML::HREF{'Index'} = href($element_chapter_index, $element->{'file'}) if (defined($element_chapter_index));
                    #print STDERR "Top ";
                    $Texi2HTML::HREF{'Top'} = href($element_top, $element->{'file'});
                    foreach my $direction (('Up', 'Forward', 'Back', 'Next', 
                        'Prev', 'FastForward', 'FastBack', 'This', 'NodeUp', 
                        'NodePrev', 'NodeNext', 'Following' ))
                    {
                        my $elem = $element->{$direction};
                        $Texi2HTML::NODE{$direction} = undef;
                        $Texi2HTML::HREF{$direction} = undef;
                        next unless (defined($elem));
                        #print STDERR "$direction ";
                        if ($elem->{'node'} or $elem->{'external_node'} or $elem->{'index_page'})
                        {
                            $Texi2HTML::NODE{$direction} = $elem->{'text'};
                        }
                        elsif ($elem->{'node_ref'})
                        {
                            $Texi2HTML::NODE{$direction} = $elem->{'node_ref'}->{'text'};
                        }
                        if ($elem->{'menu_node'} and ! $elem->{'seen'})
                        {
                            $Texi2HTML::HREF{$direction} = '';
                        }
                        elsif ($elem->{'external_node'})
                        {
                            $Texi2HTML::HREF{$direction} = $elem->{'file'};
                        }
                        else
                        {
                            $Texi2HTML::HREF{$direction} = href($elem, $element->{'file'});
                        }
                        $Texi2HTML::NAME{$direction} = $elem->{'text'};
                        $Texi2HTML::NO_TEXI{$direction} = $elem->{'no_texi'};
                    }
                    #print STDERR "\nDone hrefs for $element->{'texi'}\n";
                    $files{$element->{'file'}}->{'counter'}--;
                    #if (! defined($FH))
                    if (!defined($previous_file) or ($element->{'file'} ne $previous_file))
                    {
                        my $file = $element->{'file'};
                        print STDERR "\n" if ($T2H_VERBOSE and !$T2H_DEBUG);
                        print STDERR "# Writing to $docu_rdir$file " if $T2H_VERBOSE;
                        my $do_page_head = 0;
                        if ($files{$file}->{'filehandle'})
                        {
                             $FH = $files{$file}->{'filehandle'};
                        }
                        else
                        {
                             $FH = open_out("$docu_rdir$file");
                             $files{$file}->{'filehandle'} = $FH;
                             $do_page_head = 1;
                        }
                        if ($element->{'top'})
                        {
                             &$Texi2HTML::Config::print_Top_header($FH, $do_page_head);
                        }
                        else
                        {
                             &$Texi2HTML::Config::print_page_head($FH) if ($do_page_head);
                             &$Texi2HTML::Config::print_chapter_header($FH) if $Texi2HTML::Config::SPLIT eq 'chapter';
                             &$Texi2HTML::Config::print_section_header($FH) if $Texi2HTML::Config::SPLIT eq 'section';
                        }
                        $first_section = 1;
                    }
                    print STDERR "." if ($T2H_VERBOSE);
                    print STDERR "\n" if ($T2H_DEBUG);
                }
                my $label = &$Texi2HTML::Config::anchor($current_element->{'id'}) . "\n";
                if (@section_lines)
                {
                    push (@section_lines, $label);
                }
                else
                {
                    push @head_lines, $label;
                }
                if ($index_pages)
                {
                    push @section_lines, &$Texi2HTML::Config::heading($element);
		    #print STDERR "Do index page $index_pages_nr\n";
                    my $page = do_index_page($index_pages, $index_pages_nr);
                    push @section_lines, $page;
                    if (defined ($index_pages->[$index_pages_nr + 1]))
                    {
                        $index_pages_nr++;
                    }
                    else
                    {
                        $index_pages = undef;
                    }
                    next;
                }
                push @section_lines, &$Texi2HTML::Config::heading($current_element) if ($current_element->{'element'} and !$current_element->{'top'});
                next;
            }
            elsif ($tag eq 'printindex')
            {
                s/\s+(\w+)\s*//;
                my $name = $1;
                close_stack(\$text, \@stack, \%state, $line_nr, '');
                close_paragraph (\$text, \@stack, \%state);
                next if (!index_name2prefix($name) or $empty_indices{$name});
                $printed_indices{$name} = 1;
                print STDERR "print index $name($index_nr) in `$element->{'texi'}', element->{'indices'}: $element->{'indices'},\n" if ($T2H_DEBUG & $DEBUG_ELEMENTS or $T2H_DEBUG & $DEBUG_INDEX);
                print STDERR "element->{'indices'}->[index_nr]: $element->{'indices'}->[$index_nr] (@{$element->{'indices'}->[$index_nr]})\n" if ($T2H_DEBUG & $DEBUG_ELEMENTS or $T2H_DEBUG & $DEBUG_INDEX);
                $index_pages = $element->{'indices'}->[$index_nr] if (@{$element->{'indices'}->[$index_nr]} > 1);
                $index_pages_nr = 0;
                add_prev(\$text, \@stack, do_index_page($element->{'indices'}->[$index_nr], 0));  
                $index_pages_nr++;
                $index_nr++;
                begin_paragraph (\@stack, \%state) if ($state{'preformatted'});
                next if (@stack);
                push @section_lines, $text;
                $text = '';
                next;
            }
            elsif ($tag eq 'contents')
            {
                next;
            }
        }
        scan_line ($_, \$text, \@stack, \%state, $line_nr);
        next if (@stack);
        push @section_lines, $text;
        $text = '';
    }
    if (@stack)
    {
        close_stack(\$text, \@stack, \%state, $line_nr);
        push @section_lines, $text;
    }
    print STDERR "\n" if ($T2H_VERBOSE);
    $Texi2HTML::THIS_SECTION = \@section_lines;
    # if no sections, then simply print document as is
    if ($one_section)
    {
        if (@foot_lines)
        {
            &$Texi2HTML::Config::foot_section (\@foot_lines);
            push @section_lines, @foot_lines;
        }
        $Texi2HTML::THIS_HEADER = \@head_lines;
        if ($element->{'top'})
        {
            print STDERR "Bug: `$element->{'texi'}' level undef\n" if (!$element->{'node'} and !defined($element->{'level'}));
            $element->{'level'} = 1 if (!defined($element->{'level'}));
            $element->{'node'} = 0; # otherwise Texi2HTML::Config::heading may uses the node level
            $element->{'text'} = $Texi2HTML::NAME{'Top'};
            print STDERR "[Top]" if ($T2H_VERBOSE);
            unless ($element->{'has_heading'})
            {
                unshift @section_lines, &$Texi2HTML::Config::heading($element);
            }
        }
        print STDERR "# Write the section $element->{'texi'}\n" if ($T2H_VERBOSE);
        &$Texi2HTML::Config::one_section($FH);
        close_out($FH);
        return;
    }

    finish_element ($FH, $element, undef, $first_section);

    ############################################################################
    # Print ToC, Overview, Footnotes
    #
    for my $direction (('Prev', 'Next', 'Back', 'Forward', 'Up', 'NodeUp', 
        'NodePrev', 'NodeNext', 'Following', 'This'))
    {
        delete $Texi2HTML::HREF{$direction};
        # it is better to undef in case the references to these hash entries
        # are used, as if deleted, the
        # references are still refering to the old, undeleted element
        # (we could do both)
        $Texi2HTML::NAME{$direction} = undef;
        $Texi2HTML::NO_TEXI{$direction} = undef;
        $Texi2HTML::NODE{$direction} = undef;

        $Texi2HTML::THIS_ELEMENT = undef;
    }
    if (@foot_lines)
    {
        print STDERR "# writing Footnotes in $docu_foot_file\n" if $T2H_VERBOSE;
        #open (FILE, "> $docu_foot_file") || die "$ERROR: Can't open $docu_foot_file for writing: $!\n"
        $FH = open_out ($docu_foot_file)
            if $Texi2HTML::Config::SPLIT;
        $Texi2HTML::HREF{'This'} = $Texi2HTML::HREF{'Footnotes'};
        $Texi2HTML::HREF{'Footnotes'} = '#' . $footnote_element->{'id'};
        $Texi2HTML::NAME{'This'} = $Texi2HTML::NAME{'Footnotes'};
        $Texi2HTML::NO_TEXI{'This'} = $Texi2HTML::NO_TEXI{'Footnotes'};
        $Texi2HTML::THIS_SECTION = \@foot_lines;
        $Texi2HTML::THIS_HEADER = [ &$Texi2HTML::Config::anchor($footnote_element->{'id'}) . "\n" ];
        #&$Texi2HTML::Config::print_Footnotes(\*FILE);
        &$Texi2HTML::Config::print_Footnotes($FH);
        #close(FILE) if $Texi2HTML::Config::SPLIT;
        close_out($FH, $docu_foot_file) 
            #|| die "$ERROR: Error occurred when closing $docu_foot_file: $!\n"
               if ($Texi2HTML::Config::SPLIT);
        $Texi2HTML::HREF{'Footnotes'} = $Texi2HTML::HREF{'This'};
    }

    if (@{$Texi2HTML::TOC_LINES})
    {
        print STDERR "# writing Toc in $docu_toc_file\n" if $T2H_VERBOSE;
        #open (FILE, "> $docu_toc_file") || die "$ERROR: Can't open $docu_toc_file for writing: $!\n"
        $FH = open_out ($docu_toc_file)
            if $Texi2HTML::Config::SPLIT;
        $Texi2HTML::HREF{'This'} = $Texi2HTML::HREF{'Contents'};
        $Texi2HTML::HREF{'Contents'} = "#SEC_Contents";
        $Texi2HTML::NAME{'This'} = $Texi2HTML::NAME{'Contents'};
        $Texi2HTML::NO_TEXI{'This'} = $Texi2HTML::NO_TEXI{'Contents'};
        $Texi2HTML::THIS_SECTION = $Texi2HTML::TOC_LINES;
        $Texi2HTML::THIS_HEADER = [ &$Texi2HTML::Config::anchor("SEC_Contents") . "\n" ];
        #&$Texi2HTML::Config::print_Toc(\*FILE);
        #close(FILE) if $Texi2HTML::Config::SPLIT;
        &$Texi2HTML::Config::print_Toc($FH);
        close_out($FH, $docu_toc_file) 
        #|| die "$ERROR: Error occurred when closing $docu_toc_file: $!\n"
               if ($Texi2HTML::Config::SPLIT);
        $Texi2HTML::HREF{'Contents'} = $Texi2HTML::HREF{'This'};
    }

    if (@{$Texi2HTML::OVERVIEW})
    {
        print STDERR "# writing Overview in $docu_stoc_file\n" if $T2H_VERBOSE;
        #open (FILE, "> $docu_stoc_file") || die "$ERROR: Can't open $docu_stoc_file for writing: $!\n"
        $FH = open_out ($docu_stoc_file)
            if $Texi2HTML::Config::SPLIT;
        $Texi2HTML::HREF{This} = $Texi2HTML::HREF{Overview};
        $Texi2HTML::HREF{Overview} = "#SEC_Overview";
        $Texi2HTML::NAME{This} = $Texi2HTML::NAME{Overview};
        $Texi2HTML::NO_TEXI{This} = $Texi2HTML::NO_TEXI{Overview};
        $Texi2HTML::THIS_SECTION = $Texi2HTML::OVERVIEW;
        $Texi2HTML::THIS_HEADER = [ &$Texi2HTML::Config::anchor("SEC_Overview") . "\n" ];
        #&$Texi2HTML::Config::print_Overview(\*FILE);
        #close(FILE) if $Texi2HTML::Config::SPLIT;
        &$Texi2HTML::Config::print_Overview($FH);
        close_out($FH,$docu_stoc_file) 
         #|| die "$ERROR: Error occurred when closing $docu_stoc_file: $!\n"
               if ($Texi2HTML::Config::SPLIT);
        $Texi2HTML::HREF{Overview} = $Texi2HTML::HREF{This};
    }
    my $about_body;
    if ($about_body = &$Texi2HTML::Config::about_body())
    {
        print STDERR "# writing About in $docu_about_file\n" if $T2H_VERBOSE;
        #open (FILE, "> $docu_about_file") || die "$ERROR: Can't open $docu_about_file for writing: $!\n"
        $FH = open_out ($docu_about_file)
            if $Texi2HTML::Config::SPLIT;

        $Texi2HTML::HREF{This} = $Texi2HTML::HREF{About};
        $Texi2HTML::HREF{About} = "#SEC_About";
        $Texi2HTML::NAME{This} = $Texi2HTML::NAME{About};
        $Texi2HTML::NO_TEXI{This} = $Texi2HTML::NO_TEXI{About};
        $Texi2HTML::THIS_SECTION = [$about_body];
        $Texi2HTML::THIS_HEADER = [ &$Texi2HTML::Config::anchor("SEC_About") . "\n" ];
        #&$Texi2HTML::Config::print_About(\*FILE);
        #close(FILE) if $Texi2HTML::Config::SPLIT;
        &$Texi2HTML::Config::print_About($FH);
        close_out($FH, $docu_stoc_file) 
           #|| die "$ERROR: Error occurred when closing $docu_stoc_file: $!\n"
               if ($Texi2HTML::Config::SPLIT);
        $Texi2HTML::HREF{About} = $Texi2HTML::HREF{This};
    }

    unless ($Texi2HTML::Config::SPLIT)
    {
        &$Texi2HTML::Config::print_page_foot($FH);
        close_out ($FH);
          # || die "$ERROR: Error occurred when closing: $!\n";
    }
}

# print section, close file and undef FH if needed.
sub finish_element($$$$)
{
    my $FH = shift;
    my $element = shift;
    my $new_element = shift;
    my $first_section = shift;
#print STDERR "FINISH_ELEMENT($FH)($element->{'texi'})[$element->{'file'}] counter $files{$element->{'file'}}->{'counter'}\n";
    if ($element->{'top'})
    {
        my $top_file = $docu_top_file;
        #print STDERR "TOP $element->{'texi'}, @section_lines\n";
        print STDERR "[Top]" if ($T2H_VERBOSE);
        $Texi2HTML::HREF{'Top'} = href($element_top, $element->{'file'});
        &$Texi2HTML::Config::print_Top($FH, $element->{'has_heading'});
        my $end_page = 0;
        if ($Texi2HTML::Config::SPLIT)
        {
            if (!$files{$element->{'file'}}->{'counter'})
            {
                $end_page = 1;
            }
        }
        &$Texi2HTML::Config::print_Top_footer($FH, $end_page);
        close_out($FH, $top_file) if ($end_page);
    }
    else
    {
        print STDERR "# do element $element->{'texi'}\n"
           if ($T2H_DEBUG & $DEBUG_ELEMENTS);
        &$Texi2HTML::Config::print_section($FH, $first_section);
        if (defined($new_element) and ($new_element->{'file'} ne $element->{'file'}))
        {
             if (!$files{$element->{'file'}}->{'counter'})
             {
                 &$Texi2HTML::Config::print_chapter_footer($FH) if ($Texi2HTML::Config::SPLIT eq 'chapter');
                 &$Texi2HTML::Config::print_section_footer($FH) if ($Texi2HTML::Config::SPLIT eq 'section');
                 #print STDERR "Close file after $element->{'texi'}\n";
                 &$Texi2HTML::Config::print_page_foot($FH);
                 close_out($FH);
             }
             else
             {
                 print STDERR "counter $files{$element->{'file'}}->{'counter'} ne 0, file $element->{'file'}\n";
             }
             undef $FH;
        }
        elsif (!defined($new_element))
        {
            if ($Texi2HTML::Config::SPLIT)
            { # end of last splitted section
                &$Texi2HTML::Config::print_chapter_footer($FH) if ($Texi2HTML::Config::SPLIT eq 'chapter');
                &$Texi2HTML::Config::print_section_footer($FH) if ($Texi2HTML::Config::SPLIT eq 'section');
                &$Texi2HTML::Config::print_page_foot($FH);
                close_out($FH);
            }
            else
            {
                &$Texi2HTML::Config::end_section($FH, 1);
            }
        }
        elsif ($new_element->{'top'})
        {
            &$Texi2HTML::Config::end_section($FH, 1);
        }
        else
        {
            &$Texi2HTML::Config::end_section($FH);
        }
    }
    return $FH;
}

# write to files with name the node name for cross manual references.
sub do_node_files()
{
    foreach my $key (keys(%nodes))
    {
        my $node = $nodes{$key};
        next unless ($node->{'node_file'});
        my $redirection_file = $docu_doc;
        $redirection_file = $node->{'file'} if ($Texi2HTML::Config::SPLIT);
        if (!$redirection_file)
        {
             print STDERR "Bug: file for redirection for `$node->{'texi'}' don't exist\n";
             next;
        }
        next if ($redirection_file eq $node->{'node_file'});
        my $file = "${docu_rdir}$node->{'node_file'}";
        $Texi2HTML::NODE{'This'} = $node->{'text'};
        $Texi2HTML::NO_TEXI{'This'} = $node->{'no_texi'};
        $Texi2HTML::NAME{'This'} = $node->{'text'};
        $Texi2HTML::HREF{'This'} = "$node->{'file'}#$node->{'id'}";
        open (NODEFILE, "> $file") || die "$ERROR Can't open $file for writing: $!\n";
        &$Texi2HTML::Config::print_redirection_page (\*NODEFILE);
        close NODEFILE || die "$ERROR: Can't close $file: $!\n";
    }
}

#+++############################################################################
#                                                                              #
# Low level functions                                                          #
#                                                                              #
#---############################################################################

sub locate_include_file($)
{
    my $file = shift;

    # APA: Don't implicitely search ., to conform with the docs!
    # return $file if (-e $file && -r $file);
    foreach my $dir (@Texi2HTML::Config::INCLUDE_DIRS)
    {
        return "$dir/$file" if (-e "$dir/$file" && -r "$dir/$file");
    }
    return undef;
}

sub open_file($$)
{
    my $name = shift;
    my $line_number = shift;
    local *FH;
    if ((defined($from_encoding) and open(*FH, ":encoding($from_encoding)", $name)) or  open(*FH, $name))
    { 
        
        my $file = { 'fh' => *FH, 
           'input_spool' => { 'spool' => [], 
                              'macro' => '' },
           'name' => $name, 
           'line_nr' => 0 };
        unshift(@fhs, $file);
        $input_spool = $file->{'input_spool'};
        $line_number->{'file_name'} = $name;
        $line_number->{'line_nr'} = 1;
    }
    else
    {
        warn "$ERROR Can't read file $name: $!\n";
    }
}

sub open_out($)
{
    my $file = shift;
    if ($file eq '-')
    {
        binmode(STDOUT, ":encoding($to_encoding)") if (defined($to_encoding));
        return \*STDOUT;
    }
    unless ((defined($to_encoding) and open(FILE, ">:encoding($to_encoding)", $file)) or open(FILE, "> $file"))
    {
        die "$ERROR Can't open $file for writing: $!\n";
    }
    return \*FILE;
}

sub close_out($;$)
{
    my $FH = shift;
    my $file = shift;
    $file = '' if (!defined($file));
    return if ($Texi2HTML::Config::OUT eq '');
    close ($FH) || die "$ERROR: Error occurred when closing $file: $!\n";
}

sub next_line($)
{
    my $line_number = shift;
    while (@fhs)
    {
        my $file = $fhs[0];
        $line_number->{'file_name'} = $file->{'name'};
        $input_spool = $file->{'input_spool'};
        if (@{$input_spool->{'spool'}})
        {
             $line_number->{'macro'} = $file->{'input_spool'}->{'macro'};
             $line_number->{'line_nr'} = $file->{'line_nr'};
             my $line = shift(@{$input_spool->{'spool'}});
             print STDERR "# unspooling $line" if ($T2H_DEBUG & $DEBUG_MACROS);
             return($line);
        }
        else
        {
             $file->{'input_spool'}->{'macro'} = '';
             $line_number->{'macro'} = '';
        }
        my $fh = $file->{'fh'};
        no strict "refs";
        my $line = <$fh>;
        use strict "refs";
        my $chomped_line = $line;
        $file->{'line_nr'}++ if (defined($line) and chomp($chomped_line));
        $line_number->{'line_nr'} = $file->{'line_nr'};
        return($line) if (defined($line));
        no strict "refs";
        close($fh);
        use strict "refs";
        shift(@fhs);
    }
    return(undef);
}

# echo a warning
sub echo_warn($;$)
{
    my $text = shift;
    chomp ($text);
    my $line_number = shift;
    warn "$WARN $text " . format_line_number($line_number) . "\n";
}

sub echo_error($;$)
{
    my $text = shift;
    chomp ($text);
    my $line_number = shift;
    warn "$ERROR $text " . format_line_number($line_number) . "\n";
}

sub format_line_number($)
{
    my $line_number = shift;
    my $macro_text = '';
    #$line_number = undef;
    return '' unless (defined($line_number));
    $macro_text = " in $line_number->{'macro'}" if ($line_number->{'macro'} ne '');
    my $file_text = '(';
    $file_text = "(in $line_number->{'file_name'} " if ($line_number->{'file_name'} ne $docu);
    return "${file_text}l. $line_number->{'line_nr'}" . $macro_text . ')';
}

# to debug, dump the result of pass_texi and pass_structure in a file
sub dump_texi($$;$$)
{
    my $lines = shift;
    my $pass = shift;
    my $numbers = shift;
    my $file = shift;
    $file = "$docu_rdir$docu_name" . ".pass$pass" if (!defined($file));
    unless (open(DMPTEXI, ">$file"))
    {
         warn "Can't open $file for writing: $!\n";
    }
    print STDERR "# Dump texi\n" if ($T2H_VERBOSE);
    my $index = 0;
    foreach my $line (@$lines)
    {
        my $number_information = '';
        my $chomped_line = $line;
        $number_information = "$numbers->[$index]->{'file_name'}($numbers->[$index]->{'macro'},$numbers->[$index]->{'line_nr'}) " if (defined($numbers));
        print DMPTEXI "${number_information}$line";
        $index++ if (chomp($chomped_line));
    }
    close DMPTEXI;
}
 
# return next tag on the line
sub next_tag($)
{
    my $line = shift;
    if ($line =~ /^\s*\@(["'~\@\}\{,\.!\?\s\*\-\^`=:\|\/])/o or $line =~ /^\s*\@([a-zA-Z]\w*)([\s\{\}\@])/ or $line =~ /^\s*\@([a-zA-Z]\w*)$/)
    {
        return ($1);
    }
    return '';
}

sub top_stack($)
{
    my $stack = shift;
    return undef unless(@$stack);
    return $stack->[-1];
}

# return the next element with balanced {}
sub next_bracketed ($)
{
    my $line = shift;
    my $opened_braces = 0;
    my $result = '';
    while ($line !~ /^\s*$/)
    {
        if (!$opened_braces)
        {
            $line =~ s/^\s*//;
            #if ($line =~ s/^([^\{\}\s]+)//)
            if ($line =~ s/^([^\{\}]+?)(\s)/$2/ or $line =~ s/^([^\{\}]+?)$//)
            {
                my $text = $1;
                $text =~ s/\s*$//;
                return ($text, $line);
            }
        }
        elsif($line =~ s/^([^\{\}]+)//)
        {
            $result .= $1;
        }
        if ($line =~ s/^(\{|\})//)
        {
            my $brace = $1;
            $opened_braces++ if ($brace eq '{');
            $opened_braces-- if ($brace eq '}');
    
            if ($opened_braces < 0)
            {
                warn "$ERROR too much '}' in specification";
                $opened_braces = 0;
                next;
            }
            $result .= $brace;
            return ($result, $line) if ($opened_braces == 0);
        }
    }
    if ($opened_braces)
    {
        warn "$ERROR '{' not closed in specification";
        return ($result . ( '}' x $opened_braces));
    }
    return undef;
}

# do a href using file and id and taking care of ommitting file if it is 
# the same
sub href($$)
{
    my $element = shift;
    my $file = shift;
    return '' unless defined($element);
    my $href = '';
    print STDERR "Bug: $element->{'texi'}, id undef\n" if (!defined($element->{'id'}));
    print STDERR "Bug: $element->{'texi'}, file undef\n" if (!defined($element->{'file'}));
    $href .= $element->{'file'} if (defined($element->{'file'}) and $file ne $element->{'file'});
    $href .= "#$element->{'id'}" if (defined($element->{'id'}));
    return $href;
}

sub normalise_space($)
{
    return undef unless (defined ($_[0]));
    my $text = shift;
    $text =~ s/\s+/ /go;
    $text =~ s/ $//;
    $text =~ s/^ //;
    return $text;
}

sub normalise_node($)
{
    return undef unless (defined ($_[0]));
    my $text = shift;
    $text = normalise_space($text);
    $text =~ s/^top$/Top/i;
    return $text;
}

sub do_math($;$)
{
    #return l2h_ToLatex("\$".$_[0]."\$");
    return Texi2HTML::LaTeX2HTML::to_latex("\$".$_[0]."\$");
}

sub do_anchor_label($$$$)
{
    my $command = shift;
    #my $anchor = shift;
    my $args = shift;
    my $anchor = $args->[0];
    my $style_stack = shift;
    my $state = shift;
    my $line_nr = shift;

    $anchor = normalise_node($anchor);
    return &$Texi2HTML::Config::anchor($nodes{$anchor}->{'id'});
}

sub get_format_command($)
{
    my $format = shift;
    my $command = '';
    my $format_name = '';
    my $term = 0;
    my $item_nr;
    my $paragraph_number;
    my $enumerate_type;
    my $number;
    
    if (defined($format) and ref($format) eq "HASH")
    {
         $command = $format->{'command'};
         $command = '' if (!defined($command));
         $paragraph_number = \$format->{'paragraph_number'};
         $format_name =  $format->{'format'};
         $term = 1 if ($format->{'term'}); #This should never happen
         $item_nr = $format->{'item_nr'};
         $enumerate_type =  $format->{'spec'};
         $number = $format->{'number'};
    }
    return ($format_name, $command, $paragraph_number, $term, $item_nr, 
        $enumerate_type, $number);
}

sub do_paragraph($$)
{
    my $text = shift;
    my $state = shift;
    my ($format, $paragraph_command, $paragraph_number, $term, $item_nr, 
        $enumerate_type, $number) = get_format_command ($state->{'paragraph'});
    delete $state->{'paragraph'};
    my $paragraph_command_formatted;
    $state->{'paragraph_nr'}--;
    (print STDERR "Bug text undef in do_paragraph", return '') unless defined($text);
    my $align = '';
    $align = $state->{'paragraph_style'}->[-1] if ($state->{'paragraph_style'}->[-1]);
    
    if (exists($style_map_ref->{$paragraph_command}) and
       !exists($Texi2HTML::Config::special_list_commands{$format}->{$paragraph_command}))
    { 
        if ($format eq 'itemize')
        {
            chomp ($text);
            $text = do_simple($paragraph_command, $text, $state, [$text]);
            $text = $text . "\n";
        }
    }
    elsif (exists($things_map_ref->{$paragraph_command}))
    {
        $paragraph_command_formatted = do_simple($paragraph_command, '', $state);
    }
    return &$Texi2HTML::Config::paragraph($text, $align, $paragraph_command, $paragraph_command_formatted, $paragraph_number, $format, $item_nr, $enumerate_type, $number);
}

sub do_preformatted($$)
{
    my $text = shift;
    my $state = shift;
    my ($format, $leading_command, $preformatted_number, $term, $item_nr, $enumerate_type, 
         $number) = get_format_command($state->{'preformatted_format'});
    delete ($state->{'preformatted_format'});
    my $leading_command_formatted;
    my $pre_style = '';
    my $class = '';
    $pre_style = $state->{'preformatted_stack'}->[-1]->{'pre_style'} if ($state->{'preformatted_stack'}->[-1]->{'pre_style'});
    $class = $state->{'preformatted_stack'}->[-1]->{'class'};
    print STDERR "BUG: !state->{'preformatted_stack'}->[-1]->{'class'}\n" unless ($class);
    if (exists($style_map_ref->{$leading_command}) and
       !exists($Texi2HTML::Config::special_list_commands{$format}->{$leading_command}) and ($style_type{$leading_command} eq 'style'))
    {
        $text = do_simple($leading_command, $text, $state,[$text]) if ($format eq 'itemize');
    }
    elsif (exists($things_map_ref->{$leading_command}))
    {
        $leading_command_formatted = do_simple($leading_command, '', $state);
    }
    return &$Texi2HTML::Config::preformatted($text, $pre_style, $class, $leading_command, $leading_command_formatted, $preformatted_number, $format, $item_nr, $enumerate_type, $number);
}

sub do_external_ref($)
{
    my $node = shift;
    my $file = '';
    if ($node =~ s/^\((.+?)\)//)
    {
         $file = $1;
         if ($Texi2HTML::Config::NEW_CROSSREF_STYLE)
         {
             $file =~ s/\.[^\.]*$//;
             $file =~ s/^.*\///;
             $file = $Texi2HTML::Config::EXTERNAL_DIR . $file if (defined($Texi2HTML::Config::EXTERNAL_DIR));
             if ($Texi2HTML::Config::SPLIT)
             {
                 $file .= '/';
             }
             else
             {
                 $file .= '.' . $Texi2HTML::Config::NODE_FILE_EXTENSION;
             }
         }
         else
         {
             $file .= "/";
             $file = $Texi2HTML::Config::EXTERNAL_DIR . $file if (defined($Texi2HTML::Config::EXTERNAL_DIR));
         }
    }
    if ($node eq '')
    {
         if ($Texi2HTML::Config::NEW_CROSSREF_STYLE)
         {
             return $file . '#Top';
         }
         else
         {
             return $file;
         }
    }
    $node = normalise_node($node);
#print STDERR "NEW_CROSSREF_STYLE $Texi2HTML::Config::NEW_CROSSREF_STYLE $nodes{$node}, $nodes{$node}->{'cross_manual_target'}\n";
    if ($Texi2HTML::Config::NEW_CROSSREF_STYLE)
    {
         if (exists($nodes{$node}) and ($nodes{$node}->{'cross_manual_target'})) 
         {
              $node = $nodes{$node}->{'cross_manual_target'};
         }
         else 
         {
              $node = cross_manual_line($node);
         }
    }
    else
    {
         $node = remove_texi($node);
         $node =~ s/[^\w\.\-]/-/g;
    }
    my $target = $node;
    $node = $Texi2HTML::Config::TOP_NODE_FILE if ($node =~ /^top$/i);
    if ($Texi2HTML::Config::NEW_CROSSREF_STYLE)
    {
        if ($Texi2HTML::Config::SPLIT)
        {
            return $file . $node . ".$Texi2HTML::Config::NODE_FILE_EXTENSION" . '#' . $target;
        }
        else
        {
            return $file . '#' . $target;
        }
    }
    else
    {
        return $file . $node . ".$Texi2HTML::Config::NODE_FILE_EXTENSION";
    }
}

# return 1 if the following tag shouldn't begin a line
sub no_line($)
{
    my $line = shift;
    my $next_tag = next_tag($line);
    return 1 if (($line =~ /^\s*$/) or $no_line_macros{$next_tag} or 
       (($next_tag =~ /^(\w+?)index$/) and ($1 ne 'print')) or 
       (($line =~ /^\@end\s+(\w+)/) and  $no_line_macros{"end $1"}));
    return 0;
}

# handle raw formatting, ignored regions...
sub do_text_macro($$$$)
{
    my $type = shift;
    my $line = shift;
    my $state = shift;
    my $line_nr = shift;
    my $value;
    #print STDERR "do_text_macro $type\n";

    if (not $text_macros{$type})
    { # ignored text
        $state->{'ignored'} = $type;
        #print STDERR "IGNORED\n";
    }
    elsif ($text_macros{$type} eq 'raw' or $text_macros{$type} eq 'special')
    {
        $state->{'raw'} = $type;
        #print STDERR "RAW\n";
    }
    elsif ($text_macros{$type} eq 'value')
    {
        if (($line =~ s/(\s+)($VARRE)$//) or ($line =~ s/(\s+)($VARRE)(\s)//))
        {
            $value = $1 . $2;
            $value .= $3 if defined($3);
            if ($type eq 'ifclear')
            {
                if (defined($value{$2}))
                {
                    $state->{'ignored'} = $type;
                }
                else
                {
                    push @{$state->{'text_macro_stack'}}, $type;
                }
            }
            elsif ($type eq 'ifset')
            {
                unless (defined($value{$2}))
                {
                    $state->{'ignored'} = $type;
                }
                else
                {
                    push @{$state->{'text_macro_stack'}}, $type;
                }
            }
        }
        else
        {
            echo_error ("Bad $type line: $line", $line_nr);
            #warn "$ERROR Bad $type line: $line";
        }
    }
    else
    {
        push @{$state->{'text_macro_stack'}}, $type;
    }
    my $text = "\@$type";
    $text .= $value if defined($value); 
    return ($line, $text);
}

# do regions handled specially, currently only tex, going throug latex2html
sub do_special ($$)
{
    my $style = shift;
    my $text = shift;
    if ($style eq 'tex')
    {
        # add space to the end -- tex(i2dvi) does this, as well
        #return (l2h_ToLatex($text . " "));
        return (Texi2HTML::LaTeX2HTML::to_latex($text . " "));
    }
}

sub do_insertcopying($)
{
    my $state = shift;
    return '' unless @{$region_lines{'copying'}};
    return substitute_text(duplicate_state($state), @{$region_lines{'copying'}});
}

sub get_deff_index($$)
{
    my $tag = shift;
    my $line = shift;
   
    $tag =~ s/x$// if $tag =~ /x$/;
    my ($style, $category, $name, $type, $class, $arguments);
    ($style, $category, $name, $type, $class, $arguments) = parse_def($tag, $line); 
    # FIXME -- --- ''... should be protected for name and maybe class
    $name = &$Texi2HTML::Config::definition_category($name, $class, $style);
    return undef if (!$name or ($name =~ /^\s*$/));
    return ($style, $name);
}

sub parse_def($$)
{
    my $tag = shift;
    my $line = shift;
    if (!ref ($Texi2HTML::Config::def_map{$tag}))
    {
        # substitute shortcuts for definition commands
        my $substituted = $Texi2HTML::Config::def_map{$tag};
        $substituted =~ s/(\w+)//;
        $tag = $1;
        $line = $substituted . $line;
    }
    my ($category, $name, $type, $class, $arguments);
    my @args = @{$Texi2HTML::Config::def_map{$tag}};
    my $style = shift @args;
    while (@args)
    {
        my $arg = shift @args;
        if ($arg =~ s/^\{//)
        {
            my $item;
            ($item, $line) = next_bracketed($line);
            last if (!defined($item));
            $item =~ s/^\{(.*)\}$/$1/ if ($item =~ /^\{/);
            if ($arg eq 'category')
            {
                $category = $item;
            }
            elsif ($arg eq 'name')
            {
                $name = $item;
            }
            elsif ($arg eq 'type')
            {
                $type = $item;
            }
            elsif ($arg eq 'class')
            {
                $class = $item;
            }
        }
        else
        {
            chomp ($line);
            $line =~ s/\s*$//;
            $arguments = $line if ($line ne '');
            last;
        }
    }
    #return undef if (!$category or !defined($name) or ($name =~ /^\s*$/));
    return ($style, $category, $name, $type, $class, $arguments);
}

sub begin_deff_item($$;$)
{
    my $stack = shift;
    my $state = shift;
    my $no_paragraph = shift;
    #print STDERR "DEF push deff_item for $state->{'deff'}\n";
    push @$stack, { 'format' => 'deff_item', 'text' => '' };
    # there is no paragraph when a new deff just follows the deff we are
    # opening
    begin_paragraph($stack, $state) if ($state->{'preformatted'} and !$no_paragraph);
    delete($state->{'deff'});
    #dump_stack(undef, $stack, $state);
}

sub begin_paragraph($$)
{
    my $stack = shift;
    my $state = shift;

    my $command = 1;
    my $top_format = top_format($stack);
    if (defined($top_format))
    {
        $command = $top_format;
    }
    if ($state->{'preformatted'})
    {
        push @$stack, {'format' => 'preformatted', 'text' => '' };
        $state->{'preformatted_format'} = $command if ($command ne '1');
        push @$stack, @{$state->{'paragraph_macros'}} if $state->{'paragraph_macros'};
        delete $state->{'paragraph_macros'};
        return;
    }
    $state->{'paragraph'} = $command;
    $state->{'paragraph_nr'}++;
    push @$stack, {'format' => 'paragraph', 'text' => '' };
    # if there are macros which weren't closed when the previous 
    # paragraph was closed we reopen them here
    push @$stack, @{$state->{'paragraph_macros'}} if $state->{'paragraph_macros'};
    delete $state->{'paragraph_macros'};
}

sub parse_format_command($$)
{
    my $line = shift;
    my $tag = shift;
    my $command = 'asis';
    if (($line =~ /^\s*\@([A-Za-z]\w*)(\{\})?$/ or $line =~ /^\s*\@([A-Za-z]\w*)(\{\})?\s/) and ($things_map_ref->{$1} or defined($style_map_ref->{$1})))
    {
        $line =~ s/^\s*\@([A-Za-z]\w*)(\{\})?\s*//;
        $command = $1;
    }
    return ('', $command) if ($line =~ /^\s*$/);
    chomp $line;
    $line = substitute_text ({'keep_nr' => 1, 'keep_texi' => 1, 'check_item' => $tag}, $line);
    return ($line, $command);
}

sub parse_enumerate($)
{
    my $line = shift;
    my $spec;
    if ($line =~ /^\s*(\w)\b/ and ($1 ne '_'))
    {
        $spec = $1;
        $line =~ s/^\s*(\w)\s*//;
    }
    return ($line, $spec);
}

sub parse_multitable($$)
{
    my $line = shift;
    my $line_nr = shift;
    # first find the table width
    my $table_width = 0;
    if ($line =~ s/^\s+\@columnfractions\s+//)
    {
        my @fractions = split /\s+/, $line;
        $table_width = $#fractions + 1;
        while (@fractions)
        {
            my $fraction = shift @fractions;
            unless ($fraction =~ /^(\d*\.\d+)|(\d+)\.?$/)
            { 
                echo_error ("column fraction not a number: $fraction", $line_nr);
                #warn "$ERROR column fraction not a number: $fraction";
            }
        }
    }
    else
    {
        my $element;
        my $line_orig = $line;
        while ($line !~ /^\s*$/)
        {
            ($element, $line) = next_bracketed ($line);
            if ($element =~ /^\{/)
            {
                $table_width++; 
            }
            else
            {
                echo_error ("garbage in multitable specification: $element", $line_nr);
                #warn "$ERROR garbage in multitable specification: $line_orig";
            }
        }
    }
    return ($table_width);
}

sub end_format($$$$$)
{
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $format = shift;
    my $line_nr = shift;
    #print STDERR "END FORMAT $format\n";
    #dump_stack($text, $stack, $state);
    #sleep 1;
    close_menu($text, $stack, $state, $line_nr) if ($format_type{$format} eq 'menu');
    if (($format_type{$format} eq 'list') or ($format_type{$format} eq 'table'))
    { # those functions return if they detect an inapropriate context
        add_item($text, $stack, $state, $line_nr, '', 1); # handle lists
        add_term($text, $stack, $state, $line_nr, 1); # handle table
        add_line($text, $stack, $state, $line_nr, 1); # handle table
        add_row($text, $stack, $state, $line_nr); # handle multitable
    }

    my $format_ref = pop @$stack;
    
    # debug
    if (!defined($format_ref->{'text'}))
    {
        push @$stack, $format_ref;
        print STDERR "Bug: text undef in end_format $format\n";
        dump_stack($text, $stack, $state);
        pop @$stack;
    }
    
    if (defined($Texi2HTML::Config::def_map{$format}))
    {
        close_stack($text, $stack, $state, $line_nr, undef, 'deff_item') unless ($format_ref->{'format'} eq 'deff_item');
        add_prev($text, $stack, &$Texi2HTML::Config::def_item($format_ref->{'text'}));
        $format_ref = pop @$stack; # pop deff
        if (!defined($format_ref->{'format'}) or !defined($Texi2HTML::Config::def_map{$format_ref->{'format'}}))
        {
             print STDERR "Bug: not a def* under deff_item\n";
             push (@$stack, $format_ref);
             dump_stack($text, $stack, $state);
             pop @$stack;  
        }
        elsif ($format_ref->{'format'} ne $format)
        {
             echo_warn ("Waiting for \@end $format_ref->{'format'}, found \@end $format", $line_nr);
        }
        add_prev($text, $stack, &$Texi2HTML::Config::def($format_ref->{'text'}));
    }
    elsif ($format_type{$format} eq 'cartouche')
    {
        add_prev($text, $stack, &$Texi2HTML::Config::cartouche($format_ref->{'text'}));
    }
    elsif ($format_type{$format} eq 'menu')
    {
        if ($state->{'preformatted'})
        {
            # end the fake complex format
            $state->{'preformatted'}--;
            pop @{$state->{'preformatted_stack'}};
            pop @$stack;
        }
        $state->{'menu'}--;
        add_prev($text, $stack, &$Texi2HTML::Config::menu($format_ref->{'text'}));
    }
    elsif ($format_type{$format} eq 'complex')
    {
        $state->{'preformatted'}--;
        pop @{$state->{'preformatted_stack'}};
        # debug
        if (!defined($Texi2HTML::Config::complex_format_map->{$format_ref->{'format'}}->{'begin'}))
        {
            print STDERR "Bug undef $format_ref->{'format'}" . "->{'begin'} (for $format...)\n";
            dump_stack ($text, $stack, $state);
        }
        #print STDERR "before $format\n";
        #dump_stack ($text, $stack, $state);
        add_prev($text, $stack, &$Texi2HTML::Config::complex_format($format_ref->{'format'}, $format_ref->{'text'}));
        #print STDERR "after $format\n";
        #dump_stack ($text, $stack, $state);
    }
    elsif (($format_type{$format} eq 'table') or ($format_type{$format} eq 'list'))
    {
	    #print STDERR "CLOSE $format ($format_ref->{'format'})\n$format_ref->{'text'}\n";
        pop @{$state->{'format_stack'}};
	#dump_stack($text, $stack, $state); 
        if ($format_ref->{'format'} ne $format)
        {
             echo_warn ("Waiting for \@end $format_ref->{'format'}, found \@end $format", $line_nr);
        }
        if ($Texi2HTML::Config::format_map{$format})
        { # table or list has a simple format
            add_prev($text, $stack, end_simple_format($format_ref->{'format'}, $format_ref->{'text'}));
        }
        else
        { # table or list handler defined by the user
            add_prev($text, $stack, &$Texi2HTML::Config::table_list($format_ref->{'format'}, $format_ref->{'text'}, $format_ref->{'command'}));
        }
    } 
    elsif ($format_type{$format} eq 'paragraph_style')
    {
        if ($state->{'paragraph_style'}->[-1] eq $format)
        {
            pop @{$state->{'paragraph_style'}};
        }
        # FIXME we should call a function which just returns the text in the 
        # default case
        add_prev($text, $stack, $format_ref->{'text'});
    }
    elsif (exists($Texi2HTML::Config::format_map{$format}))
    {
        if ($format_ref->{'format'} ne $format)
        { # FIXME hasn't that case been handled before ?
             echo_warn ("Waiting for \@end $format_ref->{'format'}, found \@end $format", $line_nr);
        }
        add_prev($text, $stack, end_simple_format($format_ref->{'format'}, $format_ref->{'text'}));
    }
    else
    {
        echo_warn("Unknown format $format", $line_nr);
    }
    # We restart the preformatted format which was stopped by the format
    # if in preformatted
    begin_paragraph($stack, $state) if ($state->{'preformatted'});
}

sub do_text($;$)
{
    my $text = shift;
    my $state = shift;
    return $text if ($state->{'keep_texi'});
    if (defined($state) and !$state->{'preformatted'} and !$state->{'code_style'})
    {
        # in normal text `` and '' serve as quotes, --- is for a long dash 
        # and -- for a medium dash.
        # (see texinfo.txi, @node Conventions)
        $text = &$Texi2HTML::Config::normal_text($text);
    }
    if ($state->{'remove_texi'})
    {
        return $text;
    }
    return &$Texi2HTML::Config::protect_text($text);
}

sub end_simple_format($$)
{
    my $tag = shift;
    my $text = shift;

    my $element = $Texi2HTML::Config::format_map{$tag};
    return &$Texi2HTML::Config::format($tag, $element, $text);
}

sub close_menu($$$$)
{
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $line_nr = shift;
    close_stack($text, $stack, $state, $line_nr, '');
    if ($state->{'menu_comment'})
    {
	    #print STDERR "close MENU_COMMENT Before close_stack\n";
	    #dump_stack($text, $stack, $state);
        close_stack($text, $stack, $state, $line_nr, undef, 'menu_comment');
        # close_paragraph isn't needed in most cases, but A preformatted may 
        # appear after close_stack if we closed a format, as formats reopen
        # preformatted. However it is empty and close_paragraph will remove it
        close_paragraph ($text, $stack, $state); 
        my $menu_comment = pop @$stack;
        if (!$menu_comment->{'format'} or $menu_comment->{'format'} ne 'menu_comment')
        {
            warn "Bug waiting for menu_comment, got $menu_comment->{'format'}\n"; 
            dump_stack($text, $stack, $state);
        }
        add_prev ($text, $stack, &$Texi2HTML::Config::menu_comment($menu_comment->{'text'}));
        pop @{$state->{'preformatted_stack'}};
        $state->{'preformatted'}--;
        $state->{'menu_comment'}--;
    }
    if ($state->{'menu_entry'})
    {
        close_stack($text, $stack,$state, $line_nr, undef, 'menu_description');
        my $descr = pop(@$stack);
        print STDERR "# close_menu: close description\n" if ($T2H_DEBUG & $DEBUG_MENU);
        add_prev ($text, $stack, menu_description($descr->{'text'}, $state));
        delete $state->{'menu_entry'};
    }
}

sub menu_link($$;$)
{
    my $state = shift;
    my $line_nr = shift;
    my $simple = shift;
    my $menu_entry = $state->{'menu_entry'};
    my $file = $state->{'element'}->{'file'};
    my $node_name = normalise_node($menu_entry->{'node'});
    
    my $substitution_state = duplicate_state($state);
    my $name = substitute_line($menu_entry->{'name'}, $substitution_state);
    my $node = substitute_line($menu_entry->{'node'}, $substitution_state);

    if (($name ne '') and !$state->{'preformatted'} and $Texi2HTML::Config::AVOID_MENU_REDUNDANCY)
    {
        $name = '' unless (clean_text(remove_texi($menu_entry->{'name'}))
            ne clean_text(remove_texi($menu_entry->{'node'})))
    }

    my $entry = '';
    my $href;
    my $element = $nodes{$node_name};
    if ($element->{'seen'})
    {
        if ($element->{'with_section'})
        {
            $element = $element->{'with_section'};
        }
    
        #print STDERR "SUBHREF in menu for $element->{'texi'}\n";
        $href = href($element, $file);
        if (! $element->{'node'})
        {
            $entry = $element->{'text'}; # this is the section name without number
            $entry = $element->{'name'} if (!$Texi2HTML::Config::NUMBER_SECTIONS);
            $entry = "$Texi2HTML::Config::MENU_SYMBOL $entry" if (($entry ne '') and (!defined($element->{'number'}) or ($element->{'number'} =~ /^\s*$/)) and $Texi2HTML::Config::UNNUMBERED_SYMBOL_IN_MENU);
        }
    }
    elsif (($menu_entry->{'node'} =~ /^\s*\(.*\)/) or $novalidate)
    {
        # menu entry points to another info manual
        $href = $nodes{$node_name}->{'file'};
    }
    else
    {
        echo_error ("Unknown node in menu entry `$node_name'", $line_nr);
    }
    return &$Texi2HTML::Config::menu_link($entry, $state, $href, $node, $name, $menu_entry->{'ending'}) unless ($simple);
    return &$Texi2HTML::Config::simple_menu_link($entry, $state->{'preformatted'}, $href, $node, $name, $menu_entry->{'ending'});
}

sub menu_description($$)
{
    my $descr = shift;
    my $state = shift;
    my $menu_entry = $state->{'menu_entry'};
    my $node_name = normalise_node($menu_entry->{'node'});

    my $element = $nodes{$node_name};
    if ($element->{'seen'})
    {
        if ($element->{'with_section'})
        {
            $element = $element->{'with_section'};
        }
        if ($Texi2HTML::Config::AVOID_MENU_REDUNDANCY && ($descr ne '') && !$state->{'preformatted'})
        {
            $descr = '' if (clean_text($element->{'name'}) eq clean_text($descr));
        }
    }
    return &$Texi2HTML::Config::menu_description($descr, $state);
}

sub clean_text($)
{
    my $text = shift;
    $text =~ s/[^\w]//g;
    return $text;
}

sub do_xref($$$$)
{
    my $macro = shift;
    #my $text = shift;
    my $args = shift;
    my $style_stack = shift;
    my $state = shift;
    my $line_nr = shift;

    my $result = '';
    #$text =~ s/\s+/ /gos; # remove useless spaces and newlines
    #my @args = split(/\s*,\s*/, $text);
    my @args = @$args;
    #print STDERR "DO_XREF: $macro\n";
    my $j = 0;
    for ($j = 0; $j <= $#$args; $j++)
    {
         $args[$j] = normalise_space($args[$j]);
    #     print STDERR " ($j)$args[$j]\n";
    }
    #$args[0] = normalise_space($args[0]);
    $args[0] = '' if (!defined($args[0]));
    my $node_texi = normalise_node($args[0]);
    # a ref to a node in an info manual
    if ($args[0] =~ s/^\(([^\)]+)\)\s*//)
    {
        if ($macro eq 'inforef')
        {
            $args[2] = $1 unless ($args[2]);
        }
        else
        {
            $args[3] = $1 unless ($args[3]);
        }
    }
    if (($macro ne 'inforef') and $args[3])
    {
        $node_texi = "($args[3])" . normalise_node($args[0]);
    }

    if ($macro eq 'inforef')
    {
        if ((@args < 1) or ($args[0] eq ''))
        {
            echo_error ("Need a node name for \@$macro", $line_nr);
            return '';
        }
        if (@args > 3)
        {
            echo_warn ("Too much arguments for \@$macro", $line_nr);
        }
        $args[2] = '' if (!defined($args[2]));
        $args[1] = '' if (!defined($args[1]));
        $node_texi = "($args[2])$args[0]";
    }
    
    my $i;
    my $new_state = duplicate_state($state);
    $new_state->{'keep_texi'} = 0;
    $new_state->{'keep_nr'} = 0;
    for ($i = 0; $i < 5; $i++)
    {
        $args[$i] = substitute_line($args[$i], $new_state);
    }
    #print STDERR "(@args)\n";
    
    if (($macro eq 'inforef') or ($args[3] ne '') or ($args[4] ne ''))
    {# external ref
        if ($macro eq 'inforef')
        {
            $macro = 'xref';
            $args[3] = $args[2];
        }
        my $href = '';
        my $node_file = '';
        if ($args[3] ne '')
        {
            $href = do_external_ref($node_texi);
            $node_file = "($args[3])$args[0]";
        }
        my $section = '';
        if ($args[4] ne '')
        {
            $section = $args[0];
            if ($args[2] ne '')
            {
                $section = $args[2];
            }
        }
        $result = &$Texi2HTML::Config::external_ref($macro, $section, $args[4], $node_file, $href, $args[1]);
    }
    else
    {
        my $element = $nodes{$node_texi};
        if ($element and $element->{'seen'})
        {
            if ($element->{'with_section'})
            {
                $element = $element->{'with_section'};
            }
            my $file = '';
            if (defined($state->{'element'}))
            {
                $file = $state->{'element'}->{'file'};
            }
            else
            {
                echo_warn ("\@$macro not in text (in anchor, node, section...)", $line_nr);
                # if Texi2HTML::Config::SPLIT the file is '' which ensures a href with the file
                # name. if ! Texi2HTML::Config::SPLIT the 2 file will be the same thus there
                # won't be the file name
                $file = $element->{'file'} unless ($Texi2HTML::Config::SPLIT);
            }
	    #print STDERR "SUBHREF in ref `$node_texi': $_";
            my $href = href($element, $file);
            my $section = $args[2];
            $section = $args[1] if ($section eq '');
            my $name = $section;
            my $short_name = $section;
            if ($section eq '')
            {
                $name = $element->{'name'};
                $short_name = $args[0];
            }
            $result = &$Texi2HTML::Config::internal_ref ($macro, $href, $short_name, $name, $element->{'section'});
        }
        else
        {
           if (($node_texi eq '') or !$novalidate)
           {
               echo_error ("Undefined node `$node_texi' in \@$macro", $line_nr);
               my $text = '';
               for (my $i = 0; $i < @$args -1; $i++)
               {
                    $text .= $args->[$i] .',';
               }
               $text .= $args->[-1];
               $result = "\@$macro"."{${text}}";
           }
           else
           {
               $result = &$Texi2HTML::Config::external_ref($macro, '', '', $args[0], do_external_ref($node_texi), $args[1]);
           }
        }
    }
    return $result;
}

sub do_footnote($$$$)
{
    my $command = shift;
    my $args = shift;
    my $text = $args->[0];
    my $style_stack = shift;
    my $state = shift;
    my $line_nr = shift;

    $text .= "\n";
    $foot_num++;
    $relative_foot_num++;
    my $docid  = "DOCF$foot_num";
    my $footid = "FOOT$foot_num";
    my $from_file = '';
    if ($state->{'element'} and $Texi2HTML::Config::SPLIT and $Texi2HTML::Config::SEPARATED_FOOTNOTES)
    { 
        $from_file = $state->{'element'}->{'file'};
    }
    my %state;
    initialise_state (\%state); 
    if ($Texi2HTML::Config::SEPARATED_FOOTNOTES)
    {
        $state{'element'} = $footnote_element;
    }
    else
    {
        $state{'element'} = $state->{'element'};
    }
    my $file = '';
    $file = $docu_foot if ($Texi2HTML::Config::SPLIT and $Texi2HTML::Config::SEPARATED_FOOTNOTES);
    
    # FIXME use split_lines ? It seems to work like it is now ?
    my @lines = substitute_text(\%state, map {$_ = $_."\n"} split (/\n/, $text));
    my ($foot_lines, $foot_label) = &$Texi2HTML::Config::foot_line_and_ref ($foot_num,
         $relative_foot_num, $footid, $docid, $from_file, $file, \@lines, $state);
    push(@foot_lines, @{$foot_lines});
    return $foot_label;
}

sub do_image($$$$)
{
    # replace images
    my $command = shift;
    my $args = shift;
    my $text = $args->[0];
    my $style_stack = shift;
    my $state = shift;
    my $line_nr = shift;
    $text =~ s/\s+/ /gos; # remove useless spaces and newlines
    my @args = split (/\s*,\s*/, $text);
    my $base = $args[0];
    if ($base eq '')
    {
         echo_error ("no file argument for \@image", $line_nr);
         #warn "$ERROR no file argument for \@image\n";
         return '';
    }
    $args[4] = '' if (!defined($args[4]));
    $args[3] = '' if (!defined($args[3]));
    my $image;
    my $file_name;
    $image = locate_include_file("$base.$args[4]") if (defined($args[4]) and ($args[4] ne ''));
    if (defined($image))
    {
         $file_name = "$base.$args[4]";
    }
    elsif ($image = locate_include_file("$base.png"))
    {
         $file_name = "$base.png";
    }
    elsif ($image = locate_include_file("$base.jpg"))
    {
         $file_name = "$base.jpg";
    }
    elsif ($image = locate_include_file("$base.gif"))
    {
         $file_name = "$base.gif";
    }
    else 
    {
        $image = "$base.jpg";
        $image = "$base.$args[4]" if (defined($args[4]) and ($args[4] ne ''));
        $file_name = $image;
        echo_error ("no image file for $base, (using $image)", $line_nr); 
        #warn "$ERROR no image file for $base, (using $image) : $text\n"; 
    } # FIXME use full file name for alt instead of base when there is no
      # alttext ?
    if ($args[3] =~ /\S/)
    {
        # FIXME makeinfo don't do that.
        $args[3] = do_text($args[3]);
        $base = $args[3] if ($args[3] =~ /\S/);
    }
    return &$Texi2HTML::Config::image(
    &$Texi2HTML::Config::protect_text($path_to_working_dir . $image),
    &$Texi2HTML::Config::protect_text($base), 
    $state->{'preformatted'}, &$Texi2HTML::Config::protect_text($file_name));
}

sub duplicate_state($)
{
    my $state = shift;
    my $new_state = { 'element' => $state->{'element'}, 
           'preformatted' => $state->{'preformatted'}, 
           'code_style' => $state->{'code_style'}, 
           'keep_texi' => $state->{'keep_texi'}, 
           'keep_nr' => $state->{'keep_nr'}, 
           'preformatted_stack' => $state->{'preformatted_stack'} 
    };
    return $new_state;
}

sub expand_macro($$$$$)
{
    my $name = shift;
    my $args = shift;
    my $end_line = shift;
    my $line_nr = shift;
    my $state = shift;

    my $index = 0;
    foreach my $arg (@$args)
    { # expand @macros in arguments
        $args->[$index] = substitute_text({'texi' => 1, 'arg_expansion' => 1}, split_lines($arg));
        $index++;
    }
    my $macrobody = $macros->{$name}->{'Body'};
    my $formal_args = $macros->{$name}->{'Args'};
    my $args_index =  $macros->{$name}->{'Args_index'};
    my $i;
    
    die "Bug end_line not defined" if (!defined($end_line));
    
    for ($i=0; $i<=$#$formal_args; $i++)
    {
        $args->[$i] = "" unless (defined($args->[$i]));
        print STDERR "# arg($i): $args->[$i]\n" if ($T2H_DEBUG & $DEBUG_MACROS);
    }
    echo_error ("too much arguments for macro $name", $line_nr) if (defined($args->[$i + 1]));
    #warn "$ERROR too much arguments for macro $name" if (defined($args->[$i + 1]));
    my $result = '';
    while ($macrobody)
    {
        if ($macrobody =~ s/^([^\\]*)\\//o)
        {
            $result .= $1 if defined($1);
            if ($macrobody =~ s/^\\//)
            {
                $result .= '\\';
            }
            elsif ($macrobody =~ s/^(\@end\sr?macro)$// or $macrobody =~ s/^(\@end\sr?macro\s)// or $macrobody =~ s/^(\@r?macro\s+\w+\s*.*)//)
            { # \ protect @end macro
                $result .= $1;
            }
            elsif ($macrobody =~ s/^([^\\]*)\\//)
            {
               my $arg = $1;
               if (defined($args_index->{$arg}))
               {
                   $result .= $args->[$args_index->{$arg}];
               }
               else
               {
                   warn "$ERROR \\ not followed by \\ or an arg but by $arg in macro\n";
                   $result .= '\\' . $arg;
               }
            }
            next;
        }
        $result .= $macrobody;
        last;
    }
    #$result .= $end_line;
    my @result = split(/^/m, $result);
    #my $first_line = shift (@result);
    if ($state->{'arg_expansion'})
    {
        unshift @{$state->{'spool'}}, (@result, $end_line);
    }
    else
    {
        unshift @{$input_spool->{'spool'}}, (@result, $end_line);
        $input_spool->{'macro'} = $name if ($input_spool->{'macro'} eq '');
    }
    if ($T2H_DEBUG & $DEBUG_MACROS)
    {
        print STDERR "# macro expansion result:\n";
        #print STDERR "$first_line";
        foreach my $line (@result)
        {
            print STDERR "$line";
        }
        print STDERR "# macro expansion result end\n";
    }
    #return $first_line;
}

sub do_index_summary_file($)
{
    my $name = shift;
    my ($Pages, $Entries) = get_index($name);
    &$Texi2HTML::Config::index_summary_file_begin ($name, $printed_indices{$name});
    #open(FHIDX, ">$docu_rdir$docu_name" . "_$name.idx")
    #   || die "Can't open > $docu_rdir$docu_name" . "_$name.idx for writing: $!\n";
    #print STDERR "# writing $name index summary in $docu_rdir$docu_name" . "_$name.idx...\n" if $T2H_VERBOSE;
    print STDERR "# writing $name index summary\n" if $T2H_VERBOSE;

    foreach my $key (sort keys %$Entries)
    {
        #print FHIDX "$key\t$Entries->{$key}->{href}\n";
        my $entry = $Entries->{$key};
        my $label = $entry->{'element'};
        my $entry_element = $label;
        # notice that we use the section associated with a node even when 
        # there is no with_section, i.e. when there is another node preceding
        # the sectionning command.
        # However when it is the Top node, we use the node instead.
        # (for the Top node, 'section_ref' is himself, and 'as_section' is 
        # true)
        $entry_element = $entry_element->{'section_ref'} if ($entry_element->{'node'} and $entry_element->{'section_ref'} and !$entry_element->{'section_ref'}->{'as_section'});
        my $origin_href = $entry->{'file'};
   #print STDERR "$entry $entry->{'entry'}, real elem $label->{'texi'}, section $entry_element->{'texi'}, real $label->{'file'}, entry file $entry->{'file'}\n";
        if ($entry->{'label'})
        { 
             $origin_href .= '#' . $entry->{'label'};
        }
        else
        {
            # If the $label element and the $index entry are on the same
            # file the label is prefered. If they aren't on the same file
            # the entry id is choosed as it means that the label element
            # and the index entry are separated by a printindex.
            print STDERR "id undef ($entry) entry: $entry->{'entry'}, label: $label->{'text'}\n"  if (!defined($entry->{'id'}));
            if ($entry->{'file'} eq $label->{'file'})
            {
                $origin_href .= '#' . $label->{'id'};
            }
            else
            {
                $origin_href .= '#' . $entry->{'id'} ;
            }
        }
        #print STDERR "SUBHREF in index summary file for $entry_element->{'texi'}\n";
        #print FHIDX '' . 
        &$Texi2HTML::Config::index_summary_file_entry ($name,
          $key, $origin_href, 
          substitute_line($entry->{'entry'}), $entry->{'entry'},
          href($entry_element, ''),
          $entry_element->{'text'},
          $printed_indices{$name});
    }
    &$Texi2HTML::Config::index_summary_file_end ($name, $printed_indices{$name});
}

sub do_index_page($$;$)
{
    my $index_elements = shift;
    my $nr = shift;
    my $page = shift;
    my $index_element = $index_elements->[$nr];
    my $summary = do_index_summary($index_element->{'element'}, $index_elements);
    my $entries = do_index_entries($index_element->{'element'}, $index_element->{'page'}, $index_element->{'name'});
    return $summary . $entries . $summary;
}

sub do_index_summary($$)
{
    my $element = shift;
    my $index_elements = shift;

    my @letters;
    my @symbols;

    for my $index_element_item (@$index_elements)
    {
        my $index_element = $index_element_item->{'element'};
        my $file = '';
        $file .= $index_element->{'file'} if ($index_element->{'file'} ne $element->{'file'});
        my $index = 0;
        for my $letter (@{$index_element_item->{'page'}->{Letters}})
        {
            if ($letter =~ /^[A-Za-z]/)
            {
                push @letters, &$Texi2HTML::Config::summary_letter($letter, $file, "$index_element->{'id'}" . "_$index");
            }
            else
            {
                push @symbols, &$Texi2HTML::Config::summary_letter($letter, $file, "$index_element->{'id'}" . "_$index");
            }
            $index++;
        }
    }
    return &$Texi2HTML::Config::index_summary(\@letters, \@symbols);
}

sub do_index_entries($$$)
{
    my $element = shift;
    my $page = shift;
    my $name = shift;
 
    my $letters = '';
    my $index = 0;
    for my $letter (@{$page->{'Letters'}})
    {
       my $entries = '';
       for my $entry (@{$page->{'EntriesByLetter'}->{$letter}})
       {
           my $label = $entry->{'element'};
           my $entry_element = $label;
           # notice that we use the section associated with a node even when 
           # there is no with_section, i.e. when there is another node preceding
           # the sectionning command.
           # However when it is the Top node, we use the node instead.
           # (for the Top node, 'section_ref' is himself, and 'as_section' is 
           # true)
           $entry_element = $entry_element->{'section_ref'} if ($entry_element->{'node'} and $entry_element->{'section_ref'} and !$entry_element->{'section_ref'}->{'as_section'});
           my $origin_href = '';
           $origin_href = $entry->{'file'} if ($Texi2HTML::Config::SPLIT and $entry->{'file'} ne $element->{'file'});
	   #print STDERR "$entry $entry->{'entry'}, real elem $label->{'texi'}, section $entry_element->{'texi'}, real $label->{'file'}, entry file $entry->{'file'}\n";
           if ($entry->{'label'})
           { 
               $origin_href .= '#' . $entry->{'label'};
           }
	   else
           {
               # If the $label element and the $index entry are on the same
               # file the label is prefered. If they aren't on the same file
               # the entry id is choosed as it means that the label element
               # and the index entry are separated by a printindex.
               print STDERR "id undef ($entry) entry: $entry->{'entry'}, label: $label->{'text'}\n"  if (!defined($entry->{'id'}));
               if ($entry->{'file'} eq $label->{'file'})
               {
                   $origin_href .= '#' . $label->{'id'};
               }
               else
               {
                   $origin_href .= '#' . $entry->{'id'} ;
               }
           }
	   #print STDERR "SUBHREF in index for $entry_element->{'texi'}\n";
           $entries .= &$Texi2HTML::Config::index_entry ($origin_href, 
                     substitute_line($entry->{'entry'}),
                     href($entry_element, $element->{'file'}),
                     $entry_element->{'text'});
        }
        $letters .= &$Texi2HTML::Config::index_letter ($letter, "$element->{'id'}" . "_$index", $entries);
        $index++;
    }
    return &$Texi2HTML::Config::print_index($letters, $name);
}

# remove texi commands, replacing with what seems adequate. see simple_map_texi
# and texi_map.
# Doesn't protect html
sub remove_texi(@)
{
    return substitute_text ({ 'remove_texi' => 1 }, @_);
}

sub enter_table_index_entry($$$$)
{
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $line_nr = shift;
    if ($state->{'item'} and ($state->{'table_stack'}->[-1] =~ /^(v|f)table$/))
    {
        my $index = $1;
        my $macro = $state->{'item'};
        delete $state->{'item'};
        close_stack($text, $stack, $state, $line_nr, undef, 'index_item');
        my $item = pop @$stack;
        my $element = $state->{'element'};
        $element = $state->{'node_ref'} unless ($element);
        #print STDERR "enter_table_index_entry $item->{'text'}";
        enter_index_entry($index, $line_nr, $item->{'text'}, $state->{'place'}, $element, 0);
        add_prev($text, $stack, "\@$macro" . $item->{'text'});
    }
}

sub scan_texi($$$$;$)
{
    my $line = shift;
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $line_nr = shift;
    
    die "stack not an array ref"  unless (ref($stack) eq "ARRAY");
    local $_ = $line;

    while(1)
    {
        #print STDERR "WHILE:$_";
        #dump_stack($text, $stack, $state);

        # In ignored region
        if ($state->{'ignored'})
        {
            my $line;
            if (s/^(.*?\@end\s+$state->{'ignored'})//)
            {
                 $line = $1;
                 if (s/^$// or s/(\s+)//)
                 {
                     $line = $line . $1 if (defined($1));
                 }
                 elsif (/[^\@]/)
                 {
                      $_ .= $line;
                      $line = undef;
                 }
            }
            if (defined($line))
            {
                 delete $state->{'ignored'};
                 #dump_stack($text, $stack, $state);
                 # MACRO_ARG => keep ignored text
                 if ($state->{'arg_expansion'})
                 {
                     #add_prev ($text, $stack, $1);
                     add_prev ($text, $stack, $line);
                     next;
                 }
                 return if /^\s*$/o;
                 next;
            }
            add_prev ($text, $stack, $_) if ($state->{'arg_expansion'});
            return;
        }

        # in macro definition
        if (defined($state->{'macro'}))
        {
            if (s/^([^\\\@]*\\)//)
            {# I believe it is correct, although makeinfo don't do that.
                 $state->{'macro'}->{'Body'} .= $1;
                 if (s/^\\//)
                 {
                      $state->{'macro'}->{'Body'} .= '\\';
                      next;
                 }
                 elsif (s/^(\@end\sr?macro)$//o or s/^(\@end\sr?macro\s)//o
                      or s/^(\@r?macro\s+\w+\s*.*)//o) 
                 {
                      $state->{'macro'}->{'Body'} .= $1;
                      next;
                 }
            }
            #if (s/^(.*?)\@end\sr?macro$//o or s/^(.*?)\@end\sr?macro\s+//o)
            if (s/^(\@end\sr?macro)$//o or s/^(\@end\sr?macro\s+)//o)
            {
                 $state->{'macro_inside'}--;
                 if ($state->{'macro_inside'})
                 {
                     $state->{'macro'}->{'Body'} .= $1;
                     next;
                 }
                 #$state->{'macro'}->{'Body'} .= $1 if defined($1) ;
                 chomp $state->{'macro'}->{'Body'};
                 print STDERR "# end macro def. Body:\n$state->{'macro'}->{'Body'}"
                     if ($T2H_DEBUG & $DEBUG_MACROS);
                 delete $state->{'macro'};
                 return if (/^\s*$/);
                 next;
            }
            elsif(/^(\@r?macro\s+\w+\s*.*)/)
            {
                 $state->{'macro'}->{'Body'} .= $_;
                 $state->{'macro_inside'}++;
                 return;
            }
            elsif (s/^\@(.)//)
            {
                 $state->{'macro'}->{'Body'} .= '@' . $1;
                 next;
            }
            elsif (s/^\@//)
            {
                 $state->{'macro'}->{'Body'} .= '@';
                 next;
            }
            else
            {
                 s/([^\@\\]*)//;
                 $state->{'macro'}->{'Body'} .= $1 if (defined($1));
                 if (/^$/)
                 {
                      $state->{'macro'}->{'Body'} .= $_;
                      return;
                 }
                 next;
                 #$state->{'macro'}->{'Body'} .= $_ if defined($_) ;
                 #return;
            }
        }
        # in macro arguments parsing/expansion
        if (defined($state->{'macro_name'}))
        {
            my $special_chars = quotemeta ('\{}');
            my $multi_args = 0;
            my $formal_args = $macros->{$state->{'macro_name'}}->{'Args'};
            $multi_args = 1 if ($#$formal_args >= 1);
            $special_chars .= quotemeta(',') if ($multi_args);
            if ($state->{'macro_args'}->[-1] eq '')
            {
                s/^\s*//o;
            }
            if (s/^([^$special_chars]*)([$special_chars])//)
            {
                $state->{'macro_args'}->[-1] .= $1 if defined($1);
                # \ protects any character in macro arguments
                if ($2 eq '\\')
                {
                    print STDERR "# macro call: protected char\n" if ($T2H_DEBUG & $DEBUG_MACROS);
                    if (s/^(.)//)
                    {
                        $state->{'macro_args'}->[-1] .= $1;
                    }
                    else
                    {
                        $state->{'macro_args'}->[-1] .= '\\';
                    }
                }
                elsif ($2 eq ',')
                { # separate args
                    print STDERR "# macro call: new arg\n" if ($T2H_DEBUG & $DEBUG_MACROS);
                    s/^\s*//o;
                    push @{$state->{'macro_args'}}, '';
                }
                elsif ($2 eq '}')
                { # balanced } ends the macro call, otherwise it is in the arg
                    $state->{'macro_depth'}--;
                    if ($state->{'macro_depth'} == 0)
                    { 
                        print STDERR "# expanding macro $state->{'macro_name'}\n" if ($T2H_DEBUG & $DEBUG_MACROS);
                        $_ = expand_macro($state->{'macro_name'}, $state->{'macro_args'}, $_, $line_nr, $state);
                        delete $state->{'macro_name'};
                        delete $state->{'macro_depth'};
                        delete $state->{'macro_args'};
                        return;
                    }
                    else
                    {
                        print STDERR "# macro call: closing }\n" if ($T2H_DEBUG & $DEBUG_MACROS);
                        add_text('}', \$state->{'macro_args'}->[-1]);
                    }
                }
                elsif ($2 eq '{')
                {
                    print STDERR "# macro call: opening {\n" if ($T2H_DEBUG & $DEBUG_MACROS);
                    $state->{'macro_depth'}++;
                    add_text('{', \$state->{'macro_args'}->[-1]);
                }
                next;
            }
            print STDERR "# macro call: end of line\n" if ($T2H_DEBUG & $DEBUG_MACROS);
            $state->{'macro_args'}->[-1] .= $_;
            return;
        }

        # in a raw format, verbatim, tex or html
        if ($state->{'raw'}) 
        {
            my $tag = $state->{'raw'};

            # debugging
            if (! @$stack or ($stack->[-1]->{'style'} ne $tag))
            {
                print STDERR "Bug: raw or special: $tag but not on top of stack\n";
                print STDERR "line: $_";
                dump_stack($text, $stack, $state);
                exit 1;
            }
	    
            if (s/^(.*?)(\@end\s$tag)$// or s/^(.*?)(\@end\s$tag\s)//)
            {
                add_prev ($text, $stack, $1);
                my $end = $2;
                my $style = pop @$stack;
                if ($style->{'text'} !~ /^\s*$/ or $state->{'arg_expansion'})
                {
                    my $after_macro = '';
                    $after_macro = ' ' unless (/^\s*$/);
                    add_prev ($text, $stack, $style->{'text'} . $end . $after_macro);
                    delete $state->{'raw'};
                }
                next;
            }
            else
            {
                 add_prev ($text, $stack, $_);
                 last;
            }
        }

        # in a @verb{ .. } macro
        if (defined($state->{'verb'}))
        {
            my $char = quotemeta($state->{'verb'});
            if (s/^(.*?)$char\}/\}/)
            {
                 add_prev($text, $stack, $1 . $state->{'verb'});
                 $stack->[-1]->{'text'} = $state->{'verb'} . $stack->[-1]->{'text'};
                 delete $state->{'verb'};
                 next;
            }
            else
            {
                 add_prev($text, $stack, $_);
                 last;
            }
        }
	
        # an @end tag
        if (s/^([^{}@]*)\@end(\s+)([a-zA-Z]\w*)//)
        {
            add_prev($text, $stack, $1);
            my $space = $2;
            my $end_tag = $3;
            if (defined($state->{'text_macro_stack'})
               and @{$state->{'text_macro_stack'}}
               and ($end_tag eq $state->{'text_macro_stack'}->[-1]))
            {
                pop @{$state->{'text_macro_stack'}};
                # we keep menu and titlepage for the following pass
                if ((($end_tag eq 'menu') and $text_macros{'menu'}) or ($region_lines{$end_tag}) or $state->{'arg_expansion'})
                {
                     add_prev($text, $stack, "\@end${space}$end_tag");
                }
                else
                {
                    #print STDERR "End $end_tag\n";
                    #dump_stack($text, $stack, $state);
                    return if (/^\s*$/);
                }
            }
            elsif ($text_macros{$end_tag})
            {
                echo_error ("\@end $end_tag without corresponding element", $line_nr);
            }
            else
            {
                add_prev($text, $stack, "\@end${space}$end_tag");
            }
            next;
        }
        elsif (s/^([^{}@]*)\@(["'~\@\}\{,\.!\?\s\*\-\^`=:\|\/])//o or s/^([^{}@]*)\@([a-zA-Z]\w*)([\s\{\}\@])/$3/o or s/^([^{}@]*)\@([a-zA-Z]\w*)$//o)
        {
            add_prev($text, $stack, $1);
            my $macro = $2;
            #print STDERR "MACRO $macro\n";
            if ($Texi2HTML::Config::to_skip{$macro})
            {
                 my $line;
                 ($_, $line) = skip_texi($_, $macro, $state);
                 add_prev ($text, $stack, $line); 
                 next;
            }
            # pertusus: it seems that value substitution are performed after
            # macro argument expansions: if we have 
            # @set comma ,
            # and a call to a macro @macro {arg1 @value{comma} arg2}
            # the macro arg is arg1 , arg2 and the comma don't separate
            # args. Likewise it seems that the @value are not expanded
            # in macro definitions

            # track variables
            my $value_macro = 1;
            if ($macro eq 'set' and  s/^(\s+)($VARRE)(\s+)(.*)$//o)
            {
                if ($state->{'arg_expansion'})
                {
                    my $line = "\@$macro" . $1.$2.$3;
                    $line .= $4 if (defined($4));
                    add_prev($text, $stack, $line); 
                    next;
                }
                $value{$2} = $4;
            }
            elsif ($macro eq 'clear' and s/^(\s+)($VARRE)//o)
            {
                if ($state->{'arg_expansion'})
                {
                    add_prev($text, $stack, "\@$macro" . $1 . $2);
                    next; 
                }
                delete $value{$2};
            }
            else
            {
                 $value_macro = 0;
            }
            if ($value_macro)
            {
                return if (/^\s*$/);
                next;
            }
	    
            if ($macro =~ /^r?macro$/)
            { #FIXME what to do if 'arg_expansion' is true (ie within another
              # macro call arguments ?
                if (/^\s+(\w+)\s*(.*)/)
                {
                    my $name = $1;
                    if (exists($macros->{$name}))
                    {
                         echo_warn ("macro `$name' allready defined " . 
                             format_line_number($macros->{$name}->{'line_nr'}) . " redefined", $line_nr);
                    }
                    $state->{'macro_inside'} = 1;
                    my @args = ();
                    @args = split(/\s*,\s*/ , $1)
                       if ($2 =~ /^\s*{\s*(.*?)\s*}\s*/);
                    # keep the context information of the definition
                    $macros->{$name}->{'line_nr'} = { 'file_name' => $line_nr->{'file_name'}, 
                         'line_nr' => $line_nr->{'line_nr'}, 'macro' => $line_nr->{'macro'} } if (defined($line_nr));
                    $macros->{$name}->{'Args'} = \@args;
                    my $arg_index = 0;
                    my $debug_msg = '';
                    foreach my $arg (@args)
                    { # when expanding macros, the argument index is retrieved
                      # with Args_index
                        $macros->{$name}->{'Args_index'}->{$arg} = $arg_index;
                        $debug_msg .= "$arg($arg_index) ";
                        $arg_index++;
                    }
                    $macros->{$name}->{'Body'} = '';
                    $state->{'macro'} = $macros->{$name};
                    print STDERR "# macro def $name: $debug_msg\n"
                     if ($T2H_DEBUG & $DEBUG_MACROS);
                }
                else
                {
                    echo_error ("Bad macro defintion $_", $line_nr);
                    #warn "$ERROR Bad macro defintion $_";
                }
                return;
            }
            elsif (defined($text_macros{$macro}))
            {
                my $tag;
                ($_, $tag) = do_text_macro ($macro, $_, $state, $line_nr);
                # if it is a raw formatting command or a menu command
                # we must keep it for later
                my $macro_kept;
                if ($state->{'raw'} or (($macro eq 'menu') and $text_macros{'menu'}) or (exists($region_lines{$macro})) or $state->{'arg_expansion'})
                {
                    add_prev($text, $stack, $tag);
                    $macro_kept = 1;
                }
                if ($state->{'raw'})
                {
                    push @$stack, { 'style' => $macro, 'text' => '' };
                }
                next if $macro_kept;
                #dump_stack ($text, $stack, $state);
                return if (/^\s*$/);
            }
            elsif ($macro eq 'value')
            {
                if (s/^{($VARRE)}//)
                {
                    if ($state->{'arg_expansion'})
                    {
                        add_prev($text, $stack, "\@$macro" . '{' . $1 . '}');
                        next;
                    }
                    $_ = get_value($1) . $_;
                }
                else
                {
                    if ($state->{'arg_expansion'})
                    {
                        add_prev($text, $stack, "\@$macro");
                        next;
                    }
                    echo_error ("bad \@value macro", $line_nr);
                    #warn "$ERROR bad \@value macro";
                }
            }
            elsif ($macro eq 'definfoenclose')
            {
                if ($state->{'arg_expansion'})
                {
                    add_prev($text, $stack, "\@$macro" . $_);
                    return;
                }
                if (s/^\s+([a-z]+)\s*,\s*([^\s]+)\s*,\s*([^\s]+)//)
                {
                     $info_enclose{$1} = [ $2, $3 ];
                }
                else
                {
                     echo_error("Bad \@$macro", $line_nr);
                }
                return if (/^\s*$/);
                s/^\s*//;
            }
            elsif ($macro eq 'include')
            {
                if ($state->{'arg_expansion'})
                {
                    add_prev($text, $stack, "\@$macro" . $_);
                    return;
                }
                #if (s/^\s+([\/\w.+-]+)//o)
                if (s/^(\s+)(.*)//o)
                {
                    my $file = locate_include_file($2);
                    if (defined($file))
                    {
                        open_file($file, $line_nr);
                        print STDERR "# including $file\n" if $T2H_VERBOSE;
                    }
                    else
                    {
                        echo_error ("Can't find $2, skipping", $line_nr);
                        #warn "$ERROR Can't find $1, skipping\n";
                    }
                }
                else
                {
                    echo_error ("Bad include line: $_", $line_nr);
                    return;
                } # makeinfo remove the @include but not the end of line
                # FIXME verify if it is right
                #return if (/^\s*$/);
            }
            elsif ($macro eq 'verbatiminclude')
            {
                s/(.*)//;
                add_prev($text, $stack, "\@$macro" . $1);
                next;
            }
            elsif ($macro eq 'documentencoding')
            {
                if (s/(\s+)([0-9\w\-]+)//)
                {
                    my $encoding = $2;
                    $Texi2HTML::Config::DOCUMENT_ENCODING = $encoding;
                    $from_encoding = set_encoding($encoding);
                    if (defined($from_encoding))
                    {
                        foreach my $file (@fhs)
                        {
                            binmode($file->{'fh'}, ":encoding($from_encoding)");
                        }
                    }
                }
                add_prev($text, $stack, "\@$macro" . $1 . $2);
                #return if (/^\s*$/);
                #s/^\s*//;
            }
            elsif ($macro eq 'unmacro')
            { #FIXME with 'arg_expansion' should it be passed unmodified ?
                delete $macros->{$1} if (s/^\s+(\w+)//);
                return if (/^\s*$/);
                s/^\s*//;
            }
            elsif (exists($macros->{$macro}))
            {
                my $ref = $macros->{$macro}->{'Args'};
                # we remove any space/new line before the argument
                if (s/^\s*{\s*//)
                {
                    $state->{'macro_args'} = [ "" ];
                    $state->{'macro_name'} = $macro;
                    $state->{'macro_depth'} = 1;
                }
                elsif ($#$ref >= 1)
                { # no brace -> no arg
                    $_ = expand_macro ($macro, [], $_, $line_nr, $state);
                    return;
                }
                else
                { # macro with one arg on the line
                    chomp $_;
                    $_ = expand_macro ($macro, [$_], "\n", $line_nr, $state);
                    return;
                }
            }
            elsif ($macro eq ',')
            {# the @, causes problems when `,' separates things (in @node, @ref)
                $_ = "\@m_cedilla" . $_;
            }
            elsif (s/^{//)
            {
                if ($macro eq 'verb')
                {
                    if (/^$/)
                    {
                        echo_error ("verb at end of line", $line_nr);
                        #warn "$ERROR verb at end of line";
                    }
                    else
                    {
                        s/^(.)//;
                        $state->{'verb'} = $1;
                    }
                }
                push (@$stack, { 'style' => $macro, 'text' => '' });
            }
            else
            {
                add_prev($text, $stack, "\@$macro");
            }
            next;
        }
        #elsif(s/^([^{}@]*)\@(.)//o)
        elsif(s/^([^{}@]*)\@([^\s\}\{\@]*)//o)
        {
            # No need to warn here it is done later
            add_prev($text, $stack, $1 . "\@$2");
            next;
        }
        elsif (s/^([^{}]*)([{}])//o)
        {
            add_prev($text, $stack, $1);
            if ($2 eq '{')
            {
                push @$stack, { 'style' => '', 'text' => '' };
            }
            else
            {
                if (@$stack)
                {
                    my $style = pop @$stack;
                    my $result;
                    if (($style->{'style'} ne '') and exists($info_enclose{$style->{'style'}}) and !$state->{'arg_expansion'})
                    {
                         $result = $info_enclose{$style->{'style'}}->[0] . $style->{'text'} . $info_enclose{$style->{'style'}}->[1];      
                    }              
                    elsif ($style->{'style'} ne '')
                    {
                         $result = '@' . $style->{'style'} . '{' . $style->{'text'} . '}';
                    }
                    else
                    {
                        $result = '{' . $style->{'text'};
                        # don't close { if we are closing stack as we are not 
                        # sure this is a licit { ... } construct.
                        $result .= '}' unless ($state->{'close_stack'} or $state->{'arg_expansion'});
                    }
                    add_prev ($text, $stack, $result);
                    #print STDERR "MACRO end $style->{'style'} remaining: $_";
                    next;
                }
                else
                {
                    # we warn in the last pass
                    #echo_error ("'}' without opening '{', before: $_", $line_nr);
                    #warn "$ERROR '}' without opening '{' line: $line";
                    add_prev ($text, $stack, '}');
                }
            }
        }
        else
        {
            #print STDERR "END_LINE $_";
            add_prev($text, $stack, $_);
            last;
        }
    }
    return 1;
}

sub scan_structure($$$$;$)
{
    my $line = shift;
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $line_nr = shift;

    die "stack not an array ref"  unless (ref($stack) eq "ARRAY");
    local $_ = $line;
    #print STDERR "SCAN_STRUCTURE: $line";
    #dump_stack ($text, $stack, $state); 
    if (!$state->{'raw'} and !$state->{'special'} and (!exists($state->{'region_lines'})))
    { 
        if (!$state->{'verb'} and $state->{'menu'} and /^\*/o)
        {
        # new menu entry
            delete ($state->{'after_element'});
            my $menu_line = $_;
            my $node;
            if (/^\*\s+($NODERE)::/)
            {
                $node = $1;
            }
            elsif (/^\*\s+([^:]+):\s*([^\t,\.\n]+)[\t,\.\n]/)
            {
                #$name = $1;
                $node = $2;
            }
            if ($node)
            {
                menu_entry_texi(normalise_node($node), $state, $line_nr);
            }
        }
        if (/\S/ and !no_line($_))
        {
            delete $state->{'after_element'};
        }
        my $next_tag = next_tag($_);
        if ((/^\@(\w+)/o and $Texi2HTML::Config::to_skip{$1}) or (/^\@end\s+(\w+)/o and $Texi2HTML::Config::to_skip{"end $1"}))
        {
            s/^\@$next_tag//;
            $_ = skip ($_, $next_tag, $state);
            return unless (defined($_));
        }
    }

    while(1)
    {
        #
	#print STDERR "WHILE\n";
	#dump_stack($text, $stack, $state);

        # as texinfo 4.5
        # verbatim might begin at another position than beginning
        # of line, and end verbatim might too. To end a verbatim section
        # @end verbatim must have exactly one space between end and verbatim
        # things following end verbatim are not ignored.
        #
        # html might begin at another position than beginning
        # of line, but @end html must begin the line, and have
        # exactly one space. Things following end html are ignored.
        # tex and ignore works like html
        #
        # ifnothtml might begin at another position than beginning
        # of line, and @end  ifnothtml might too, there might be more
        # than one space between @end and ifnothtml but nothing more on 
        # the line.
        # @end itemize, @end ftable works like @end ifnothtml.
        # except that @item on the same line than @end vtable doesn't work
        # 
        # text right after the itemize before an item is displayed.
        # @item might be somewhere in a line. 
        # strangely @item on the same line than @end vtable doesn't work
        # there should be nothing else than a command following @itemize...
        #
        # see more examples in formatting directory

        if ($state->{'raw'} or $state->{'special'}) 
        {
            my $tag = $state->{'raw'};
            $tag = $state->{'special'} unless $tag;
            if (! @$stack or ($stack->[-1]->{'style'} ne $tag))
            {
                print STDERR "Bug: raw or special: $tag but not on top of stack\n";
                print STDERR "line: $_";
                dump_stack($text, $stack, $state);
                exit 1;
            }
            if (s/^(.*?)\@end\s$tag$// or s/^(.*?)\@end\s$tag\s//)
            {
                add_prev ($text, $stack, $1);
                my $style = pop @$stack;
                if ($state->{'special'})
                {
                    delete $state->{'special'};
                    if ($style->{'text'} !~ /^\s*$/)
                    {
                        add_prev ($text, $stack, do_special($style->{'style'}, $style->{'text'}));
                    }
                    
                }
                else
                {
                    my $after_macro = '';
                    $after_macro = ' ' unless (/^\s*$/);
                    add_prev ($text, $stack, $style->{'text'} . "\@end $state->{'raw'}" . $after_macro);
                    delete $state->{'raw'};
                }
                unless (no_line($_))
                {
                    delete ($state->{'after_element'});
                }
                next;
            }
            else
            {
                add_prev ($text, $stack, $_);
                last unless ($state->{'special'}); 
                return;
            }
        }
	
        if (defined($state->{'verb'}))
        {
            my $char = quotemeta($state->{'verb'});
            if (s/^(.*?)$char\}/\}/)
            {
                add_prev($text, $stack, $1 . $state->{'verb'});
                $stack->[-1]->{'text'} = $state->{'verb'} . $stack->[-1]->{'text'};
                delete $state->{'verb'};
                next;
            }
            else
            {
                add_prev($text, $stack, $_);
                last;
            }
        }
	
        unless (no_line($_))
        {
            delete $state->{'after_element'};
        }
        if (s/^([^{}@]*)\@end\s+([a-zA-Z]\w*)//)
        {
            add_prev($text, $stack, $1);
            my $end_tag = $2;
            $state->{'detailmenu'}-- if ($end_tag eq 'detailmenu' and $state->{'detailmenu'});
            next if ($Texi2HTML::Config::to_skip{"end $end_tag"});
            if (defined($state->{'text_macro_stack'}) 
               and @{$state->{'text_macro_stack'}} 
               and ($end_tag eq $state->{'text_macro_stack'}->[-1]))
            {
                pop @{$state->{'text_macro_stack'}};
                if (exists($region_lines{$end_tag}))
                {
                     print STDERR "Bug: end_tag $end_tag ne $state->{'region_lines'}->{'format'}" 
                         if ( $end_tag ne $state->{'region_lines'}->{'format'});
                     $state->{'region_lines'}->{'number'}--;
                     if ($state->{'region_lines'}->{'number'} == 0)
                     {
                         $state->{'after_element'} = 1;
                         delete $state->{'after_element'} unless 
                             ($state->{'region_lines'}->{'after_element'});
                         delete $state->{'region_lines'}->{'number'};
                         delete $state->{'region_lines'}->{'format'};
                         delete $state->{'region_lines'}->{'after_element'};
                         delete $state->{'region_lines'};
                     }
		     #dump_stack($text, $stack, $state); 
                }
                if ($end_tag eq 'menu')
                {
                    add_prev($text, $stack, "\@end $end_tag");
                    $state->{'menu'}--;
                }
                else
                {
			#print STDERR "End $end_tag\n";
			#dump_stack($text, $stack, $state);
                    return if (/^\s*$/);
                }
            }
            elsif ($text_macros{$end_tag})
            {
                echo_error ("\@end $end_tag without corresponding element", $line_nr);
                #warn "$ERROR \@end $end_tag without corresponding element\n";
                #dump_stack($text, $stack, $state);
            }
            else
            {
                if ($end_tag eq $state->{'table_stack'}->[-1])
                {
                    enter_table_index_entry($text, $stack, $state, $line_nr);
                    pop @{$state->{'table_stack'}};
                }
                #add end tag
                add_prev($text, $stack, "\@end $end_tag");
            }
            next;
        }
        #elsif (s/^([^{}@]*)\@([a-zA-Z]\w*|["'~\@\}\{,\.!\?\s\*\-\^`=:\/])//o)
        elsif (s/^([^{}@]*)\@(["'~\@\}\{,\.!\?\s\*\-\^`=:\|\/])//o or s/^([^{}@]*)\@([a-zA-Z]\w*)([\s\{\}\@])/$3/o or s/^([^{}@]*)\@([a-zA-Z]\w*)$//o)
        {
            add_prev($text, $stack, $1);
            my $macro = $2;
            #print STDERR "MACRO $macro\n";
            if ($Texi2HTML::Config::to_skip{$macro})
            {
                 $_ = skip ($_, $macro, $state);
                 return unless (defined($_));
                 next;
            }

            # track variables
            my $value_macro = 1;
            if ($macro eq 'shorttitle' and s/^\s+(.*)$//)
            {
                $value{'_shorttitle'} = substitute_texi_line($1);
            }
            if ($macro eq 'shorttitlepage' and s/^\s+(.*)$//)
            {
                $value{'_shorttitlepage'} = substitute_texi_line($1);
            }
            elsif($macro eq 'setfilename' and s/^\s+(.*)$//)
            {
                $value{'_setfilename'} = substitute_texi_line($1);
            }
            elsif($macro eq 'settitle' and s/^\s+(.*)$//)
            {
                $value{'_settitle'} = substitute_texi_line($1);
            }
            elsif($macro eq 'author' and  s/^\s+(.*)$//)
            {
                 $value{'_author'}   .= substitute_texi_line($1)."\n";
                 push @{$Texi2HTML::THISDOC{'authors'}}, substitute_texi_line($1);
            }
            elsif($macro eq 'subtitle' and s/^\s+(.*)$//)
            {
                 $value{'_subtitle'} .= substitute_texi_line($1)."\n";
                 push @{$Texi2HTML::THISDOC{'subtitles'}}, substitute_texi_line($1);
            }
            elsif($macro eq 'title' and s/^\s+(.*)$//)
            {
                $value{'_title'}    .= substitute_texi_line($1)."\n";
                push @{$Texi2HTML::THISDOC{'titles'}}, substitute_texi_line($1);
            }
            else
            {
                 $value_macro = 0;
            }
            if ($value_macro)
            {
                return if (/^\s*$/);
                next;
            }
            if ($macro =~ /^(\w+?)index/ and ($1 ne 'print') and ($1 ne 'syncode') and ($1 ne 'syn') and ($1 ne 'def') and ($1 ne 'defcode'))
            {
                my $index_prefix = $1;
                if (/^\s+(.*)/)
                {
                    my $key = $1;
                    $_ = substitute_texi_line($_);
                    my $index_entry = enter_index_entry($index_prefix, $line_nr, $key, $state->{'place'}, $state->{'element'}, $state->{'after_element'});
                    if ($index_entry)
                    {
                        add_prev ($text, $stack, "\@$macro" .  $_);
                        last;
                    }
                    elsif (!defined($index_entry))
                    {
                        echo_warn ("Bad index entry: $_", $line_nr);
                        #warn "$WARN Bad index entry: $_";
                    }
                }
                else
                {
                     echo_warn ("empty index entry", $line_nr);
                     #warn "$WARN empty index entry\n";
                }
                return;
            }
            elsif (defined($text_macros{$macro}))
            {
		    #print STDERR "TEXT_MACRO: $macro\n";
                if ($text_macros{$macro} eq 'special')
                {
                     $state->{'special'} = $macro;
                }
                elsif ($text_macros{$macro} eq 'raw')
                {
                    $state->{'raw'} = $macro;
                    #print STDERR "RAW\n";
                }
                elsif ($format_type{$macro} and $format_type{$macro} eq 'menu')
                {
                    $state->{'menu'}++;
                    delete ($state->{'prev_menu_node'});
                    push @{$state->{'text_macro_stack'}}, $macro;
                    #print STDERR "MENU (text_macro_stack: @{$state->{'text_macro_stack'}})\n";
                }
                elsif (exists($region_lines{$macro}))
                {
                    if (exists($state->{'region_lines'}) and ($state->{'region_lines'}->{'format'} ne $macro))
                    {
                         echo_error("\@$macro not allowed within $state->{'region_lines'}->{'format'}", $line_nr);
                         next;
                    }
                    if (!exists($state->{'region_lines'}))
                    {
                         $state->{'region_lines'}->{'format'} = $macro;
                         $state->{'region_lines'}->{'number'} = 1;
                         $state->{'region_lines'}->{'after_element'} = 1 if ($state->{'after_element'});
                    }
                    else
                    {
                         $state->{'region_lines'}->{'number'}++;
                    }
                    push @{$state->{'text_macro_stack'}}, $macro;
                }
                # if it is a raw formatting command or a menu command
                # we must keep it for later
                my $macro_kept;
                if ($state->{'raw'} or ($macro eq 'menu'))
                {
                    add_prev($text, $stack, "\@$macro");
                    $macro_kept = 1;
                }
                if ($state->{'raw'} or $state->{'special'})
                {
                    push @$stack, { 'style' => $macro, 'text' => '' };
                }
                next if $macro_kept;
                #dump_stack ($text, $stack, $state);
                return if (/^\s*$/);
            }
            elsif ($macro eq 'synindex' || $macro eq 'syncodeindex')
            {
                if (s/^\s+(\w+)\s+(\w+)//)
                {
                    my $from = $1;
                    my $to = $2;
                    my $prefix_from = index_name2prefix($from);
                    my $prefix_to = index_name2prefix($to);

                    echo_error ("unknown from index name $from in \@$macro", $line_nr)
                        unless $prefix_from;
                    echo_error ("unknown to index name $to in \@$macro", $line_nr)
                        unless $prefix_to;
                    if ($prefix_from and $prefix_to)
                    {

                        if ($macro eq 'syncodeindex')
                        {
                            $index_properties->{$prefix_to}->{'from_code'}->{$prefix_from} = 1;
                        }
                        else
                        {
                            $index_properties->{$prefix_to}->{'from'}->{$prefix_from} = 1;
                        }
                    }
                }
                else
                {
                    echo_error ("Bad $macro line: $_", $line_nr);
                }
            }
            elsif ($macro eq 'defindex' || $macro eq 'defcodeindex')
            {
                if (s/^\s+(\w+)\s*$//)
                {
                    my $name = $1;
                    $index_properties->{$name}->{'name'} = $name;
                    $index_properties->{$name}->{'code'} = 1 if $macro eq 'defcodeindex';
                }
                else
                {# FIXME makeinfo don't warn ?
                    echo_error ("Bad $macro line: $_", $line_nr);
                }
                return;
            }
            elsif ($macro eq 'documentlanguage')
            {
                if (s/\s+(\w+)//)
                {
                    my $lang = $1;
                    set_document_language($lang, 0, $line_nr) if (!$cmd_line_lang && $lang);
                }
                return if (/^\s*$/);
                s/^\s*//;
            }
            elsif ($macro eq 'documentencoding')
            {
                s/\s+([0-9\w\-]+)//;
                return if (/^\s*$/);
                s/^\s*//;
            }
            elsif ($macro eq 'kbdinputstyle')
            {# FIXME makeinfo ignores that with --html
                if (s/\s+([a-z]+)//)
                {
                    if ($1 eq 'code')
                    {
                        $style_map_ref->{'kbd'} = $style_map_ref->{'code'};
                        $style_map_pre_ref->{'kbd'} = $style_map_pre_ref->{'code'};
                    }
                    elsif ($1 eq 'example')
                    {
                        $style_map_pre_ref->{'kbd'} = $style_map_pre_ref->{'code'};
                    }
                    elsif ($1 ne 'distinct')
                    {
                        echo_error ("Unknown argument for \@$macro: $1", $line_nr);
                    }
                }
                else
                {
                    echo_error ("Bad \@$macro", $line_nr);
                }
                return if (/^\s*$/);
                s/^\s*//;
            }
            elsif ($macro eq 'verbatiminclude')
            {
                s/(.*)//;
                add_prev($text, $stack, "\@$macro" . $1);
                next;
            }
            elsif (defined($Texi2HTML::Config::def_map{$macro}))
            {
                #We must enter the index entries
                my ($prefix, $entry) = get_deff_index($macro, $_);
                enter_index_entry($prefix, $line_nr, $entry, $state->{'place'}, $state->{'element'}, 0) if ($prefix and defined($entry));
                s/(.*)//;
                add_prev($text, $stack, "\@$macro" . $1);
            }
            elsif ($macro =~ /^itemx?$/)
            {
                enter_table_index_entry($text, $stack, $state, $line_nr);
                if ($state->{'table_stack'}->[-1] =~ /^(v|f)table$/)
                {
                    $state->{'item'} = $macro;
                    push @$stack, { 'format' => 'index_item', 'text' => "" };
                }
                else
                {
                    add_prev($text, $stack, "\@$macro");
                }
            }
            elsif ($format_type{$macro} and ($format_type{$macro} eq 'table' or $format_type{$macro} eq 'list'))
            { # We must enter the index entries of (v|f)table thus we track
              # in which table we are
                push @{$state->{'table_stack'}}, $macro;
                add_prev($text, $stack, "\@$macro");
            }
            elsif (s/^{//)
            {
                if ($macro eq 'verb')
                {
                    if (/^$/)
                    {
                        # We allready warned in pass texi
                        #warn "$ERROR verb at end of line";
                    }
                    else
                    {
                        s/^(.)//;
                        $state->{'verb'} = $1;
                    }
                }
                elsif ($macro eq 'footnote' and $Texi2HTML::Config::SEPARATED_FOOTNOTES)
                {
                    $state->{'footnote_element'} = $state->{'element'};
                    $state->{'footnote_place'} = $state->{'place'};
                    $state->{'element'} = $footnote_element;
                    $state->{'place'} = $footnote_element->{'place'};
                }
                push (@$stack, { 'style' => $macro, 'text' => '' });
            }
            else
            {
                add_prev($text, $stack, "\@$macro");
            }
            next;
        }
        #elsif(s/^([^{}@]*)\@(.)//o)
        elsif(s/^([^{}@]*)\@([^\s\}\{\@]*)//o)
        {
            add_prev($text, $stack, $1 . "\@$2");
            next;
        }
        elsif (s/^([^{}]*)([{}])//o)
        {
            add_prev($text, $stack, $1);
            if ($2 eq '{')
            {
                push @$stack, { 'style' => '', 'text' => '' };
            }
            else
            {
                if (@$stack)
                {
                    my $style = pop @$stack;
                    my $result;
                    if ($style->{'style'} eq 'anchor')
                    {
                        my $anchor = $style->{'text'};
                        $anchor = normalise_node($anchor);
                        if ($nodes{$anchor})
                        {
                            echo_error ("Duplicate node for anchor found: $anchor", $line_nr);
                            next;
                        }
                        $anchor_num++;
                        $nodes{$anchor} = { 'anchor' => 1, 'seen' => 1, 'texi' => $anchor, 'id' => 'ANC' . $anchor_num};
                        push @{$state->{'place'}}, $nodes{$anchor};
                    }
                    elsif ($style->{'style'} eq 'footnote')
                    {
                        if ($Texi2HTML::Config::SEPARATED_FOOTNOTES)
                        {
                            $state->{'element'} = $state->{'footnote_element'};
                            $state->{'place'} = $state->{'footnote_place'};
                        }
                    }
                    elsif ($style->{'style'} eq 'math' and $Texi2HTML::Config::L2H)
                    {
                        add_prev ($text, $stack, do_math($style->{'text'}));
                        next;
                    }
                    if (($style->{'style'} eq 'titlefont') and ($style->{'text'} =~ /\S/))
                    {
                        $state->{'element'}->{'titlefont'} = $style->{'text'} unless ((exists($state->{'region_lines'}) and ($state->{'region_lines'}->{'format'} eq 'titlepage')) or defined($state->{'element'}->{'titlefont'})) ;
                    }
                    if ($style->{'style'})
                    {
                         $result = '@' . $style->{'style'} . '{' . $style->{'text'} . '}';
                    }
                    else
                    {
                        $result = '{' . $style->{'text'};
                        # don't close { if we are closing stack as we are not
                        # sure this is a licit { ... } construct.
                        $result .= '}' unless $state->{'close_stack'};
                    }
                    add_prev ($text, $stack, $result);
                    #print STDERR "MACRO end $style->{'style'} remaining: $_";
                    next;
                }
                else
                {
                    # We warn in the last pass
                    #warn "$ERROR '}' without opening '{' line: $line";
                    #echo_error ("'}' without opening '{' line: $line", $line_nr);
                    add_prev ($text, $stack, '}');
                }
            }
        }
        else
        {
            #print STDERR "END_LINE $_";
            add_prev($text, $stack, $_);
            enter_table_index_entry($text, $stack, $state, $line_nr);
            last;
        }
    }
    return 1;
}

sub scan_line($$$$;$)
{
    my $line = shift;
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $line_nr = shift;

    die "stack not an array ref"  unless (ref($stack) eq "ARRAY");
    local $_ = $line;
    #print STDERR "SCAN_LINE: $line";
    #dump_stack($text, $stack,  $state );
    my $new_menu_entry; # true if there is a new menu entry
    my $menu_description_in_format; # true if we are in a menu description 
                                # but in another format section (@table....)
    if (!$state->{'raw'} and !$state->{'verb'} and $state->{'menu'})
    { # new menu entry
        my ($node, $name, $ending);
        if (s/^\*(\s+$NODERE)(::)//o)
        {
            $node = $1;
            $ending = $2;
        }
        elsif (s/^\*(\s+[^:]+):(\s*[^\t,\.\n]+)([\t,\.\n])//o)
        {
            $name = $1;
            $node = $2;
            $ending = $3;
        }
        if ($node)
        {
            my $top_stack = top_stack($stack);
            if ($top_stack and $top_stack->{'format'} and 
                (
                 ($top_stack->{'format'} eq 'menu_description') or
                 ($top_stack->{'format'} eq 'menu') or
                 (($top_stack->{'format'} eq 'preformatted') and (stack_order($stack, 'preformatted', 'menu_comment'))) or
                 ($top_stack->{'format'} eq 'menu_preformatted') or
                 ($top_stack->{'format'} eq 'menu_comment')
                )
               )
            { # we are in a normal menu state.
                close_menu($text, $stack, $state, $line_nr);
                $new_menu_entry = 1;
                $state->{'menu_entry'} = { 'name' => $name, 'node' => $node,
                   'ending' => $ending };
                add_prev ($text, $stack, menu_link($state, $line_nr));
                print STDERR "# New menu entry: $node\n" if ($T2H_DEBUG & $DEBUG_MENU);
                push @$stack, {'format' => 'menu_description', 'text' => ''};
            }
            else
            { # we are within a macro or a format. In that case we use
              # a simplified formatting of menu which should be right whatever
              # the context
                my $menu_entry = $state->{'menu_entry'};
                $state->{'menu_entry'} = { 'name' => $name, 'node' => $node };
                add_prev ($text, $stack, menu_link($state, $line_nr, 1));
                $state->{'menu_entry'} = $menu_entry;
            }
        }
    }
    # we're in a menu entry description
    if ($state->{'menu_entry'} and !$new_menu_entry)
    {
        my $top_stack = top_stack($stack);
        if (/^\s+\S.*$/ or (!$top_stack->{'format'} or ($top_stack->{'format'} ne 'menu_description')))
        { # description continues
            $menu_description_in_format = 1 if ($top_stack->{'format'} and ($top_stack->{'format'} ne 'menu_description'));
            print STDERR "# Description continues\n" if ($T2H_DEBUG & $DEBUG_MENU);
	    #dump_stack ($text, $stack, $state);
        }
        else
        { # enter menu comment after menu entry
            if (!$top_stack->{'format'} or ($top_stack->{'format'} ne 'menu_description'))
            {
                print STDERR "Bug: begin menu comment but previous isn't menu_description\n";
                dump_stack ($text, $stack, $state);
            }
            print STDERR "# Menu comment begins\n" if ($T2H_DEBUG & $DEBUG_MENU);
	    #dump_stack ($text, $stack, $state);
            my $descr = pop(@$stack);
            add_prev ($text, $stack, menu_description($descr->{'text'}, $state));
            delete $state->{'menu_entry'};
            unless (/^\s*\@end\s+menu\b/)
            {
                $state->{'menu_comment'}++;
                push @$stack, {'format' => 'menu_comment', 'text' => ''};
                push @{$state->{'preformatted_stack'}}, {'pre_style' => $Texi2HTML::Config::MENU_PRE_STYLE, 'class' => 'menu-comment' };
                $state->{'preformatted'}++;
                begin_paragraph($stack, $state);
            }
	    #dump_stack ($text, $stack, $state);
        }
    }
    if (($state->{'menu_entry'} and !$menu_description_in_format) or $state->{'raw'} or $state->{'preformatted'}  or $state->{'no_paragraph'} or $state->{'keep_texi'} or $state->{'remove_texi'})
    { # empty lines are left unmodified
        if (/^\s*$/)
        {
             add_prev($text, $stack, $_);
             return;
        }
        else
        {
            my $next_tag = next_tag($_);
            if ($state->{'deff'} and !defined($Texi2HTML::Config::def_map{$next_tag}))
            {
                 begin_deff_item($stack, $state);
            }
        }
    }
    else
    {
        if (/^\s*$/)
        {
            #ignore the line if it just follows a deff
            return if ($state->{'deff'});
            #if a paragraph is open and empty, it is removed
            return if (abort_empty_paragraph ($stack, $state));
                    
            if ($state->{'paragraph'})
            { # An empty line ends a paragraph
                my $new_stack;
                add_prev($text, $stack, &$Texi2HTML::Config::empty_line($_));
                #print STDERR "empty line ends a paragraph\n";
                #dump_stack ($text, $stack, $state);
                # We close the stack, duplicating commands still opened
                $new_stack = close_stack($text, $stack, $state, $line_nr, 1);
                my $paragraph = pop @$stack;
                if (!$paragraph->{'format'} or ($paragraph->{'format'} ne 'paragraph'))
                { # After closing the stack, if no paragraph, it is a bug
                    my $format = "UNDEF";
                    $format = "format $paragraph->{'format'}" if ($paragraph->{'format'});
                    $format = "style $paragraph->{'style'}" if ($paragraph->{'style'});
                    print STDERR "Bug: paragraph closed but no paragraph ($format), line: $_\n";
                    dump_stack ($text, $stack, $state);
                }
                add_prev ($text, $stack, do_paragraph($paragraph->{'text'}, $state));
                # paragraph_macros is a macros stack containing macros 
                # which were open before paragraph end
                $state->{'paragraph_macros'} = $new_stack;
                return;
            }
            else
            {
                $_ =  &$Texi2HTML::Config::empty_line($_);
                #print STDERR "no paragraph, but newline\n";
            }
        }
        else
        {
            my $next_tag = next_tag($_);
            if ($state->{'deff'} and !defined($Texi2HTML::Config::def_map{$next_tag}))
            { # finish opening the deff, as this is not a deff tag, it can't be 
              # a deff macro with x
                begin_deff_item($stack, $state);
            }
            #print STDERR "NEXT_TAG $next_tag\n";
            if (!$state->{'paragraph'} and !no_line($_) and ($next_tag ne 'html'))
            { # index entries, @html and @* don't trigger new paragraph beginning
              # otherwise we begin a new paragraph
                begin_paragraph($stack, $state);
            }
        }
    }
    # an index entry at beginning of line is handled here FIXME why ?
    if (!$state->{'raw'} and !$state->{'verb'} and !$state->{'remove_texi'} and /^\@(\w+?)index\s+(.*)/ and ($1 ne 'print'))
    {
        if ($state->{'keep_texi'})
        {
            add_prev($text, $stack, $_);
        }
        else
        {
            add_prev($text, $stack, do_index_entry_label($state));
        }
        return;
    }

    while(1)
    {
        #print STDERR "WHILE: $_";
        #dump_stack($text, $stack, $state);
        # we're in a raw format (html, tex if !L2H, verbatim)
        if (defined($state->{'raw'})) 
        {
            (dump_stack($text, $stack, $state), die "Bug for raw ($state->{'raw'})") if (! @$stack or ! ($stack->[-1]->{'style'} eq $state->{'raw'}));
            if (s/^(.*?)\@end\s$state->{'raw'}$// or s/^(.*?)\@end\s$state->{'raw'}\s+//)
            # don't protect html, it is done by Texi2HTML::Config::raw if needed
            {
                print STDERR "# end raw $state->{'raw'}\n" if ($T2H_DEBUG & $DEBUG_FORMATS);
                add_prev ($text, $stack, $1);
                my $style = pop @$stack;
                if ($style->{'text'} !~ /^\s*$/)
                {
                    if ($state->{'remove_texi'})
                    {
                        add_prev ($text, $stack, $style->{'text'});
                    }
                    elsif ($state->{'keep_texi'})
                    {
                        add_prev ($text, $stack, $style->{'text'} . "\@end $state->{'raw'}");
                    }
                    else
                    { 
                        add_prev($text, $stack, &$Texi2HTML::Config::raw($style->{'style'}, $style->{'text'}));
                    }
                }
                if (!$state->{'keep_texi'} and !$state->{'remove_texi'})
                {
                    # reopen preformatted if it was interrupted by the raw format
                    # if raw format is html the preformatted wasn't interrupted
                    begin_paragraph($stack, $state) if ($state->{'preformatted'} and ($state->{'raw'} ne 'html')); 
                    delete $state->{'raw'};
                    return if (/^\s*$/);
                }
                delete $state->{'raw'};
                next;
            }
            else
            {
                print STDERR "#within raw $state->{'raw'}:$_" if ($T2H_DEBUG & $DEBUG_FORMATS);
                add_prev ($text, $stack, $_);
                last;
            }
        }
	
        # we are within a @verb
        if (defined($state->{'verb'}))
        {
            my $char = quotemeta($state->{'verb'});
            if (s/^(.*?)$char\}/\}/)
            {
                 if ($state->{'keep_texi'})
                 {
                     add_prev($text, $stack, $1 . $state->{'verb'});
                     $stack->[-1]->{'text'} = $state->{'verb'} . $stack->[-1]->{'text'};
                 }
                 elsif ($state->{'remove_texi'})
                 {
                     add_prev($text, $stack, $1);
                 }
                 else
                 {
                     add_prev($text, $stack, do_text($1, $state));
                 }
                 delete $state->{'verb'};
                 next;
            }
            else
            {
                 add_prev($text, $stack, $_);
                 last;
            }
        }

        # We handle now the end tags 
        if ($state->{'keep_texi'} and s/^([^{}@]*)\@end\s+([a-zA-Z]\w*)//)
        {
            my $end_tag = $2;
            add_prev($text, $stack, $1 . "\@end $end_tag");
            next;
        }
        elsif ($state->{'remove_texi'} and s/^([^{}@]*)\@end\s+([a-zA-Z]\w*)//)
        {
            add_prev($text, $stack, $1);
            next;
        }
	
        if (s/^([^{}@,]*)\@end\s+([a-zA-Z]\w*)\s// or s/([^{}@,]*)^\@end\s+([a-zA-Z]\w*)$//)
        {
            add_prev($text, $stack, do_text($1, $state));
            my $end_tag = $2;
	    #print STDERR "END_MACRO $end_tag\n";
	    #dump_stack ($text, $stack, $state);
            
            # First we test if the stack is not empty.
            # Then we test if the end tag is a format tag.
            # If so, we close the styles.
            # We then close paragraphs and preformatted at top of the stack.
            # We handle the end tag (even when it was not the tag which appears
            # on the top of the stack; in that case we close anything 
            # until that element)
            my $top_stack = top_stack($stack);
            if (!$top_stack)
            {
                echo_error ("\@end $end_tag without corresponding opening element", $line_nr);
                add_prev($text, $stack, "\@end $end_tag");
                next;
            }

            #if (!$top_stack->{'format'})
            #{
            #    echo_error ("waiting for closing of $top_stack->{'style'}, found \@end $end_tag", $line_nr);
            #}
            
            if (!$format_type{$end_tag})
            {
                echo_warn ("Unknown \@end $end_tag", $line_nr);
                #warn "$ERROR Unknown \@end $end_tag\n";
                add_prev($text, $stack, "\@end $end_tag");
                next;
            }

            # we close all the style macros with braces
            my $new_stack = close_stack($text, $stack, $state, $line_nr, 1);
            $top_stack = top_stack($stack);
            if (!$top_stack or (!defined($top_stack->{'format'})))
            {
                echo_error ("\@end $end_tag without corresponding opening element", $line_nr);
                add_prev($text, $stack, "\@end $end_tag");
                next;
            }
            # if the previous format is a paragraph or a preformatted 
            # it is ended 
            if ($top_stack->{'format'} eq 'paragraph')
            {
                my $paragraph = pop @$stack;
                add_prev($text, $stack, do_paragraph($paragraph->{'text'}, $state));
            }
            elsif ($top_stack->{'format'} eq 'preformatted')
            {
                my $paragraph = pop @$stack;
                add_prev($text, $stack, do_preformatted($paragraph->{'text'}, $state));
            }
            
            $state->{'paragraph_macros'} = $new_stack;

            $top_stack = top_stack($stack);
            if (!$top_stack or (!defined($top_stack->{'format'})))
            {
                echo_error ("\@end $end_tag without corresponding opening element", $line_nr);
                add_prev($text, $stack, "\@end $end_tag");
                next;
            }
            # Warn if the format on top of stack is not compatible with the 
            # end tag, and find the end tag.
            unless (
                ($top_stack->{'format'} eq $end_tag)
                or
                ( 
                 ($format_type{$end_tag} eq 'menu') and
                 (
                  ($top_stack->{'format'} eq 'menu_preformatted') or
                  ($top_stack->{'format'} eq 'menu_comment') or
                  ($top_stack->{'format'} eq 'menu_description')
                 )
                ) or
                ( 
                 ($end_tag eq 'multitable') and 
                 (
                  ($top_stack->{'format'} eq 'cell') or
                  ($top_stack->{'format'} eq 'null')
                 )
                ) or
                ( 
                 ($format_type{$end_tag} eq 'list' ) and 
                 ($top_stack->{'format'} eq 'item')
                ) or
                (
                 (
                  ($format_type{$end_tag} eq 'table') and 
                  ($end_tag ne 'multitable')
                 ) and 
                 (
                   ($top_stack->{'format'} eq 'term') or
                   ($top_stack->{'format'} eq 'line') 
                 )
                ) or
                (
                 (defined($Texi2HTML::Config::def_map{$end_tag})) and 
                 ($top_stack->{'format'} eq 'deff_item')
                ) or
                (
                 ($end_tag eq 'row') and 
                 ($top_stack->{'format'} eq 'cell')
                )
               )
            {
                my $waited_format = $top_stack->{'format'};
                $waited_format = $fake_format{$top_stack->{'format'}} if ($format_type{$top_stack->{'format'}} eq 'fake');
                echo_error ("waiting for end of $waited_format, found \@end $end_tag", $line_nr);
                close_stack($text, $stack, $state, $line_nr, undef, $end_tag);
                # an empty preformatted may appear when closing things as
                # when closed, formats reopen the preformatted environment
                # in case there is some text following, but we know it isn't 
                # the case here, thus we can close paragraphs.
                close_paragraph($text, $stack, $state);
                my $new_top_stack = top_stack($stack);
                next unless ($new_top_stack and defined($new_top_stack->{'format'}) and (($new_top_stack->{'format'} eq $end_tag) 
                   or (($format_type{$new_top_stack->{'format'}} eq 'fake') and ($fake_format{$new_top_stack->{'format'}} eq $format_type{$end_tag}))));
            }
            # We should now be able to handle the format
            if (defined($format_type{$end_tag}) and $format_type{$end_tag} ne 'fake')
            {
                end_format($text, $stack, $state, $end_tag, $line_nr);
            }
            else 
            { # this is a fake format, ie a format used internally, inside
              # a real format. We do nothing, hoping the real format will
              # get closed, closing the fake internal formats
		    #print STDERR "FAKE \@end $end_tag\n";
		    #add_prev($text, $stack, "\@end $end_tag");
            }
            next;
        }
        # This is a macro
	#elsif (s/^([^{}@]*)\@([a-zA-Z]\w*|["'~\@\}\{,\.!\?\s\*\-\^`=:\/])//o)
        elsif (s/^([^{},@]*)\@(["'~\@\}\{,\.!\?\s\*\-\^`=:\|\/])//o or s/^([^{}@,]*)\@([a-zA-Z]\w*)([\s\{\}\@])/$3/o or s/^([^{},@]*)\@([a-zA-Z]\w*)$//o)
        {
            add_prev($text, $stack, do_text($1, $state));
            my $macro = $2;
	    #print STDERR "MACRO $macro\n";
	    #dump_stack ($text, $stack, $state);
            # This is a macro added by close_stack to mark paragraph end
            if ($macro eq 'end_paragraph')
            {
                my $top_stack = top_stack($stack);
                if (!$top_stack or !$top_stack->{'format'} 
                    or ($top_stack->{'format'} ne 'paragraph'))
                {
                    print STDERR "Bug: end_paragraph but no paragraph to end\n";
                    dump_stack ($text, $stack, $state);
                    next;
                }
                s/^\s//;
                my $paragraph = pop @$stack;
                add_prev ($text, $stack, do_paragraph($paragraph->{'text'}, $state));
                next;
            }
            # Handle macro added by close_stack to mark preformatted region end
            elsif ($macro eq 'end_preformatted')
            {
                my $top_stack = top_stack($stack);
                if (!$top_stack or !$top_stack->{'format'} 
                    or ($top_stack->{'format'} ne 'preformatted'))
                {
                    print STDERR "Bug: end_preformatted but no preformatted to end\n";
                    dump_stack ($text, $stack, $state);
                    next;
                }
                my $paragraph = pop @$stack;
                s/^\s//;
                add_prev ($text, $stack, do_preformatted($paragraph->{'text'}, $state));
                next;
            }
            if ($macro eq 'sp')
            {
                my ($space1, $sp_number, $space2);
                if (s/^(\s+)(\d+)(\s*)//)
                {
                    $space1 = $1;
                    $sp_number = $2;
                    $space2 = $3;
                }
                elsif (s/(\s*)$//)
                {
                    $space1 = $1;
                    $sp_number = '';
                    $space2 = '';
                }
                else
                {
                    next if ($state->{'remove_texi'});
                    if ($state->{'keep_texi'})
                    {
                        add_prev($text, $stack, "\@$macro");
                        next;
                    }
                    echo_error ("\@$macro needs a numeric arg or no arg", $line_nr);
                    next;
                }
                next if ($state->{'remove_texi'});
                if ($state->{'keep_texi'})
                {
                    add_prev($text, $stack, "\@$macro" . $space1 . $sp_number . $space2);
                    next;
                }
                $sp_number = 1 if ($sp_number eq '');
                add_prev($text, $stack, &$Texi2HTML::Config::sp($sp_number, $state->{'preformatted'}));
                next;
            }
            if ($macro eq 'verbatiminclude')
            {
                if ($state->{'keep_texi'})
                {
                    if (s/(.*)//o)
                    {
                        add_prev($text, $stack, "\@$macro" . $1);
                    }
                    next;
                }
                return undef if ($state->{'remove_texi'});
                if (s/^(\s+)(.*)//o)
                {
                    my $file = locate_include_file($2);
                    if (defined($file))
                    {
                        if (!open(VERBINCLUDE, $file))
                        {
                            warn "$ERROR Can't read file $file: $!\n";
                        }
                        else
                        {
                            my $verb_text = '';
                            while (my $line = <VERBINCLUDE>)
                            {
                                $verb_text .= $line;
                            }
                            add_prev($text, $stack, &$Texi2HTML::Config::raw('verbatim',$verb_text));
                            close VERBINCLUDE;
                        }
                    }
                    else
                    {
                        echo_error ("Can't find $2, skipping", $line_nr);
                        #warn "$ERROR Can't find $1, skipping\n";
                    }
                    return undef;
                }
                else
                {
                    echo_error ("Bad \@$macro line: $_", $line_nr);
                    return;
                } 
            }
                    
            # This is a @macroname{...} construct. We add it on top of stack
            # It will be handled when we encounter the '}'
            if (s/^{//)
            {
                if ($macro eq 'verb')
                {
                    if (/^$/)
                    {
                        # Allready warned 
                        #warn "$ERROR verb at end of line";
                    }
                    else
                    {
                        s/^(.)//;
                        $state->{'verb'} = $1;
                    }
                } #FIXME what to do if remove_texi and anchor/ref/footnote ?
                elsif ($macro eq 'm_cedilla' and !$state->{'keep_texi'})
                {
                    $macro = ',';
                }
                push (@$stack, { 'style' => $macro, 'text' => '', 'arg_nr' => 0 });
                $state->{'no_paragraph'}++ if ($no_paragraph_macro{$macro});
                open_arg($macro, 0, $state);
                push (@{$state->{'style_stack'}}, $macro) if (defined($style_type{$macro}) and (($style_type{$macro} eq 'style') or ($style_type{$macro} eq 'accent')));
                next;
            }

            # special case if we are checking items

            if ($state->{'check_item'} and $macro =~ /^itemx?$/)
            {
                echo_error("\@$macro on \@$state->{'check_item'} line", $line_nr);
                next;
            }

            # if we're keeping texi unmodified we can do it now
            if ($state->{'keep_texi'})
            {
                add_prev($text, $stack, "\@$macro");
                if ($text_macros{$macro} and $text_macros{$macro} eq 'raw')
                {
                    $state->{'raw'} = $macro;
                    push (@$stack, {'style' => $macro, 'text' => ''});
                }
                next;
            }
            # If we are removing texi, the following macros are not removed 
            # as is but modified

            # a raw macro beginning
            if ($text_macros{$macro} and $text_macros{$macro} eq 'raw')
            {
                my $new_stack = close_stack($text, $stack, $state, $line_nr, 1);
                unless ($macro eq 'html')
                { # close paragraph before verbatim (and tex if !L2H)
                    close_paragraph($text, $stack, $state, $line_nr);
                }
                $state->{'paragraph_macros'} = $new_stack;
                $state->{'raw'} = $macro;
                push (@$stack, {'style' => $macro, 'text' => ''});
                return if (/^\s*$/);
                next;
            }
            my $simple_macro = 1;
            # An accent macro
            if (exists($Texi2HTML::Config::accent_map{$macro}))
            {
                if (s/^(\S)//o)
                {
                    add_prev ($text, $stack, do_simple($macro, $1, $state, [ $1 ], $line_nr));
                }
                else
                { # The accent is at end of line
                    add_prev ($text, $stack, do_text($macro, $state));
                }
            }
            # a macro which should be like @macroname{}. We handle it...
            elsif ($things_map_ref->{$macro})
            {
                echo_warn ("$macro requires {}", $line_nr);
                add_prev ($text, $stack, do_simple($macro, '', $state));
            }
            # a macro like @macroname
            elsif (defined($simple_map_ref->{$macro}))
            {
                add_prev($text, $stack, do_simple($macro, '', $state));
            }
            else
            {
                 $simple_macro = 0;
            }
            if ($simple_macro)
            {
                begin_paragraph($stack, $state) if (!$state->{'paragraph'} and $no_line_macros{$macro} and !$state->{'preformatted'} and !$state->{'remove_texi'} and !no_line($_) and !(next_tag($_) eq 'html'));
                next;
            }
            # the following macros are not modified but just ignored 
            # if we are removing texi
            if ($macro =~ /^tex_(\d+)$/o)
            {
                add_prev ($text, $stack, Texi2HTML::LaTeX2HTML::do_tex($1));
                next;
            }
            if ($state->{'remove_texi'})
            {
                 if ((($macro =~ /^(\w+?)index$/) and ($1 ne 'print')) or 
                      ($macro eq 'itemize') or ($macro =~ /^(|v|f)table$/)
                      or ($macro eq 'multitable'))
                 {
                      return;
                 }
                 elsif ($macro eq 'enumerate')
                 {
                      my $spec;
                      ($_, $spec) = parse_enumerate ($_);
                      return if (/^\s*$/);
                      next;
                 }
                 elsif (defined($Texi2HTML::Config::def_map{$macro}))
                 {
                     my ($style, $category, $name, $type, $class, $arguments);
                     ($style, $category, $name, $type, $class, $arguments) = parse_def($macro, $_); 
                     $category = remove_texi($category) if (defined($category));
                     # FIXME -- --- ''... should be protected (not by makeinfo)
                     $name = remove_texi($name) if (defined($name));
                     # FIXME -- --- ''... should be protected (not by makeinfo)
                     $type = remove_texi($type) if (defined($type));
                     # FIXME -- --- ''... should be protected (not by makeinfo)
                     $class = remove_texi($class) if (defined($class));
                     # FIXME -- --- ''... should be protected 
                     $arguments = remove_texi($arguments) if (defined($arguments));
                     add_prev ($text, $stack, &$Texi2HTML::Config::def_line_no_texi($category, $name, $type, $arguments));
                     return;
                }
                next;
            }
            if (($macro =~ /^(\w+?)index$/) and ($1 ne 'print'))
            {
                add_prev($text, $stack, do_index_entry_label($state));
                return;
            }
            # a macro which triggers paragraph closing
            if ($macro eq 'insertcopying')
            {
                if (close_paragraph($text, $stack, $state, $line_nr))
                {
                    $_ = "\@$macro " . $_;
                }
                else
                {
                    add_prev ($text, $stack, do_insertcopying($state));
                    # reopen a preformatted format if it was interrupted by the macro
                    begin_paragraph ($stack, $state) if ($state->{'preformatted'});
                }
                next;
            }
            if ($macro =~ /^itemx?$/)
            {
		    #print STDERR "ITEM: $_";
		    #dump_stack($text, $stack, $state);
                # these functions return true if the context was their own
                my $format;
                if ($format = add_item($text, $stack, $state, $line_nr, $_))
                { # handle lists
                }
                elsif ($format = add_term($text, $stack, $state, $line_nr))
                {# handle table
                }
                elsif ($format = add_line($text, $stack, $state, $line_nr)) 
                {# handle table
                }
                if ($format)
                {
                    if (defined($format->{'appended'}))
                    { 
                        $_ = $format->{'appended'} . ' ' . $_ if ($format->{'appended'} ne '');
                    }
                    if (defined($format->{'command'}))
                    { 
                         open_arg($format->{'command'},0, $state);
                    }
                    next;
                }
                $format = add_row ($text, $stack, $state, $line_nr); # handle multitable
                unless ($format)
                {
                    echo_warn ("\@item outside of table or list", $line_nr);
                    next;
                }
                push @$stack, {'format' => 'row', 'text' => ''};
                if ($format->{'max_columns'})
                {
                    push @$stack, {'format' => 'cell', 'text' => ''};
                    $format->{'cell'} = 1;
                    begin_paragraph($stack, $state) unless (!$state->{'preformatted'} and no_line($_));			
                }
                else
                {
                    echo_warn ("\@$macro in empty multitable", $line_nr);
                }
                next;
            }
            if ($macro eq 'tab')
            {
                my $format = add_cell ($text, $stack, $state);
                #print STDERR "tab, $format->{'cell'}, max $format->{'max_columns'}\n";
                if (!$format)
                {
                    echo_warn ("\@tab outside of multitable", $line_nr);
                    #warn "$WARN \@tab outside of multitable\n";
                }
                elsif (!$format->{'max_columns'})
                {
                    echo_warn ("\@$macro in empty multitable", $line_nr);
                    #warn "$WARN \@$macro in empty multitable\n";
                    push @$stack, {'format' => 'null', 'text' => ''};
                    next;
                }
                elsif ($format->{'cell'} > $format->{'max_columns'})
                {
                    echo_warn ("too much \@$macro (multitable has only $format->{'max_columns'} column(s))", $line_nr);
                    #warn "$WARN cell over table width\n";
                    push @$stack, {'format' => 'null', 'text' => ''};
                    next;
                }
                else
                {
                    push @$stack, {'format' => 'cell', 'text' => ''};
                }
                begin_paragraph($stack, $state) unless (!$state->{'preformatted'} and no_line($_));			
                next;
            }
            # Macro opening a format (table, list, deff, example...)
            if ($format_type{$macro} and ($format_type{$macro} ne 'fake'))
            {
                close_paragraph($text, $stack, $state, $line_nr);
                #print STDERR "begin $macro\n";
                # A deff like macro
                if (defined($Texi2HTML::Config::def_map{$macro}))
                {
                    if ($state->{'deff'} and ("$state->{'deff'}x" eq $macro))
                    {
                         $macro =~ s/x$//o;
                         #print STDERR "DEFx $macro\n";
                    }
                    else
                    {
                         begin_deff_item($stack, $state, 1) if ($state->{'deff'});
                         $macro =~ s/x$//o;
                         #print STDERR "DEF begin $macro\n";
                         push @$stack, { 'format' => $macro, 'text' => '' };
                    }
                    #print STDERR "BEGIN_DEFF $macro\n";
                    #dump_stack ($text, $stack, $state);
                    $state->{'deff'} = $macro;
                    my ($style, $category, $name, $type, $class, $arguments);
                    ($style, $category, $name, $type, $class, $arguments) = parse_def($macro, $_); 
                    # duplicate_state ?
                    $category = substitute_line($category) if (defined($category));
                    # FIXME -- --- ''... should be protected (not by makeinfo)
                    $name = substitute_line($name) if (defined($name));
                    # FIXME -- --- ''... should be protected (not by makeinfo)
                    $type = substitute_line($type) if (defined($type));
                    # FIXME -- --- ''... should be protected (not by makeinfo)
                    $class = substitute_line($class) if (defined($class));
                    # FIXME -- --- ''... should be protected 
                    $arguments = substitute_line($arguments) if (defined($arguments));
                    $category = &$Texi2HTML::Config::definition_category($category, $class, $style);
                    if (! $category) # category cannot be 0
                    {
                        echo_warn("Bad definition line $_", $line_nr);
                        return;
                    }
                    my $index_label = main::do_index_entry_label ($state) if ($name ne '');
                    add_prev ($text, $stack, &$Texi2HTML::Config::def_line($category, $name, $type, $arguments, $index_label));
                    return;
                }
                elsif ($format_type{$macro} eq 'menu')
                {
                    # if we are allready in a menu we must close it first
                    # in order to close the menu comments and entries
                    close_menu($text, $stack, $state, $line_nr);
                    $state->{'menu'}++;
                    push @$stack, { 'format' => $macro, 'text' => '' };
                    if ($state->{'preformatted'})
                    {
                    # Start a fake complex format in order to have a given pre style
                        $state->{'preformatted'}++;
                        push @$stack, { 'format' => 'menu_preformatted', 'text' => '', 'pre_style' => $Texi2HTML::Config::MENU_PRE_STYLE };
                        push @{$state->{'preformatted_stack'}}, {'pre_style' => $Texi2HTML::Config::MENU_PRE_STYLE, 'class' => 'menu-preformatted' };
                    }
                }
                elsif (exists ($Texi2HTML::Config::complex_format_map->{$macro}))
                {
                    $state->{'preformatted'}++;
                    my $format = { 'format' => $macro, 'text' => '', 'pre_style' => $Texi2HTML::Config::complex_format_map->{$macro}->{'pre_style'} };
                    push @{$state->{'preformatted_stack'}}, {'pre_style' =>$Texi2HTML::Config::complex_format_map->{$macro}->{'pre_style'}, 'class' => $macro };
                    push @$stack, $format;
                    begin_paragraph($stack, $state);
                }
                elsif ($Texi2HTML::Config::paragraph_style{$macro})
                {
                    # if there are only spaces after the @center, then the end 
                    # of line has allready been removed and the code triggered
                    # by end of line for @center closing won't be called.
                    # thus we don't open it (opening @center means pushing it
                    # on the paragraph_style stack)
                    next if (($macro eq 'center') and /^\s*$/); 
                    push @{$state->{'paragraph_style'}}, $macro;
                    push (@$stack, { 'format' => $macro, 'text' => '' }) unless ($macro eq 'center');
                    begin_paragraph($stack, $state) unless (!$state->{'preformatted'} and no_line($_));
                }
                elsif (($format_type{$macro} eq 'list') or ($format_type{$macro} eq 'table'))
                {
                    my $format;
		    #print STDERR "BEGIN $macro\n";
		    #dump_stack($text, $stack, $state);
                    if (($macro eq 'itemize') or ($macro =~ /^(|v|f)table$/))
                    {
                        my $command;
                        my $appended;
                        ($appended, $command) = parse_format_command ($_,$macro);
                        $format = { 'format' => $macro, 'text' => '', 'command' => $command, 'appended' => $appended, 'term' => 0 };
                        $_ = '';
                    }
                    elsif ($macro eq 'enumerate')
                    {
                        my $spec;
                        ($_, $spec) = parse_enumerate ($_);
                        $spec = 1 if (!defined($spec)); 
                        $format = { 'format' => $macro, 'text' => '', 'spec' => $spec, 'item_nr' => 0 };
                    }
                    elsif ($macro eq 'multitable')
                    {
                        my $max_columns = parse_multitable ($_, $line_nr);
                        if (!$max_columns)
                        {
                            echo_warn ("empty multitable", $line_nr);
			    #warn "$WARN empty multitable\n";
                            $max_columns = 0;
                        }
                        $format = { 'format' => $macro, 'text' => '', 'max_columns' => $max_columns, 'cell' => 1 };
                    }
                    $format->{'first'} = 1;
                    $format->{'paragraph_number'} = 0;
                    push @$stack, $format;
                    push @{$state->{'format_stack'}}, $format;
                    if ($macro =~ /^(|v|f)table$/)
                    {
                        push @$stack, { 'format' => 'line', 'text' => ''};
                    }
                    elsif ($macro eq 'multitable')
                    {
                        if ($format->{'max_columns'})
                        {
                            push @$stack, { 'format' => 'row', 'text' => ''};
                            push @$stack, { 'format' => 'cell', 'text' => ''};
                        }
                        else 
                        {
                            # multitable without row... We use the special null
                            # format which content is ignored
                            push @$stack, { 'format' => 'null', 'text' => ''};
                            push @$stack, { 'format' => 'null', 'text' => ''};
                        }
                    }
                    if ($format_type{$macro} eq 'list')
                    {
                        push @$stack, { 'format' => 'item', 'text' => ''};
                    }
		    #if (($macro ne 'multitable') or ($format->{'max_columns'}))
                    if ($macro ne 'multitable')
                    {
                        begin_paragraph($stack, $state) unless (!$state->{'preformatted'} and no_line($_));	
                        #dump_stack ($text, $stack, $state);
                    }
		    #dump_stack ($text, $stack, $state);
                    return if ($format_type{$macro} eq 'table' or $macro eq 'itemize');
                }
                # keep this one at the end as there are some other formats
                # which are also in format_map
                elsif (defined($Texi2HTML::Config::format_map{$macro}) or ($format_type{$macro} eq 'cartouche'))
                {
                    push @$stack, { 'format' => $macro, 'text' => '' };
                    begin_paragraph($stack, $state) if ($state->{'preformatted'});
                }
                return if (/^\s*$/);
                next;
            }
            #warn "$WARN Unknown macro `$macro' (left as is)\n";
            $_ = do_unknown ($macro, $_, $text, $stack, $state, $line_nr);
            #echo_warn ("Unknown macro `$macro' (left as is)", $line_nr);
            #add_prev ($text, $stack, do_text("\@$macro"));
            next;
        }
        elsif(s/^([^{}@,]*)\@([^\s\}\{\@]*)//o)
        { # A macro with a character which shouldn't appear in macro name
            add_prev($text, $stack, do_text($1, $state));
            $_ = do_unknown ($2, $_, $text, $stack, $state, $line_nr);
            #add_prev($text, $stack, do_text($1 ."\@$2", $state));
            next;
        }
        elsif (s/^([^{},]*)([{}])//o)
        {
            add_prev($text, $stack, do_text($1, $state));
            if ($2 eq '{')
            {
                if ($state->{'keep_texi'} or $state->{'remove_texi'})
                {
                    add_prev($text, $stack, '{');
                }
                else
                {
                    add_prev($text, $stack, '{');
                    echo_error ("'{' without macro before: $_", $line_nr);
                }
            }
            else
            { # A @macroname{ ...} is closed
                if (@$stack and defined($stack->[-1]->{'style'}))
                {
                    my $style = pop @$stack;
                    my $result;
                    my $macro = $style->{'style'};
                    if (ref($style_map_ref->{$macro}) eq 'HASH')
                    #if (exists($style_args{$macro}))
                    {
                         push (@{$style->{'args'}}, $style->{'text'});
                         $style->{'fulltext'} .= $style->{'text'};
                         my $number = 0;
                         #foreach my $arg(@{$style->{'args'}})
                         #{
                              #print STDERR "  $number: $arg\n";
                         #     $number++;
                         #}
                         $style->{'text'} = $style->{'fulltext'};
                         $state->{'keep_texi'} = 0 if (#$state->{'keep_texi'} 
                             ($style_map_ref->{$macro}->{'args'}->[$style->{'arg_nr'}] eq 'keep') 
                             and ($state->{'keep_nr'} == 1));
                    }
                    $state->{'no_paragraph'}-- if ($no_paragraph_macro{$macro});
                    if ($macro)
                    {
                        $style->{'no_close'} = 1 if ($state->{'no_close'});
                        if ($state->{'keep_texi'})
                        { # don't expand macros in anchor and ref
                            close_arg ($macro, $style->{'arg_nr'}, $state);
                            $result = '@' . $macro . '{' . $style->{'text'} . '}';
                        }
                        else
                        {
                            if ($style_map_ref->{$macro} and !$style->{'no_close'} and (defined($style_type{'$macro'})) and (($style_type{'$macro'} eq 'style') or ($style_type{'$macro'} eq 'accent')))
                            {
                                my $style = pop @{$state->{'style_stack'}};
                                print STDERR "Bug: $style on 'style_stack', not $macro\n" if ($style ne $macro);
                            }
                            $result = do_simple($macro, $style->{'text'}, $state, $style->{'args'}, $line_nr, $style->{'no_open'}, $style->{'no_close'});
                            if ($state->{'code_style'} < 0)
                            {
                                echo_error ("Bug: negative code_style: $state->{'code_style'}, line:$_", $line_nr);
                            }
                        }
                    }
                    else
                    {
                        #$result = $style->{'text'} . '}';
                        print STDERR "Bug: empty style in pass_text\n";
                    }
                    add_prev ($text, $stack, $result);
                    next;
                }
                else
                {
                    echo_error ("'}' without opening '{' before: $_", $line_nr);
                    add_prev ($text, $stack, '}') if ($state->{'keep_texi'});
                }
            }
        }
        elsif (s/^([^,]*)([,])//o)
        {
             add_prev($text, $stack, do_text($1, $state));
             if (@$stack and defined($stack->[-1]->{'style'})
                  and (ref($style_map_ref->{$stack->[-1]->{'style'}}) eq 'HASH'))
             {
                  my $macro = $stack->[-1]->{'style'};
                  my $style_args = $style_map_ref->{$macro}->{'args'};
                  if (exists($style_args->[$stack->[-1]->{'arg_nr'} + 1]))
                  {
                       push (@{$stack->[-1]->{'args'}}, $stack->[-1]->{'text'});
                       $stack->[-1]->{'fulltext'} .= $stack->[-1]->{'text'} . do_text(',', $state);
                       $stack->[-1]->{'text'} = '';
                       close_arg ($macro, $stack->[-1]->{'arg_nr'}, $state);
                       $stack->[-1]->{'arg_nr'}++;
                       open_arg ($macro, $stack->[-1]->{'arg_nr'}, $state);
                       next;
                  }
             }
             add_prev($text, $stack, do_text(',', $state));
        }
        else
        { # no macro nor '}', but normal text
            add_prev($text, $stack, do_text($_, $state));
	    #print STDERR "END LINE: $_";
	    #dump_stack($text, $stack, $state);

            # @item line is closed by end of line
            add_term($text, $stack, $state, $line_nr);
            # FIXME test @center @item and @item @center 
            if ($state->{'paragraph_style'}->[-1] eq 'center')
            {
                close_stack($text, $stack, $state, $line_nr, '');
                my $top_stack = top_stack($stack);

                if (defined($top_stack))
                {
                    if ($top_stack->{'format'} eq 'paragraph')
                    {
                        my $paragraph = pop @$stack;
                        add_prev($text, $stack, do_paragraph($paragraph->{'text'}, $state));
                    }
                    elsif ($top_stack->{'format'} eq 'preformatted')
                    {
                        my $paragraph = pop @$stack;
                        add_prev($text, $stack, do_preformatted($paragraph->{'text'}, $state));
                    }
                }
                pop @{$state->{'paragraph_style'}};
                #$_ = $/ if (chomp($_));
                $_ = '';
                next;
            }
            last;
        }
    }
    return 1;
}

sub open_arg($$$)
{
    my $macro = shift;
    my $arg_nr = shift;
    my $state = shift;
    if (ref($style_map_ref->{$macro}) eq 'HASH')
    {
         my $arg = $style_map_ref->{$macro}->{'args'}->[$arg_nr];
         if ($arg eq 'code' and !$state->{'keep_texi'})
         {
             $state->{'code_style'}++;
         }
         elsif ($arg eq 'keep')
         {
             $state->{'keep_nr'}++;
             $state->{'keep_texi'} = 1;
         }
    }
    elsif ($code_style_map{$macro} and !$state->{'keep_texi'})
    {
         $state->{'code_style'}++;
    }
}

sub close_arg($$$)
{
    my $macro = shift;
    my $arg_nr = shift;
    my $state = shift;
    if (ref($style_map_ref->{$macro}) eq 'HASH')
    {
         my $arg = $style_map_ref->{$macro}->{'args'}->[$arg_nr];
         if ($arg eq 'code' and !$state->{'keep_texi'})
         {
             $state->{'code_style'}--;
         }
         elsif ($arg eq 'keep')
         {
             $state->{'keep_nr'}--;
             $state->{'keep_texi'} = 0 if ($state->{'keep_nr'} == 0);
         }
#print STDERR "c $arg_nr $macro $arg $state->{'code_style'}\n";
    }
    elsif ($code_style_map{$macro} and !$state->{'keep_texi'})
    {
         $state->{'code_style'}--;
    }
}

sub get_value($)
{
    my $value = shift;
    return $value{$value} if ($value{$value});
    return "No value for $value";
} 

sub add_term($$$$;$)
{
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $line_nr = shift;
    my $end = shift;
    return unless (exists ($state->{'format_stack'}));
    my $format = $state->{'format_stack'}->[-1];
    return unless (($format_type{$format->{'format'}} eq 'table') and ($format->{'format'} ne 'multitable' ) and $format->{'term'});
    #print STDERR "ADD_TERM\n";
    # we set 'term' = 0 early such that if we encounter an end of line
    # during close_stack we don't try to do the term once more
    $state->{'format_stack'}->[-1]->{'term'} = 0;
    $format->{'paragraph_number'} = 0;
    # no <pre> allowed in <dt>, thus it is possible there is a @t added
    # to have teletype in preformatted.
    if ($state->{'preformatted'} and $stack->[-1]->{'style'} and ($stack->[-1]->{'style'} eq 't'))
    {
        my $style = pop @$stack;
        add_prev($text, $stack, do_simple($style->{'style'}, $style->{'text'}, $state, [$style->{'text'}]));
    }

    #dump_stack($text, $stack, $state);
    close_stack($text, $stack, $state, $line_nr, undef, 'term');
    my $term = pop @$stack;
    my $command_formatted;
    chomp ($term->{'text'});
    if (exists($style_map_ref->{$format->{'command'}}) and 
       !exists($Texi2HTML::Config::special_list_commands{$format->{'format'}}->{$format->{'command'}}) and ($style_type{$format->{'command'}} eq 'style'))
    {
         $term->{'text'} = do_simple($format->{'command'}, $term->{'text'}, $state, [$term->{'text'}]); 
    }
    elsif (exists($things_map_ref->{$format->{'command'}}))
    {
        $command_formatted = do_simple($format->{'command'}, '', $state);
    }
    my $index_label;
    if ($format->{'format'} =~ /^(f|v)/)
    {
        $index_label = do_index_entry_label($state);
        print STDERR "Bug: no index entry for $text" unless defined($index_label);
    }
    add_prev($text, $stack, &$Texi2HTML::Config::table_item($term->{'text'}, $index_label,$format->{'format'},$format->{'command'}, $command_formatted));
    #add_prev($text, $stack, &$Texi2HTML::Config::table_item($term->{'text'}, $index_entry, $state));
    unless ($end)
    {
        push (@$stack, { 'format' => 'line', 'text' => '' });
        begin_paragraph($stack, $state) if ($state->{'preformatted'});
    }
    return $format;
}

sub add_row($$$$)
{
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $line_nr = shift;
    my $format = $state->{'format_stack'}->[-1];
    return unless ($format->{'format'} eq 'multitable');
    if ($format->{'cell'} > $format->{'max_columns'})
    {
        close_stack($text, $stack, $state, $line_nr, undef, 'null');
        pop @$stack;
    }
    unless ($format->{'max_columns'})
    { # empty multitable
        pop @$stack; # pop 'row'
        return $format;
    }
    if ($format->{'first'})
    { # first row
        $format->{'first'} = 0;
        #dump_stack($text, $stack, $state);
	#if ($stack->[-1]->{'format'} and ($stack->[-1]->{'format'} eq 'paragraph') and ($stack->[-1]->{'text'} =~ /^\s*$/) and ($format->{'cell'} == 1))
        if ($stack->[-1]->{'format'} and ($stack->[-1]->{'format'} eq 'cell') and ($stack->[-1]->{'text'} =~ /^\s*$/) and ($format->{'cell'} == 1))
        {
            pop @$stack;
            pop @$stack;
	    #pop @$stack;
            return $format;
        }
    }
    add_cell($text, $stack, $state);
    my $row = pop @$stack;    
    add_prev($text, $stack, &$Texi2HTML::Config::row($row->{'text'}));
    return $format;
}

sub add_cell($$$$)
{
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $line_nr = shift;
    my $format = $state->{'format_stack'}->[-1];
    return unless ($format->{'format'} eq 'multitable');
    if ($format->{'cell'} <= $format->{'max_columns'})
    {
        close_stack($text, $stack, $state, $line_nr, undef, 'cell');
        my $cell = pop @$stack;
        add_prev($text, $stack, &$Texi2HTML::Config::cell($cell->{'text'}));
        $format->{'cell'}++;
    }
    return $format;
}

sub add_line($$$$;$)
{
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $line_nr = shift;
    my $end = shift;
    my $format = $state->{'format_stack'}->[-1]; 
    return unless ($format_type{$format->{'format'}} eq 'table' and ($format->{'format'} ne 'multitable') and ($format->{'term'} == 0));
    #print STDERR "ADD_LINE\n";
    #dump_stack($text, $stack, $state);
    # as in pre the end of line are kept, we must explicitely abort empty
    # preformatted, close_stack doesn't abort the empty preformatted regions.
    abort_empty_preformatted($stack, $state) if ($format->{'first'});
    close_stack($text, $stack, $state, $line_nr, undef, 'line');
    my $line = pop @$stack;
    $format->{'paragraph_number'} = 0;
    my $first = 0;
    $first = 1 if ($format->{'first'});
    if ($first)
    {
        $format->{'first'} = 0;
        # we must have <dd> or <dt> following <dl> thus we do a 
        # &$Texi2HTML::Config::table_line here too, although it could have been nice to
        # have a normal paragraph.
        add_prev($text, $stack, &$Texi2HTML::Config::table_line($line->{'text'})) if ($line->{'text'} =~ /\S/o);
    }
    else
    {
        add_prev($text, $stack, &$Texi2HTML::Config::table_line($line->{'text'}));
    }
    unless($end)
    {
        push (@$stack, { 'format' => 'term', 'text' => '' });
        # we cannot have a preformatted in table term (no <pre> in <dt>)
        # thus we set teletyped style @t if there is no pre_style
        push (@$stack, { 'style' => 't', 'text' => '' }) if ($state->{'preformatted'} and (!$state->{'preformatted_stack'}->[-1]->{'pre_style'}));
        #push (@$stack, { 'style' => $format->{'command'}, 'text' => $format->{'appended'} });
    }
    $format->{'term'} = 1;
    return $format;
}

sub add_item($$$$;$)
{
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $line_nr = shift;
    my $line = shift;
    my $end = shift;
    my $format = $state->{'format_stack'}->[-1];
    return unless ($format_type{$format->{'format'}} eq 'list');
    #print STDERR "ADD_ITEM: \n";
    # as in pre the end of line are kept, we must explicitely abort empty
    # preformatted, close_stack doesn't do that.
    abort_empty_preformatted($stack, $state) if ($format->{'first'});
    close_stack($text, $stack, $state, $line_nr, undef, 'item');
    $format->{'paragraph_number'} = 0;
    if ($format->{'format'} eq 'enumerate')
    {
         $format->{'number'} = '';
         my $spec = $format->{'spec'};
         $format->{'item_nr'}++;
         if ($spec =~ /^[0-9]$/)
         {
              $format->{'number'} = $spec + $format->{'item_nr'} - 1;
         }
         else
         {
              my $base_letter = ord('a');
              $base_letter = ord('A') if (ucfirst($spec) eq $spec);

              my @letter_ords = decompose(ord($spec) - $base_letter + $format->{'item_nr'} - 1, 26);
              foreach my $ord (@letter_ords)
              {#FIXME? we go directly to 'ba' after 'z', and not 'aa'
               #because 'ba' is 1,0 and 'aa' is 0,0.
                   $format->{'number'} = chr($base_letter + $ord) . $format->{'number'};
              }
         }
    }
    
    #dump_stack ($text, $stack, $state);
    my $item = pop @$stack;
    # the element following ol or ul must be li. Thus even though there
    # is no @item we put a normal item.

    # don't do an item if it is the first and it is empty
    if (!$format->{'first'} or ($item->{'text'} =~ /\S/o))
    {
        my $formatted_command;
        if (defined($format->{'command'}) and exists($things_map_ref->{$format->{'command'}}))
        {
             $formatted_command = do_simple($format->{'command'}, '', $state);
        }
        chomp($item->{'text'});
        add_prev($text, $stack, &$Texi2HTML::Config::list_item($item->{'text'},$format->{'format'},$format->{'command'}, $formatted_command, $format->{'item_nr'}, $format->{'spec'}, $format->{'number'}));
    } 
    if ($format->{'first'})
    {
        $format->{'first'} = 0;
    }

    # Now prepare the new item
    unless($end)
    {
        push (@$stack, { 'format' => 'item', 'text' => '' });
        begin_paragraph($stack, $state) unless (!$state->{'preformatted'} and no_line($line));
    }
    return $format;
}

sub do_simple($$$;$$$$)
{
    my $macro = shift;
    my $text = shift;
    my $state = shift;
    my $args = shift;
    my $line_nr = shift;
    my $no_open = shift;
    my $no_close = shift;
    my $result;
    
    my $arg_nr = 0;
    $arg_nr = @$args - 1 if (defined($args));
    
#print STDERR "DO_SIMPLE $macro $arg_nr $args @$args\n" if (defined($args));
    if (defined($simple_map_ref->{$macro}))
    {
        if ($state->{'keep_texi'})
        {
             return "\@$macro";
        }
        elsif ($state->{'remove_texi'})
        {
#print STDERR "DO_SIMPLE remove_texi $macro\n";
             return  $simple_map_texi_ref->{$macro};
        }
        elsif ($state->{'preformatted'})
        {
             return $simple_map_pre_ref->{$macro};
        }
        else
        {
             return $simple_map_ref->{$macro};
        }
    }
    if (defined($things_map_ref->{$macro}))
    {
        if ($state->{'keep_texi'})
        {
             $result = "\@$macro" . '{}';
        }
        elsif ($state->{'remove_texi'})
        {
             $result =  $texi_map_ref->{$macro};
#print STDERR "DO_SIMPLE remove_texi texi_map $macro\n";
        }
        elsif ($state->{'preformatted'})
        {
             $result = $pre_map_ref->{$macro};
        }
        else 
        {
             $result = $things_map_ref->{$macro};
        }
        return $result . $text;
    }
    elsif (defined($style_map_ref->{$macro}))
    {
        if ($state->{'keep_texi'})
        {
             $result = "\@$macro" . '{' . $text . '}';
        }
        else 
        {
             my $style;
             if ($state->{'remove_texi'})
             {
#print STDERR "REMOVE $macro, $style_map_texi_ref->{$macro}, fun $style_map_texi_ref->{$macro}->{'function'} remove cmd " . \&Texi2HTML::Config::t2h_remove_command . " ascii acc " . \&t2h_default_ascii_accent;
                  $style = $style_map_texi_ref->{$macro};
             }
             elsif ($state->{'preformatted'})
             {
                  $style = $style_map_pre_ref->{$macro};
             }
             else
             {
                  $style = $style_map_ref->{$macro};
             }
             if (defined($style))
             {                           # known style
                  $result = &$Texi2HTML::Config::style($style, $macro, $text, $args, $no_close, $no_open, $line_nr, $state, $state->{'style_stack'});
             }
             if (!$no_close)
             { 
                  close_arg($macro,$arg_nr, $state);
             }
        }
        return $result;
    }
    # Unknown macro
    $result = '';
    my ($done, $result_text, $message) = &$Texi2HTML::Config::unknown_style($macro, $text);
    if ($done)
    {
         echo_warn($message, $line_nr) if (defined($message));
         if (defined($result_text))
         {
             $result = $result_text;
         }
    }
    else 
    { 
        unless ($no_open)
        { # we warn only if no_open is true, i.e. it is the first time we 
          # close the macro for a multiline macro
             echo_warn ("Unknown command with braces `\@$macro'", $line_nr);
             $result = do_text("\@$macro") . "{";
        }
        $result .= $text;
        $result .= '}' unless ($no_close);
    }
    return $result;
}

sub do_unknown($$$$$$)
{
    my $macro = shift;
    my $line = shift;
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $line_nr = shift;
    my ($result_line, $result, $result_text, $message) = &$Texi2HTML::Config::unknown($macro, $line);
    if ($result)
    {
         add_prev ($text, $stack, $result_text) if (defined($result_text));
         echo_warn($message, $line_nr) if (defined($message));
         return $result_line;
    }
    else
    {
         echo_warn ("Unknown command `\@$macro' (left as is)", $line_nr);
         add_prev ($text, $stack, do_text("\@$macro"));
         return $line;
    }
}

sub add_text($@)
{
    my $string = shift;

    return if (!defined($string));
    foreach my $scalar_ref (@_)
    {
        next unless defined($scalar_ref);
        if (!defined($$scalar_ref))
        {
            $$scalar_ref = $string;
        }
        else
        {
            $$scalar_ref .= $string;
        }
        return;
    }
}

sub add_prev ($$;$)
{
    my $text = shift;
    my $stack = shift;
    my $string = shift;

    unless (defined($text) and ref($text) eq "SCALAR")
    {
       die "text not a SCALAR ref: " . ref($text) . "";
    }
    if (!defined($stack) or (ref($stack) ne "ARRAY"))
    {
        $string = $stack;
        $stack = [];
    }
    
    return if (!defined($string));
    if (@$stack)
    {
        $stack->[-1]->{'text'} .= $string;
        return;
    }

    if (!defined($$text))
    {
         $$text = $string;
    }
    else
    {
         $$text .= $string;
    }
}

# close the stack, closing macros and formats left open.
# the precise behavior of the function depends on $close_paragraph:
#  undef   -> close everything
#  defined -> remove empty paragraphs, close until the first format or paragraph.
#      1          -> don't close styles, duplicate stack of styles not closed
#      ''         -> close styles, don't duplicate
# if a $format is given the stack is closed according to $close_paragraph but
# if $format is encountered the closing stops

sub close_stack($$$$;$$)
{
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $line_nr = shift;
    my $close_paragraph = shift;
    my $format = shift;
    my $new_stack;
    
    # abort empty paragraphs
    abort_empty_paragraph($stack, $state) if (defined($close_paragraph));

    # cancel paragraph states
    $state->{'paragraph_style'} = [ '' ] unless (defined($close_paragraph) or defined($format));
    return $new_stack unless (@$stack or $state->{'raw'} or $state->{'macro'} or $state->{'macro_name'} or $state->{'ignored'});
    
    my $stack_level = $#$stack + 1;
    my $string = '';
    my $verb = '';
    
    if ($state->{'ignored'})
    {
        $string .= "\@end $state->{'ignored'} ";
        echo_warn ("closing $state->{'ignored'}", $line_nr); 
    }
    if ($state->{'texi'})
    {
        if ($state->{'macro'})
        {
            $string .= "\@end macro ";
            echo_warn ("closing macro", $line_nr); 
        }
        elsif ($state->{'macro_name'})
        {
            $string .= ('}' x $state->{'macro_depth'}) . " ";
            echo_warn ("closing $state->{'macro_name'} ($state->{'macro_depth'} braces missing)", $line_nr); 
        }
        elsif ($state->{'verb'})
        {
            echo_warn ("closing \@verb", $line_nr); 
            $string .= $state->{'verb'} . '}';
            $verb = $state->{'verb'};
        }
        elsif ($state->{'raw'})
        {
            if (defined($close_paragraph))
            {
                print STDERR "Bug: close_paragraph is true and we're in raw";
            }
            echo_warn ("closing \@$state->{'raw'} raw format", $line_nr);
            $string .= "\@end $state->{'raw'} ";
        }
        if ($string ne '')
        {
            #print STDERR "scan_texi ($string)\n";
            scan_texi ($string, $text, $stack, $state, $line_nr);
            $string = '';
        }
    }
    elsif ($state->{'verb'})
    {
        $string .= $state->{'verb'};
        $verb = $state->{'verb'};
    }

    if ($state->{'texi'} or $state->{'structure'})
    {
        while ($stack_level--)
        {
              my $stack_text = $stack->[$stack_level]->{'text'};
              $stack_text = '' if (!defined($stack_text));
              if ($stack->[$stack_level]->{'format'})
              {
                   my $format = $stack->[$stack_level]->{'format'};
                   if ($format eq 'index_item')
                   {
                        enter_table_index_entry($text, $stack, $state, $line_nr);
                        next;
                   }
                   elsif (!defined($format_type{$format}) or ($format_type{$format} ne 'fake'))
                   {
                        $stack_text = "\@$format\n" . $stack_text;
                   }
              }
              elsif (defined($stack->[$stack_level]->{'style'}))
              {
                  my $style = $stack->[$stack_level]->{'style'};
                  if ($style ne '')
                  {
                       $stack_text = "\@$style\{" . $stack_text;
                  }
                  else
                  {
                       $stack_text = "\{" . $stack_text;
                  }
             }
             pop @$stack;
             add_prev($text, $stack, $stack_text);
        }
        $stack = [ ];
        $stack_level = 0;
        #return ($text, [ ], $state);
    } 

    #debugging
    #my $print_format = 'NO FORMAT';
    #$print_format = $format if ($format);
    #my $print_close_paragraph = 'close everything';
    #$print_close_paragraph = 'close paragraph without duplicating' if (defined($close_paragraph));
    #$print_close_paragraph = $close_paragraph if ($close_paragraph);
    #print STDERR "Close_stack: format $print_format, close_paragraph: $print_close_paragraph\n";
    
    while ($stack_level--)
    {
        if ($stack->[$stack_level]->{'format'})
        {
            my $stack_format = $stack->[$stack_level]->{'format'};
            last if (defined($close_paragraph) or (defined($format) and $stack_format eq $format));
            # We silently close paragraphs, preformatted sections and fake formats
            if ($stack_format eq 'paragraph')
            {
                $string .= "\@end_paragraph ";
            }
            elsif ($stack_format eq 'preformatted')
            {
                $string .= "\@end_preformatted ";
            }
            else
            {
                if ($fake_format{$stack_format})
                {
                    warn "# Closing a fake format `$stack_format'\n" if ($T2H_VERBOSE);
                }
                else
                {
                    echo_warn ("closing `$stack_format'", $line_nr); 
                    #dump_stack ($text, $stack, $state);
                    #warn "$WARN closing `$stack_format'\n"; 
                }
                $string .= "\@end $stack_format ";
            }
        }
        else
        {
            my $style =  $stack->[$stack_level]->{'style'};
            # FIXME images, footnotes, xrefs, anchors with $close_paragraphs ?
            if ($close_paragraph)
            { #duplicate the stack
                if (exists($style_type{$style}) and ($style_type{$style} eq 'style') or (!exists($style_type{$style})))
                {
                    push @$new_stack, { 'style' => $style, 'text' => '', 'no_open' => 1, 'arg_nr' => 0 };
                    $string .= '} ';
                }
                elsif (exists($style_type{$style}) and ($style_type{$style} eq 'simple'))
                {
                    $string .= '} ';
                }
            }
            else
            {
                dump_stack ($text, $stack, $state) if (!defined($style)); 
                $string .= '}';
                echo_warn ("closing $style", $line_nr) if ($style); 
            }
        }
    }
    $state->{'no_close'} = 1 if ($close_paragraph);
    $state->{'close_stack'} = 1;
    if ($string ne '')
    {
        if ($state->{'texi'})
        {
            #print STDERR "scan_texi in close_stack ($string)\n";
            scan_texi ($string, $text, $stack, $state, $line_nr);
        }
        elsif ($state->{'structure'})
        {
            #print STDERR "scan_structure in close_stack ($string)\n";
            scan_structure ($string, $text, $stack, $state, $line_nr);
        }
        else
        {
            #print STDERR "scan_line in CLOSE_STACK ($string)\n";
            #dump_stack ($text, $stack, $state);
            scan_line ($string, $text, $stack, $state, $line_nr);
        }
    }
    delete $state->{'no_close'};
    delete $state->{'close_stack'};
    $state->{'verb'} = $verb if (($verb ne '') and $close_paragraph);
    return $new_stack;
}

# given a stack and a list of formats, return true if the stack contains 
# these formats, first on top
sub stack_order($@)
{
    my $stack = shift;
    my $stack_level = $#$stack + 1;
    while (@_)
    {
        my $format = shift;
        while ($stack_level--)
        {
            if ($stack->[$stack_level]->{'format'})
            {
                if ($stack->[$stack_level]->{'format'} eq $format)
                {
                    $format = undef;
                    last;
                }
                else
                {
                    return 0;
                }
            }
        }
        return 0 if ($format);
    }
    return 1;
}	

sub top_format($)
{
    my $stack = shift;
    my $stack_level = $#$stack + 1;
    while ($stack_level--)
    {
        if ($stack->[$stack_level]->{'format'} and !$fake_format{$stack->[$stack_level]->{'format'}})
        {
             return $stack->[$stack_level];
        }
    }
    return undef;
}

sub close_paragraph($$$;$)
{
    my $text = shift;
    my $stack = shift;
    my $state = shift;
    my $line_nr = shift;
    #my $macro = shift;
    #print STDERR "CLOSE_PARAGRAPH\n";
    #dump_stack($text, $stack, $state);
    my $new_stack = close_stack($text, $stack, $state, $line_nr, 1);
    my $top_stack = top_stack($stack);
    if ($top_stack and !defined($top_stack->{'format'}))
    { #debug
         print STDERR "Bug: no format on top stack\n";
         dump_stack($text, $stack, $state);
    }
    if ($top_stack and ($top_stack->{'format'} eq 'paragraph'))
    {
        my $paragraph = pop @$stack;
        add_prev($text, $stack, do_paragraph($paragraph->{'text'}, $state));
        $state->{'paragraph_macros'} = $new_stack;
        return 1;
	#return "\@$macro ";
    }
    elsif ($top_stack and ($top_stack->{'format'} eq 'preformatted'))
    {
        my $paragraph = pop @$stack;
        add_prev($text, $stack, do_preformatted($paragraph->{'text'}, $state));
        $state->{'paragraph_macros'} = $new_stack;
        return 1;
	#return "\@$macro ";
    }
    return;
}

sub abort_empty_paragraph($$)
{
    my $stack = shift;
    my $state = shift;
    if (@$stack and $stack->[-1]->{'format'} 
       and ($stack->[-1]->{'format'} eq 'paragraph')
       and ($stack->[-1]->{'text'} !~ /\S/))
    {
        pop @$stack;
        delete $state->{'paragraph'};
        $state->{'paragraph_nr'}--;
        return 1;
    }
    return 0;
}

sub abort_empty_preformatted($$)
{
    my $stack = shift;
    my $state = shift;
    if (@$stack and $stack->[-1]->{'format'} 
       and ($stack->[-1]->{'format'} eq 'preformatted')
       and ($stack->[-1]->{'text'} !~ /\S/))
    {
        pop @$stack;
        return 1;
    }
    return 0;
}

# for debugging
sub dump_stack($$$)
{
    my $text = shift;
    my $stack = shift;
    my $state = shift;

    if (defined($$text))
    {
        print STDERR "text: $$text\n";
    }
    else
    {
        print STDERR "text: UNDEF\n";
    }
    print STDERR "state: ";
    foreach my $key (keys(%$state))
    {
        my $value = 'UNDEF';
        $value = $state->{$key} if (defined($state->{$key}));
        print STDERR "$key: $value ";
    }
    print STDERR "\n";
    my $stack_level = $#$stack + 1;
    while ($stack_level--)
    {
        print STDERR " $stack_level-> ";
        foreach my $key (keys(%{$stack->[$stack_level]}))
        {
            my $value = 'UNDEF';
            $value = $stack->[$stack_level]->{$key} if 
                (defined($stack->[$stack_level]->{$key}));
            print STDERR "$key: $value ";
        }
        print STDERR "\n";
    }
    if (defined($state->{'format_stack'}))
    {
        print STDERR "format_stack: ";
        foreach my $format (@{$state->{'format_stack'}})
        {
            print STDERR "$format->{'format'} ";
        }
        print STDERR "\n";
    }
}

# for debugging 
sub print_elements($)
{
    my $elements = shift;
    foreach my $elem(@$elements)
    {
        if ($elem->{'node'})
        {
            print STDERR "node-> $elem ";
        }
        else
        {
            print STDERR "chap=> $elem ";
        }
        foreach my $key (keys(%$elem))
        {
            my $value = "UNDEF";
            $value = $elem->{$key} if (defined($elem->{$key}));
            print STDERR "$key: $value ";
        }
        print STDERR "\n";
    }
}

sub substitute_line($;$)
{
    my $line = shift;
    my $state = shift;
    $state = {} if (!defined($state));
    $state->{'no_paragraph'} = 1;
    return substitute_text($state, $line);
}

sub substitute_text($@)
{
    my $state = shift;
    my @stack = ();
    my $text = '';
    my $result = '';
    if ($state->{'structure'})
    {
        initialise_state_structure($state);
    }
    elsif ($state->{'texi'})
    {
        initialise_state_texi($state);
    }
    else
    {
        initialise_state($state);
    }
    $state->{'spool'} = [];
    #print STDERR "SUBST_TEXT begin\n";
    
    while (@_ or @{$state->{'spool'}})
    {
        my $line;
        if (@{$state->{'spool'}})
        {
             $line = shift @{$state->{'spool'}};
        }
        else
        {
            $line = shift @_;
        }
        next unless (defined($line));
        if ($state->{'structure'})
        {
            scan_structure ($line, \$text, \@stack, $state);
        }
        elsif ($state->{'texi'})
        {
            scan_texi ($line, \$text, \@stack, $state);
        }
        else
        {
            scan_line ($line, \$text, \@stack, $state);
        }
        next if (@stack);
        $result .= $text;
        $text = '';
    }
    # FIXME could we have the line number ?
    close_stack(\$text, \@stack, $state, undef);
    #print STDERR "SUBST_TEXT end\n";
    return $result . $text;
}

sub substitute_texi_line($)
{
    my $text = shift;  
    my @text = substitute_text({'structure' => 1}, $text);
    my @result = ();
    while (@text)
    {
        push @result,  split (/\n/, shift (@text));
    }
    return '' unless (@result);
    my $result = shift @result;
    return $result . "\n" unless (@result);
    foreach my $line (@result)
    {
        chomp $line;
        $result .= ' ' . $line;
    }
    return $result . "\n";
}

sub print_lines($;$)
{
    my ($fh, $lines) = @_;
    $lines = $Texi2HTML::THIS_SECTION unless $lines;
    my @cnt;
    my $cnt;
    for my $line (@$lines)
    {
        print $fh $line;
	if (defined($Texi2HTML::Config::WORDS_IN_PAGE) and ($Texi2HTML::Config::SPLIT eq 'node'))
        {
            @cnt = split(/\W*\s+\W*/, $line);
            $cnt += scalar(@cnt);
        }
    }
    return $cnt;
}

sub do_index_entry_label($)
{
    my $state = shift;
    my $entry = shift @index_labels;
    if (!defined($entry))
    {
        print STDERR "Bug: not enough index entries\n";
        return '';
    }
    
    print STDERR "[(index) $entry->{'entry'} $entry->{'label'}]\n"
        if ($T2H_DEBUG & $DEBUG_INDEX);
    return &$Texi2HTML::Config::index_entry_label ($entry->{'label'}, $state->{'preformatted'}, substitute_line($entry->{'entry'}), $index_properties->{$entry->{'prefix'}}->{'name'}); 
    return '';
}

# decompose a decimal number on a given base. The algorithm looks like
# the division with growing powers (division suivant les puissances
# croissantes) ?
sub decompose($$)
{  
    my $number = shift;
    my $base = shift;
    my @result = ();

    return (0) if ($number == 0);
    my $power = 1;
    my $remaining = $number;

    while ($remaining)
    {
         my $factor = $remaining % ($base ** $power);
         $remaining -= $factor;
         push (@result, $factor / ($base ** ($power - 1)));
         $power++;
    }
    return @result;
}

# main processing is called here
set_document_language('en') unless ($lang_set);
# APA: There's got to be a better way:
$T2H_TODAY = Texi2HTML::I18n::pretty_date($Texi2HTML::Config::LANG);  # like "20 September 1993";
$T2H_USER = &$I('unknown');

if ($Texi2HTML::Config::TEST)
{
    # to generate files similar to reference ones to be able to check for
    # real changes we use these dummy values if -test is given
    $T2H_TODAY = 'a sunny day';
    $T2H_USER = 'a tester';
    $THISPROG = 'texi2html';
    setlocale( LC_ALL, "C" );
} 
else
{ 
    # the eval prevents this from breaking on system which do not have
    # a proper getpwuid implemented
    eval { ($T2H_USER = (getpwuid ($<))[6]) =~ s/,.*//;}; # Who am i
    # APA: Provide Windows NT workaround until getpwuid gets
    # implemented there.
    $T2H_USER = $ENV{'USERNAME'} unless defined $T2H_USER;
}
$things_map_ref->{'today'} = $T2H_TODAY;
$pre_map_ref->{'today'} = $T2H_TODAY;
$texi_map_ref->{'today'} = $T2H_TODAY;

open_file($docu, $texi_line_number);
Texi2HTML::LaTeX2HTML::init($docu_name, $docu_rdir, $T2H_DEBUG & $DEBUG_L2H)
 if ($Texi2HTML::Config::L2H);
pass_texi();
dump_texi(\@lines, 'texi', \@lines_numbers) if ($T2H_DEBUG & $DEBUG_TEXI);
if (defined($Texi2HTML::Config::MACRO_EXPAND))
{
    my @texi_lines = (@first_lines, @lines);
    dump_texi(\@texi_lines, '', undef, $Texi2HTML::Config::MACRO_EXPAND);
}
pass_structure();
if ($T2H_DEBUG & $DEBUG_TEXI)
{
    dump_texi(\@doc_lines, 'first', \@doc_numbers);
    if (defined($Texi2HTML::Config::MACRO_EXPAND and $Texi2HTML::Config::DUMP_TEXI))
    {
        unshift (@doc_lines, @first_lines);
        push (@doc_lines, "\@bye\n");
        dump_texi(\@doc_lines, '', undef, $Texi2HTML::Config::MACRO_EXPAND . ".first");
    }
}
exit(0) if ($Texi2HTML::Config::DUMP_TEXI or defined($Texi2HTML::Config::MACRO_EXPAND));
rearrange_elements();
do_names();
if (@{$region_lines{'documentdescription'}} and (!defined($Texi2HTML::Config::DOCUMENT_DESCRIPTION)))
{
    my $documentdescription = remove_texi(@{$region_lines{'documentdescription'}}); 
    my @documentdescription = split (/\n/, $documentdescription);
    $Texi2HTML::Config::DOCUMENT_DESCRIPTION = shift @documentdescription;
    chomp $Texi2HTML::Config::DOCUMENT_DESCRIPTION;
    foreach my $line (@documentdescription)
    {
        chomp $line;
        $Texi2HTML::Config::DOCUMENT_DESCRIPTION .= ' ' . $line;
    }
}
# do copyright notice inserted in comment at the begining of the files
if (@{$region_lines{'copying'}})
{
    $copying_comment = remove_texi(@{$region_lines{'copying'}});
    while ($copying_comment =~ /-->/) # --> ends an html comment !
    { 
        $copying_comment =~ s/-->/->/go;
    }
    $copying_comment = &$Texi2HTML::Config::comment($copying_comment) . "\n";
}
&$Texi2HTML::Config::toc_body(\@elements_list);
#&$Texi2HTML::Config::toc_body(\@elements_list, $do_contents, $do_scontents);
&$Texi2HTML::Config::css_lines(\@css_import_lines, \@css_rule_lines);
$sec_num = 0;
#$Texi2HTML::Config::L2H = l2h_FinishToLatex() if ($Texi2HTML::Config::L2H);
#$Texi2HTML::Config::L2H = l2h_ToHtml()        if ($Texi2HTML::Config::L2H);
#$Texi2HTML::Config::L2H = l2h_InitFromHtml()  if ($Texi2HTML::Config::L2H);
Texi2HTML::LaTeX2HTML::latex2html();
pass_text();
#do_node_files() if ($Texi2HTML::Config::SPLIT ne 'node' and $Texi2HTML::Config::NODE_FILES);
if ($Texi2HTML::Config::IDX_SUMMARY)
{
    foreach my $entry (keys(%$index_properties))
    {
         my $name = $index_properties->{$entry}->{'name'};
         do_index_summary_file($name) 
            unless ($empty_indices{$name});
    }
}
do_node_files() if ($Texi2HTML::Config::NODE_FILES);
#l2h_FinishFromHtml() if ($Texi2HTML::Config::L2H);
#l2h_Finish() if($Texi2HTML::Config::L2H);
Texi2HTML::LaTeX2HTML::finish();
&$Texi2HTML::Config::finish_out();
print STDERR "# that's all folks\n" if $T2H_VERBOSE;

exit(0);


##############################################################################

# These next few lines are legal in both Perl and nroff.

.00 ;                           # finish .ig

'di			\" finish diversion--previous line must be blank
.nr nl 0-1		\" fake up transition to first page again
.nr % 0			\" start at page 1
'; __END__ ############# From here on it's a standard manual page ############
    .so @mandir@/man1/texi2html.1
