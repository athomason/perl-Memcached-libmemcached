
# tests for functions documented in memcached_quit.pod

use strict;
use warnings;

use Test::More;

use Memcached::libmemcached
    #   functions explicitly tested by this file
    qw(
        memcached_quit
    ),
    #   other functions used by the tests
    qw(
        memcached_set
        memcached_get
    );

use lib 't/lib';
use libmemcached_test;

my $memc = libmemcached_test_create();

plan tests => 2;

ok $memc;

memcached_set($memc, 'foo' => 'bar', 0);

memcached_quit($memc); # closes connections but they'll be recreated
memcached_quit(undef); # does nothing but shouldn't die

is memcached_get($memc, 'foo'), 'bar';
