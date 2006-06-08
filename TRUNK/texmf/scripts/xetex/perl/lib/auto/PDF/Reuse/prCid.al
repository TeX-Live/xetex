# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3524 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prCid.al)"
sub prCid
{   $checkId = shift;
    if ($runfil)
    {  $log .= "Cid~$checkId\n";
    }
    1;    
}

# end of PDF::Reuse::prCid
1;
