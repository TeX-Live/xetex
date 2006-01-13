# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 2960 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prMbox.al)"
sub prMbox
{  my $lx = shift || 0;
   my $ly = shift || 0;
   my $ux = shift || 595;
   my $uy = shift || 842;
   
   if ((defined $lx) && ($lx =~ m'^[\d\-\.]+$'o))
   { $genLowerX = $lx; }
   if ((defined $ly) && ($ly =~ m'^[\d\-\.]+$'o))
   { $genLowerY = $ly; } 
   if ((defined $ux) && ($ux =~ m'^[\d\-\.]+$'o))
   { $genUpperX = $ux; } 
   if ((defined $uy) && ($uy =~ m'^[\d\-\.]+$'o))
   { $genUpperY = $uy; } 
   if ($runfil)
   {  $log .= "Mbox~$lx~$ly~$ux~$uy\n";
   }
   if (! $pos)
   {  errLog("No output file, you have to call prFile first");
   }
   1;
}

# end of PDF::Reuse::prMbox
1;
