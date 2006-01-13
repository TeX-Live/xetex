# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3957 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/getObject.al)"
sub getObject
{   my ($nr, $noId, $noEnd) = @_;
    
    my $buf;
    my ($offs, $siz, $embedded) = @{$oldObject{$nr}};
    
    if ($offs)
    {  sysseek INFIL, $offs, 0;
       sysread INFIL, $buf, $siz;
       if (($noId) && ($noEnd))
       {   if ($buf =~ m'^\d+ \d+ obj\s*(.*)endobj'os)
           {   if (wantarray)
               {   return ($1, $offs, $siz, $embedded);
               }
               else
               {   return $1;
               } 
           }
       }
       elsif ($noId)
       {   if ($buf =~ m'^\d+ \d+ obj\s*(.*)'os)
           {   if (wantarray)
               {   return ($1, $offs, $siz, $embedded);
               }
               else
               {   return $1;
               } 
           }
       }
       if (wantarray)
       {   return ($buf, $offs, $siz, $embedded)
       }
       else
       {   return $buf;
       } 
    }
    elsif (exists $unZipped{$nr})
    {  ;
    }
    elsif ($embedded)
    {   unZipPrepare($embedded);
    }
    if ($noEnd)
    {   if (wantarray)
        {   return ($unZipped{$nr}, $offs, $siz, $embedded)
        }
        else
        {   return $unZipped{$nr};
        }
    }
    else
    {   if (wantarray)
        {   return ("$unZipped{$nr}endobj\n", $offs, $siz, $embedded)
        }
        else
        {   return "$unZipped{$nr}endobj\n";
        }
    } 
}

# end of PDF::Reuse::getObject
1;
