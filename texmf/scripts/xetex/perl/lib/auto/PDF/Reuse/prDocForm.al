# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3110 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prDocForm.al)"
############# Ett interaktivt + grafiskt "formulär" ##########

sub prDocForm
{my ($sidnr, $adjust, $effect, $tolerant, $infil, $x, $y, $size, $xsize,
      $ysize, $rotate);
  my $param = shift;
  if (ref($param) eq 'HASH')
  {  $infil    = $param->{'file'};
     $sidnr    = $param->{'page'} || 1;
     $adjust   = $param->{'adjust'} || '';
     $effect   = $param->{'effect'} || 'print';
     $tolerant = $param->{'tolerant'} || '';
     $x        = $param->{'x'} || 0;
     $y        = $param->{'y'} || 0;
     $rotate   = $param->{'rotate'} || 0;
     $size     = $param->{'size'} || 1;
     $xsize    = $param->{'xsize'} || 1;
     $ysize    = $param->{'ysize'} || 1;
  }
  else
  {  $infil    = $param;
     $sidnr    = shift || 1;
     $adjust   = shift || '';
     $effect   = shift || 'print';
     $tolerant = shift || '';
     $x        = shift || 0;
     $y        = shift || 0;
     $rotate   = shift || 0;
     $size     = shift || 1;
     $xsize    = shift || 1;
     $ysize    = shift || 1;
  }
  my $namn;
  my $refNr;
  $type = 'docform';
  my $fSource = $infil . '_' . $sidnr; 
  my $action;
  if (! exists $form{$fSource})
  {  $formNr++;
     $namn = 'Fm' . $formNr;
     $knownToFile{$fSource} = $namn;
     if ($effect eq 'load')
     {  $action = 'load'
     }
     else
     {  $action = 'print'
     }     
     $refNr         = getPage($infil, $sidnr, $action);
     if ($refNr)
     {  $objRef{$namn} = $refNr; 
     }
     else
     {  if ($tolerant)
        {  if ((defined $refNr) && ($refNr eq '0'))   # Sidnumret existerar inte, men ok
           {   $namn = '0';
           }
           else
           {   undef $namn;   # Sidan kan inte användas som form
           }
        }
        elsif (! defined $refNr)
        {  my $mess = "$fSource can't be used as a form. Try e.g. to\n"
                    . "save the file as postscript, and redistill\n";
           errLog($mess);
        }
        else
        {  errLog("File : $infil  Page: $sidnr  doesn't exist");
        }
     }
  }
  else
  {  if (exists $knownToFile{$fSource})
     {   $namn = $knownToFile{$fSource};
     }
     else
     {  $formNr++;
        $namn = 'Fm' . $formNr;
        $knownToFile{$fSource} = $namn; 
     }
     if (exists $objRef{$namn})
     {  $refNr = $objRef{$namn};
     }
     else
     {  if (! $form{$fSource}[fVALID])
        {  my $mess = "$fSource can't be used as a form. Try e.g. to\n"
                    . "concatenate the streams of the page\n";
           if ($tolerant)
           {  cluck $mess;
              undef $namn;
           }
           else
           {  errLog($mess);
           }
        }
        elsif ($effect ne 'load')
        {  $refNr         =  byggForm($infil, $sidnr);
           $objRef{$namn} = $refNr;
        }
     }  
  }
  my @BBox = @{$form{$fSource}[fBBOX]} if ($refNr);
  if (($effect eq 'print') && ($form{$fSource}[fVALID]) && ($refNr))
  {   if ((! defined $interActive)
      && ($sidnr == 1)
      &&  (defined %{$intAct{$fSource}[0]}) )
      {  $interActive = $infil . ' ' . $sidnr;
         $interAktivSida = 1;
      }
      if (! defined $defGState)
      { prDefaultGrState();
      }
      if ($adjust)
      {   $stream .= "q\n";
          $stream .= fillTheForm(@BBox, $adjust);
          $stream .= "\n/Gs0 gs\n";
          $stream .= "/$namn Do\n";
          $stream .= "Q\n";
      }
      elsif (($x) || ($y) || ($rotate) || ($size != 1) 
                  || ($xsize != 1)     || ($ysize != 1))
      {   $stream .= "q\n";
          $stream .= calcMatrix($x, $y, $rotate, $size, 
                               $xsize, $ysize, $BBox[2], $BBox[3]);
          $stream .= "\n/Gs0 gs\n";
          $stream .= "/$namn Do\n";
          $stream .= "Q\n";
      }
      else
      {   $stream .= "\n/Gs0 gs\n";   
          $stream .= "/$namn Do\n";          
      }
      $sidXObject{$namn} = $refNr;
      $sidExtGState{'Gs0'} = $defGState;
  }
  if ($runfil)
  {   $infil = prep($infil); 
      $log .= "Form~$infil~$sidnr~$adjust~$effect~$tolerant";
      $log .= "~$x~$y~$rotate~$size~$xsize~$ysize\n";
  }
  if (! $pos)
  {  errLog("No output file, you have to call prFile first");
  }
  if (($effect ne 'print') && ($effect ne 'add'))
  {  undef $namn;
  }
  if (wantarray)
  {  my $images = 0;
     if (exists $form{$fSource}[fIMAGES])
     {  $images = scalar(@{$form{$fSource}[fIMAGES]});
     } 
     return ($namn, $BBox[0], $BBox[1], $BBox[2], 
             $BBox[3], $images);
  }
  else
  {  return $namn;
  }
}

# end of PDF::Reuse::prDocForm
1;
