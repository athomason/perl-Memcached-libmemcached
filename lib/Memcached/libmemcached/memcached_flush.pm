package Memcached::libmemcached::memcached_flush;

=head1 NAME

memcached_flush

=head1 LIBRARY

C Client Library for memcached (libmemcached, -lmemcached)

=head1 SYNOPSIS

  #include <memcached.h>

  memcached_return
    memcached_flush (memcached_st *ptr,
                     time_t expiration);

=head1 DESCRIPTION

memcached_flush() is used to wipe clean the contents of memcached(1) servers.
It will either do this immediately or expire the content based on the
expiration time passed to the method (a value of zero causes an immediate
flush). The operation is not atomic to multiple servers, just atomic to a
single server. That is, it will flush the servers in the order that they were
added.

=head1 RETURN

A value of type C<memcached_return> is returned
On success that value will be C<MEMCACHED_SUCCESS>.
Use memcached_strerror() to translate this value to a printable string.

=head1 HOME

To find out more information please check:
L<http://tangent.org/552/libmemcached.html>

=head1 AUTHOR

Brian Aker, E<lt>brian@tangent.orgE<gt>

=head1 SEE ALSO

memcached(1) libmemcached(3) memcached_strerror(3)

=cut

1;
