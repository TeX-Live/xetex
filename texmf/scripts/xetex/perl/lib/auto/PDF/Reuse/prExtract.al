# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 3036 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/prExtract.al)"
sub prExtract
{  my $name = shift;
   my $form = shift;
   my $page = shift || 1;
   if ($name =~ m'^/(\w+)'o)
   {  $name = $1;
   }
   my $fullName = "$name~$form~$page";
   if (exists $knownToFile{$fullName})
   {   return $knownToFile{$fullName};
   }
   else
   {   if ($runfil)
       {  $log = "Extract~$fullName\n";
       }
       if (! $pos)
       {  errLog("No output file, you have to call prFile first");
       }
   
       if (! exists $form{$form . '_' . $page})
       {  prForm($form, $page, undef, 'load', 1);
       }
       $name = extractName($form, $page, $name);
       if ($name)
       {  $knownToFile{$fullName} = $name;
       }
       return $name;
   }
}

# end of PDF::Reuse::prExtract
1;
