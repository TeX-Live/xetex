# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3649 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/findDir.al)"
sub findDir
{ my $dir = shift;
  if ($dir eq '.')
  { return undef; }
  if (! -e $dir)
   {  mkdir $dir || errLog("Couldn't create directory $dir, $!");
   }

  if ((-e $dir) && (-d $dir))
  {  if (substr($dir, length($dir), 1) eq '/')
     {  return $dir; }
     else
     {  return ($dir . '/');
     }
  }
  else
  { errLog("Error finding/creating directory $dir, $!");
  }
}

# end of PDF::Reuse::findDir
1;
