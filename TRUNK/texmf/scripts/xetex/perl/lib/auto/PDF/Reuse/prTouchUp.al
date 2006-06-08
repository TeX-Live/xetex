# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3669 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prTouchUp.al)"
sub prTouchUp
{ $touchUp = shift;
  if ($runfil)
  {  $log .= "TouchUp~$touchUp\n";
  }
  if (! $pos)
  {  errLog("No output file, you have to call prFile first");
  }
  1;
}

# end of PDF::Reuse::prTouchUp
1;
