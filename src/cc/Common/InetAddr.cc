/**
 * Copyright (C) 2007 Doug Judd (Zvents, Inc.)
 *
 * This file is part of Hypertable.
 *
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or any later version.
 *
 * Hypertable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "Common/Compat.h"

#include <cstdlib>
#include <cstring>

extern "C" {
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>
}

#include "Logger.h"
#include "InetAddr.h"
#include "StringExt.h"

namespace Hypertable {

InetAddr::InetAddr() {
  HT_EXPECT(sizeof(sockaddr_in) == sizeof(InetAddr), Error::UNPOSSIBLE);
  memset(this, 0, sizeof(InetAddr));
}

InetAddr::InetAddr(const String &host, uint16_t port) {
  HT_EXPECT(sizeof(sockaddr_in) == sizeof(InetAddr), Error::UNPOSSIBLE);
  HT_EXPECT(initialize(this, host.c_str(), port), Error::BAD_DOMAIN_NAME);
}

InetAddr::InetAddr(const String &endpoint) {
  HT_EXPECT(sizeof(sockaddr_in) == sizeof(InetAddr), Error::UNPOSSIBLE);
  HT_EXPECT(initialize(this, endpoint.c_str()), Error::BAD_DOMAIN_NAME);
}

InetAddr::InetAddr(uint32_t ip32, uint16_t port) {
  HT_EXPECT(sizeof(sockaddr_in) == sizeof(InetAddr), Error::UNPOSSIBLE);
  initialize(this, ip32, port);
}

bool InetAddr::initialize(sockaddr_in *addr, const char *host, uint16_t port) {
  memset(addr, 0, sizeof(struct sockaddr_in));
  {
    struct hostent *he = gethostbyname(host);
    if (he == 0) {
      String errmsg = (String)"gethostbyname(\"" + host + "\")";
      herror(errmsg.c_str());
      return false;
    }
    memcpy(&addr->sin_addr.s_addr, he->h_addr_list[0], sizeof(uint32_t));
  }
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  return true;
}

Endpoint InetAddr::parse_endpoint(const char *endpoint, int default_port) {
  const char *colon = strchr(endpoint, ':');

  if (colon) {
    String host = String(endpoint, colon - endpoint);
    return Endpoint(host, atoi(colon + 1));
  }
  return Endpoint(endpoint, default_port);
}

bool InetAddr::initialize(sockaddr_in *addr, const char *addr_str) {
  Endpoint e = parse_endpoint(addr_str);

  if (e.port)
    return initialize(addr, e.host.c_str(), e.port);

  return initialize(addr, "localhost", atoi(addr_str));
}

bool InetAddr::initialize(sockaddr_in *addr, uint32_t haddr, uint16_t port) {
  memset(addr, 0 , sizeof(sockaddr_in));
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = htonl(haddr);
  addr->sin_port = htons(port);
  return true;
}

String InetAddr::format(const sockaddr_in &addr, int sep) {
  // inet_ntoa is not thread safe on many platforms and deprecated
  const uint8_t *ip = (uint8_t *)&addr.sin_addr.s_addr;
  return Hypertable::format("%d.%d.%d.%d%c%d", (int)ip[0], (int)ip[1],
      (int)ip[2],(int)ip[3], sep, (int)ntohs(addr.sin_port));
}

const char *InetAddr::string_format(String &addr_str, const sockaddr_in &addr) {
  addr_str = InetAddr::format(addr);
  return addr_str.c_str();
}

String InetAddr::get_hostname() {
  struct utsname u;
  uname(&u);
  return u.nodename;
}

String &InetAddr::get_hostname(String &hostname) {
  hostname = get_hostname();
  return hostname;
}

std::ostream &operator<<(std::ostream &out, const Endpoint &e) {
  out << e.host <<':'<< e.port;
  return out;
}

std::ostream &operator<<(std::ostream &out, const sockaddr_in &a) {
  out << InetAddr::format(a);
  return out;
}

} // namespace Hypertable
