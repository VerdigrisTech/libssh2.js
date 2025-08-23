#include <libssh2.h>
#include <emscripten.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Global variables for custom transport
static char* send_buffer = NULL;
static size_t send_buffer_size = 0;
static char* recv_buffer = NULL;
static size_t recv_buffer_size = 0;
static size_t recv_buffer_pos = 0;

// Custom socket functions for WebSocket bridge
EMSCRIPTEN_KEEPALIVE
int custom_send(libssh2_socket_t socket, const void *buffer, size_t length, int flags, void **abstract) {
    // This will be called from JavaScript via EM_ASM
    return EM_ASM_INT({
        if (Module.customSend) {
            return Module.customSend($0, $1, $2);
        }
        return $2; // Default: assume all bytes sent
    }, (int)socket, (int)buffer, (int)length);
}

EMSCRIPTEN_KEEPALIVE
int custom_recv(libssh2_socket_t socket, void *buffer, size_t length, int flags, void **abstract) {
    // This will be called from JavaScript via EM_ASM
    return EM_ASM_INT({
        if (Module.customRecv) {
            return Module.customRecv($0, $1, $2);
        }
        return 0; // Default: no bytes received
    }, (int)socket, (int)buffer, (int)length);
}

// Initialize libssh2
EMSCRIPTEN_KEEPALIVE
int ssh2_init() {
    return libssh2_init(0);
}

// Cleanup libssh2
EMSCRIPTEN_KEEPALIVE
void ssh2_exit() {
    libssh2_exit();
}

// Session management
EMSCRIPTEN_KEEPALIVE
LIBSSH2_SESSION* ssh2_session_init_custom() {
    LIBSSH2_SESSION* session = libssh2_session_init();
    if (session) {
        // Set custom transport functions
        libssh2_session_callback_set(session, LIBSSH2_CALLBACK_SEND, (void*)custom_send);
        libssh2_session_callback_set(session, LIBSSH2_CALLBACK_RECV, (void*)custom_recv);
    }
    return session;
}

EMSCRIPTEN_KEEPALIVE
int ssh2_session_handshake_custom(LIBSSH2_SESSION* session) {
    return libssh2_session_handshake(session, 1); // 1 = custom socket descriptor
}

EMSCRIPTEN_KEEPALIVE
int ssh2_session_set_blocking(LIBSSH2_SESSION* session, int blocking) {
    libssh2_session_set_blocking(session, blocking);
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int ssh2_session_last_errno(LIBSSH2_SESSION* session) {
    return libssh2_session_last_errno(session);
}

EMSCRIPTEN_KEEPALIVE
void ssh2_session_free(LIBSSH2_SESSION* session) {
    if (session) {
        libssh2_session_disconnect(session, "Normal shutdown");
        libssh2_session_free(session);
    }
}

// Authentication
EMSCRIPTEN_KEEPALIVE
int ssh2_userauth_password_custom(LIBSSH2_SESSION* session, const char* username, const char* password) {
    return libssh2_userauth_password(session, username, password);
}

EMSCRIPTEN_KEEPALIVE
int ssh2_userauth_publickey_fromfile(LIBSSH2_SESSION* session, const char* username,
                                   const char* publickey, const char* privatekey,
                                   const char* passphrase) {
    return libssh2_userauth_publickey_fromfile(session, username, publickey, privatekey, passphrase);
}

// Channel management
EMSCRIPTEN_KEEPALIVE
LIBSSH2_CHANNEL* ssh2_channel_open_session_custom(LIBSSH2_SESSION* session) {
    return libssh2_channel_open_session(session);
}

EMSCRIPTEN_KEEPALIVE
int ssh2_channel_request_pty_custom(LIBSSH2_CHANNEL* channel, const char* term) {
    return libssh2_channel_request_pty(channel, term);
}

EMSCRIPTEN_KEEPALIVE
int ssh2_channel_request_pty_size(LIBSSH2_CHANNEL* channel, int width, int height) {
    return libssh2_channel_request_pty_size(channel, width, height);
}

EMSCRIPTEN_KEEPALIVE
int ssh2_channel_shell_custom(LIBSSH2_CHANNEL* channel) {
    return libssh2_channel_shell(channel);
}

EMSCRIPTEN_KEEPALIVE
int ssh2_channel_read_custom(LIBSSH2_CHANNEL* channel, char* buf, size_t buflen) {
    return libssh2_channel_read(channel, buf, buflen);
}

EMSCRIPTEN_KEEPALIVE
int ssh2_channel_write_custom(LIBSSH2_CHANNEL* channel, const char* buf, size_t buflen) {
    return libssh2_channel_write(channel, buf, buflen);
}

EMSCRIPTEN_KEEPALIVE
int ssh2_channel_eof(LIBSSH2_CHANNEL* channel) {
    return libssh2_channel_eof(channel);
}

EMSCRIPTEN_KEEPALIVE
int ssh2_channel_close(LIBSSH2_CHANNEL* channel) {
    return libssh2_channel_close(channel);
}

EMSCRIPTEN_KEEPALIVE
void ssh2_channel_free(LIBSSH2_CHANNEL* channel) {
    if (channel) {
        libssh2_channel_free(channel);
    }
}

// Utility functions for debugging
EMSCRIPTEN_KEEPALIVE
const char* ssh2_version() {
    return libssh2_version(0);
}

EMSCRIPTEN_KEEPALIVE
void ssh2_trace(LIBSSH2_SESSION* session, int bitmask) {
    libssh2_trace(session, bitmask);
}