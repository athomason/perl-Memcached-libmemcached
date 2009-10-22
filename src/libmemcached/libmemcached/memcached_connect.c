#include "common.h"
#include <netdb.h>
#include <poll.h>
#include <sys/time.h>

static memcached_return set_hostinfo(memcached_server_st *server)
{
  struct addrinfo *ai;
  struct addrinfo hints;
  int e;
  char str_port[NI_MAXSERV];

  sprintf(str_port, "%u", server->port);

  memset(&hints, 0, sizeof(hints));

 // hints.ai_family= AF_INET;
  if (server->type == MEMCACHED_CONNECTION_UDP)
  {
    hints.ai_protocol= IPPROTO_UDP;
    hints.ai_socktype= SOCK_DGRAM;
  }
  else
  {
    hints.ai_socktype= SOCK_STREAM;
    hints.ai_protocol= IPPROTO_TCP;
  }

  e= getaddrinfo(server->hostname, str_port, &hints, &ai);
  if (e != 0)
  {
    WATCHPOINT_STRING(server->hostname);
    WATCHPOINT_STRING(gai_strerror(e));
    return MEMCACHED_HOST_LOOKUP_FAILURE;
  }

  if (server->address_info)
  {
    freeaddrinfo(server->address_info);
    server->address_info= NULL;
  }
  server->address_info= ai;

  return MEMCACHED_SUCCESS;
}

static memcached_return set_socket_options(memcached_server_st *ptr)
{
  WATCHPOINT_ASSERT(ptr->fd != -1);

  if (ptr->type == MEMCACHED_CONNECTION_UDP)
    return MEMCACHED_SUCCESS;

#ifdef HAVE_SNDTIMEO
  if (ptr->root->snd_timeout)
  {
    int error;
    struct timeval waittime;

    waittime.tv_sec= 0;
    waittime.tv_usec= ptr->root->snd_timeout;

    error= setsockopt(ptr->fd, SOL_SOCKET, SO_SNDTIMEO,
                      &waittime, (socklen_t)sizeof(struct timeval));
    WATCHPOINT_ASSERT(error == 0);
  }
#endif

#ifdef HAVE_RCVTIMEO
  if (ptr->root->rcv_timeout)
  {
    int error;
    struct timeval waittime;

    waittime.tv_sec= 0;
    waittime.tv_usec= ptr->root->rcv_timeout;

    error= setsockopt(ptr->fd, SOL_SOCKET, SO_RCVTIMEO,
                      &waittime, (socklen_t)sizeof(struct timeval));
    WATCHPOINT_ASSERT(error == 0);
  }
#endif

  if (ptr->root->flags & MEM_NO_BLOCK)
  {
    int error;
    struct linger linger;

    linger.l_onoff= 1;
    linger.l_linger= 0; /* By default on close() just drop the socket */
    error= setsockopt(ptr->fd, SOL_SOCKET, SO_LINGER,
                      &linger, (socklen_t)sizeof(struct linger));
    WATCHPOINT_ASSERT(error == 0);
  }

  if (ptr->root->flags & MEM_TCP_NODELAY)
  {
    int flag= 1;
    int error;

    error= setsockopt(ptr->fd, IPPROTO_TCP, TCP_NODELAY,
                      &flag, (socklen_t)sizeof(int));
    WATCHPOINT_ASSERT(error == 0);
  }

  if (ptr->root->send_size)
  {
    int error;

    error= setsockopt(ptr->fd, SOL_SOCKET, SO_SNDBUF,
                      &ptr->root->send_size, (socklen_t)sizeof(int));
    WATCHPOINT_ASSERT(error == 0);
  }

  if (ptr->root->recv_size)
  {
    int error;

    error= setsockopt(ptr->fd, SOL_SOCKET, SO_RCVBUF,
                      &ptr->root->recv_size, (socklen_t)sizeof(int));
    WATCHPOINT_ASSERT(error == 0);
  }

  /* libmemcached will always use nonblocking IO to avoid write deadlocks */
  int flags;

  do
    flags= fcntl(ptr->fd, F_GETFL, 0);
  while (flags == -1 && (errno == EINTR || errno == EAGAIN));

  unlikely (flags == -1)
    return MEMCACHED_CONNECTION_FAILURE;
  else if ((flags & O_NONBLOCK) == 0)
  {
    int rval;

    do
      rval= fcntl(ptr->fd, F_SETFL, flags | O_NONBLOCK);
    while (rval == -1 && (errno == EINTR || errno == EAGAIN));

    unlikely (rval == -1)
      return MEMCACHED_CONNECTION_FAILURE;
  }

  return MEMCACHED_SUCCESS;
}

static memcached_return unix_socket_connect(memcached_server_st *ptr)
{
  struct sockaddr_un servAddr;
  socklen_t addrlen;

  if (ptr->fd == -1)
  {
    if ((ptr->fd= socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
      ptr->cached_errno= errno;
      return MEMCACHED_CONNECTION_SOCKET_CREATE_FAILURE;
    }

    memset(&servAddr, 0, sizeof (struct sockaddr_un));
    servAddr.sun_family= AF_UNIX;
    strcpy(servAddr.sun_path, ptr->hostname); /* Copy filename */

    addrlen= (socklen_t) (strlen(servAddr.sun_path) + sizeof(servAddr.sun_family));

test_connect:
    if (connect(ptr->fd,
                (struct sockaddr *)&servAddr,
                sizeof(servAddr)) < 0)
    {
      switch (errno)
      {
      case EINPROGRESS:
      case EALREADY:
      case EINTR:
        goto test_connect;
      case EISCONN: /* We were spinning waiting on connect */
        break;
      default:
        WATCHPOINT_ERRNO(errno);
        ptr->cached_errno= errno;
        return MEMCACHED_ERRNO;
      }
    }
  }

  WATCHPOINT_ASSERT(ptr->fd != -1);
  return MEMCACHED_SUCCESS;
}

static memcached_return network_connect(memcached_server_st *ptr)
{
  if (ptr->fd == -1)
  {
    struct addrinfo *use;

    if (!ptr->sockaddr_inited ||
        (!(ptr->root->flags & MEM_USE_CACHE_LOOKUPS)))
    {
      memcached_return rc;

      rc= set_hostinfo(ptr);
      if (rc != MEMCACHED_SUCCESS)
        return rc;
      ptr->sockaddr_inited= true;
    }

    use= ptr->address_info;
    /* Create the socket */
    while (use != NULL)
    {
      /* Memcache server does not support IPV6 in udp mode, so skip if not ipv4 */
      if (ptr->type == MEMCACHED_CONNECTION_UDP && use->ai_family != AF_INET)
      {
        use= use->ai_next;
        continue;
      }

      if ((ptr->fd= socket(use->ai_family,
                           use->ai_socktype,
                           use->ai_protocol)) < 0)
      {
        ptr->cached_errno= errno;
        WATCHPOINT_ERRNO(errno);
        return MEMCACHED_CONNECTION_SOCKET_CREATE_FAILURE;
      }

      (void)set_socket_options(ptr);

      /* connect to server */
      while (ptr->fd != -1 &&
             connect(ptr->fd, use->ai_addr, use->ai_addrlen) < 0)
      {
        ptr->cached_errno= errno;
        if (errno == EINPROGRESS || /* nonblocking mode - first return, */
            errno == EALREADY) /* nonblocking mode - subsequent returns */
        {
          struct pollfd fds[1];
          fds[0].fd = ptr->fd;
          fds[0].events = POLLOUT;
          int error= poll(fds, 1, ptr->root->connect_timeout);

          if (error != 1 || fds[0].revents & POLLERR)
          {
            if (fds[0].revents & POLLERR)
            {
              int err;
              socklen_t len = sizeof (err);
              (void)getsockopt(ptr->fd, SOL_SOCKET, SO_ERROR, &err, &len);
              ptr->cached_errno= (err == 0) ? errno : err;
            }

            (void)close(ptr->fd);
            ptr->fd= -1;
          }
        }
        else if (errno == EISCONN) /* we are connected :-) */
        {
          break;
        }
        else if (errno != EINTR)
        {
          (void)close(ptr->fd);
          ptr->fd= -1;
          break;
        }
      }

      if (ptr->fd != -1)
      {
        WATCHPOINT_ASSERT(ptr->cursor_active == 0);
        ptr->server_failure_counter= 0;
        return MEMCACHED_SUCCESS;
      }
      use = use->ai_next;
    }
  }

  if (ptr->fd == -1)
  {
    /* Failed to connect. schedule next retry */
    if (ptr->root->retry_timeout)
    {
      struct timeval next_time;

      if (gettimeofday(&next_time, NULL) == 0)
        ptr->next_retry= next_time.tv_sec + ptr->root->retry_timeout;
    }
    ptr->server_failure_counter++;
    if (ptr->cached_errno == 0)
      return MEMCACHED_TIMEOUT;

    return MEMCACHED_ERRNO; /* The last error should be from connect() */
  }

  ptr->server_failure_counter= 0;
  return MEMCACHED_SUCCESS; /* The last error should be from connect() */
}


memcached_return memcached_connect(memcached_server_st *ptr)
{
  memcached_return rc= MEMCACHED_NO_SERVERS;
  LIBMEMCACHED_MEMCACHED_CONNECT_START();

  /* both retry_timeout and server_failure_limit must be set in order to delay retrying a server on error. */
  WATCHPOINT_ASSERT(ptr->root);
  if (ptr->root->retry_timeout && ptr->root->server_failure_limit)
  {
    struct timeval curr_time;

    gettimeofday(&curr_time, NULL);

    /* if we've had too many consecutive errors on this server, mark it dead. */
    if (ptr->server_failure_counter >= ptr->root->server_failure_limit)
    {
      ptr->next_retry= curr_time.tv_sec + ptr->root->retry_timeout;
      ptr->server_failure_counter= 0;
    }

    if (curr_time.tv_sec < ptr->next_retry)
    {
      if (memcached_behavior_get(ptr->root, MEMCACHED_BEHAVIOR_AUTO_EJECT_HOSTS))
        run_distribution(ptr->root);

      ptr->root->last_disconnected_server = ptr;
      return MEMCACHED_SERVER_MARKED_DEAD;
    }
  }

  /* We need to clean up the multi startup piece */
  switch (ptr->type)
  {
  case MEMCACHED_CONNECTION_UNKNOWN:
    WATCHPOINT_ASSERT(0);
    rc= MEMCACHED_NOT_SUPPORTED;
    break;
  case MEMCACHED_CONNECTION_UDP:
  case MEMCACHED_CONNECTION_TCP:
    rc= network_connect(ptr);
    break;
  case MEMCACHED_CONNECTION_UNIX_SOCKET:
    rc= unix_socket_connect(ptr);
    break;
  default:
    WATCHPOINT_ASSERT(0);
  }

  unlikely ( rc != MEMCACHED_SUCCESS) ptr->root->last_disconnected_server = ptr;

  LIBMEMCACHED_MEMCACHED_CONNECT_END();

  return rc;
}
