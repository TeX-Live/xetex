# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 4049 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/unZipPrepare.al)"
sub unZipPrepare
{  my ($nr, $offs, $size) = @_;
   my $buf;
   if ($offs)
   {   sysseek INFIL, $offs, 0;
       sysread INFIL, $buf, $size;
   }
   else
   {   $buf = getObject($nr);
   }
   my (%param, $stream, $str);
   
   if ($buf =~ m'^(\d+ \d+ obj\s*<<[\w\d\/\s\[\]<>]+)stream\b'os)
   {  $str  = $1;
      $offs = length($str) + 7;
      if (substr($buf, $offs, 1) eq "\n")
      {  $offs++;
      }

      for (split('/',$str))
      {  if ($_ =~ m'^(\w+)(.*)'o)
         {  $param{$1} = $2 || ' ';
         }
      }
      $stream = substr($buf, $offs, $param{'Length'});
      my $x = inflateInit()
           || die "Cannot create an inflation stream\n";
      my ($output, $status) = $x->inflate($stream);
      die "inflation failed\n"
                     unless $status == 1;

      my $first = $param{'First'};
      my @oOffsets = (substr($output, 0, $first) =~ m'(\d+)\b'osg);
      my $i = 0;
      my $j = 1;
      my $bytes;
      while ($oOffsets[$i])
      {  my $k = $j + 2;
         if ($oOffsets[$k])
         {  $bytes = $oOffsets[$k] - $oOffsets[$j];
         }
         else 
         {  $bytes = length($output) - $first - $oOffsets[$j];
         }         
         $unZipped{$oOffsets[$i]} = substr($output,($first + $oOffsets[$j]), $bytes); 
         $i += 2;
         $j += 2;
      }
   }
}

# end of PDF::Reuse::unZipPrepare
1;
