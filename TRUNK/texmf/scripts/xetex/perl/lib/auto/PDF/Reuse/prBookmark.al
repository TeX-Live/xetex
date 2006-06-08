# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 2628 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prBookmark.al)"
sub prBookmark
{   my $param = shift;
    if (! ref($param))
    {   $param = eval ($param);
    }
    if (! ref($param))
    {   return undef;
    }
    if (! $pos)
    {  errLog("No output file, you have to call prFile first");
    }
    if (ref($param) eq 'HASH')
    {   push @bookmarks, $param;
    }
    else
    {   push @bookmarks, (@$param);       
    }
    if ($runfil)
    {   local $Data::Dumper::Indent = 0;
        $param = Dumper($param);
        $param =~ s/^\$VAR1 = //;
        $param = prep($param);
        $log .= "Bookmark~$param\n";
    }
    return 1;
}

# end of PDF::Reuse::prBookmark
1;
