# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 2655 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/ordnaBookmarks.al)"
sub ordnaBookmarks
{   my ($first, $last, $me, $entry, $rad);
    $totalCount = 0;
    if (defined $objekt[$objNr])
    {  $objNr++;
    }
    $me = $objNr;
        
    my $number = $#bookmarks;
    for (my $i = 0; $i <= $number ; $i++)
    {   my %hash = %{$bookmarks[$i]};
        $objNr++;
        $hash{'this'} = $objNr;
        if ($i == 0)
        {   $first = $objNr;           
        }
        if ($i == $number)
        {   $last = $objNr;
        } 
        if ($i < $number)
        {  $hash{'next'} = $objNr + 1;
        }
        if ($i > 0)
        {  $hash{'previous'} = $objNr - 1;
        }
        $bookmarks[$i] = \%hash;
    } 
    
    for $entry (@bookmarks)
    {  my %hash = %{$entry};
       descend ($me, %hash);
    }

    $objekt[$me] = $pos;

    $rad = "$me 0 obj<<";
    $rad .= "/Type/Outlines";
    $rad .= "/Count $totalCount";
    if (defined $first)
    {  $rad .= "/First $first 0 R";
    }
    if (defined $last)
    {  $rad .= "/Last $last 0 R";
    }
    $rad .= ">>endobj\n";
    $pos += syswrite UTFIL, $rad;

    return $me;

}

# end of PDF::Reuse::ordnaBookmarks
1;
