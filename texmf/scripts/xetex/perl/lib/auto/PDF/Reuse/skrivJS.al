# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 5866 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/skrivJS.al)"
sub skrivJS
{  my $kod = shift;
   my $obj;
   if (($compress) && (length($kod) > 99))
   {  $objNr++;
      $objekt[$objNr] = $pos;
      my $spar = $objNr;
      $kod = compress($kod);
      my $langd = length($kod);
      $obj = "$objNr 0 obj<</Filter/FlateDecode"
                           .  "/Length $langd>>stream\n" . $kod 
                           .  "\nendstream\nendobj\n";
      $pos += syswrite UTFIL, $obj;
      $objNr++;
      $objekt[$objNr] = $pos;
      $obj = "$objNr 0 obj<</S/JavaScript/JS $spar 0 R >>endobj\n";
   }
   else
   {  $kod =~ s'\('\\('gso;
      $kod =~ s'\)'\\)'gso;
      $objNr++;
      $objekt[$objNr] = $pos;
      $obj = "$objNr 0 obj<</S/JavaScript/JS " . '(' . $kod . ')';
      $obj .= ">>endobj\n";
   }
   $pos += syswrite UTFIL, $obj;           
   return $objNr;
}

# end of PDF::Reuse::skrivJS
1;
