
# tests for memcached_prefix_set & memcached_prefix_get

use strict;
use warnings;

use Test::More;

use Memcached::libmemcached
    #   functions explicitly tested by this file
    qw(
    ),
    #   other functions used by the tests
    qw(
    memcached_get
    memcached_set
    memcached_errstr
    );

use lib 't/lib';
use libmemcached_test;

my $prefix = "prefix:";

my $memc_prefix = libmemcached_test_create();
my $memc_noprefix = libmemcached_test_create();

plan tests => 10;

my ($rv, $rc, $flags, $tmp);
my $t1= time();
my $k1= "$0-test-key-$t1"; # can't have spaces
my $v1= "$0 test value $t1";

is $memc_prefix->memcached_prefix_get(), undef;

$rc = $memc_prefix->memcached_prefix_set($prefix);
ok $rc;

is $memc_noprefix->memcached_prefix_get(), undef;
is $memc_prefix->memcached_prefix_get(), $prefix;

ok memcached_set($memc_prefix, $k1, $v1, 1, 0xDEADCAFE);
is memcached_errstr($memc_prefix), 'SUCCESS';

is memcached_get($memc_prefix, $k1, $flags=0, $rc=0), $v1;
ok $rc;

is memcached_get($memc_noprefix, $prefix . $k1, $flags=0, $rc=0), $v1;
ok $rc;
