# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3541 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prId.al)"
sub prId
{   $id = shift;
    if ($runfil)
    {  $log .= "Id~$id\n";
    }
    if (! $pos)
    {  errLog("No output file, you have to call prFile first");
    }
    1;
}

# end of PDF::Reuse::prId
1;
