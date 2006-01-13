# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 2835 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prImage.al)"
####################
# Behandla en bild
####################

sub prImage
{ my $param = shift;
  my ($infil, $sidnr, $bildnr, $effect, $adjust, $x, $y, $size, $xsize,
      $ysize, $rotate);

  if (ref($param) eq 'HASH')
  {  $infil  = $param->{'file'};
     $sidnr  = $param->{'page'} || 1;
     $bildnr = $param->{'imageNo'} || 1;
     $effect = $param->{'effect'} || 'print';
     $adjust = $param->{'adjust'} || '';
     $x      = $param->{'x'} || 0;
     $y      = $param->{'y'} || 0;
     $rotate = $param->{'rotate'} || 0;
     $size   = $param->{'size'} || 1;
     $xsize  = $param->{'xsize'} || 1;
     $ysize  = $param->{'ysize'} || 1;
  }
  else
  {  $infil  = $param;
     $sidnr  = shift || 1;
     $bildnr = shift || 1;
     $effect = shift || 'print';
     $adjust = shift || '';
     $x      = shift || 0;
     $y      = shift || 0;
     $rotate = shift || 0;
     $size   = shift || 1;
     $xsize  = shift || 1;
     $ysize  = shift || 1;
  }

  my ($refNr, $inamn, $bildIndex, $xc, $yc, $xs, $ys);
  $type = 'image';
  
  $bildIndex = $bildnr - 1;
  my $fSource = $infil . '_' . $sidnr;
  my $iSource = $fSource . '_' . $bildnr;
  if (! exists $image{$iSource})
  {  $imageNr++;
     $inamn = 'Im' . $imageNr;
     $knownToFile{'Im:' . $iSource} = $inamn;
     $image{$iSource}[imXPOS]   = 0;
     $image{$iSource}[imYPOS]   = 0;
     $image{$iSource}[imXSCALE] = 1;
     $image{$iSource}[imYSCALE] = 1;
     if (! exists $form{$fSource} )
     {  $refNr = getPage($infil, $sidnr, '');
        if ($refNr)
        {  $formNr++;
           my $namn = 'Fm' . $formNr;
           $knownToFile{$fSource} = $namn;
        }
        elsif ($refNr eq '0')
        {  errLog("File: $infil  Page: $sidnr can't be found");
        }          
     }
     my $in = $form{$fSource}[fIMAGES][$bildIndex];
     $image{$iSource}[imWIDTH]  = $form{$fSource}->[fOBJ]->{$in}->[oWIDTH];
     $image{$iSource}[imHEIGHT] = $form{$fSource}->[fOBJ]->{$in}->[oHEIGHT];
     $image{$iSource}[imIMAGENO] = $form{$fSource}[fIMAGES][$bildIndex];
  }
  if (exists $knownToFile{'Im:' . $iSource})
  {   $inamn = $knownToFile{'Im:' . $iSource};
  }
  else
  {   $imageNr++;
      $inamn = 'Im' . $imageNr;
      $knownToFile{'Im:' . $iSource} = $inamn;
  }
  if (! exists $objRef{$inamn})         
  {  $refNr = getImage($infil,  $sidnr, 
                       $bildnr, $image{$iSource}[imIMAGENO]);
     $objRef{$inamn} = $refNr;
  }
  else
  {   $refNr = $objRef{$inamn};
  }
     
  my @iData = @{$image{$iSource}};

  if (($effect eq 'print') && ($refNr))
  {  if (! defined  $defGState)
     { prDefaultGrState();}
     $stream .= "\n/Gs0 gs\n";
     $stream .= "q\n";
     
     if ($adjust)
     {  $stream .= fillTheForm(0, 0, $iData[imWIDTH], $iData[imHEIGHT],$adjust);        
     }
     else
     {   my $tX     = ($x + $iData[imXPOS]);
         my $tY     = ($y + $iData[imYPOS]);
         $stream .= calcMatrix($tX, $tY, $rotate, $size, 
                               $xsize, $ysize, $iData[imWIDTH], $iData[imHEIGHT]);
     }
     $stream .= "$iData[imWIDTH] 0 0 $iData[imHEIGHT] 0 0 cm\n";
     $stream .= "/$inamn Do\n";
     $sidXObject{$inamn} = $refNr;
     $stream .= "Q\n";
     $sidExtGState{'Gs0'} = $defGState;
  }
  if ($runfil)
  {  $infil = prep($infil);
     $log .= "Image~$infil~$sidnr~$bildnr~$effect~$adjust";
     $log .= "$x~$y~$size~$xsize~$ysize~$rotate\n";
  }
  if (! $pos)
  {  errLog("No output file, you have to call prFile first");
  }

  if (wantarray)
  {   return ($inamn, $iData[imWIDTH], $iData[imHEIGHT]);
  }
  else
  {   return $inamn;
  }
}

# end of PDF::Reuse::prImage
1;
