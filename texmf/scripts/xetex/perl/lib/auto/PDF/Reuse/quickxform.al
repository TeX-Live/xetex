# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 5834 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/quickxform.al)"
sub quickxform
{  my $inNr = shift;
   if (exists $old{$inNr})
   {  $old{$inNr}; }
   else
   {  push @skapa, [$inNr, ++$objNr];
      $old{$inNr} = $objNr;                   
   } 
} 

# end of PDF::Reuse::quickxform
1;
