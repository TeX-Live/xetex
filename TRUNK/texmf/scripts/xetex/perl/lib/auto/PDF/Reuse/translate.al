# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 5614 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/translate.al)"
sub translate
{ if (exists $old{$1})
  { $old{$1}; }
  else
  {  $old{$1} = ++$objNr;
  }     
}  

# end of PDF::Reuse::translate
1;
