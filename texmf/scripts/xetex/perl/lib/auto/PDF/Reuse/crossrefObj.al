# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3758 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/crossrefObj.al)"
sub crossrefObj
{   my ($nr, $xref) = @_;
    my ($buf, %param, $len, $tempRoot);
    my $from = $xref;
    sysseek INFIL, $xref, 0;
    sysread INFIL, $buf, 400;
    my $str;
    if ($buf =~ m'^(.+>>\s*)stream'os)
    {  $str = $1;
       $from = length($str) + 7;
       if (substr($buf, $from, 1) eq "\n")
       {  $from++;
       }
       $from += $xref;
    }
    
    for (split('/',$str))
    {  if ($_ =~ m'^(\w+)(.*)'o)
       {  $param{$1} = $2 || ' ';
       }
    }
    if ((exists $param{'Root'}) && ($param{'Root'} =~ m'^\s*(\d+)'o))
    {  $tempRoot = $1;
    }
    my @keys = ($param{'W'} =~ m'(\d+)'og);
    my $keyLength = 0;
    for (@keys)
    {  $keyLength += $_;
    }
    my $recLength = $keyLength + 1;
    my $upTo = 1 + $keys[0] + $keys[1];
    if ((exists $param{'Length'}) && ($param{'Length'} =~ m'(\d+)'o))
    {  $len = $1;
       sysseek INFIL, $from, 0;
       sysread INFIL, $buf, $len;
       my $x = inflateInit()
               || die "Cannot create an inflation stream\n" ;
       my ($output, $status) = $x->inflate(\$buf) ;
       die "inflation failed\n"
                     unless $status == 1;
       
       my $i = 0;
       my @last = (0, 0, 0, 0, 0, 0, 0);
       my @word = ('0', '0', '0', '0', '0', '0', '0');
       my $recTyp;
       my @intervall = ($param{'Index'} =~ m'(\d+)\D'osg);
       my $m = 0;
       my $currObj = $intervall[$m];
       $m++;
       my $max     = $currObj + $intervall[$m];   
       
       for (unpack ("C*", $output))
       {  if (($_ != 0) && ($i > 0) && ($i < $upTo))
          {   my $tal = $_ + $last[$i] ;
              if ($tal > 255)
              {$tal -= 256;
              }
          
              $last[$i] = $tal;
              $word[$i] = sprintf("%x", $tal);
              if (length($word[$i]) == 1)
              {  $word[$i] = '0' . $word[$i];
              }
          }                    
          $i++;
          if ($i == $recLength)
          {  $i = 0;
             my $j = 0;
             my $offsObj;               # offset or object
             if ($keys[0] == 0)
             {  $recTyp = 1;
                $j = 1;
             }
             else
             {  $recTyp = $word[1];
                $j = 2;
             }
             my $k = 0;
             while ($k < $keys[1])
             {  $offsObj .= $word[$j];
                $k++;
                $j++;
             }
                       
             if ($recTyp == 1)
             {   if (! (exists $oldObject{$currObj}))
                 {  $oldObject{$currObj} = hex($offsObj); }
                 else
                 {  $nr++;
                    $oldObject{'xref' . "$nr"} = hex($offsObj);
                 }
             }
             elsif ($recTyp == 2)
             {   if (! (exists $oldObject{$currObj}))
                 {  $oldObject{$currObj} = 0; 
                 }
                 $embedded{$currObj} = hex($offsObj);
             }
             if ($currObj < $max)
             {  $currObj++;
             }
             else
             {  $m++;
                $currObj = $intervall[$m];
                $m++;
                $max     = $currObj + $intervall[$m];
             } 
          }
       }       
    }
    return ($param{'Prev'}, $tempRoot, $nr);
}

# end of PDF::Reuse::crossrefObj
1;
