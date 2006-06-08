# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3336 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prJpeg.al)"
sub prJpeg
{  my ($iFile, $iWidth, $iHeight) = @_;
   my ($iLangd, $namnet, $utrad);
   if (! $pos)                    # If no output is active, it is no use to continue
   {   return undef;
   }
   my $checkidOld = $checkId;
   ($iFile, $checkId) = findGet($iFile, $checkidOld);
   if ($iFile)
   {  $iLangd = (stat($iFile))[7];
      $imageNr++;
      $namnet = 'Im' . $imageNr;
      $objNr++;
      $objekt[$objNr] = $pos;
      open (BILDFIL, "<$iFile") || errLog("Couldn't open $iFile, $!, aborts");
      binmode BILDFIL;
      my $iStream;
      sysread BILDFIL, $iStream, $iLangd;
      $utrad = "$objNr 0 obj\n<</Type/XObject/Subtype/Image/Name/$namnet" .
                "/Width $iWidth /Height $iHeight /BitsPerComponent 8 /Filter/DCTDecode/ColorSpace/DeviceRGB"
                . "/Length $iLangd >>stream\n$iStream\nendstream\nendobj\n";
      close BILDFIL;
      $pos += syswrite UTFIL, $utrad;
      if ($runfil)
      {  $log .= "Cid~$checkId\n";
         $log .= "Jpeg~$iFile~$iWidth~$iHeight\n";
      }
      $objRef{$namnet} = $objNr;
   }
   if (! $pos)
   {  errLog("No output file, you have to call prFile first");
   }
   undef $checkId;
   return $namnet;
}

# end of PDF::Reuse::prJpeg
1;
