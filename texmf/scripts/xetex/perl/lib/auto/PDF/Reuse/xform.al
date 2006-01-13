# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 4571 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/xform.al)"
##################################################
# Översätter ett gammalt objektnr till ett nytt
# och sparar en tabell med vad som skall skapas
##################################################

sub xform
{  if (exists $old{$1})
   {  $old{$1}; 
   }
   else
   {  push @skapa, [$1, ++$objNr];
      $old{$1} = $objNr;                   
   } 
}

# end of PDF::Reuse::xform
1;
