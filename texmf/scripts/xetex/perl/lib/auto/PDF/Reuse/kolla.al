# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 4586 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/kolla.al)"
sub kolla
{  #
   # Resurser
   #
   my $obj       = shift;
   my $resources = shift;
   my $valid;
    
   if ($obj =~ m'MediaBox\s*\[\s*([\-\.\d]+)\s+([\-\.\d]+)\s+([\-\.\d]+)\s+([\-\.\d]+)'os)
   { $formBox[0] = $1;
     $formBox[1] = $2;
     $formBox[2] = $3;
     $formBox[3] = $4;
   }
  
   if ($obj =~ m'/Contents\s+(\d+)'so)
   { $formCont = $1;
     $valid    = 1;
   }
   
   if ($obj =~ m'^(.+/Resources)'so)
   {  if ($obj =~ m'Resources(\s+\d+\s{1,2}\d+\s{1,2}R)'os)   # Hänvisning
      {  $resources = $1; }
      else                 # Resurserna är ett dictionary. Hela kopieras
      {  my $dummy;
         my $i;
         my $k;
         undef $resources;
         ($dummy, $obj) = split /\/Resources/, $obj;
         $obj =~ s/\<\</\#\<\</gs;
         $obj =~ s/\>\>/\>\>\#/gs;
         my @ord = split /\#/, $obj;
         for ($i = 0; $i <= $#ord; $i++)
         {   $resources .= $ord[$i];
             if ($ord[$i] =~ m'\S+'s)
             {  if ($ord[$i] =~ m'<<'s)
                {  $k++; }
                if ($ord[$i] =~ m'>>'s)
                {  $k--; }
                if ($k == 0)
                {  last; }
             } 
          }
       }
    }
    return ($resources, $valid);
}

# end of PDF::Reuse::kolla
1;
