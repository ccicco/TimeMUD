// The door, R14. Absolute-deadline aligned with the heartbeat: every
// wait for the next tick IS the wait for the client. One client at a
// time - a second knock is answered and shown the door honestly. Line
// input is assembled byte-by-byte so partial packets and telnet's
// CRLF cannot corrupt a command; login and command routing live in
// session/, which is the design's home for that state machine.

#include "server.h"

#include "log.h"
#include "session.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

static int g_listen_fd = -1;
static int g_client_fd = -1;
static char g_inbuf[2048];
static int g_inlen = 0;

static long now_ms() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void server_start(long port) {
    g_listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (g_listen_fd < 0) {
        log_boot("server: socket() failed - running headless");
        return;
    }
    int one = 1;
    ::setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<unsigned short>(port));
    if (::bind(g_listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0) {
        log_boot("server: bind() failed - running headless");
        ::close(g_listen_fd);
        g_listen_fd = -1;
        return;
    }
    if (::listen(g_listen_fd, 4) < 0) {
        log_boot("server: listen() failed - running headless");
        ::close(g_listen_fd);
        g_listen_fd = -1;
        return;
    }
    ::fcntl(g_listen_fd, F_SETFL, ::fcntl(g_listen_fd, F_GETFL, 0) | O_NONBLOCK);
    std::printf("TimeMUD listens on port %ld. Come in, stranger.\n", port);
    log_boot("server: listening on port %ld", port);
}

void server_write(const char* text) {
    if (g_client_fd < 0 || text == nullptr) return;
    const int len = static_cast<int>(std::strlen(text));
    int sent = 0;
    while (sent < len) {
        const ssize_t n = ::send(g_client_fd, text + sent, static_cast<size_t>(len - sent), 0);
        if (n <= 0) return;  // the client vanished: text is not worth a crash
        sent += static_cast<int>(n);
    }
}

void server_close_client() {
    if (g_client_fd >= 0) {
        ::close(g_client_fd);
        g_client_fd = -1;
    }
    g_inlen = 0;
}

static void accept_knock() {
    const int fd = ::accept(g_listen_fd, nullptr, nullptr);
    if (fd < 0) return;
    if (g_client_fd >= 0) {
        ::send(fd, "The grove holds one stranger at a time. Return soon.\n", 52, 0);
        ::close(fd);
        return;
    }
    ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    int one = 1;
    ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    g_client_fd = fd;
    g_inlen = 0;
    server_write("The moonlit grove, TimeMUD, era one. What name do you go by, stranger? ");
}

static void feed(char b) {
    if (b == '\n') {
        g_inbuf[g_inlen] = 0;
        session_line(g_inbuf);   // the state machine owns it from here
        g_inlen = 0;
        return;
    }
    if (b == '\r') return;      // telnet CRLF: the CR is decoration
    if (g_inlen < static_cast<int>(sizeof g_inbuf) - 1) g_inbuf[g_inlen++] = b;
}


// THE DOOR KEEPS ITS APPOINTMENT (R31 finding, three rounds measured):
// the old loop polled only while a tick had time left, so a world-heavy
// tick that overran its deadline returned without ever polling - input
// was served from tick SLACK, and a chatty world starved the player for
// seconds at a time (measured ~15s per command in the R30 flood and
// the daylight greet-storm). On overrun the door still gets one
// zero-timeout poll: queued commands drain every tick, late or not.

static void poll_once(int timeout_ms) {
    pollfd fds[2];
    int n = 0;
    int listen_idx = -1;
    int client_idx = -1;
    if (g_listen_fd >= 0) {
        fds[n].fd = g_listen_fd;
        fds[n].events = POLLIN;
        fds[n].revents = 0;
        listen_idx = n++;
    }
    if (g_client_fd >= 0) {
        fds[n].fd = g_client_fd;
        fds[n].events = POLLIN;
        fds[n].revents = 0;
        client_idx = n++;
    }
    if (n == 0) return;      // headless with no door: sleep is pure heartbeat
    const int rc = ::poll(fds, static_cast<nfds_t>(n), timeout_ms);
    if (rc <= 0) return;     // timeout or EINTR: an appointment, not an error
    if (listen_idx >= 0 && (fds[listen_idx].revents & POLLIN)) accept_knock();
    if (client_idx >= 0 && (fds[client_idx].revents & POLLIN)) {
        char tmp[1024];
        const ssize_t r = ::recv(g_client_fd, tmp, sizeof tmp, 0);
        if (r < 0) {
            if (errno == EAGAIN or errno == EWOULDBLOCK or errno == EINTR) return;
            server_close_client();
            return;
        }
        if (r == 0) {
            server_close_client();
            return;
        }
        for (ssize_t i = 0; i < r; ++i) feed(tmp[i]);
    }
}

void server_serve_until(long deadline_ms) {
    while (now_ms() < deadline_ms) {
        long remain = deadline_ms - now_ms();
        if (remain > 100) remain = 100;
        poll_once(static_cast<int>(remain));
    }
    if (now_ms() >= deadline_ms) poll_once(0);   // late ticks still drain the door
}
