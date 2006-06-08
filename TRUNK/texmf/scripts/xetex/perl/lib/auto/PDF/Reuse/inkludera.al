# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 5895 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/inkludera.al)"
sub inkludera
{   my $jsfil = shift;
    my $fil;
    if ($jsfil !~ m'\{'os)
    {   open (JSFIL, "<$jsfil") || return;
        while (<JSFIL>)
        { $fil .= $_;}

        close JSFIL;
    }
    else
    {  $fil = $jsfil;
    }
    $fil =~ s|function\s+([\w\_\d\$]+)\s*\(|"zXyZcUt function $1 ("|sge;
    my @funcs = split/zXyZcUt /, $fil;
    for my $kod (@funcs)
    {   if ($kod =~ m'^function ([\w\_\d\$]+)'os)
        {   $nyaFunk{$1} = $kod;
        }
    }   
}

# end of PDF::Reuse::inkludera
1;
