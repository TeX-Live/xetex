# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3307 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/fillTheForm.al)"
sub fillTheForm
{  my $left   = shift || 0;
   my $bottom = shift || 0;
   my $right  = shift || 0;
   my $top    = shift || 0; 
   my $how    = shift || 1;
   my $image  = shift;
   my $str;
   my $scaleX = 1;
   my $scaleY = 1; 
   
   my $xDim = $genUpperX - $genLowerX;
   my $yDim = $genUpperY - $genLowerY;
   my $xNy  = $right - $left;
   my $yNy  = $top - $bottom;
   $scaleX  = $xDim / $xNy;
   $scaleY  = $yDim / $yNy;
   if ($how == 1)
   {  if ($scaleX < $scaleY)
      {  $scaleY = $scaleX;
      }
      else
      {  $scaleX = $scaleY;
      }
   }
   $str = "$scaleX 0 0 $scaleY $left $bottom cm\n";
   return $str;
}

# end of PDF::Reuse::fillTheForm
1;
