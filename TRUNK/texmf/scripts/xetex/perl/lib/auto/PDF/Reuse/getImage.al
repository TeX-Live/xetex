# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 4793 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/getImage.al)"
##################
#  En bild läses
##################

sub getImage
{  my ($infil, $sidnr, $bildnr, $key) =  @_;
   if (! defined $key)
   {  errLog("Can't find image $bildnr on page $sidnr in file $infil, aborts");
   } 
   
   @skapa = ();
   my ($res, $corr, $nyDel1, $del1, $del2, $nr, $utrad);
   my $fSource = $infil . '_' . $sidnr;
   my $iSource = $fSource . '_' . $bildnr;

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
   {    errLog("$stati[9] ne $form{$fSource}[fID], modification time has changed, aborting");
   }

   if (exists $old{$key})
   {  return $old{$key}; 
   }

   open (INFIL, "<$infil") || errLog("The file $infil couldn't be opened, $!");
   binmode INFIL; 

   #########################################################
   # En bild med referenser kopieras, behandlas och skrivs
   #########################################################

   $nr = ++$objNr;
   $old{$key} = $nr;
   
   $objekt[$nr] = $pos;

   ($del1, $del2) = getKnown(\$form{$fSource}, $key);

   $$del1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;      
   if (defined $$del2)
   {  $utrad = "$nr 0 obj\n<<" . $$del1 . $$del2;
   }
   else
   {  $utrad = "$nr 0 obj " . $$del1;
   }
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

         ($del1, $del2) = getKnown(\$form{$fSource}, $gammal);

         $$del1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;      
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
   return $nr;   
   
}

# end of PDF::Reuse::getImage
1;
