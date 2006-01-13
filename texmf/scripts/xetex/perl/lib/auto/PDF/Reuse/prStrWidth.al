# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 2590 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prStrWidth.al)"
sub prStrWidth 
{  require PDF::Reuse::Util;
   my $string   = shift;
   my $Font     = shift;
   my $FontSize = shift || $fontSize;
   my $w = 0;
    
  if (! $Font)
  {   if (! $aktuellFont[foEXTNAMN])
      {  findFont();
      }
      $Font = $aktuellFont[foEXTNAMN];
  }

  if (! exists $PDF::Reuse::Util::font_widths{$Font})
  {  if (exists $stdFont{$Font})
     {  $Font = $stdFont{$Font};
     }
     if (! exists $PDF::Reuse::Util::font_widths{$Font})
     {   $Font = 'Helvetica';
     }
  }
  
  if (ref($PDF::Reuse::Util::font_widths{$Font}) eq 'ARRAY')
  {   my @font_table = @{ $PDF::Reuse::Util::font_widths{$Font} };
      for (unpack ("C*", $string)) 
      {  $w += $font_table[$_];	
      }
  }
  else
  {   $w = length($string) * $PDF::Reuse::Util::font_widths{$Font};
  }
  $w = $w / 1000 * $FontSize;
  
  return $w;
}

# end of PDF::Reuse::prStrWidth
1;
