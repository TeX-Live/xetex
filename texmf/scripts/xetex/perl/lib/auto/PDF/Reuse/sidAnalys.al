# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 5535 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/sidAnalys.al)"
sub sidAnalys
{  my ($oNr, $obj, $resources) = @_;
   my ($ny, $strPos, $spar, $closeProc, $del1, $del2, $utrad, $Annots);

   if (! $parents[0])
   { $objNr++;
     $parents[0] = $objNr;
   }
   my $parent = $parents[0];
   $objNr++;
   $ny = $objNr; 

   $old{$oNr} = $ny;
     
   if ($obj =~ m'/Parent\s+(\d+)\s{1,2}\d+\s{1,2}R\b'os)
   {  $old{$1} = $parent;
   }
   
   if ($obj =~ m'^(\d+ \d+ obj\s*<<)(.+)(>>\s*stream)'os)
   {  $del1   = $2;
      $strPos = length($2) + length($3) + length($1);
      $del2   = substr($obj, $strPos);
   }
   elsif ($obj =~ m'^\d+ \d+ obj\s*<<(.+)>>\s*endobj'os)
   {  $del1 = $1;
   }
   if (%links)
   {   my $tSida = $sida + 1;
       if (defined (@{$links{'-1'}}) || (defined @{$links{$tSida}}))
       {   if ($del1 =~ m'/Annots\s*([^\/\<\>]+)'os)
           {  $Annots  = $1;
              @annots = (); 
              if ($Annots =~ m'\[([^\[\]]*)\]'os)
              {  ; }
              else
              {  if ($Annots =~ m'\b(\d+)\s{1,2}\d+\s{1,2}R\b'os)
                 {  $Annots = getObject($1);
                 }
              }
              while ($Annots =~ m'\b(\d+)\s{1,2}\d+\s{1,2}R\b'ogs)
              {   push @annots, xform();
              }
              $del1 =~ s?/Annots\s*([^\/\<\>]+)??os;
           }
           $Annots = '/Annots ' . mergeLinks() . ' 0 R';
       }
   }

   if (! $taInterAkt)
   {  $del1 =~ s?\s*/AA\s*<<[^>]*>>??os;
   }
   if ($del1 !~ m'/Resources'o)
   {  $del1 .= "/Resources $resources";
   }
       
   $del1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
   if ($Annots)
   {  $del1 .= $Annots;
   }

   $utrad = "$ny 0 obj<<$del1>>";
   if (defined $del2)
   {   $utrad .= "stream\n$del2";
   }
   else
   {  $utrad .= "endobj\n";
   }

   $objekt[$ny] = $pos;
   $pos += syswrite UTFIL, $utrad;
     
   push @{$kids[0]}, $ny;
   $counts[0]++;
   if ($counts[0] > 9)
   {  ordnaNoder(8); 
   }
}  

# end of PDF::Reuse::sidAnalys
1;
