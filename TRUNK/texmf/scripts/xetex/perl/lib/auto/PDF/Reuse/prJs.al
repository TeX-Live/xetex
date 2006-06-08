# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3552 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prJs.al)"
sub prJs
{   my $filNamnIn = shift;
    my $filNamn;
    if ($filNamnIn !~ m'\{'os)
    {  my $checkIdOld = $checkId;
       ($filNamn, $checkId) = findGet($filNamnIn, $checkIdOld);
       if (($runfil) && ($checkId) && ($checkId ne $checkIdOld))
       {  $log .= "Cid~$checkId\n";
       }
       $checkId = '';
    }
    else
    {  $filNamn = $filNamnIn;
    }
    if ($runfil)
    {  my $filnamn = prep($filNamn);
       $log .= "Js~$filnamn\n";
    }
    if (($interAktivSida) || ($interActive))
    {  errLog("Too late, has already tried to merge JAVA SCRIPTS within an interactive page");
    }
    elsif (! $pos)
    {  errLog("Too early for JAVA SCRIPTS, create a file first"); 
    }
    push @jsfiler, $filNamn;
    1;
}

# end of PDF::Reuse::prJs
1;
