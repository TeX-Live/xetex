# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 2518 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/mergeLinks.al)"
sub mergeLinks
{   my $tSida = $sida + 1;
    my $rad;
    my ($linkObject, $linkObjectNo);
    for my $link (@{$links{'-1'}}, @{$links{$tSida}} )
    {   my $x2 = $link->{x} + $link->{width};
        my $y2 = $link->{y} + $link->{height};
        if (defined $link->{action})
        {
            if (exists $links{$link->{action}})
            {   $linkObjectNo = $links{$link->{action}};
            }
            else
            {   $objNr++;
                $objekt[$objNr] = $pos;
                $rad = "$objNr 0 obj<<$link->{action}>>endobj\n";
                $linkObjectNo = $objNr;
                $links{$link->{action}} = $objNr;
                $pos += syswrite UTFIL, $rad;
            }
        }
        else
        {
            if (exists $links{$link->{URI}})
            {   $linkObjectNo = $links{$link->{URI}};
            }
            else
            {   $objNr++;
                $objekt[$objNr] = $pos;
                $rad = "$objNr 0 obj<</S/URI/URI($link->{URI})>>endobj\n";
                $linkObjectNo = $objNr;
                $links{$link->{URI}} = $objNr;
                $pos += syswrite UTFIL, $rad;
            }
        }
        $rad = "/Subtype/Link/Rect[$link->{x} $link->{y} "
             . "$x2 $y2]/A $linkObjectNo 0 R";
        if (defined $link->{border})
        {   $rad .= "/Border$link->{border}";
        }
        else
        {   $rad .= "/Border[0 0 0]";
        }
        if (defined $link->{color})
        {   $rad .= "/C$link->{color}";
        }
        if (exists $links{$rad})
        {   push @annots, $links{$rad};
        }
        else
        {   $objNr++;
            $objekt[$objNr] = $pos;
            $links{$rad} = $objNr;
            $rad = "$objNr 0 obj<<$rad>>endobj\n";
            $pos += syswrite UTFIL, $rad;
            push @annots, $objNr;
        }
    }
    @{$links{'-1'}}   = ();
    @{$links{$tSida}} = ();
    $objNr++;
    $objekt[$objNr] = $pos;
    $rad = "$objNr 0 obj[\n";
    for (@annots)
    {  $rad .= "$_ 0 R\n";
    }
    $rad .= "]endobj\n";
    $pos += syswrite UTFIL, $rad;
    @annots = ();
    return $objNr;
}

# end of PDF::Reuse::mergeLinks
1;
