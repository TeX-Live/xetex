# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3008 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prBar.al)"
############################################################
sub prBar
{ my ($xPos, $yPos, $TxT) = @_; 
 
  $TxT   =~ tr/G/2/;
    
  my @fontSpar = @aktuellFont;
         
  findBarFont();
  
  my $Font = $aktuellFont[foINTNAMN];                # Namn i strömmen
  
  if (($xPos) && ($yPos))
  {  $stream .= "\nBT /$Font $fontSize Tf ";
     $stream .= "$xPos $yPos Td \($TxT\) Tj ET\n";
  }
  if ($runfil)
  {  $log .= "Bar~$xPos~$yPos~$TxT\n";
  }
  if (! $pos)
  {  errLog("No output file, you have to call prFile first");
  }
  @aktuellFont = @fontSpar;
  return $Font;
  
}

# end of PDF::Reuse::prBar
1;
