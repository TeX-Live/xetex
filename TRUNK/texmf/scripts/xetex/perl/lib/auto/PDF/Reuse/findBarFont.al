# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3420 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/findBarFont.al)"
##############################################################
# Streckkods fonten lokaliseras och objekten skrivs ev. ut
##############################################################

sub findBarFont()
{  my $Font = 'Bar';
   
   if (exists $font{$Font})              #  Objekt är redan definierat
   {  $aktuellFont[foEXTNAMN]   = $Font;
      $aktuellFont[foREFOBJ]    = $font{$Font}[foREFOBJ];
      $aktuellFont[foINTNAMN]   = $font{$Font}[foINTNAMN];
   }
   else
   {  $objNr++;
      $objekt[$objNr]  = $pos;
      my $encodObj     = $objNr;
      my $fontObjekt   = "$objNr 0 obj\n<< /Type /Encoding\n" .
                         '/Differences [48 /tomt /streck /lstreck]' . "\n>>\nendobj\n";
      $pos += syswrite UTFIL, $fontObjekt;
      my $charProcsObj = createCharProcs();
      $objNr++;
      $objekt[$objNr]  = $pos;
      $fontNr++;
      my $fontAbbr     = 'Ft' . $fontNr; 
      $fontObjekt      = "$objNr 0 obj\n<</Type/Font/Subtype/Type3\n" .
                         '/FontBBox [0 -250 75 2000]' . "\n" .
                         '/FontMatrix [0.001 0 0 0.001 0 0]' . "\n" .
                         "\/CharProcs $charProcsObj 0 R\n" .
                         "\/Encoding $encodObj 0 R\n" .
                         '/FirstChar 48' . "\n" .
                         '/LastChar 50' . "\n" .
                         '/Widths [75 75 75]' . "\n>>\nendobj\n";

      $font{$Font}[foINTNAMN]  = $fontAbbr; 
      $font{$Font}[foREFOBJ]   = $objNr;
      $objRef{$fontAbbr}       = $objNr;
      $objekt[$objNr]          = $pos;
      $aktuellFont[foEXTNAMN]  = $Font;
      $aktuellFont[foREFOBJ]   = $objNr;
      $aktuellFont[foINTNAMN]  = $fontAbbr;
      $pos += syswrite UTFIL, $fontObjekt;
   }
   if (! $pos)
   {  errLog("No output file, you have to call prFile first");
   }
      
   $sidFont{$aktuellFont[foINTNAMN]} = $aktuellFont[foREFOBJ];
}

# end of PDF::Reuse::findBarFont
1;
