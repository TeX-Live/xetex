#!perl

# make name list for teckit compiler from UnicodeData.txt

print << '__END__';
struct CharName {
	unsigned long	usv;
	const char*		name;
};

CharName	gUnicodeNames[] = {
__END__

open FH, "<UnicodeData.txt" or die;
while (<FH>) {
	chomp;
	@fields = split(/;/);
	$uc = $fields[0];
	$un = $fields[1];

	next if $un =~ /</;
	
	print "{0x$uc,\"$un\"},\n";
}
close FH;

print << '__END__';
{0,0}
};
__END__
