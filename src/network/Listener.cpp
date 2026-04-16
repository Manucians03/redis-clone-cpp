#include "network/Listener.h"
#include "network/ConnectionManager.h"
#include "network/EventLoop.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <stdexcept>

namespace
{
void setNonBlocking(int fd)
{
  int flags = ::fcntl(fd, F_GETFL, 0);
  if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    throw std::runtime_error("failed to set non-blocking mode");
}
}

Listener::Listener(const std::string &host, int port, ConnectionManager &connectionManager) : _connectionManager(connectionManager)
{
  _listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (_listen_fd < 0)
    throw std::runtime_error("socket creation failed");

  // immediate reuse of the address
  int v = 1;
  if (setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) < 0)
    throw std::runtime_error("setsockopt failed");

  setNonBlocking(_listen_fd);

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(host.c_str());
  addr.sin_port = htons(port);

  if (bind(_listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    throw std::runtime_error("socket bind failed");

  if (listen(_listen_fd, SOMAXCONN) < 0)
    throw std::runtime_error("socket listen failed");
}

Listener::~Listener()
{
  ::close(_listen_fd);
}

int Listener::fd() const
{
  return _listen_fd;
}

void Listener::onAccept()
{
  int newConnFD = ::accept(_listen_fd, nullptr, nullptr);
  if (newConnFD < 0)
  {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
      return;

    throw std::runtime_error("connection accept failed");
  }

  setNonBlocking(newConnFD);

  _connectionManager.addConnection(newConnFD);
}
