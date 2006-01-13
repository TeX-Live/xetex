# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 5960 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/errLog.al)"
sub errLog
{   no strict 'refs';
    my $mess = shift;
    my $endMess  = " $mess \n More information might be found in"; 
    if ($runfil)
    {   $log .= "Log~Err: $mess\n";
        $endMess .= "\n   $runfil";
        if (! $pos)
        {  $log .= "Log~Err: No pdf-file has been initiated\n";
        }
        elsif ($pos > 15000000)
        {  $log .= "Log~Err: Current pdf-file is very big: $pos bytes, will not try to finnish it\n"; 
        }
        else
        {  $log .= "Log~Err: Will try to finnish current pdf-file\n";
           $endMess .= "\n   $utfil";
        }
    }
    my $errLog = 'error.log';
    my $now = localtime();
    my $lpos = $pos || 'undef';
    my $lobjNr = $objNr || 'undef';
    my $lutfil = $utfil || 'undef';
    
    my $lrunfil = $runfil || 'undef'; 
    open (ERRLOG, ">$errLog") || croak "$mess can't open an error logg, $!";
    print ERRLOG "\n$mess\n\n";
    print ERRLOG Carp::longmess("The error occurred when executing:\n");
    print ERRLOG "\nSituation when the error occurred\n\n";
    print ERRLOG "   Bytes written to the current pdf-file,    pos    = $lpos\n";
    print ERRLOG "   Object processed, not necessarily written objNr  = $lobjNr\n";
    print ERRLOG "   Current pdf-file,                         utfil  = $lutfil\n";
    print ERRLOG "   File logging the run,                     runfil = $lrunfil\n";
    print ERRLOG "   Local time                                       = $now\n"; 
    print ERRLOG "\n\n";    
    close ERRLOG;
    $endMess .= "\n   $errLog";
    if (($pos) && ($pos < 15000000))
    {  prEnd();
    }
    print STDERR Carp::shortmess("An error occurred \n");
    croak "$endMess\n";      
}

1;
# end of PDF::Reuse::errLog
