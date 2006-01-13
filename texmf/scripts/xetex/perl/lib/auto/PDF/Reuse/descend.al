# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 2706 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/descend.al)"
sub descend
{   my ($parent, %entry) = @_;
    my ($first, $last, $count, $me, $rad, $jsObj);
    if (! exists $entry{'close'})
    {  $totalCount++; }
    $count = $totalCount;
    $me = $entry{'this'};
    if (exists $entry{'kids'})
    {   if (ref($entry{'kids'}) eq 'ARRAY')
        {   my @array = @{$entry{'kids'}};
            my $number = $#array;
            for (my $i = 0; $i <= $number ; $i++)
            {   $objNr++;
                $array[$i]->{'this'} = $objNr;
                if ($i == 0)
                {   $first = $objNr;           
                }
                if ($i == $number)
                {   $last = $objNr;
                } 

                if ($i < $number)
                {  $array[$i]->{'next'} = $objNr + 1;
                }
                if ($i > 0)
                {  $array[$i]->{'previous'} = $objNr - 1;
                }
                if (exists $entry{'close'})
                {  $array[$i]->{'close'} = 1;
                }
            } 

            for my $element (@array)
            {   descend($me, %{$element})
            }
        }
        else                                          # a hash
        {   my %hash = %{$entry{'kids'}};
            $objNr++;
            $hash{'this'} = $objNr;
            $first        = $objNr;           
            $last         = $objNr;
            descend($me, %hash)
        }
     }     

     if (exists $entry{'act'})
     {   my $code = $entry{'act'};
         if ($code =~ m/^\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*$/os)
         {  $code = "this.pageNum = $1; this.scroll($2, $3);";
         }
         $jsObj = skrivJS($code);         
     }

     $objekt[$me] = $pos;
     $rad = "$me 0 obj<<";
     if (exists $entry{'text'})
     {   $rad .= "/Title ($entry{'text'})";
     }
     $rad .= "/Parent $parent 0 R";
     if (defined $jsObj)
     {  $rad .= "/A $jsObj 0 R";
     }
     elsif (exists $entry{'pdfact'})
     {  $rad .= "/A << $entry{'pdfact'} >>";
     }
     if (exists $entry{'previous'})
     {  $rad .= "/Prev $entry{'previous'} 0 R";
     }
     if (exists $entry{'next'})
     {  $rad .= "/Next $entry{'next'} 0 R";
     }
     if (defined $first)
     {  $rad .= "/First $first 0 R";
     }
     if (defined $last)
     {  $rad .= "/Last $last 0 R";
     }
     if ($count != $totalCount)
     {   $count = $totalCount - $count;
         $rad .= "/Count $count";
     }
     if (exists $entry{'color'})
     {   $rad .= "/C [$entry{'color'}]";
     }
     if (exists $entry{'style'})
     {   $rad .= "/F $entry{'style'}";
     }

     $rad .= ">>endobj\n";
     $pos += syswrite UTFIL, $rad;
}  

# end of PDF::Reuse::descend
1;
