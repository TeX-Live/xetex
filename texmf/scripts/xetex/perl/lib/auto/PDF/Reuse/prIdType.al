# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3532 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prIdType.al)"
sub prIdType
{   $idTyp = shift;
    if ($runfil)
    {  $log .= "IdType~rep\n";
    }
    1;
}

# end of PDF::Reuse::prIdType
1;
