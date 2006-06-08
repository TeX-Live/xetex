# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 5945 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/defInit.al)"
sub defInit
{  my $code = "function Init()\r{\r";
   $code .= 'if (typeof this.info.ModDate == "object")' . "\r{ return true; }\r"; 
   for (@inits)
   {  $code .= $_ . "\n";
   }
   $code .= '}';
             
   my $ny = skrivJS($code);        
   return $ny;

}

# end of PDF::Reuse::defInit
1;
