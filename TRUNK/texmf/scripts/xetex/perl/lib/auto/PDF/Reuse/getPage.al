# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 4100 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/getPage.al)"
############################################
# En definitionerna för en sida extraheras
############################################

sub getPage
{  my ($infil, $sidnr, $action)  = @_;

   my ($res, $i, $referens,$objNrSaved,$validStream, $formRes, @objData, 
       @underObjekt, @sidObj, $strPos, $startSida, $sidor, $filId, $del1, $del2,
       $offs, $siz, $embedded, $vektor, $utrad, $robj, $valid, $Annots, $Names,
       $AcroForm, $AARoot, $AAPage);

   my $sidAcc = 0;
   my $seq    = 0;
   $imSeq     = 0;
   @skapa     = ();   
   undef $formCont;
   
   
   $objNrSaved = $objNr;   
   my $fSource = $infil . '_' . $sidnr;
   my $checkidOld = $checkId;
   ($infil, $checkId) = findGet($infil, $checkidOld);
   if (($ldir) && ($checkId) && ($checkId ne $checkidOld))
   {  $log .= "Cid~$checkId\n";
   }
   $form{$fSource}[fID] =  $checkId;
   $checkId = '';
   $behandlad{$infil}->{old} = {} 
        unless (defined $behandlad{$infil}->{old});
   $processed{$infil}->{oldObject} = {} 
        unless (defined $processed{$infil}->{oldObject});   
   $processed{$infil}->{unZipped} = {} 
        unless (defined $processed{$infil}->{unZipped});

   if ($action eq 'print')
   {  *old = $behandlad{$infil}->{old};
   }
   else
   {  $behandlad{$infil}->{dummy} = {};
      *old = $behandlad{$infil}->{dummy};
   }
   
   *oldObject =  $processed{$infil}->{oldObject};
   *unZipped  = $processed{$infil}->{unZipped};
   $root      = (exists $processed{$infil}->{root}) 
                    ? $processed{$infil}->{root} : 0;
   
   
   my @stati = stat($infil);
   open (INFIL, "<$infil") || errLog("Couldn't open $infil, $!");
   binmode INFIL;

   if (! $root)
   {  $root = xRefs($stati[7], $infil);
   }

   #############
   # Hitta root
   #############           

   my $objektet = getObject($root);;
   
   if ($sidnr == 1) 
   {  if ($objektet =~ m'/AcroForm(\s+\d+\s{1,2}\d+\s{1,2}R)'so)
      {  $AcroForm = $1;
      }
      if ($objektet =~ m'/Names\s+(\d+)\s{1,2}\d+\s{1,2}R'so)
      {  $Names = $1;
      } 
      #################################################
      #  Finns ett dictionary för Additional Actions ?
      #################################################
      if ($objektet =~ m'/AA\s*\<\<\s*[^\>]+[^\>]+'so) # AA är ett dictionary
      {  my $k;
         my ($dummy, $obj) = split /\/AA/, $objektet;
         $obj =~ s/\<\</\#\<\</gs;
         $obj =~ s/\>\>/\>\>\#/gs;
         my @ord = split /\#/, $obj;
         for ($i = 0; $i <= $#ord; $i++)
         {   $AARoot .= $ord[$i];
             if ($ord[$i] =~ m'\S+'os)
             {  if ($ord[$i] =~ m'<<'os)
                {  $k++; }
                if ($ord[$i] =~ m'>>'os)
                {  $k--; }
                if ($k == 0)
                {  last; }
             } 
          }
      }
   }
     
   #
   # Hitta pages
   #
 
   if ($objektet =~ m'/Pages\s+(\d+)\s{1,2}\d+\s{1,2}R'os)
   {  $objektet = getObject($1);
      if ($objektet =~ m'/Count\s+(\d+)'os)
      {  $sidor = $1;
         if ($sidnr <= $sidor)
         {  ($formRes, $valid) = kolla($objektet); 
         }
         else
         {   return 0;
         }
         if ($sidor > 1)
         {   undef $AcroForm;
             undef $Names;
             undef $AARoot;
             if ($type eq 'docform')
             {  errLog("prDocForm can only be used for single page documents - try prDoc or reformat $infil");
             }
         }
      }
   }
   else
   { errLog("Didn't find Pages in $infil - aborting"); }

   if ($objektet =~ m'/Kids\s*\[([^\]]+)'os)
   {  $vektor = $1; } 
   while ($vektor =~ m'(\d+)\s{1,2}\d+\s{1,2}R'go)
   {   push @sidObj, $1;       
   }

   my $bryt1 = -20;                     # Hängslen
   my $bryt2 = -20;                     # Svångrem för att undvika oändliga loopar
   
   while ($sidAcc < $sidnr)
   {  @underObjekt = @sidObj;
      @sidObj     = ();
      $bryt1++;
      for my $uO (@underObjekt)
      {  $objektet = getObject($uO);
         if ($objektet =~ m'/Count\s+(\d+)'os)
         {  if (($sidAcc + $1) < $sidnr)
            {  $sidAcc += $1; }
            else
            {  ($formRes, $valid) = kolla($objektet, $formRes);
               if ($objektet =~ m'/Kids\s*\[([^\]]+)'os)
               {  $vektor = $1; } 
               while ($vektor =~ m'(\d+)\s{1,2}\d+\s{1,2}R'gso)
               {   push @sidObj, $1;  }
               last; 
            }
         }
         else
         {  $sidAcc++; }
         if ($sidAcc == $sidnr)
         {   $seq = $uO;
             last;  }
         $bryt2++;
      }
      if (($bryt1 > $sidnr) || ($bryt2 > $sidnr))   # Bryt oändliga loopar 
      {  last; } 
   }    

   ($formRes, $validStream) = kolla($objektet, $formRes);
   $startSida = $seq;
       
   if ($sidor == 1)
   {  #################################################
      # Kontrollera Page-objektet för annoteringar
      #################################################

      if ($objektet =~ m'/Annots\s*([^\/]+)'so)
      {  $Annots = $1;
      } 
      #################################################
      #  Finns ett dictionary för Additional Actions ?
      #################################################
      if ($objektet =~ m'/AA\s*\<\<\s*[^\>]+[^\>]+'so)  # AA är ett dictionary. Hela kopieras
      {  my $k;
         my ($dummy, $obj) = split /\/AA/, $objektet;
         $obj =~ s/\<\</\#\<\</gs;
         $obj =~ s/\>\>/\>\>\#/gs;
         my @ord = split /\#/, $obj;
         for ($i = 0; $i <= $#ord; $i++)
         {   $AAPage .= $ord[$i];
             if ($ord[$i] =~ m'\S+'s)
             {  if ($ord[$i] =~ m'<<'s)
                {  $k++; }
                if ($ord[$i] =~ m'>>'s)
                {  $k--; }
                if ($k == 0)
                {  last; }
             } 
          }
      }      
   }

   my $rform = \$form{$fSource};
   @$$rform[fRESOURCE]  = $formRes;
   my @BBox;
   if (defined $formBox[0])
   {  $BBox[0] = $formBox[0]; }
   else
   {  $BBox[0] = $genLowerX; }
 
   if (defined $formBox[1])
   {  $BBox[1] = $formBox[1]; }
   else
   {  $BBox[1] = $genLowerY; }
 
   if (defined $formBox[2])
   {  $BBox[2] = $formBox[2]; }
   else
   {  $BBox[2] = $genUpperX; }
 
   if (defined $formBox[3])
   {  $BBox[3] = $formBox[3]; }
   else
   {  $BBox[3] = $genUpperY; }
 
   @{$form{$fSource}[fBBOX]} = @BBox;

   if ($formCont) 
   {   $seq = $formCont;
       ($objektet, $offs, $siz, $embedded) = getObject($seq);
       
       $robj  = \$$$rform[fOBJ]->{$seq};
       @{$$$robj[oNR]} = ($offs, $siz, $embedded);
       $$$robj[oFORM] = 'Y';
       $form{$fSource}[fMAIN] = $seq;
       if ($objektet =~ m'^(\d+ \d+ obj\s*<<)(.+)(>>\s*stream)'so)
       {  $del1   = $2;
          $strPos           = length($1) + length($2) + length($3);
          $$$robj[oPOS]     = length($1);      
          $$$robj[oSTREAMP] = $strPos; 
          my $nyDel1;
          $nyDel1 = '<</Type/XObject/Subtype/Form/FormType 1'; 
          $nyDel1 .= "/Resources $formRes" .
                     "/BBox \[ $BBox[0] $BBox[1] $BBox[2] $BBox[3]\]" .
                     # "/Matrix \[ 1 0 0 1 0 0 \]" .
                     $del1;
          if ($action eq 'print')
          {  $objNr++;
             $objekt[$objNr] = $pos;
          }
          $referens = $objNr;

          $res = ($nyDel1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs);
          if ($res)
          { $$$robj[oKIDS] = 1; }
          if ($action eq 'print')
          {   $utrad  = "$referens 0 obj\n" . "$nyDel1" . ">>\nstream";
              $del2   = substr($objektet, $strPos);
              $utrad .= $del2;
              $pos   += syswrite UTFIL, $utrad;
          }
          $form{$fSource}[fVALID] = $validStream;
      }
      else                              # Endast resurserna kan behandlas
      {   $formRes =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;  
      }
   }
   else                                # Endast resurserna kan behandlas
   {  $formRes =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
   } 
      
   my $preLength;
   while (scalar @skapa)
   {  my @process = @skapa;
      @skapa = ();
      for (@process)
      {  my $Font;
         my $gammal = $$_[0];
         my $ny     = $$_[1];
         ($objektet, $offs, $siz, $embedded)  = getObject($gammal);
         $robj      = \$$$rform[fOBJ]->{$gammal};
         @{$$$robj[oNR]} = ($offs, $siz, $embedded);      
         if ($objektet =~ m'^(\d+ \d+ obj\s*<<)(.+)(>>\s*stream)'os)
         {  $del1             = $2;
            $strPos           = length ($1) + length($2) + length($3);
            $$$robj[oPOS]     = length($1);
            $$$robj[oSTREAMP] = $strPos;
 
            ######## En bild ########
            if ($del1 =~ m'/Subtype\s*/Image'so)
            {  $imSeq++;
               $$$robj[oIMAGENR] = $imSeq;
               push @{$$$rform[fIMAGES]}, $gammal;

               if ($del1 =~ m'/Width\s+(\d+)'os)
               {  $$$robj[oWIDTH] = $1; }
               if ($del1 =~ m'/Height\s+(\d+)'os)
               {  $$$robj[oHEIGHT] = $1; }
            }     
            $res = ($del1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs);
            if ($res)
            { $$$robj[oKIDS] = 1; }            
            if ($action eq 'print')
            {   $objekt[$ny] = $pos;
                $utrad  = "$ny 0 obj\n<<" . "$del1" . '>>stream';
                $del2   = substr($objektet, $strPos);
                $utrad .= $del2; 
            }
         }
         else
         {  if ($objektet =~ m'^(\d+ \d+ obj\s*)'os)
            {  $preLength = length($1);
               $$$robj[oPOS] = $preLength;               
               $objektet     = substr($objektet, $preLength);
            }
            else
            {  $$$robj[oPOS] = 0;
            }
            $res = ($objektet =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs);
            if ($res)
            { $$$robj[oKIDS] = 1; }
            if ($objektet =~ m'/Subtype\s*/Image'so)
            {  $imSeq++;
               $$$robj[oIMAGENR] = $imSeq;
               push @{$$$rform[fIMAGES]}, $gammal;
               ###################################
               # Sparar dimensionerna för bilden
               ###################################
               if ($del1 =~ m'/Width\s+(\d+)'os)
               {  $$$robj[oWIDTH] = $1; }
      
               if ($del1 =~ m'/Height\s+(\d+)'os)
               {  $$$robj[oHEIGHT] = $1; }
            }
            elsif ($objektet =~ m'/BaseFont\s*/([^\s\/]+)'os)
            {  $Font = $1;
               $$$robj[oTYPE] = 'Font';
               $$$robj[oNAME] = $Font;
               if ((! exists $font{$Font}) 
               && ($action))
               {  $fontNr++;
                  $font{$Font}[foINTNAMN]          = 'Ft' . $fontNr;
                  $font{$Font}[foORIGINALNR]       = $gammal;
                  $fontSource{$Font}[foSOURCE]     = $fSource;
                  $fontSource{$Font}[foORIGINALNR] = $gammal;
                  if ($action eq 'print')
                  {  $font{$Font}[foREFOBJ]  = $ny;
                     $objRef{'Ft' . $fontNr} = $ny;
                  }
               }   
            }
               
            if ($action eq 'print')
            {   $objekt[$ny] = $pos;
                $utrad = "$ny 0 obj $objektet";
            }
         }
         if ($action eq 'print')
         {   $pos += syswrite UTFIL, $utrad;
         }
       }
   }
   
   my $ref = \$form{$fSource};
   my @kids;
   my @nokids;  
   
   #################################################################
   # lägg upp vektorer över vilka objekt som har KIDS eller NOKIDS
   #################################################################   

   for my $key (keys %{$$$ref[fOBJ]})
   {   $robj  = \$$$ref[fOBJ]->{$key};
       if (! defined  $$$robj[oFORM])
       {   if (defined  $$$robj[oKIDS])
           {   push @kids, $key; }
           else
           {   push @nokids, $key; }
       }
       if ((defined $$$robj[0]->[2]) && (! exists $$$ref[fOBJ]->{$$$robj[0]->[2]}))
       {  $$$ref[fOBJ]->{$$$robj[0]->[2]}->[0] = $oldObject{$$$robj[0]->[2]};
       }
   }
   if (scalar @kids)
   {  $form{$fSource}[fKIDS] = \@kids; 
   } 
   if (scalar @nokids)
   {  $form{$fSource}[fNOKIDS] = \@nokids; 
   } 
   
   if ($action ne 'print')
   {  $objNr = $objNrSaved;            # Restore objNo if nothing was printed
   }

   $behandlad{$infil}->{dummy} = {};
   *old = $behandlad{$infil}->{dummy};
    
   $objNrSaved = $objNr;               # Save objNo

   if ($sidor == 1)
   {   @skapa = ();
       $old{$startSida} = $sidObjNr;
       my $ref = \$intAct{$fSource};
       @$$ref[iSTARTSIDA] = $startSida;
       if (defined $Names)
       {   @$$ref[iNAMES] = $Names;
           quickxform($Names);
       }
       if (defined $AcroForm)
       {   @$$ref[iACROFORM] = $AcroForm;
           $AcroForm =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
       }
       if (defined $AARoot)
       {   @$$ref[iAAROOT] = $AARoot;
           $AARoot =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
       }
       if (defined $AAPage)
       {   @$$ref[iAAPAGE] = $AAPage;
           $AAPage =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
       }
       if (defined $Annots)
       {   my @array;
           if ($Annots =~ m'\[([^\[\]]*)\]'os)
           {  $Annots = $1;
              @array = ($Annots =~ m'\b(\d+)\s{1,2}\d+\s{1,2}R\b'ogs);  
           }
           else
           {  if ($Annots =~ m'\b(\d+)\s{1,2}\d+\s{1,2}R\b'os)
              {  $Annots = getObject($1);
                 @array = ($Annots =~ m'\b(\d+)\s{1,2}\d+\s{1,2}R\b'ogs);
              }
           }             
           @$$ref[iANNOTS] = \@array;
           $Annots =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs;
       }
      
      while (scalar @skapa)
      {  my @process = @skapa;
         @skapa = ();
         for (@process)
         {  my $gammal = $$_[0];
            my $ny     = $$_[1];
            ($objektet, $offs, $siz, $embedded) = getObject($gammal);
            $robj  = \$$$ref[fOBJ]->{$gammal};
            @{$$$robj[oNR]} = ($offs, $siz, $embedded);
            if ($objektet =~ m'^(\d+ \d+ obj\s*<<)(.+)(>>\s*stream)'os)
            {  $del1             = $2;
               $$$robj[oPOS]     = length($1);
               $$$robj[oSTREAMP] = length($1) + length($2) + length($3);
                  
               $res = ($del1 =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs);
               if ($res)
               { $$$robj[oKIDS] = 1; }  
            }
            else
            {  if ($objektet =~ m'^(\d+ \d+ obj)'os)
               {  my $preLength = length($1);
                  $$$robj[oPOS] = $preLength;
                  $objektet = substr($objektet, $preLength);
                                 
                  $res = ($objektet =~ s/\b(\d+)\s{1,2}\d+\s{1,2}R\b/xform() . ' 0 R'/oegs);
                  if ($res)
                  { $$$robj[oKIDS] = 1; }
                }
             }
         }
      }
      for my $key (keys %{$$$ref[fOBJ]})
      {   $robj  = \$$$ref[fOBJ]->{$key};
          if ((defined $$$robj[0]->[2]) && (! exists $$$ref[fOBJ]->{$$$robj[0]->[2]}))
          {  $$$ref[fOBJ]->{$$$robj[0]->[2]}->[0] = $oldObject{$$$robj[0]->[2]};
          }
      }
  }

  $objNr = $objNrSaved;
  $processed{$infil}->{root}         = $root;
  close INFIL;
  return $referens;
}  

# end of PDF::Reuse::getPage
1;
