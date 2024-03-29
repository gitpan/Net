# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use Test;
BEGIN { plan tests => 4 };
require Net;
ok(1); # If we made it this far, we're ok.

#########################

# Insert your test code below, the Test module is use()ed here so read
# its man page ( perldoc Test ) for help writing this test script.

#print &Net::foo(1, 2, "0.0") == 7 ? "ok 3\n" : "not ok 3\n";
#print abs(&Net::foo(0, 0, "-3.4") - 0.6) <= 0.01 ? "ok 4\n" : "not ok 4\n";  

use Devel::Peek;
use Data::Dumper;


# SSL
print "SSL: \n";
if(exists $ENV{SSL}) {
	my $PORT = 2055;
	if($ENV{SSL}) {
		Net->init("./root.pem","./server.pem","bee");
		my $server = Net->server("SSL",$PORT);
		print "listening on ".$server->peer."\n";
		my $net = $server->accept(1);
		print "client: ".$net->peer." (".$net->auth.")\n";

		print "sleeping...\n";
		sleep 3;
		print "writing...\n";
		$net->write("Hello OpenSSL");
		sleep 2;

	} else {
		Net->init("./root.pem","./client.pem","bee");
		my $net = Net->connect("SSL","localhost",$PORT);
		print "server: ".$net->peer." (".$net->auth.")\n";
	
		until($net->peek) { }
		print "read: '".$net->read."'\n";
	}
} else {
	print "use: 'SSL=1 make test' ";
	print  "and 'SSL=0 make test' to try client-server\n";
}
ok(2);

# INET
print "INET: ";
if(exists $ENV{INET}) {
	my $PORT = 2052;
	if($ENV{INET}) {
		my $server = Net->server("INET",$PORT);
		print "server listenning on ".$server->peer."\n";
		my $client = $server->accept(1);
		print "client ".$client->peer." connected\n";
		$client->write("hello from windoze");
		until($client->peek) { }
		print "read: '",$client->read,"'\n";
	} else {
		my $net = Net->connect('INET',"localhost",$PORT);
		print "client connected to server ".$net->peer."\n";
		sleep 1;
		until($net->peek) {  }
		print "read: '",$net->read,"'\n";
		$net->write("data\n");
	}
} else {
#	print "use: 'INET=1 make test' ";
#	print  "and 'INET=0 make test' to try client-server\n";
}
ok(3);


# UNIX
print "UNIX: ";
if(exists $ENV{UNIX}) {
	my($a,$b) = Net->socketpair;
	die "nefunguje" unless($a);

	print "UNIX: socketpair [ ".$a->peer." <-> ".$b->peer." ] created\n";
	$a->write("ahojky");
	until($b->peek) {  }
	print "write -> read: ".$b->read."\n";
}
ok(4);



