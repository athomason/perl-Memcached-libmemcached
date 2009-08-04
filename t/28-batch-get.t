
# tests for batch gets by memcached_get_batch
# documented in memcached_batch.pod

use strict;
use warnings;

use Test::More;

use Memcached::libmemcached
    #   functions explicitly tested by this file
    qw(
        memcached_batch_create
        memcached_batch_get
        memcached_batch_get_by_hash
        memcached_batch_get_by_key
        memcached_mget_batch
    ),
    #   other functions used by the tests
    qw(
        memcached_fetch
        memcached_set
    );

use lib 't/lib';
use libmemcached_test;

my $memc = libmemcached_test_create();

my $items = 5;
plan tests => ($items * 3) + 3
    + 2 * (1 + $items * 2 + 1)
    + $items + 6
    + $items + 7
    + 1;

my ($rv, $rc, $flags, $tmp);
my $t1= time();
my $m1= "master-key"; # can't have spaces

my %data = map { ("k$_.$t1" => "v$_.$t1") } (1..$items-2);
# add extra long and extra short items to help spot buffer issues
$data{"kL.LLLLLLLLLLLLLLLLLL"} = "vLLLLLLLLLLLLLLLLLLLL";
$data{"kS.S"} = "vS";

ok memcached_set($memc, $_, $data{$_})
    for keys %data;

my $batch = memcached_batch_create($memc);
ok $batch;
is ref $batch, 'Memcached::libmemcached::batch';

my $m1h = 1;
for my $k1 (keys %data) {
    memcached_batch_get($batch, $k1);
    memcached_batch_get_by_key($batch, $k1, $m1);
    memcached_batch_get_by_hash($batch, $k1, $m1h);
}

memcached_mget_batch($memc, $batch);

my %got;
my $key;
while (defined( my $value = memcached_fetch($memc, $key, $flags, $rc) )) {
    ok $rc, 'rc should be true';
    is $flags, 0, 'flags should be 0';
    print "memcached_fetch($key) => $value\n";
    $got{ $key } = $value;
}

is_deeply \%got, \%data;
