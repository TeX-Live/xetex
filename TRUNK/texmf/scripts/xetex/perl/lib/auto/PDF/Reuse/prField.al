# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 2983 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prField.al)"
sub prField
{  my ($fieldName, $fieldValue) = @_;
   if (($interAktivSida) || ($interActive))
   {  errLog("Too late, has already tried to INITIATE FIELDS within an interactive page");
   }
   elsif (! $pos)
   {  errLog("Too early INITIATE FIELDS, create a file first");
   }
   $fields{$fieldName} = $fieldValue;
   if ($fieldValue =~ m'^\s*js\s*\:(.*)'oi)
   {  my $code = $1;
      my @fall = ($code =~ m'([\w\d\_\$]+)\s*\(.*?\)'gs);
      for (@fall)
      {  if (! exists $initScript{$_})
         { $initScript{$_} = 0; 
         }
      }
   }
   if ($runfil)
   {   $fieldName  = prep($fieldName);
       $fieldValue = prep($fieldValue);
       $log .= "Field~$fieldName~$fieldValue\n";
   } 
   1;
}

# end of PDF::Reuse::prField
1;
