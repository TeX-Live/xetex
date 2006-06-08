# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 5622 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/behandlaNames.al)"
sub behandlaNames
{  my ($namnObj, $iForm) = @_;
   
   my ($low, $high, $antNod0, $entry, $nyttNr, $ny, $obj,
       $fObjnr, $offSet, $bytes, $res, $key, $func, $corr, @objData);
   my (@nod0, @nodUpp, @kid, @soek, %nytt);
   
   my $objektet  = '';
   my $vektor    = '';   
   my $antal     = 0;
   my $antNodUpp = 0;
   if ($namnObj)
   {  if ($iForm)                                # Läsning via interntabell
      {   $objektet = getObject($namnObj, 1);

          if ($objektet =~ m'<<(.+)>>'ogs)
          { $objektet = $1; }
          if ($objektet =~ s'/JavaScript\s+(\d+)\s{1,2}\d+\s{1,2}R''os)
          {  my $byt = $1; 
             push @kid, $1;
             while (scalar @kid)
             {  @soek = @kid;
                @kid = ();
                for my $sObj (@soek)
                {  $obj = getObject($sObj, 1);
                   if ($obj =~ m'/Kids\s*\[([^]]+)'ogs)
                   {  $vektor = $1;
                   }
                   while ($vektor =~ m'\b(\d+)\s{1,2}\d+\s{1,2}R\b'ogs)
                   {  push @kid, $1;
                   }
                   $vektor = '';
                   if ($obj =~ m'/Names\s*\[([^]]+)'ogs)
                   {   $vektor = $1;
                   }
                   while ($vektor =~ m'\(([^\)]+)\)\s*(\d+) \d R'gos)
                   {   $script{$1} = $2;                
                   }
                }
             }
          }
      }
      else                                #  Läsning av ett "doc"
      {  $objektet = getObject($namnObj);             
         if ($objektet =~ m'<<(.+)>>'ogs)
         {  $objektet = $1; }
         if ($objektet =~ s'/JavaScript\s+(\d+)\s{1,2}\d+\s{1,2}R''os)
         {  my $byt = $1; 
            push @kid, $1;
            while (scalar @kid)
            {  @soek = @kid;
               @kid = ();
               for my $sObj (@soek)
               {  $obj = getObject($sObj);  
                  if ($obj =~ m'/Kids\s*\[([^]]+)'ogs)
                  {  $vektor = $1;
                  }
                  while ($vektor =~ m'\b(\d+)\s{1,2}\d+\s{1,2}R\b'ogs)
                  {  push @kid, $1;
                  }
                  undef $vektor;
                  if ($obj =~ m'/Names\s*\[([^]]+)'ogs)
                  {  $vektor = $1;
                  }
                  while ($vektor =~ m'\(([^\)]+)\)\s*(\d+) \d R'gos)
                  {   $script{$1} = $2;                
                  }
               }        
             }
          }
      } 
   }
   for my $filnamn (@jsfiler)
   {   inkludera($filnamn);
   }
   my @nya = (keys %nyaFunk);
   while (scalar @nya)
   {   my @behandla = @nya;
       @nya = ();
       for $key (@behandla)
       {   if (exists $initScript{$key})
           {  if (exists $nyaFunk{$key})
              {   $initScript{$key} = $nyaFunk{$key};
              }
              if (exists $script{$key})   # företräde för nya funktioner !
              {   delete $script{$key};    # gammalt script m samma namn plockas bort
              } 
              my @fall = ($initScript{$key} =~ m'([\w\d\_\$]+)\s*\('ogs);
              for (@fall)
              {   if (($_ ne $key) && (exists $nyaFunk{$_}))
                  {  $initScript{$_} = $nyaFunk{$_}; 
                     push @nya, $_; 
                  }
              }
           }
       }
   }
   while  (($key, $func) = each %nyaFunk)
   {  $fObjnr = skrivJS($func);
      $script{$key} = $fObjnr;
      $nytt{$key}   = $fObjnr;
   }
     
   if (scalar %fields)
   {  push @inits, 'Ladda();';
      $fObjnr = defLadda();
      if ($duplicateInits)
      {  $script{'Ladda'} = $fObjnr;
         $nytt{'Ladda'} = $fObjnr;
      }
   }

   if ((scalar @inits) && ($duplicateInits))
   {  $fObjnr = defInit();
      $script{'Init'} = $fObjnr;
      $nytt{'Init'} = $fObjnr;
   }
   undef @jsfiler;
 
   for my $key (sort (keys %script))
   {  if (! defined $low)
      {  $objNr++;
         $ny = $objNr;     
         $objekt[$ny] = $pos;
         $obj = "$ny 0 obj\n";
         $low  = $key;
         $obj .= '<< /Names [';
      }
      $high = $key;
      $obj .= '(' . "$key" . ')';
      if (! exists $nytt{$key})
      {  $nyttNr = quickxform($script{$key});
      }
      else
      {  $nyttNr = $script{$key};
      }
      $obj .= "$nyttNr 0 R\n";      
      $antal++;
      if ($antal > 9)
      {   $obj .= ' ]/Limits [(' . "$low" . ')(' . "$high" . ')] >>' . "endobj\n";
          $pos += syswrite UTFIL, $obj;
          push @nod0, \[$ny, $low, $high];
          $antNod0++; 
          undef $low;
          $antal = 0; 
      }
   }
   if ($antal)
   {   $obj .= ']/Limits [(' . $low . ')(' . $high . ')]>>' . "endobj\n";
       $pos += syswrite UTFIL, $obj;
       push @nod0, \[$ny, $low, $high];
       $antNod0++;
   }
   $antal = 0;

   while (scalar @nod0)
   {   for $entry (@nod0)
       {   if ($antal == 0)
           {   $objNr++;     
               $objekt[$objNr] = $pos;
               $obj = "$objNr 0 obj\n";
               $low  = $$entry->[1];
               $obj .= '<</Kids [';
           }
           $high = $$entry->[2];
           $obj .= " $$entry->[0] 0 R";
           $antal++;
           if ($antal > 9)
           {   $obj .= ']/Limits [(' . $low . ')(' . $high . ')]>>' . "endobj\n";
               $pos += syswrite UTFIL, $obj;
               push @nodUpp, \[$objNr, $low, $high];
               $antNodUpp++; 
               undef $low;
               $antal = 0; 
           } 
       }
       if ($antal > 0)
       {   if ($antNodUpp == 0)     # inget i noderna över
           {   $obj .= ']>>' . "endobj\n";
               $pos += syswrite UTFIL, $obj;
           }
           else
           {   $obj .= ']/Limits [(' . "$low" . ')(' . "$high" . ')]>>' . "endobj\n";
               $pos += syswrite UTFIL, $obj;
               push @nodUpp, \[$objNr, $low, $high];
               $antNodUpp++; 
               undef $low;
               $antal = 0; 
           }
       }
       @nod0    = @nodUpp;
       $antNod0 = $antNodUpp;
       undef @nodUpp;
       $antNodUpp = 0;
   }
      
  
   $ny = $objNr;
   $objektet =~ s|\s*/JavaScript\s*\d+\s{1,2}\d+\s{1,2}R||os;
   $objektet =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
   if (scalar %script)
   {  $objektet .= "\n/JavaScript $ny 0 R\n";
   }
   $objNr++;
   $ny = $objNr;
   $objekt[$ny] = $pos;
   $objektet = "$ny 0 obj<<" . $objektet . ">>endobj\n";
   $pos += syswrite UTFIL, $objektet;
   return $ny;
}

# end of PDF::Reuse::behandlaNames
1;
