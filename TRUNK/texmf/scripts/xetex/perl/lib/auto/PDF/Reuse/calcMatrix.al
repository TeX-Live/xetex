# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3268 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/calcMatrix.al)"
sub calcMatrix
{  my ($x, $y, $rotate, $size, $xsize, $ysize, $upperX, $upperY) = @_;
   my ($str, $xSize, $ySize);
   $size  = 1 if ($size  == 0);
   $xsize = 1 if ($xsize == 0);
   $ysize = 1 if ($ysize == 0);
   $xSize = $xsize * $size;
   $ySize = $ysize * $size;   
   $str = "$xSize 0 0 $ySize $x $y cm\n";
   if ($rotate)
   {   if ($rotate =~ m'q(\d)'oi)
       {  my $tal = $1;
          if ($tal == 1)
          {  $upperY = $upperX;
             $upperX = 0;
             $rotate = 270;
          }
          elsif ($tal == 2)
          {  $rotate = 180;
          }
          else
          {  $rotate = 90;
             $upperX = $upperY;
             $upperY = 0;
          }
       }
       else
       {   $upperX = 0;
           $upperY = 0;
       }  
       my $radian = sprintf("%.6f", $rotate / 57.2957795);    # approx. 
       my $Cos    = sprintf("%.6f", cos($radian));
       my $Sin    = sprintf("%.6f", sin($radian));
       my $negSin = $Sin * -1;
       $str .= "$Cos $Sin $negSin $Cos $upperX $upperY cm\n";
   }
   return $str;
}

# end of PDF::Reuse::calcMatrix
1;
