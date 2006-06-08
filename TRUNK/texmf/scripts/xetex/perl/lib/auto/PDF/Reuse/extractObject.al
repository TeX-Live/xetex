# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 5176 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/extractObject.al)"
########################
# Ett objekt extraheras
########################

sub extractObject
{  no warnings;
   my ($infil, $sidnr, $key, $typ) = @_;
   
   my ($res, $del1, $corr, $namn, $kids, $nr, $utrad);
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
       my $indata = prep($infil);
       $log .= "Form~$indata~$sidnr~~load~1\n";
    }

   open (INFIL, "<$infil") || errLog("The file $infil couldn't be opened, aborting $!");
   binmode INFIL;

   ########################################
   #  Read the top object of the hierarchy
   ########################################

   ($del1, $del2, $kids) = getKnown(\$form{$fSource}, $key);
        
   if (exists $old{$key})
   {  $nr = $old{$key}; }
   else
   {  $old{$key} = ++$objNr;
      $nr = $objNr;
   }     
 
   if ($typ eq 'Font')
   {  my ($Font, $extNamn);
      if ($$del1 =~ m'/BaseFont\s*/([^\s\/]+)'os)
      {  $extNamn = $1;
         $fontNr++;
         $Font = 'Ft' . $fontNr;
         $font{$extNamn}[foINTNAMN]    = $Font;
         $font{$extNamn}[foORIGINALNR] = $key;
         if ( ! defined $fontSource{$extNamn}[foSOURCE])
         {  $fontSource{$extNamn}[foSOURCE]     = $fSource;
            $fontSource{$extNamn}[foORIGINALNR] = $key;            
         }
         $font{$extNamn}[foREFOBJ]   = $nr;
         $Font = $font{$extNamn}[foINTNAMN];
         $namn = $Font;
         $objRef{$Font}  = $nr;
      }
      else
      {  errLog("Error in $fSource, $key is not a font, aborting");
      }
   }
   elsif ($typ eq 'ColorSpace')
   {  $colorSpace++;
      $namn = 'Cs' . $colorSpace;
      $objRef{$namn} = $nr;
   }
   elsif ($typ eq 'Pattern')
   {  $pattern++;
      $namn = 'Pt' . $pattern;
      $objRef{$namn} = $nr;
   }
   elsif ($typ eq 'Shading')
   {  $shading++;
      $namn = 'Sh' . $shading;
      $objRef{$namn} = $nr;
   }
   elsif ($typ eq 'ExtGState')
   {  $gSNr++;
      $namn = 'Gs' . $gSNr;
      $objRef{$namn} = $nr;
   }
   elsif ($typ eq 'XObject')
   {  if (defined $form{$fSource}->[0]->{$nr}->[oIMAGENR])
      {  $namn = 'Im' . $form{$fSource}->[0]->{$nr}->[oIMAGENR];
      }
      else
      {  $formNr++;
         $namn = 'Fo' . $formNr;
      }
      
      $objRef{$namn} = $nr;
   }

   $$del1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs 
                                  unless (! defined $kids);      
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
                                 unless (! defined  $kids);
       
         if (defined $$del2)          
         {  $utrad = "$ny 0 obj<<" . $$del1 . $$del2;
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

# end of PDF::Reuse::extractObject
1;
