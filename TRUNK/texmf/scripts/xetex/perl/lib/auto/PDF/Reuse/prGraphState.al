# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3402 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prGraphState.al)"
sub prGraphState
{  my $string = shift;
   $gSNr++;
   my $name = 'Gs' . $gSNr ;
   $objNr++;
   $objekt[$objNr] = $pos;
   my $utrad = "$objNr 0 obj\n" . $string  . "\nendobj\n";
   $pos += syswrite UTFIL, $utrad;
   $objRef{$name} = $objNr;
   if ($runfil)
   {  $log .= "GraphStat~$string\n";
   }
   if (! $pos)
   {  errLog("No output file, you have to call prFile first");
   }
   return $name;
}

# end of PDF::Reuse::prGraphState
1;
