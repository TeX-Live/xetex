# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3607 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prVers.al)"
sub prVers
{   my $vers = shift;            
    ############################################################
    # Om programmet körs om så kontrolleras VERSION
    ############################################################
    if ($vers ne $VERSION)
    {  warn  "$vers \<\> $VERSION might give different results, if comparing two runs \n";
       return undef;
    }
    else
    {  return 1;
    }
}

# end of PDF::Reuse::prVers
1;
