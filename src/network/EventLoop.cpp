#include "network/EventLoop.h"

#include <poll.h>
#include <cerrno>
#include <stdexcept>
#include <vector>
#include <unistd.h>

EventLoop::EventLoop(const std::string &host, int port, ConnectionManager &connectionManager) : _connectionManager(connectionManager), _running(true)
{
  _listener = std::make_unique<Listener>(host, port, _connectionManager);
}

EventLoop::~EventLoop()
{
  _listener.reset();
}

void EventLoop::poll()
{
  if (!_running)
    return;

  std::vector<struct pollfd> pollfds;
  pollfds.reserve(kMaxEvents);

  struct pollfd listenerPollFd{};
  listenerPollFd.fd = _listener->fd();
  listenerPollFd.events = POLLIN;
  pollfds.push_back(listenerPollFd);

  const std::vector<Connection *> &conns = _connectionManager.getActiveConnections();
  for (Connection *conn : conns)
  {
    if (!conn || conn->isClosed() || int(pollfds.size()) >= kMaxEvents)
      continue;

    short events = 0;

    if (conn->wantsRead())
      events |= POLLIN;
    if (conn->wantsWrite())
      events |= POLLOUT;
    if (events == 0)
      continue;

    struct pollfd connectionPollFd{};
    connectionPollFd.fd = conn->fd();
    connectionPollFd.events = events;
    pollfds.push_back(connectionPollFd);
  }

  int n = ::poll(pollfds.data(), pollfds.size(), 100);

  if (n < 0 && errno == EINTR)
    return;

  if (n < 0)
    throw std::runtime_error("poll failed");

  if (n == 0)
    return;

  for (const struct pollfd &pfd : pollfds)
  {
    if (pfd.revents == 0)
      continue;

    int fd = pfd.fd;
    uint32_t ev = static_cast<uint32_t>(pfd.revents);

    if (fd == _listener->fd())
      handleListenerEvents(ev);
    else
      handleConnectionEvents(fd, ev);
  }
}

void EventLoop::stop()
{
  _running = false;
}

bool EventLoop::isRunning() const
{
  return _running;
}

void EventLoop::handleListenerEvents(uint32_t events)
{
  if (events & POLLIN)
    _listener->onAccept();
}

void EventLoop::handleConnectionEvents(int fd, uint32_t events)
{
  Connection *conn = _connectionManager.getConnection(fd);
  if (!conn)
    return;

  if (events & POLLIN)
    conn->handleRead();

  if (events & POLLOUT)
    conn->handleWrite();

  if (events & (POLLERR | POLLHUP))
    conn->setWantClose(true);
}
