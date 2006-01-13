# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 4983 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/extractName.al)"
##############################
# Ett namnobjekt extraheras
##############################

sub extractName
{  my ($infil, $sidnr, $namn) = @_;
   
   my ($res, $del1, $resType, $key, $corr, $formRes, $kids, $nr, $utrad);
   my $del2 = '';
   @skapa = ();
   
   $behandlad{$infil}->{old} = {} 
        unless (defined $behandlad{$infil}->{old});
   $processed{$infil}->{oldObject} = {} 
        unless (defined $processed{$infil}->{oldObject});   
   $processed{$infil}->{unZipped} = {} 
        unless (defined $processed{$infil}->{unZipped});

   *old       = $behandlad{$infil}->{old};
   *oldObject = $processed{$infil}->{oldObject};
   *unZipped  = $processed{$infil}->{unZipped};
   
   my $fSource = $infil . '_' . $sidnr;

   my @stati = stat($infil);

   if ($form{$fSource}[fID] != $stati[9])
   {    errLog("$stati[9] ne $form{$fSource}[fID] aborts");
   }
   if ($checkId) 
   {  if ($checkId ne $stati[9])
      {  my $mess =  "$checkId \<\> $stati[9] \n"
                  . "The Pdf-file $fSource has not the correct modification time. \n"
                  .  "The program is aborted";
         errLog($mess);
      }
      undef $checkId;
    }
    if ($ldir)
    {  $log .= "Cid~$stati[9]\n";
    }

   open (INFIL, "<$infil") || errLog("The file $infil couldn't be opened, aborting $!");
   binmode INFIL;

   #################################
   # Resurserna läses
   #################################

   $formRes = $form{$fSource}->[fRESOURCE];
   
   if ($formRes !~ m'<<.*>>'os)                   # If not a directory, get it
   {   if ($formRes =~ m'\b(\d+)\s{1,2}\d+\s{1,2}R'o)
       {  $key   = $1;
          $formRes = getKnown(\$form{$fSource}, $key);
       }
       else
       {  return undef;
       }
   }
   undef $key;
   while ($formRes =~ m'\/(\w+)\s*\<\<([^>]+)\>\>'osg)
   {   $resType = $1;
       my $str  = $2;
       if ($str =~ m|$namn\s+(\d+)\s{1,2}\d+\s{1,2}R|s)
       {   $key = $1;
           last;
       }
   }
   if (! defined $key)                      # Try to expand the references
   {   my ($str, $del1, $del2);
       while ($formRes =~ m'(\/\w+)\s+(\d+)\s{1,2}\d+\s{1,2}R'ogs)
       { $str .= $1 . ' ';
         ($del1, $del2) = getKnown(\$form{$fSource}, $2);
         my $string =  $$del1;
         $str .= $string . ' ';
       }
       $formRes = $str;
       while ($formRes =~ m'\/(\w+)\s*\<\<([^>]+)\>\>'osg)
       {   $resType = $1;
           my $str  = $2;
           if ($str =~ m|$namn (\d+)\s{1,2}\d+\s{1,2}R|s)
           {   $key = $1;
               last;
           }
       }
       return undef unless $key;
   }
    
   ########################################
   #  Read the top object of the hierarchy
   ########################################

   ($del1, $del2) = getKnown(\$form{$fSource}, $key);

   $objNr++;
   $nr = $objNr;

   if ($resType eq 'Font')
   {  my ($Font, $extNamn);
      if ($$del1 =~ m'/BaseFont\s*/([^\s\/]+)'os)
      {  $extNamn = $1;
         if (! exists $font{$extNamn})
         {  $fontNr++;
            $Font = 'Ft' . $fontNr;
            $font{$extNamn}[foINTNAMN]       = $Font;
            $font{$extNamn}[foORIGINALNR]    = $nr;
            $fontSource{$Font}[foSOURCE]     = $fSource;
            $fontSource{$Font}[foORIGINALNR] = $nr;            
         }
         $font{$extNamn}[foREFOBJ]   = $nr;
         $Font = $font{$extNamn}[foINTNAMN];
         $namn = $Font;
         $objRef{$Font}  = $nr;
      }
      else
      {  errLog("Inconsitency in $fSource, font $namn can't be found, aborting");
      }
   }
   elsif ($resType eq 'ColorSpace')
   {  $colorSpace++;
      $namn = 'Cs' . $colorSpace;
      $objRef{$namn} = $nr;
   }
   elsif ($resType eq 'Pattern')
   {  $pattern++;
      $namn = 'Pt' . $pattern;
      $objRef{$namn} = $nr;
   }
   elsif ($resType eq 'Shading')
   {  $shading++;
      $namn = 'Sh' . $shading;
      $objRef{$namn} = $nr;
   }
   elsif ($resType eq 'ExtGState')
   {  $gSNr++;
      $namn = 'Gs' . $gSNr;
      $objRef{$namn} = $nr;
   }
   elsif ($resType eq 'XObject')
   {  if (defined $form{$fSource}->[0]->{$nr}->[oIMAGENR])
      {  $namn = 'Im' . $form{$fSource}->[0]->{$nr}->[oIMAGENR];
      }
      else
      {  $formNr++;
         $namn = 'Fo' . $formNr;
      }
      
      $objRef{$namn} = $nr;
   }

   $$del1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;

   if (defined $$del2)
   {  $utrad = "$nr 0 obj\n<<" . $$del1 . $$del2;
   }
   else
   {  $utrad = "$nr 0 obj " . $$del1;
   }
   $objekt[$nr] = $pos;
   $pos += syswrite UTFIL, $utrad;

   ##################################
   #  Skriv ut underordnade objekt
   ##################################
 
   while (scalar @skapa)
   {  my @process = @skapa;
      @skapa = ();
      for (@process)
      {  my $gammal = $$_[0];
         my $ny     = $$_[1];
         
         ($del1, $del2, $kids) = getKnown(\$form{$fSource}, $gammal);
         
         $$del1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs
                                     unless (! defined $kids);      
         if (defined $$del2)
         {  $utrad = "$ny 0 obj\n<<" . $$del1 . $$del2;
         }
         else
         {  $utrad = "$ny 0 obj " . $$del1;
         }
         $objekt[$ny] = $pos;
         $pos += syswrite UTFIL, $utrad;
      }
   }
   close INFIL;
   
   return $namn;   
 
}

# end of PDF::Reuse::extractName
1;
