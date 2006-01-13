# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3580 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prInit.al)"
sub prInit
{   my $initText  = shift;
    my $duplicate = shift || '';
    my @fall = ($initText =~ m'([\w\d\_\$]+)\s*\(.*?\)'gs);
    for (@fall)
    {  if (! exists $initScript{$_})
       { $initScript{$_} = 0; 
       }
    }
    if ($duplicate)
    {  $duplicateInits = 1;
    }
    push @inits, $initText;
    if ($runfil)
    {   $initText = prep($initText);
        $log .= "Init~$initText~$duplicate\n";
    }
    if (($interAktivSida) || ($interActive))
    {  errLog("Too late, has already tried to create INITIAL JAVA SCRIPTS within an interactive page");
    }
    elsif (! $pos)
    {  errLog("Too early for INITIAL JAVA SCRIPTS, create a file first");
    }
    1;
    
}

# end of PDF::Reuse::prInit
1;
