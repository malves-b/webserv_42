#!/usr/bin/perl
use strict;
use warnings;

print "Content-Type: text/html\r\n\r\n";
print "<html><body><h2>Environment Variables</h2><pre>";
foreach my $key (sort keys %ENV) {
    print "$key = $ENV{$key}\n";
}
print "</pre></body></html>";