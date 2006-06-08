# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3680 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prCompress.al)"
sub prCompress
{ $compress = shift;
  if ($runfil)
  {  $log .= "Compress~$compress\n";
  }
  if (! $pos)
  {  errLog("No output file, you have to call prFile first");
  }
  1;

}

# end of PDF::Reuse::prCompress
1;
