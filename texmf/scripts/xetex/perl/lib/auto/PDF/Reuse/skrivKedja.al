# NOTE: Derived from blib/lib/PDF/Reuse.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package PDF::Reuse;

#line 5845 "blib/lib/PDF/Reuse.pm (autosplit into blib/lib/auto/PDF/Reuse/skrivKedja.al)"
sub skrivKedja
{  my $code = ' ';
   
   for (values %initScript)
   {   $code .= $_ . "\n";
   }
   $code .= "function Init()\r{\r";
   $code .= 'if (typeof this.info.ModDate == "object")' . "\r{ return true; }\r"; 
   for (@inits)
   {  $code .= $_ . "\n";
   }
   $code .= "}\r Init(); ";

   my $spar = skrivJS($code);
   undef @inits;
   undef %initScript;
   return $spar;
}

# end of PDF::Reuse::skrivKedja
1;
