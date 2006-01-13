# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3469 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/createCharProcs.al)"
sub createCharProcs()
{   #################################
    # Fonten (objektet) för 0 skapas
    #################################
    
    $objNr++;
    $objekt[$objNr]  = $pos;
    my $tomtObj = $objNr;
    my $str = "\n75 0 d0\n6 0 69 2000 re\n1.0 g\nf\n";
    my $strLength = length($str);
    my $obj = "$objNr 0 obj\n<< /Length $strLength >>\nstream" .
           $str . "\nendstream\nendobj\n";
    $pos += syswrite UTFIL, $obj;

    #################################
    # Fonten (objektet) för 1 skapas
    #################################

    $objNr++;
    $objekt[$objNr]  = $pos;
    my $streckObj = $objNr;
    $str = "\n75 0 d0\n4 0 71 2000 re\n0.0 g\nf\n";
    $strLength = length($str);
    $obj = "$objNr 0 obj\n<< /Length $strLength >>\nstream" .
           $str . "\nendstream\nendobj\n";
    $pos += syswrite UTFIL, $obj;

    ###################################################
    # Fonten (objektet) för 2, ett långt streck skapas
    ###################################################

    $objNr++;
    $objekt[$objNr]  = $pos;
    my $lStreckObj = $objNr;
    $str = "\n75 0 d0\n4 -250 71 2250 re\n0.0 g\nf\n";
    $strLength = length($str);
    $obj = "$objNr 0 obj\n<< /Length $strLength >>\nstream" .
           $str . "\nendstream\nendobj\n";
    $pos += syswrite UTFIL, $obj;
   
    #####################################################
    # Objektet för "CharProcs" skapas
    #####################################################

    $objNr++;
    $objekt[$objNr]  = $pos;
    my $charProcsObj = $objNr;
    $obj = "$objNr 0 obj\n<</tomt $tomtObj 0 R\n/streck $streckObj 0 R\n" .
           "/lstreck $lStreckObj 0 R>>\nendobj\n";
    $pos += syswrite UTFIL, $obj;
    return $charProcsObj;
}

# end of PDF::Reuse::createCharProcs
1;
