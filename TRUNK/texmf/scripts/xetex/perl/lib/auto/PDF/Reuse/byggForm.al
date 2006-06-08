# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 4634 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/byggForm.al)"
##############################
# Ett formulär (åter)skapas
##############################

sub byggForm
{  no warnings; 
   my ($infil, $sidnr) = @_;
   
   my ($res, $corr, $nyDel1, $formRes, $del1, $del2, $kids, $typ, $nr,
       $utrad);
      
   my $fSource = $infil . '_' . $sidnr;
   my @stati = stat($infil);

   $behandlad{$infil}->{old} = {} 
        unless (defined $behandlad{$infil}->{old});
   $processed{$infil}->{oldObject} = {} 
        unless (defined $processed{$infil}->{oldObject});   
   $processed{$infil}->{unZipped} = {} 
        unless (defined $processed{$infil}->{unZipped});

   *old       = $behandlad{$infil}->{old};
   *oldObject = $processed{$infil}->{oldObject};
   *unZipped  = $processed{$infil}->{unZipped};
      
   if ($form{$fSource}[fID] != $stati[9])
   {    errLog("$stati[9] ne $form{$fSource}[fID] aborts");
   }
   if ($checkId) 
   {  if ($checkId ne $stati[9])
      {  my $mess =  "$checkId \<\> $stati[9] \n"
                  . "The Pdf-file $fSource has not the correct modification time. \n"
                  .  "The program is aborted";
         errLog($mess);
      }
      undef $checkId;
    }
    if ($ldir)
    {  $log .= "Cid~$stati[9]\n";
    }

   open (INFIL, "<$infil") || errLog("The file $infil couldn't be opened, aborting $!");
   binmode INFIL;

   ####################################################
   # Objekt utan referenser  kopieras och skrivs
   ####################################################   

   for my $key (@{$form{$fSource}->[fNOKIDS]})
   {   if ((defined $old{$key}) && ($objekt[$old{$key}]))    # already processed
       {  next;
       }
              
       if (! defined $old{$key})
       {  $old{$key} = ++$objNr;
       }
       $nr = $old{$key};
       $objekt[$nr] = $pos;
     
       ($del1, $del2, $kids, $typ) = getKnown(\$form{$fSource},$key);
             
       if ($typ eq 'Font')
       {  my $Font = ${$form{$fSource}}[0]->{$key}->[oNAME];
          if (! defined $font{$Font}[foINTNAMN])
          {  $fontNr++;
             $font{$Font}[foINTNAMN]  = 'Ft' . $fontNr;
             $font{$Font}[foREFOBJ]   = $nr;
             $objRef{'Ft' . $fontNr}  = $nr;
          }
       }
       if (! defined $$del2)
       {   $utrad = "$nr 0 obj " . $$del1;
       }
       else
       {   $utrad = "$nr 0 obj\n<<" . $$del1 . $$del2;
       }     
       $pos += syswrite UTFIL, $utrad;     
   }

   #######################################################
   # Objekt med referenser kopieras, behandlas och skrivs
   #######################################################
   for my $key (@{$form{$fSource}->[fKIDS]})
   {   if ((defined $old{$key}) && ($objekt[$old{$key}]))  # already processed
       {  next;
       }
        
       if (! defined $old{$key})
       {  $old{$key} = ++$objNr;
       }
       $nr = $old{$key};
       
       $objekt[$nr] = $pos;
       
       ($del1, $del2, $kids, $typ) = getKnown(\$form{$fSource},$key);

       $$del1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/translate() . ' 0 R'/oegs;
       
       if (defined $$del2)          
       {  $utrad = "$nr 0 obj\n<<" . $$del1 . $$del2;
       }
       else
       {  $utrad = "$nr 0 obj " . $$del1;
       } 
       
       if (($typ) && ($typ eq 'Font'))
       {  my $Font = $form{$fSource}[0]->{$key}->[oNAME];
          if (! defined $font{$Font}[foINTNAMN])
          {  $fontNr++;
             $font{$Font}[foINTNAMN]  = 'Ft' . $fontNr;
             $font{$Font}[foREFOBJ]   = $nr;
             $objRef{'Ft' . $fontNr} = $nr;
          }
       }   
       
       $pos += syswrite UTFIL, $utrad;                
   }

   #################################
   # Formulärobjektet behandlas 
   #################################
   
   my $key = $form{$fSource}->[fMAIN];
   if (! defined $key)
   {  return undef;
   }

   if (exists $old{$key})                      # already processed
   {  close INFIL;
      return $old{$key}; 
   }

   $nr = ++$objNr;
   
   $objekt[$nr] = $pos;
   
   $formRes = $form{$fSource}->[fRESOURCE];   
    
   ($del1, $del2) = getKnown(\$form{$fSource}, $key);  

   $nyDel1 = '<</Type/XObject/Subtype/Form/FormType 1'; 
   $nyDel1 .= "/Resources $formRes" .
                 '/BBox [' .
                 $form{$fSource}->[fBBOX]->[0]  . ' ' .
                 $form{$fSource}->[fBBOX]->[1]  . ' ' .
                 $form{$fSource}->[fBBOX]->[2]  . ' ' .
                 $form{$fSource}->[fBBOX]->[3]  . ' ]' .  
                 # "\]/Matrix \[ $sX 0 0 $sX $tX $tY \]" .
                 $$del1;
   $nyDel1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/translate() . ' 0 R'/oegs;

   $utrad = "$nr 0 obj" . $nyDel1 . $$del2;
   
   $pos += syswrite UTFIL, $utrad;                    
   close INFIL;

   return $nr;   
}

# end of PDF::Reuse::byggForm
1;
