# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3631 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prLog.al)"
sub prLog
{  my $mess = shift;
   if ($runfil)
   {  $mess  = prep($mess);
      $log .= "Log~$mess\n";
      return 1;
   }
   else
   {  errLog("You have to give a directory for the logfiles first : prLogDir <dir> , aborts");
   }
   
}

# end of PDF::Reuse::prLog
1;
