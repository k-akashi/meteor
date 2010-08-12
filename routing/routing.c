
/*
 * Copyright (c) 2006-2009 The StarBED Project  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/************************************************************************
 *
 * QOMET Emulator Implementation
 *
 * File name: routing.c
 * Function: Source file of the routing control API
 *
 * Authors: Takashi Okada, Razvan Beuran
 *
 *   $Revision: 128 $
 *   $LastChangedDate: 2009-02-06 10:21:50 +0900 (Fri, 06 Feb 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifdef	HAVE_SOCKADDR_DL_STRUCT
# include	<net/if_dl.h>
#endif

#include "routing.h"
#include "message.h"

static int rtm_seq = 0;

// get route address
void get_rtaddrs(int addrs, struct sockaddr *sa, struct sockaddr **rti_info)
{
  int i;

  for (i = 0; i < RTAX_MAX; i++)
    {
      if (addrs & (1 << i)) 
	{
	  rti_info[i] = sa;
	  NEXT_SA(sa);
	} 
      else
	rti_info[i] = NULL;
    }
}

// host_serv
struct addrinfo * host_serv(const char *host, const char *serv, int family, 
			    int socktype)
{
  int n;
  struct addrinfo hints, *res;

  bzero(&hints, sizeof(struct addrinfo));
  hints.ai_flags = AI_CANONNAME;	/* always return canonical name */
  hints.ai_family = family;		/* AF_UNSPEC, AF_INET, AF_INET6, etc. */
  hints.ai_socktype = socktype;	/* 0, SOCK_STREAM, SOCK_DGRAM, etc. */

  if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
    return(NULL);

  return(res);	/* return pointer to first on linked list */
}


/*
 * There is no easy way to pass back the integer return code from
 * getaddrinfo() in the function above, short of adding another argument
 * that is a pointer, so the easiest way to provide the wrapper function
 * is just to duplicate the simple function as we do here.
 */

struct addrinfo * Host_serv(const char *host, const char *serv, int family, 
			    int socktype)
{
  int n;
  struct addrinfo hints, *res;

  bzero(&hints, sizeof(struct addrinfo));
  hints.ai_flags = AI_CANONNAME;	/* always return canonical name */
  hints.ai_family = family;		/* 0, AF_INET, AF_INET6, etc. */
  hints.ai_socktype = socktype;	/* 0, SOCK_STREAM, SOCK_DGRAM, etc. */

  if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
    fprintf(stderr, "host_serv error for %s, %s: %s",
	    (host == NULL) ? "(no hostname)" : host,
	    (serv == NULL) ? "(no service name)" : serv,
	    gai_strerror(n));

  return(res);	/* return pointer to first on linked list */
}

// interface name to index
unsigned int if_nametoindex(const char *name)
{
  unsigned int index;
  char *buf, *next, *lim;
  size_t len;
  struct if_msghdr *ifm;
  struct sockaddr *sa, *rti_info[RTAX_MAX];
  struct sockaddr_dl *sdl;

  if ((buf = net_rt_iflist(0, 0, &len)) == NULL)
    return 0;
  lim = buf + len;
  for (next = buf; next < lim; next += ifm->ifm_msglen) 
    {
      ifm = (struct if_msghdr *) next;
      if (ifm->ifm_type == RTM_IFINFO)
	{
	  sa = (struct sockaddr *) (ifm + 1);
	  get_rtaddrs(ifm->ifm_addrs, sa, rti_info);
	  if ((sa = rti_info[RTAX_IFP]) != NULL)
	    {
	      if (sa->sa_family == AF_LINK)
		{
		  sdl = (struct sockaddr_dl *) sa;
		  if (strncmp(&sdl->sdl_data[0], name, sdl->sdl_nlen) == 0) 
		    {
		      index = sdl->sdl_index;
		      free(buf);
		      return index;
		    }
		}
	    }
	}
    }
  free(buf);
  return 0;
}

// net route interface list
char * net_rt_iflist(int family, int flags, size_t *lenp)
{
  int mib[6];
  char *buf;

  mib[0] = CTL_NET;
  mib[1] = AF_ROUTE;
  mib[2] = 0;
  mib[3] = family;		/* only addresses of this family */
  mib[4] = NET_RT_IFLIST;
  mib[5] = flags;			/* interface index, or 0 */
  if (sysctl(mib, 6, NULL, lenp, NULL, 0) < 0)
    return(NULL);

  if ( (buf = malloc(*lenp)) == NULL)
    return(NULL);
  if (sysctl(mib, 6, buf, lenp, NULL, 0) < 0)
    return(NULL);
  
  return(buf);
}

// Net route interface list
char *Net_rt_iflist(int family, int flags, size_t *lenp)
{
  char *ptr;

  if ( (ptr = net_rt_iflist(family, flags, lenp)) == NULL)
    {
      perror("net_rt_iflist error");
      exit(0);
    }
  return(ptr);
}

// socket mask op
char *sock_masktop(struct sockaddr *sa, socklen_t salen)
{
  static char str[INET6_ADDRSTRLEN];
  unsigned char	*ptr = &sa->sa_data[2];

  if (sa->sa_family == 0)
    return("0.0.0.0");
  else if (sa->sa_family == 5)
    snprintf(str, sizeof(str), "%d.0.0.0", *ptr);
  else if (sa->sa_family == 6)
    snprintf(str, sizeof(str), "%d.%d.0.0", *ptr, *(ptr+1));
  else if (sa->sa_family == 7)
    snprintf(str, sizeof(str), "%d.%d.%d.0", *ptr, *(ptr+1), *(ptr+2));
  else if (sa->sa_family == 8)
    snprintf(str, sizeof(str), "%d.%d.%d.%d", 
	     *ptr, *(ptr+1), *(ptr+2), *(ptr+3));
  else
    snprintf(str, sizeof(str), "(unknown mask, len = %d, family = %d)",
	     sa->sa_family, sa->sa_family);
  return(str);
}

// socket to host
char *sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
  static char str[128];		/* Unix domain is largest */

  switch (sa->sa_family)
    {
      case AF_INET: 
	{
	  struct sockaddr_in	*sin = (struct sockaddr_in *) sa;
	  
	  if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
	    return(NULL);
	  return(str);
	}

#ifdef IPV6
      case AF_INET6:
	{
	  struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;

	  if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL)
	    return(NULL);
	  return(str);
	}
#endif

#ifdef AF_UNIX
      case AF_UNIX:
	{
	  struct sockaddr_un *unp = (struct sockaddr_un *) sa;

	  /* OK to have no pathname bound to the socket: happens on
	     every connect() unless client calls bind() first. */
	  if (unp->sun_path[0] == 0)
	    strcpy(str, "(no pathname bound)");
	  else
	    snprintf(str, sizeof(str), "%s", unp->sun_path);
	  return(str);
	}
#endif

#ifdef HAVE_SOCKADDR_DL_STRUCT
      case AF_LINK:
	{
	  struct sockaddr_dl	*sdl = (struct sockaddr_dl *) sa;
	  
	  if (sdl->sdl_nlen > 0)
	    snprintf(str, sizeof(str), "%*s",
		     sdl->sdl_nlen, &sdl->sdl_data[0]);
	  else
	    snprintf(str, sizeof(str), "AF_LINK, index=%d", sdl->sdl_index);
	  return(str);
	}
#endif
      default:
	snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, \
len %d", sa->sa_family, salen);
	return(str);
    }
  return (NULL);
}

// sock to host
char * Sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
  char *ptr;

  if ( (ptr = sock_ntop_host(sa, salen)) == NULL)
    perror("sock_ntop_host error");	/* inet_ntop() sets errno */
  return(ptr);
}

int rtctl_add(char *daddr,	/* destination IP address */
	      char *gaddr,	/* gateway IP address */
	      char *maddr,	/* subnet mask */
	      char *ifname	/* network interface name */)
{
  return rtctl(daddr, gaddr, maddr, ifname, RTM_ADD);
}

int rtctl_delete(char *daddr,	/* destination IP address */
		 char *gaddr,	/* gateway IP address */
		 char *maddr,	/* subnet mask */
		 char *ifname	/* network interface name */)
{
  return rtctl(daddr, gaddr, maddr, ifname, RTM_DELETE);
}

// function used to configure a route to "daddr" by specifying either
// the gateway address "gaddr" or the interface name "ifname"; 
// a mask given by "maddr" can also be specified
// the command codes "cmd" indicate the type of operation
// Note: addresses are given as strings (c.f. rtctl2)
int rtctl(char *daddr,	/* destination IP address */
	  char *gaddr,	/* gateway IP address */
	  char *maddr,	/* subnet mask */
	  char *ifname,	/* network interface name */
	  u_char cmd)	/* RTM_ADD RTM_DELETE RTM_CHANGE */
{
  int sockfd, mib[6], len;
  char *buf, rta_cmd[RTCTL_CMD_LEN], *ifbuf;
  u_char flag;
  pid_t pid;
  struct rt_msghdr *rtm;
  struct sockaddr_in *dst, *gate, *mask;
  struct sockaddr_dl *ifp, *sdl;
  struct if_msghdr *ifm;

#ifdef RTCTL_PRINT_RESPONSE
  ssize_t n;
  struct sockaddr *sa, *rti_info[RTAX_MAX];
#endif	

  /* check the arguments */
  if (daddr == NULL)
    {
      fprintf(stderr, "Invalid destination IP address\n");
      return -1;
    }
  flag = 0;

#if 0
  if (!strncmp(cmd, cmd_add, sizeof(cmd_add)))
    {
      command = (uchar_t) RTM_ADD;
    }
  else if(!strncmp(cmd, cmd_delete, sizeof(cmd_delete)))
    command = (uchar_t) RTM_DELETE;
  else if(!strncmp(cmd, cmd_change, sizeof(cmd_change)))
    command = (uchar_t) RTM_CHANGE;
#endif

  if(cmd == RTM_ADD)
    {
      flag |= RTCTL_ADD;
      strncpy(rta_cmd, "add", sizeof("add"));
      if (gaddr == NULL)
	{
	  if (ifname == NULL)
	    flag |= RTCTL_UNKNOWN;
	  else if (maddr != NULL)
	    flag |= RTCTL_UNKNOWN;
	  else
	    flag |= RTCTL_IF;
	} 
      else
	{
	  flag |= RTCTL_GATEWAY;
	  if (ifname != NULL)
	    flag |= RTCTL_UNKNOWN;
	  else
	    if (maddr != NULL)
	      flag |= RTCTL_NETMASK;
	}
    }
  else if(cmd == RTM_DELETE)
    {
      flag |= RTCTL_DELETE;
      strncpy(rta_cmd, "delete", sizeof("delete"));
      if ((ifname != NULL) | (gaddr != NULL) | (maddr != NULL))
	flag |= RTCTL_UNKNOWN;
    } 
  else if (cmd == RTM_CHANGE)
    {
      flag |= RTCTL_CHANGE;
      strncpy(rta_cmd, "change", sizeof("change"));
      if (gaddr == NULL)
	{
	  if (ifname == NULL)
	    flag |= RTCTL_UNKNOWN;
	  else if (maddr != NULL)
	    flag |= RTCTL_UNKNOWN;
	  else
	    flag |= RTCTL_IF;
	}
      else
	{
	  flag |= RTCTL_GATEWAY;
	  if (ifname != NULL)
	    flag |= RTCTL_UNKNOWN;
	  else if (maddr != NULL)
	    flag |= RTCTL_NETMASK;
	}
    }
  else
    {
      fprintf(stderr, "Invalid command name\n");
      return -1;
    }
  if (flag & RTCTL_UNKNOWN)
    {
      fprintf(stderr, "Unknown arguments are inputed.\n");
      return -1;
    }

  if ((buf = calloc(1, BUFLEN)) == NULL)
    {
      perror("calloc");
      return -1;
    }

  /* set address of rt messages */
  rtm = (struct rt_msghdr *) buf;
  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_type = cmd;
  rtm->rtm_pid = pid = getpid();
  rtm->rtm_seq = ++rtm_seq;
  rtm->rtm_errno = 0;
  if ((flag & RTCTL_ADD) | (flag & RTCTL_CHANGE))
    {
      if (flag & RTCTL_IF)
	{
	  rtm->rtm_msglen = sizeof(struct rt_msghdr) +
	    sizeof(struct sockaddr_in) + 
	    sizeof(struct sockaddr_dl);
	  rtm->rtm_addrs = RTA_DST | RTA_GATEWAY;
	  rtm->rtm_flags = RTF_UP | RTF_HOST | RTF_LLINFO
	    | RTF_STATIC;
	}
      else if (flag & RTCTL_NETMASK)
	{
	  rtm->rtm_msglen = sizeof(struct rt_msghdr) +
	    3 * sizeof(struct sockaddr_in);
	  rtm->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
	  rtm->rtm_flags = RTF_UP | RTF_GATEWAY | RTF_STATIC;
	}
      else
	{
	  rtm->rtm_msglen = sizeof(struct rt_msghdr) +
	    2 * sizeof(struct sockaddr_in);
	  rtm->rtm_addrs = RTA_DST | RTA_GATEWAY;
	  rtm->rtm_flags = RTF_UP | RTF_HOST | RTF_GATEWAY
	    | RTF_STATIC;
	}
    }
  else if (flag & RTCTL_DELETE)
    {
      rtm->rtm_msglen = sizeof(struct rt_msghdr) +
	sizeof(struct sockaddr_in);
      rtm->rtm_addrs = RTA_DST;
    }

  /* set destination RTA_GATEWAY */
  dst = (struct sockaddr_in *) (rtm + 1);
  dst->sin_family = sizeof(struct sockaddr_in);
  dst->sin_family = AF_INET;
  if ((dst->sin_addr.s_addr = inet_addr(daddr)) < 0)
    {
      fprintf(stderr, "Destination IP address is "
	      "invalid argument");
      return -1;
    }

  if (flag & RTCTL_GATEWAY)
    {
      /* set gateway */
      gate = (struct sockaddr_in *) (dst + 1);
      gate->sin_family = sizeof(struct sockaddr_in);
      gate->sin_family = AF_INET;
      if ((gate->sin_addr.s_addr = inet_addr(gaddr)) < 0)
	{
	  fprintf(stderr, "Gateway IP address is "
		  "invalid argument\n");
	  return -1;
	}
      if (flag & RTCTL_NETMASK)
	{
	  mask = (struct sockaddr_in *) (gate + 1);
	  mask->sin_family = sizeof(struct sockaddr_in);
	  mask->sin_family = AF_INET;
	  if ((inet_aton(maddr, &mask->sin_addr) != 1) ||
	      (mask->sin_addr.s_addr == -1))
	    {
	      fprintf(stderr, "Invalid subnet mask\n");
	      return -1;
	    }
	}
    }
  else if (flag & RTCTL_IF)
    {
      /* get sockaddr_dl info */
      mib[0] = CTL_NET;
      mib[1] = AF_ROUTE;
      mib[2] = 0;
      mib[3] = AF_LINK;
      mib[4] = NET_RT_IFLIST;
      if ((mib[5] = if_nametoindex(ifname)) == 0)
	{
	  perror("if_nametoindex");
	  return -1;
	}
      if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0) 
	{
	  perror("sysctl");
	  return -1;
      }
      if ((ifbuf = calloc(1, len)) == NULL)
	{
	  perror("calloc");
	  return -1;
	}
      if (sysctl(mib, 6, ifbuf, &len, NULL, 0) < 0)
	{
	  perror("sysctl2");
	  return -1;
	}
      ifm = (struct if_msghdr *) ifbuf;
      sdl = (struct sockaddr_dl *) (ifm + 1);

      /* set network interface */
      ifp = (struct sockaddr_dl *) (dst + 1);
      bcopy(sdl, ifp, sizeof(struct sockaddr_dl));
      ifp->sdl_len = sizeof(struct sockaddr_dl);
    }

  if ((sockfd = socket(PF_ROUTE, SOCK_RAW, 0)) < 0)
    {
      perror("socket");
      return -1;
    }
  if (write(sockfd, rtm, rtm->rtm_msglen) < 0)
    {
      perror("write");
      //return -1;
    }
  else
    DEBUG("\troute %s dst:%s gate:%s mask:%s ifp:%s\n",
	  rta_cmd, daddr, gaddr, maddr, ifname);

#ifdef RTCTL_PRINT_RESPONSE
  fprintf(stderr, "---print response\n");
  do
    {
      n = read(sockfd, rtm, BUFLEN);
    }
  while (rtm->rtm_seq != rtm_seq || rtm->rtm_pid != pid);

  /* handle response */
  rtm = (struct rt_msghdr *) buf;
  sa = (struct sockaddr *) (rtm + 1);
  get_rtaddrs(rtm->rtm_addrs, sa, rti_info);
  if ((sa = rti_info[RTAX_DST]) != NULL)
    fprintf(stderr, "dest: %s\n", sock_ntop_host(sa, sa->sa_family));
  if ((sa = rti_info[RTAX_GATEWAY]) != NULL)
    fprintf(stderr, "gateway: %s\n", sock_ntop_host(sa, sa->sa_family));
  if ((sa = rti_info[RTAX_NETMASK]) != NULL)
    fprintf(stderr, "netmask: %s\n", sock_masktop(sa, sa->sa_family));
  if ((sa = rti_info[RTAX_GENMASK]) != NULL)
    fprintf(stderr, "genmask: %s\n", sock_masktop(sa, sa->sa_family));
  if ((sa = rti_info[RTAX_IFP]) != NULL)
    fprintf(stderr, "ifp: len:%d fami:%d data:%s\n", sa->sa_family, 
	    sa->sa_family, sa->sa_data);
  if ((sa = rti_info[RTAX_IFA]) != NULL)
    fprintf(stderr, "ifa: len:%d fami:%d data:%s\n", sa->sa_family, 
	    sa->sa_family, sa->sa_data);
  if ((sa = rti_info[RTAX_AUTHOR]) != NULL)
    fprintf(stderr, "author: len:%d fami:%d data:%s\n", sa->sa_family, 
	    sa->sa_family, sa->sa_data);
  if ((sa = rti_info[RTAX_BRD]) != NULL)
    fprintf(stderr, "brd: len:%d fami:%d data:%s\n", sa->sa_family, 
	    sa->sa_family, sa->sa_data);
#endif

  free(buf);
  if (flag & RTCTL_IF)
    free(ifbuf);
  return 0;
}

int getrt(char *ipaddr)
{
  int sockfd;
  char *buf;
  pid_t pid;
  ssize_t n;
  struct rt_msghdr *rtm;
  struct sockaddr *sa, *rti_info[RTAX_MAX];
  struct sockaddr_in *sin;

  if (ipaddr == NULL)
    {
      fprintf(stderr, "getrt: IP address.\n");
      return -1;
    }
  if ((sockfd = socket(AF_ROUTE, SOCK_RAW, 0)) < 0)
    {
      perror("socket");
      return -1;
    }
  buf = calloc(1, BUFLEN);
  rtm = (struct rt_msghdr *) buf;
  rtm->rtm_msglen = sizeof(struct rt_msghdr) + sizeof(struct sockaddr_in);
  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_type = RTM_GET;
  rtm->rtm_addrs = RTA_DST;
  rtm->rtm_pid = pid = getpid();
  rtm->rtm_seq = SEQ;
  sin = (struct sockaddr_in *) (rtm + 1);
  sin->sin_family = sizeof(struct sockaddr_in);
  sin->sin_family = AF_INET;
  inet_pton(AF_INET, ipaddr, &sin->sin_addr);
  if (write(sockfd, rtm, rtm->rtm_msglen) < 0)
    {
      perror("write");
      exit(1);
    }
  do
    {
      n = read(sockfd, rtm, BUFLEN);
    }
  while (rtm->rtm_type != RTM_GET || rtm->rtm_seq != SEQ ||
	 rtm->rtm_pid != pid);

  /* handle response */
  rtm = (struct rt_msghdr *) buf;
  sa = (struct sockaddr *) (rtm + 1);
  get_rtaddrs(rtm->rtm_addrs, sa, rti_info);
  if ((sa = rti_info[RTAX_DST]) != NULL)
    fprintf(stderr, "dest: %s\n", sock_ntop_host(sa, sa->sa_family));
  if ((sa = rti_info[RTAX_GATEWAY]) != NULL)
    fprintf(stderr, "gateway: %s\n", sock_ntop_host(sa, sa->sa_family));
  if ((sa = rti_info[RTAX_NETMASK]) != NULL)
    fprintf(stderr, "netmask: %s\n", sock_masktop(sa, sa->sa_family));
  if ((sa = rti_info[RTAX_GENMASK]) != NULL)
    fprintf(stderr, "genmask: %s\n", sock_masktop(sa, sa->sa_family));
  if ((sa = rti_info[RTAX_IFP]) != NULL)
    fprintf(stderr, "ifp: len:%d fami:%d data:%s\n", sa->sa_family, 
	    sa->sa_family, sa->sa_data);
  if ((sa = rti_info[RTAX_IFA]) != NULL)
    fprintf(stderr, "ifa: len:%d fami:%d data:%s\n", sa->sa_family, 
	    sa->sa_family, sa->sa_data);
  return 0;
}

// function used to configure a route to "daddr" by specifying either
// the gateway address "gaddr" or the interface name "ifname"; 
// a mask given by "maddr" can also be specified
// the command codes "cmd" indicate the type of operation
// Note: addresses are given as 4-byte words (c.f. rtctl)
int rtctl2(uint32_t *daddr,	/* destination IP address */
	   uint32_t *gaddr,	/* gateway IP address */
	   uint32_t *maddr,	/* subnet mask */
	   char *ifname,	/* network interface name */
	   u_char cmd)		/* RTM_ADD RTM_DELETE RTM_CHANGE */
{
  int sockfd, mib[6], len;
  char *buf, rta_cmd[RTCTL_CMD_LEN], *ifbuf;
  char dstaddr[16], gateaddr[16], maskaddr[16];
  u_char flag;
  pid_t pid;
  struct rt_msghdr *rtm;
  struct sockaddr_in *dst, *gate, *mask;
  struct sockaddr_dl *ifp, *sdl;
  struct if_msghdr *ifm;

#ifdef RTCTL_PRINT_RESPONSE
  ssize_t			n;
  struct sockaddr		*sa, *rti_info[RTAX_MAX];
#endif	

  /* check the arguments */
  if (daddr == NULL)
    {
      fprintf(stderr, "Invalid destination IP address\n");
      return -1;
    }
  flag = 0;
  if (cmd == RTM_ADD)
    {
      flag |= RTCTL_ADD;
      strncpy(rta_cmd, "add", sizeof("add"));
      if (gaddr == NULL)
	{
	  if (ifname == NULL)
	    flag |= RTCTL_UNKNOWN;
	  else if (maddr != NULL)
	    flag |= RTCTL_UNKNOWN;
	  else
	    flag |= RTCTL_IF;
	}
      else
	{
	  flag |= RTCTL_GATEWAY;
	  if (ifname != NULL)
	    flag |= RTCTL_UNKNOWN;
	  else if (maddr != NULL)
	    flag |= RTCTL_NETMASK;
	}
    }
  else if (cmd == RTM_DELETE)
    {
      flag |= RTCTL_DELETE;
      strncpy(rta_cmd, "delete", sizeof("delete"));
      if ((ifname != NULL) | (gaddr != NULL) | (maddr != NULL))
	flag |= RTCTL_UNKNOWN;
    }
  else if (cmd == RTM_CHANGE)
    {
      flag |= RTCTL_CHANGE;
      strncpy(rta_cmd, "change", sizeof("change"));
      if (gaddr == NULL)
	{
	  if (ifname == NULL)
	    flag |= RTCTL_UNKNOWN;
	  else if (maddr != NULL)
	    flag |= RTCTL_UNKNOWN;
	  else
	    flag |= RTCTL_IF;
	}
      else
	{
	  flag |= RTCTL_GATEWAY;
	  if (ifname != NULL)
	    flag |= RTCTL_UNKNOWN;
	  else if (maddr != NULL)
	    flag |= RTCTL_NETMASK;
	}
    }
  else
    {
      fprintf(stderr, "Invalid command name\n");
      return -1;
    }

  if (flag & RTCTL_UNKNOWN)
    {
      fprintf(stderr, "Unknown arguments are inputed.\n");
      return -1;
    }
  
  if ((buf = calloc(1, BUFLEN)) == NULL)
    {
      perror("calloc");
      return -1;
    }

  /* set address of rt messages */
  rtm = (struct rt_msghdr *) buf;
  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_type = cmd;
  rtm->rtm_pid = pid = getpid();
  rtm->rtm_seq = ++rtm_seq;
  rtm->rtm_errno = 0;
  if ((flag & RTCTL_ADD) | (flag & RTCTL_CHANGE))
    {
      if (flag & RTCTL_IF)
	{
	  rtm->rtm_msglen = sizeof(struct rt_msghdr) +
	    sizeof(struct sockaddr_in) + 
	    sizeof(struct sockaddr_dl);
	  rtm->rtm_addrs = RTA_DST | RTA_GATEWAY;
	  rtm->rtm_flags = RTF_UP | RTF_HOST | RTF_LLINFO
	    | RTF_STATIC;
	}
      else if (flag & RTCTL_NETMASK)
	{
	  rtm->rtm_msglen = sizeof(struct rt_msghdr) +
	    3 * sizeof(struct sockaddr_in);
	  rtm->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
	  rtm->rtm_flags = RTF_UP | RTF_GATEWAY | RTF_STATIC;
	}
      else
	{
	  rtm->rtm_msglen = sizeof(struct rt_msghdr) +
	    2 * sizeof(struct sockaddr_in);
	  rtm->rtm_addrs = RTA_DST | RTA_GATEWAY;
	  rtm->rtm_flags = RTF_UP | RTF_HOST | RTF_GATEWAY
	    | RTF_STATIC;
	}
    }
  else if (flag & RTCTL_DELETE)
    {
      rtm->rtm_msglen = sizeof(struct rt_msghdr) +
	sizeof(struct sockaddr_in);
      rtm->rtm_addrs = RTA_DST;
    }

  /* set destination RTA_GATEWAY */
  dst = (struct sockaddr_in *) (rtm + 1);
  dst->sin_family = sizeof(struct sockaddr_in);
  dst->sin_family = AF_INET;
  fflush(stderr);
  dst->sin_addr.s_addr = *daddr;
  if (flag & RTCTL_GATEWAY)
    {
      /* set gateway */
      gate = (struct sockaddr_in *) (dst + 1);
      gate->sin_family = sizeof(struct sockaddr_in);
      gate->sin_family = AF_INET;
      gate->sin_addr.s_addr = *gaddr;
      if (flag & RTCTL_NETMASK)
	{
	  mask = (struct sockaddr_in *) (gate + 1);
	  mask->sin_family = sizeof(struct sockaddr_in);
	  mask->sin_family = AF_INET;
	  mask->sin_addr.s_addr = *maddr;
	}
    }
  else if (flag & RTCTL_IF)
    {
      /* get sockaddr_dl info */
      mib[0] = CTL_NET;
      mib[1] = AF_ROUTE;
      mib[2] = 0;
      mib[3] = AF_LINK;
      mib[4] = NET_RT_IFLIST;
      if ((mib[5] = if_nametoindex(ifname)) == 0) 
	{
	  perror("if_nametoindex");
	  return -1;
	}
      if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0) 
	{
	  perror("sysctl");
	  return -1;
	}
      if ((ifbuf = calloc(1, len)) == NULL)
	{
	  perror("calloc");
	  return -1;
	}
      if (sysctl(mib, 6, ifbuf, &len, NULL, 0) < 0) 
	{
	  perror("sysctl2");
	  return -1;
	}
      ifm = (struct if_msghdr *) ifbuf;
      sdl = (struct sockaddr_dl *) (ifm + 1);

      /* set network interface */
      ifp = (struct sockaddr_dl *) (dst + 1);
      bcopy(sdl, ifp, sizeof(struct sockaddr_dl));
      ifp->sdl_len = sizeof(struct sockaddr_dl);
    }

  if ((sockfd = socket(PF_ROUTE, SOCK_RAW, 0)) < 0)
    {
      perror("socket");
      return -1;
    }
  if (write(sockfd, rtm, rtm->rtm_msglen) < 0)
    {
      perror("write");
      //return -1;
    }
  else
    {
      strncpy(dstaddr, "", sizeof(dstaddr));
      strncpy(gateaddr, "", sizeof(gateaddr));
      strncpy(maskaddr, "", sizeof(maskaddr));
      strncpy(dstaddr, inet_ntoa(dst->sin_addr), sizeof(dstaddr));
      if (gaddr != NULL)
	strncpy(gateaddr, inet_ntoa(gate->sin_addr), sizeof(gateaddr));
      if (maddr != NULL)
	strncpy(maskaddr, inet_ntoa(mask->sin_addr), sizeof(maskaddr));
      fprintf(stderr, "\troute %s dst:%s gate:%s mask:%s ifp:%s\n",
	      rta_cmd, dstaddr, gateaddr, maskaddr, ifname);


/* A chunk of lines like this: */
	
/*  strncpy(dstaddr, "", sizeof(dstaddr));                                   */
/*  strncpy(gateaddr, "", sizeof(gateaddr));                                 */
/*  strncpy(maskaddr, "", sizeof(maskaddr));                                 */
/*  strncpy(dstaddr, inet_ntoa(dst->sin_addr), sizeof(dstaddr));             */
/*  if (gaddr != NULL)                                                       */
/*    strncpy(gateaddr, inet_ntoa(gate->sin_addr),                     */
/* 	   sizeof(gateaddr));                                               */
/*  if (maddr != NULL)                                                       */
/*    strncpy(maskaddr, inet_ntoa(mask->sin_addr),                     */
/* 	   sizeof(maskaddr));                                               */
/*  fprintf(stderr, "\troute %s dst:%s gate:%s mask:%s ifp:%s\n",            */
/* 	 rta_cmd, dstaddr, gateaddr, maskaddr, ifname);                   */
 
/* can be rewritten as: */
  
/*   fprintf(stderr, "\troute %s dst:%s gate:%s ", rta_cmd, dstaddr,         */
/* 	  (gaddr != NULL) : inet_ntoa(gate->sin_addr) ? "");                 */
/*  fprintf(stderr, "mask:%s ifp:%s\n",                                      */
/* 	 (maddr != NULL) : inet_ntoa(mask->sin_addr) ? "", ifname);          */

    }
  
#ifdef RTCTL_PRINT_RESPONSE
  fprintf(stderr, "---print response\n");
  do
    {
      n = read(sockfd, rtm, BUFLEN);
    }
  while (rtm->rtm_seq != rtm_seq || rtm->rtm_pid != pid);

  /* handle response */
  rtm = (struct rt_msghdr *) buf;
  sa = (struct sockaddr *) (rtm + 1);
  get_rtaddrs(rtm->rtm_addrs, sa, rti_info);
  if ((sa = rti_info[RTAX_DST]) != NULL)
    fprintf(stderr, "dest: %s\n", sock_ntop_host(sa, sa->sa_family));
  if ((sa = rti_info[RTAX_GATEWAY]) != NULL)
    fprintf(stderr, "gateway: %s\n", sock_ntop_host(sa, sa->sa_family));
  if ((sa = rti_info[RTAX_NETMASK]) != NULL)
    fprintf(stderr, "netmask: %s\n", sock_masktop(sa, sa->sa_family));
  if ((sa = rti_info[RTAX_GENMASK]) != NULL)
    fprintf(stderr, "genmask: %s\n", sock_masktop(sa, sa->sa_family));
  if ((sa = rti_info[RTAX_IFP]) != NULL)
    fprintf(stderr, "ifp: len:%d fami:%d data:%s\n", sa->sa_family, 
	    sa->sa_family, sa->sa_data);
  if ((sa = rti_info[RTAX_IFA]) != NULL)
    fprintf(stderr, "ifa: len:%d fami:%d data:%s\n", sa->sa_family, 
	    sa->sa_family, sa->sa_data);
  if ((sa = rti_info[RTAX_AUTHOR]) != NULL)
    fprintf(stderr, "author: len:%d fami:%d data:%s\n", sa->sa_family, 
	    sa->sa_family, sa->sa_data);
  if ((sa = rti_info[RTAX_BRD]) != NULL)
    fprintf(stderr, "brd: len:%d fami:%d data:%s\n", sa->sa_family, 
	    sa->sa_family, sa->sa_data);
#endif

  free(buf);
  if (flag & RTCTL_IF)
    free(ifbuf);
  return 0;
}

