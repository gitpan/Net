use ExtUtils::MakeMaker;
use Config;

# The existence of the ./libnet/Makefile.PL file causes MakeMaker
# to automatically include Makefile code for the targets
#	config, all, clean, realclean and libnet/Makefile
# which perform the corresponding actions in the subdirectory.

my $ms = ($^O eq 'MSWin32');

my $define   = $ms ? '-DMSDOS' : '';
my $myextlib = $ms ? 'libnet\\libnet$(LIB_EXT)' : 'libnet/libnet$(LIB_EXT)';

my $usage = <<USAGE;
OpenSSL not found in $openssl
Usage: ./Makefile.PL [OpenSSL install path]
USAGE

my $openssl = $ms ? "c:\\openssl" : "/usr";
die $usage unless(-r $openssl."/include/openssl/ssl.h");

my $ssl_exe;
foreach("bin/openssl","bin/openssl.exe","sbin/openssl") {
	$ssl_exe = "$openssl/$_";
	last if -x $ssl_exe;
}
die $usage unless(-x $ssl_exe);

my $ssl_ver = `$ssl_exe version` or die "Couldn't run `$ssl_exe` ($?)\n";
chomp $ssl_ver;
print "Using OpenSSL from '$openssl' ($ssl_ver)\n";

my $libs = $ms ? "-llibeay32 -lssleay32" : "-lssl -lcrypto",

open F,">libnet/openssl_path" or die "Can't write libnet/openssl_path: $!";
print F $openssl; # used by libnet make
close F;

WriteMakefile(
	NAME		=> 'Net',
	ABSTRACT	=> 'tcp/ip & ssl network wrapper',
	AUTHOR		=> 'Martin Sarfy <xsarfy@fi.muni.cz>',
	VERSION		=> '1.2',

	LIBS        => ["-L$openssl -L$openssl/lib $libs"],
	INC			=> "-I$openssl/include",

	MYEXTLIB 	=> $myextlib,
	MAN3PODS 	=> {}, 	# Pods will be built by installman.
	XSPROTOARG	=> '-noprototypes', 		# XXX remove later?
	VERSION_FROM=> 'Net.pm',
	DEFINE		=> $define,
	PERL_MALLOC_OK => 1,
             );

sub MY::postamble {
  if ($^O =~ /MSWin32/ && Win32::IsWin95()) {
	if ($Config{'make'} =~ /dmake/i) {
	    # dmake-specific
	    return <<'EOT';
$(MYEXTLIB): libnet/Makefile
@[
	cd libnet
	$(MAKE) all
	cd ..
]
EOT
	} elsif ($Config{'make'} =~ /nmake/i) {
	    #
	    return <<'EOT';
$(MYEXTLIB): libnet/Makefile
	cd libnet
	$(MAKE) all
	cd ..
EOT
	} 
} elsif ($^O ne 'VMS') {
    '
$(MYEXTLIB): libnet/Makefile
	cd libnet && $(MAKE) all
';
  }
  else {
    '   
$(MYEXTLIB) : [.libnet]descrip.mms
	set def [.libnet]
    $(MMS) all
    set def [-]
';  
  }
}
