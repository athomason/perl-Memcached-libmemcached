package Memcached::libmemcached::memcached_servers;

=head1 NAME

memcached_server_count, memcached_server_list, memcached_server_add, memcached_server_push

=head1 LIBRARY

C Client Library for memcached (libmemcached, -lmemcached)

=head1 SYNOPSIS

  #include <memcached.h>

  unsigned int memcached_server_count (memcached_st *ptr);

  memcached_server_st *
    memcached_server_list (memcached_st *ptr);

  memcached_return
    memcached_server_add (memcached_st *ptr,
                          char *hostname,
                          unsigned int port);

  memcached_return
    memcached_server_add_unix_socket (memcached_st *ptr,
                                      char *socket);

  memcached_return
    memcached_server_push (memcached_st *ptr,
                           memcached_server_st *list);

=head1 DESCRIPTION

libmemcached(3) performs operations on a list of hosts. The order of these
hosts determine routing to keys. Functions are provided to add keys to
memcached_st structures. To manipulate lists of servers see
memcached_server_st(3).

memcached_server_count() provides you a count of the current number of
servers being used by a C<memcached_st> structure.

memcached_server_list() is used to provide an array of all defined hosts.
You are responsible for freeing this list (aka it is not a pointer to the
currently used structure).

memcached_server_add() pushes a single server into the C<memcached_st>
structure. This server will be placed at the end. Duplicate servers are
allowed, so duplication is not checked.

memcached_server_add_unix_socket() pushes a single UNIX socket into the 
C<memcached_st> structure. This UNIX socket will be placed at the end. 
Duplicate servers are allowed, so duplication is not checked. The length
of the filename must be one character less then MEMCACHED_MAX_HOST_LENGTH.

memcached_server_push() pushes an array of C<memcached_server_st> into
the C<memcached_st> structure. These servers will be placed at the
end. Duplicate servers are allowed, so duplication is not checked. A
copy is made of structure so the list provided (and any operations on
the list) are not saved.

=head1 RETURN

Varies, see particular functions.

=head1 HOME

To find out more information please check: 
L<http://tangent.org/552/libmemcached.html>

=head1 AUTHOR

Brian Aker, E<lt>brian@tangent.orgE<gt>

=head1 SEE ALSO

memcached(1) libmemcached(3) memcached_strerror(3)

=cut

1;
