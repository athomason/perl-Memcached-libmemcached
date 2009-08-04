
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
        memcached_generate_hash
    ),
    #   other functions used by the tests
    qw(
        memcached_fetch
        memcached_set
        memcached_set_by_key
    );

use lib 't/lib';
use libmemcached_test;

my $memc = libmemcached_test_create();

my $items = 5;
plan tests => ($items * 9) + 3;

my ($rv, $rc, $flags, $tmp);
my $t1= time();
my $m1= "master-key";
my $m2= "master-key2";
my $m2h = memcached_generate_hash($memc, $m2);

my %data = map { ("k$_.$t1" => "v$_.$t1") } (1..$items-2);
# add extra long and extra short items to help spot buffer issues
$data{"kL.LLLLLLLLLLLLLLLLLL"} = "vLLLLLLLLLLLLLLLLLLLL";
$data{"kS.S"} = "vS";

for (keys %data) {
    ok memcached_set($memc, $_, $data{$_}), "set $_";
    ok memcached_set_by_key($memc, $m1, $_ . '_by_key', $data{$_}), "set $_ by key";
    ok memcached_set_by_key($memc, $m2, $_ . '_by_hash', $data{$_}), "set $_ by hash"; # assumes that set_by_key uses memcached_generate_hash
}

my $batch = memcached_batch_create($memc);
ok $batch, 'memcached_batch_create';
is ref $batch, 'Memcached::libmemcached::batch', '$batch ISA Memcached::libmemcached::batch';

for my $k1 (keys %data) {
    memcached_batch_get($batch, $k1);
    memcached_batch_get_by_key($batch, $k1 . '_by_key', $m1);
    memcached_batch_get_by_hash($batch, $k1 . '_by_hash', $m2h);
}

memcached_mget_batch($memc, $batch);

my %got;
my $key;
while (defined( my $value = memcached_fetch($memc, $key, $flags, $rc) )) {
    ok $rc, 'rc should be true';
    is $flags, 0, 'flags should be 0';
#    print "memcached_fetch($key) => $value\n";
    $got{ $key } = $value;
}

for (keys %data) {
    $data{$_ . '_by_key'} = $data{$_};
    $data{$_ . '_by_hash'} = $data{$_};
}

is_deeply \%got, \%data;
