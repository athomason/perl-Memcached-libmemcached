package Memcached::libmemcached::memcached_delete;

=head1 NAME

memcached_delete

=head1 LIBRARY

C Client Library for memcached (libmemcached, -lmemcached)

=head1 SYNOPSIS

  #include <memcached.h>

  memcached_return
    memcached_delete (memcached_st *ptr,
                      const char *key, size_t key_length,
                      time_t expiration);

  memcached_return
  memcached_delete_by_key (memcached_st *ptr,
                           const char *master_key, size_t master_key_length,
                           const char *key, size_t key_length,
                           time_t expiration);

=head1 DESCRIPTION

memcached_delete() is used to delete a particular key. An expiration value
can be applied so that the key is deleted after that many seconds.
memcached_delete_by_key() works the same, but it takes a master key to 
find the given value.

=head1 RETURN

A value of type C<memcached_return> is returned
On success that value will be C<MEMCACHED_SUCCESS>.
Use memcached_strerror() to translate this value to a printable string.

If you are using the non-blocking mode of the library, success only
means that the message was queued for delivery.

=head1 HOME

To find out more information please check:
L<http://tangent.org/552/libmemcached.html>

=head1 AUTHOR

Brian Aker, E<lt>brian@tangent.orgE<gt>

=head1 SEE ALSO

memcached(1) libmemcached(3) memcached_strerror(3)

=cut

1;
