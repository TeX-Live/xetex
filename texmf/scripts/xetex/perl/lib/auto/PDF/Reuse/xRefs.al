# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3700 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/xRefs.al)"
sub xRefs
{  my ($bytes, $infil) = @_;
   my ($j, $nr, $xref, $i, $antal, $inrad, $Root, $tempRoot, $referens);
   my $buf = '';
   %embedded =();

   my $res = sysseek INFIL, -50, 2;
   if ($res)
   {  sysread INFIL, $buf, 100;
      if ($buf =~ m'Encrypt'o)
      {  errLog("The file $infil is encrypted, cannot be used, aborts");
      }
      if ($buf =~ m'\bstartxref\s+(\d+)'o)
      {  $xref = $1;
         while ($xref)
         {  $res = sysseek INFIL, $xref, 0;
            $res = sysread INFIL, $buf, 200;
            if ($buf =~ m '^\d+\s\d+\sobj'os)
            {  ($xref, $tempRoot, $nr) = crossrefObj($nr, $xref);
            }
            else
            {  ($xref, $tempRoot, $nr) = xrefSection($nr, $xref, $infil);
            }
            if (($tempRoot) && (! $Root))
            {  $Root = $tempRoot;
            }
         }
      }
   }

   ($Root) || errLog("The Root object in $infil couldn't be found, aborting");

   ##############################################################
   # Objekten sorteras i fallande ordning (efter offset i filen)
   ##############################################################

   my @offset = sort { $oldObject{$b} <=> $oldObject{$a} } keys %oldObject;

   my $saved;

   for (@offset)
   {   $saved  = $oldObject{$_};
       $bytes -= $saved;
       
       if ($_ !~ m'^xref'o)
       {   if ($saved == 0)
           {   $oldObject{$_} = [ 0, 0, $embedded{$_}];
           }
           else
           {   $oldObject{$_} = [ $saved, $bytes];
           }
       }
       $bytes = $saved;
   } 
   %embedded = ();
   return $Root;
}

# end of PDF::Reuse::xRefs
1;
