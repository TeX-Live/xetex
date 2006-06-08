# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 5333 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/analysera.al)"
##########################################
# En fil analyseras och sidorna kopieras
##########################################

sub analysera
{  my $infil = shift;
   my $from  = shift || 1;
   my $to    = shift || 0;
   my ($i, $res, @underObjekt, @sidObj, $vektor, $resources, $valid,
       $strPos, $sidor, $filId, $Root, $del1, $del2, $utrad);

   my $extraherade = 0;
   my $sidAcc = 0;
   @skapa     = ();
  
   $behandlad{$infil}->{old} = {}
        unless (defined $behandlad{$infil}->{old});
   $processed{$infil}->{oldObject} = {} 
        unless (defined $processed{$infil}->{oldObject});   
   $processed{$infil}->{unZipped} = {} 
        unless (defined $processed{$infil}->{unZipped});
   *old       = $behandlad{$infil}->{old};
   *oldObject = $processed{$infil}->{oldObject};
   *unZipped  = $processed{$infil}->{unZipped};

   $root      = (exists $processed{$infil}->{root}) 
                    ? $processed{$infil}->{root} : 0;
             
   my ($AcroForm, $Annots, $Names, $AARoot);
   undef $taInterAkt;
   undef %script;
   
   my $checkIdOld = $checkId;
   ($infil, $checkId) = findGet($infil, $checkIdOld);
   if (($ldir) && ($checkId) && ($checkId ne $checkIdOld))
   {  $log .= "Cid~$checkId\n";
   }
   undef $checkId;
   my @stati = stat($infil);
   open (INFIL, "<$infil") || errLog("Couldn't open $infil,aborting.  $!");
   binmode INFIL;
  
   if (! $root)
   {  $root      = xRefs($stati[7], $infil);
   }   
   #############
   # Hitta root
   #############           

   my $offSet;
   my $bytes;
   my $objektet = getObject($root);
   
   if ((! $interActive) && ( ! $to) && ($from == 1))
   {  if ($objektet =~ m'/AcroForm(\s+\d+\s{1,2}\d+\s{1,2}R)'so)
      {  $AcroForm = $1;
      }      
      if ($objektet =~ m'/Names\s+(\d+)\s{1,2}\d+\s{1,2}R'so)
      {  $Names = $1;
      }
      if ((scalar %fields) || (scalar @jsfiler) || (scalar @inits))
      {   $Names  = behandlaNames($Names);
      }
      elsif ($Names)
      {  $Names = quickxform($Names);
      }

      #################################################
      #  Finns ett dictionary för Additional Actions ?
      #################################################
      if ($objektet =~ m'/AA(\s+\d+\s{1,2}\d+\s{1,2}R)'os)   # Hänvisning
      {  $AARoot = $1; }
      elsif ($objektet =~ m'/AA\s*\<\<\s*[^\>]+[^\>]+'so) # AA är ett dictionary
      {  my $k;
         my ($dummy, $obj) = split /\/AA/, $objektet;
         $obj =~ s/\<\</\#\<\</gs;
         $obj =~ s/\>\>/\>\>\#/gs;
         my @ord = split /\#/, $obj;
         for ($i = 0; $i <= $#ord; $i++)
         {   $AARoot .= $ord[$i];
             if ($ord[$i] =~ m'\S+'os)
             {  if ($ord[$i] =~ m'<<'os)
                {  $k++; }
                if ($ord[$i] =~ m'>>'os)
                {  $k--; }
                if ($k == 0)
                {  last; }
             } 
          }
       }
       $taInterAkt = 1;   # Flagga att ta med interaktiva funktioner
   } 
   
   #
   # Hitta pages
   #
 
   if ($objektet =~ m'/Pages\s+(\d+)\s{1,2}\d+\s{1,2}R'os)
   {  $objektet = getObject($1);
      ($resources, $valid) = kolla($objektet);
      if ($objektet =~ m'/Count\s+(\d+)'os)
      {  $sidor = $1; }   
   }
   else
   { errLog("Didn't find pages "); }

   my @levels;
   my $li = -1;

   if ($objektet =~ m'/Kids\s*\[([^\]]+)'os)
   {  $vektor = $1;  
      while ($vektor =~ m'(\d+)\s{1,2}\d+\s{1,2}R'go)
      {   push @sidObj, $1;       
      }
      $li++;
      $levels[$li] = \@sidObj;
   }

   while (($li > -1) && ($sidAcc < $sidor))
   {  if (scalar @{$levels[$li]})
      {   my $j = shift @{$levels[$li]};
          $objektet = getObject($j);
          ($resources, $valid) = kolla($objektet, $resources); 
          if ($objektet =~ m'/Kids\s*\[([^\]]+)'os)
          {  $vektor = $1; 
             my @sObj; 
             while ($vektor =~ m'(\d+)\s{1,2}\d+\s{1,2}R'go)
             {   push @sObj, $1;       
             }
             $li++;
             $levels[$li] = \@sObj;
          }
          else
          {  $sidAcc++;
             if ($sidAcc >= $from)
             {   if ($to)
                 {  if ($sidAcc <= $to)
                    {  sidAnalys($j, $objektet, $resources);
                       $extraherade++;
                       $sida++;
                    }
                    else
                    {  $sidAcc = $sidor;
                    }
                 }
                 else
                 {  sidAnalys($j, $objektet, $resources);
                    $extraherade++;
                    $sida++;
                 }
              }
          }
      }
      else
      {  $li--;
      }
   }
   
   if (defined $AcroForm)
   {  $AcroForm =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
   }
   if (defined $AARoot)
   {  $AARoot =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
   }
   
   while (scalar @skapa)
   {  my @process = @skapa;
      @skapa = ();
      for (@process)
      {  my $gammal = $$_[0];
         my $ny     = $$_[1];
         $objektet  = getObject($gammal);         

         if ($objektet =~ m'^(\d+ \d+ obj\s*<<)(.+)(>>\s*stream)'os)
         {  $del1 = $2;
            $strPos = length($2) + length($3) + length($1);
            $del1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
            $objekt[$ny] = $pos;
            $utrad = "$ny 0 obj<<" . "$del1" . '>>stream';
            $del2   = substr($objektet, $strPos);
            $utrad .= $del2; 

            $pos += syswrite UTFIL, $utrad;
         }
         else
         {  if ($objektet =~ m'^(\d+ \d+ obj)'os)
            {  my $preLength = length($1);
               $objektet = substr($objektet, $preLength);
            }
            $objektet =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
            $objekt[$ny] = $pos;
            $utrad = "$ny 0 obj$objektet";
            $pos += syswrite UTFIL, $utrad;
         }
      }
  }
  close INFIL;
  $processed{$infil}->{root}         = $root;

  return ($extraherade, $Names, $AARoot, $AcroForm);
}

# end of PDF::Reuse::analysera
1;
