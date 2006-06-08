# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3871 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/xrefSection.al)"
sub xrefSection
{   my ($nr, $xref, $infil) = @_;
    my ($i, $root, $antal);    
    $nr++;
    $oldObject{('xref' . "$nr")} = $xref;  # Offset för xref sparas 
    $xref += 5;
    sysseek INFIL, $xref, 0;      
    $xref  = 0;
    my $inrad = '';
    my $buf   = '';
    my $c;
    sysread INFIL, $c, 1;
    while ($c =~ m!\s!s)   
    {  sysread INFIL, $c, 1; }

    while ( (defined $c)
    &&   ($c ne "\n")
    &&   ($c ne "\r") )   
    {    $inrad .= $c;
         sysread INFIL, $c, 1;
    }

    if ($inrad =~ m'^(\d+)\s+(\d+)'o)
    {   $i     = $1;
        $antal = $2;
    }
            
    while ($antal)
    {   for (my $l = 1; $l <= $antal; $l++)
        {  sysread INFIL, $inrad, 20;
           if ($inrad =~ m'^\s?(\d+) \d+ (\w)\s*'o)
           {  if ($2 eq 'n')
              {  if (! (exists $oldObject{$i}))
                 {  $oldObject{$i} = int($1); }
                 else
                 {  $nr++;
                    $oldObject{'xref' . "$nr"} = int($1);
                 }
              } 
           }
           $i++;
        }
        undef $antal;
        undef $inrad;
        sysread INFIL, $c, 1;
        while ($c =~ m!\s!s)   
        {  sysread INFIL, $c, 1; }

        while ( (defined $c)
        &&   ($c ne "\n")
        &&   ($c ne "\r") )   
        {    $inrad .= $c;
             sysread INFIL, $c, 1;
        }
        if ($inrad =~ m'^(\d+)\s+(\d+)'o)
        {   $i     = $1;
            $antal = $2;
        }

    }
             
    while ($inrad)
    {   if ($buf =~ m'Encrypt'o)
        {  errLog("The file $infil is encrypted, cannot be used, aborts");
        }
        if ((! $root) && ($buf =~ m'\/Root\s+(\d+)\s{1,2}\d+\s{1,2}R'so))
        {  $root = $1;
           if ($xref)
           { last; }
        }

        if ((! $xref) && ($buf =~ m'\/Prev\s+(\d+)\D'so))
        {  $xref = $1;
           if ($root)
           { last; }
        }
                
        if ($buf =~ m'xref'so)
        {  last; }
                
        sysread INFIL, $inrad, 30;
        $buf .= $inrad;
    }
    return ($xref, $root, $nr);
}

# end of PDF::Reuse::xrefSection
1;
