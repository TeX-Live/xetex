# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3692 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prep.al)"
sub prep
{  my $indata = shift;
   $indata =~ s/[\n\r]+/ /sgo;
   $indata =~ s/~/<tilde>/sgo;
   return $indata;
} 

# end of PDF::Reuse::prep
1;
