# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 4880 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/AcroFormsEtc.al)"
##############################################################
#  Interaktiva funktioner knutna till ett formulär återskapas
##############################################################

sub AcroFormsEtc
{  my ($infil, $sidnr) =  @_;
   
   my ($Names, $AARoot, $AAPage, $AcroForm);
   @skapa = ();
   
   my ($res, $corr, $nyDel1, @objData, $del1, $del2, $utrad);
   my $fSource = $infil . '_' . $sidnr;

   $behandlad{$infil}->{old} = {} 
        unless (defined $behandlad{$infil}->{old});
   $processed{$infil}->{oldObject} = {} 
        unless (defined $processed{$infil}->{oldObject});   
   $processed{$infil}->{unZipped} = {} 
        unless (defined $processed{$infil}->{unZipped});   

   *old       = $behandlad{$infil}->{old};
   *oldObject = $processed{$infil}->{oldObject};
   *unZipped  = $processed{$infil}->{unZipped};
        
   my @stati = stat($infil);
   if ($form{$fSource}[fID] != $stati[9])
   {    print "$stati[9] ne $form{$fSource}[fID]\n";
        errLog("Modification time for $fSource has changed, aborting");
   }
    
   open (INFIL, "<$infil") || errLog("The file $infil couldn't be opened, aborting $!");
   binmode INFIL;

   my $fdSidnr = $intAct{$fSource}[iSTARTSIDA];
   $old{$fdSidnr} = $sidObjNr;

   if (($intAct{$fSource}[iNAMES]) ||(scalar @jsfiler) || (scalar @inits) || (scalar %fields))
   {  $Names  = behandlaNames($intAct{$fSource}[iNAMES], $fSource);
   }
   
   ##################################
   # Referenser behandlas och skrivs
   ##################################
         
   if (defined $intAct{$fSource}[iACROFORM])
   {   $AcroForm = $intAct{$fSource}[iACROFORM];
       $AcroForm =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
   }
   if (defined $intAct{$fSource}[iAAROOT])
   {  $AARoot = $intAct{$fSource}[iAAROOT];
      $AARoot =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
   }
   
   if (defined $intAct{$fSource}[iAAPAGE])
   {   $AAPage = $intAct{$fSource}[iAAPAGE];
       $AAPage =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
   }
   if (defined $intAct{$fSource}[iANNOTS])
   {  for (@{$intAct{$fSource}[iANNOTS]})
      {  push @annots, quickxform($_);
      }
   }

   ##################################
   #  Skriv ut underordnade objekt
   ################################## 
   while (scalar @skapa)
   {  my @process = @skapa;
      @skapa = ();
      for (@process)
      {  my $gammal = $$_[0];
         my $ny     = $$_[1];
         
         my $oD   = \@{$intAct{$fSource}[0]->{$gammal}};
         @objData = @{$$oD[oNR]};

         if (defined $$oD[oSTREAMP])
         {  $res = sysseek INFIL, ($objData[0] + $$oD[oPOS]), 0;
            $corr = sysread INFIL, $del1, ($$oD[oSTREAMP] - $$oD[oPOS]) ;
            if (defined  $$oD[oKIDS]) 
            {   $del1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
            }
            $res = sysread INFIL, $del2, ($objData[1] - $corr); 
            $utrad = "$ny 0 obj\n<<" . $del1 . $del2;
         }
         else
         {  $del1 = getObject($gammal);
            $del1 = substr($del1, $$oD[oPOS]);
            if (defined  $$oD[oKIDS])
            {   $del1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
            }
            $utrad = "$ny 0 obj " . $del1;
         }

         $objekt[$ny] = $pos;
         $pos += syswrite UTFIL, $utrad;
      }
   }
    
   close INFIL;
   return ($Names, $AARoot, $AAPage, $AcroForm);
} 

# end of PDF::Reuse::AcroFormsEtc
1;
