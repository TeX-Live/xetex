# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 2799 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prInitVars.al)"
sub prInitVars
{   my $exit = shift;
    $genLowerX    = 0;
    $genLowerY    = 0;
    $genUpperX    = 595,
    $genUpperY    = 842;
    $fontSize     = 12;
    ($utfil, $slutNod, $formCont, $imSeq, 
    $page, $sidObjNr, $interActive, $NamesSaved, $AARootSaved, $AAPageSaved,
    $root, $AcroFormSaved, $id, $ldir, $checkId, $formNr, $imageNr, 
    $filnamn, $interAktivSida, $taInterAkt, $type, $runfil, $checkCs,
    $confuseObj, $compress,$pos, $fontNr, $objNr,
    $defGState, $gSNr, $pattern, $shading, $colorSpace) = '';

    (@kids, @counts, @formBox, @objekt, @parents, @aktuellFont, @skapa,
     @jsfiler, @inits, @bookmarks, @annots) = ();

    ( %resurser,  %objRef, %nyaFunk,%oldObject, %unZipped, 
      %sidFont, %sidXObject, %sidExtGState, %font, %fields, %script,
      %initScript, %sidPattern, %sidShading, %sidColorSpace, %knownToFile,
      %processed, %dummy) = ();

     $stream = '';
     $idTyp  = '';
     $ddir   = '';
     $log    = '';

     if ($exit)
     {  return 1;
     }
   
     ( %form, %image, %fontSource, %intAct) = ();

     return 1;
}

# end of PDF::Reuse::prInitVars
1;
