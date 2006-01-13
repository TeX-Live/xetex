# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3372 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/checkContentStream.al)"
sub checkContentStream
{  for (@_)
   {  if (my $value = $objRef{$_})
      {   my $typ = substr($_, 0, 2);
          if ($typ eq 'Ft')
          {  $sidFont{$_} = $value;
          }
          elsif ($typ eq 'Gs')
          {  $sidExtGState{$_} = $value;
          }
          elsif ($typ eq 'Pt')
          {  $sidPattern{$_} = $value;
          }
          elsif ($typ eq 'Sh')
          {  $sidShading{$_} = $value;
          }
          elsif ($typ eq 'Cs')
          {  $sidColorSpace{$_} = $value;
          }
          else
          {  $sidXObject{$_} = $value;
          }
      }
      elsif (($_ eq 'Gs0') && (! defined $defGState))
      {  my ($dummy, $oNr) = prDefaultGrState();
         $sidExtGState{'Gs0'} = $oNr;
      }
   }    
}

# end of PDF::Reuse::checkContentStream
1;
