# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 4017 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/getKnown.al)"
sub getKnown
{   my ($p, $nr) = @_;
    my ($del1, $del2);
    my @objData = @{$$$p[0]->{$nr}};
    if (defined $objData[oSTREAMP])
    {  sysseek INFIL, ($objData[oNR][0] + $objData[oPOS]), 0;
       sysread INFIL, $del1, ($objData[oSTREAMP] - $objData[oPOS]);
       sysread INFIL, $del2, ($objData[oNR][1]   - $objData[oSTREAMP]);
    }
    else
    {  my $buf;
       my ($offs, $siz, $embedded) = @{$objData[oNR]};
       if ($offs)
       {  sysseek INFIL, $offs, 0;
          sysread INFIL, $buf, $siz;
          if ($buf =~ m'^\d+ \d+ obj\s*(.*)'os)
          {   $del1 = $1;
          }  
       }
       elsif (exists $unZipped{$nr})
       {  $del1 = "$unZipped{$nr} endobj";
       }
       elsif ($embedded)
       {   @objData = @{$$$p[0]->{$embedded}};
           unZipPrepare($embedded, $objData[oNR][0], $objData[oNR][1]);
           $del1 = "$unZipped{$nr} endobj"; 
       }        
    }
    return (\$del1, \$del2, $objData[oKIDS], $objData[oTYPE]);
}

# end of PDF::Reuse::getKnown
1;
