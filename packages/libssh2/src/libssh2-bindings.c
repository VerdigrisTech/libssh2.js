#include <libssh2.h>
#include <libssh2_sftp.h>
#include <emscripten.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// =====================================
// Core Library Functions
// =====================================

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

// Get libssh2 version
EMSCRIPTEN_KEEPALIVE
const char* ssh2_version() {
    return libssh2_version(0);
}

// =====================================
// Custom socket functions
// =====================================

EMSCRIPTEN_KEEPALIVE
int custom_send(libssh2_socket_t socket, const void *buffer, size_t length, int flags, void **abstract) {
    // Since we only support one session per WebSocket, we don't need socket/flags/abstract
    return EM_ASM_INT({
      return Module.customSend($0, $1);
    }, (int)buffer, (int)length);
}

EMSCRIPTEN_KEEPALIVE
int custom_recv(libssh2_socket_t socket, void *buffer, size_t length, int flags, void **abstract) {
    // Since we only support one session per WebSocket, we don't need socket/flags/abstract
    return EM_ASM_INT({
      return Module.customRecv($0, $1);
    }, (int)buffer, (int)length);
}

// =====================================
// Session Management
// =====================================

// Create new session
EMSCRIPTEN_KEEPALIVE
LIBSSH2_SESSION* ssh2_session_init() {
    return libssh2_session_init();
}

// Free session
EMSCRIPTEN_KEEPALIVE
void ssh2_session_free(LIBSSH2_SESSION* session) {
    if (session) {
        libssh2_session_disconnect(session, "Normal shutdown");
        libssh2_session_free(session);
    }
}

// Set session callback (modified to use predefined callbacks)
EMSCRIPTEN_KEEPALIVE
void ssh2_session_callback_set_custom(LIBSSH2_SESSION* session, int cbtype) {
    if (cbtype == LIBSSH2_CALLBACK_SEND) { // LIBSSH2_CALLBACK_SEND
        libssh2_session_callback_set2(session, cbtype, (libssh2_cb_generic*)custom_send);
    } else if (cbtype == LIBSSH2_CALLBACK_RECV) { // LIBSSH2_CALLBACK_RECV
        libssh2_session_callback_set2(session, cbtype, (libssh2_cb_generic*)custom_recv);
    }
}

// Perform handshake
EMSCRIPTEN_KEEPALIVE
int ssh2_session_handshake(LIBSSH2_SESSION* session, int socket) {
    return libssh2_session_handshake(session, socket);
}

// Perform handshake with custom transport (socket = 1)
EMSCRIPTEN_KEEPALIVE
int ssh2_session_handshake_custom(LIBSSH2_SESSION* session) {
    return libssh2_session_handshake(session, 1); // 1 = custom socket descriptor
}

// Disconnect session
EMSCRIPTEN_KEEPALIVE
int ssh2_session_disconnect(LIBSSH2_SESSION* session, const char* description) {
    return libssh2_session_disconnect(session, description ? description : "Goodbye");
}

// Set session to blocking/non-blocking mode
EMSCRIPTEN_KEEPALIVE
void ssh2_session_set_blocking(LIBSSH2_SESSION* session, int blocking) {
    libssh2_session_set_blocking(session, blocking);
}

// Get blocking mode
EMSCRIPTEN_KEEPALIVE
int ssh2_session_get_blocking(LIBSSH2_SESSION* session) {
    return libssh2_session_get_blocking(session);
}

// Get last error
EMSCRIPTEN_KEEPALIVE
int ssh2_session_last_errno(LIBSSH2_SESSION* session) {
    return libssh2_session_last_errno(session);
}

// Get last error message
EMSCRIPTEN_KEEPALIVE
char* ssh2_session_last_error(LIBSSH2_SESSION* session) {
    char* errmsg;
    int errmsg_len;
    libssh2_session_last_error(session, &errmsg, &errmsg_len, 0);
    return errmsg;
}

// Set session timeout
EMSCRIPTEN_KEEPALIVE
void ssh2_session_set_timeout(LIBSSH2_SESSION* session, long timeout) {
    libssh2_session_set_timeout(session, timeout);
}

// Get session timeout
EMSCRIPTEN_KEEPALIVE
long ssh2_session_get_timeout(LIBSSH2_SESSION* session) {
    return libssh2_session_get_timeout(session);
}

// Enable/disable trace
EMSCRIPTEN_KEEPALIVE
void ssh2_session_trace(LIBSSH2_SESSION* session, int bitmask) {
    libssh2_trace(session, bitmask);
}

// =====================================
// Authentication
// =====================================

// Get authentication list
EMSCRIPTEN_KEEPALIVE
char* ssh2_userauth_list(LIBSSH2_SESSION* session, const char* username) {
    return libssh2_userauth_list(session, username, strlen(username));
}

// Check if user is authenticated
EMSCRIPTEN_KEEPALIVE
int ssh2_userauth_authenticated(LIBSSH2_SESSION* session) {
    return libssh2_userauth_authenticated(session);
}

// Password authentication
EMSCRIPTEN_KEEPALIVE
int ssh2_userauth_password(LIBSSH2_SESSION* session, const char* username, const char* password) {
    return libssh2_userauth_password(session, username, password);
}

// Public key authentication from file
EMSCRIPTEN_KEEPALIVE
int ssh2_userauth_publickey_fromfile(LIBSSH2_SESSION* session, const char* username,
                                   const char* publickey, const char* privatekey,
                                   const char* passphrase) {
    return libssh2_userauth_publickey_fromfile(session, username, publickey, privatekey, passphrase);
}

// Public key authentication from memory
EMSCRIPTEN_KEEPALIVE
int ssh2_userauth_publickey_frommemory(LIBSSH2_SESSION* session, const char* username,
                                     const char* publickeydata, size_t publickeydata_len,
                                     const char* privatekeydata, size_t privatekeydata_len,
                                     const char* passphrase) {
    return libssh2_userauth_publickey_frommemory(session, username, strlen(username),
                                               publickeydata, publickeydata_len,
                                               privatekeydata, privatekeydata_len,
                                               passphrase);
}

// =====================================
// Channel Management
// =====================================

// Open session channel
EMSCRIPTEN_KEEPALIVE
LIBSSH2_CHANNEL* ssh2_channel_open_session(LIBSSH2_SESSION* session) {
    return libssh2_channel_open_session(session);
}

// Open direct TCP/IP channel
EMSCRIPTEN_KEEPALIVE
LIBSSH2_CHANNEL* ssh2_channel_direct_tcpip(LIBSSH2_SESSION* session, const char* host, int port) {
    return libssh2_channel_direct_tcpip(session, host, port);
}

// Free channel
EMSCRIPTEN_KEEPALIVE
void ssh2_channel_free(LIBSSH2_CHANNEL* channel) {
    if (channel) {
        libssh2_channel_free(channel);
    }
}

// Close channel
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_close(LIBSSH2_CHANNEL* channel) {
    return libssh2_channel_close(channel);
}

// Wait for channel to close
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_wait_closed(LIBSSH2_CHANNEL* channel) {
    return libssh2_channel_wait_closed(channel);
}

// Check if channel is EOF
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_eof(LIBSSH2_CHANNEL* channel) {
    return libssh2_channel_eof(channel);
}

// Send EOF to channel
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_send_eof(LIBSSH2_CHANNEL* channel) {
    return libssh2_channel_send_eof(channel);
}

// Request PTY
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_request_pty(LIBSSH2_CHANNEL* channel, const char* term) {
    return libssh2_channel_request_pty(channel, term);
}

// Request PTY with size (using the actual API call)
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_request_pty_size(LIBSSH2_CHANNEL* channel, int width, int height,
                                 int width_px, int height_px) {
    return libssh2_channel_request_pty_size_ex(channel, width, height, width_px, height_px);
}

// Start shell
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_shell(LIBSSH2_CHANNEL* channel) {
    return libssh2_channel_shell(channel);
}

// Execute command
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_exec(LIBSSH2_CHANNEL* channel, const char* command) {
    return libssh2_channel_exec(channel, command);
}

// Start subsystem
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_subsystem(LIBSSH2_CHANNEL* channel, const char* subsystem) {
    return libssh2_channel_subsystem(channel, subsystem);
}

// Set environment variable
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_setenv(LIBSSH2_CHANNEL* channel, const char* varname, const char* value) {
    return libssh2_channel_setenv(channel, varname, value);
}

// Read from channel
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_read(LIBSSH2_CHANNEL* channel, char* buf, size_t buflen) {
    return libssh2_channel_read(channel, buf, buflen);
}

// Read stderr from channel
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_read_stderr(LIBSSH2_CHANNEL* channel, char* buf, size_t buflen) {
    return libssh2_channel_read_stderr(channel, buf, buflen);
}

// Write to channel
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_write(LIBSSH2_CHANNEL* channel, const char* buf, size_t buflen) {
    return libssh2_channel_write(channel, buf, buflen);
}

// Write to channel stderr
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_write_stderr(LIBSSH2_CHANNEL* channel, const char* buf, size_t buflen) {
    return libssh2_channel_write_stderr(channel, buf, buflen);
}

// Flush channel
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_flush(LIBSSH2_CHANNEL* channel) {
    return libssh2_channel_flush(channel);
}

// Get exit status
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_get_exit_status(LIBSSH2_CHANNEL* channel) {
    return libssh2_channel_get_exit_status(channel);
}

// Get exit signal
EMSCRIPTEN_KEEPALIVE
char* ssh2_channel_get_exit_signal(LIBSSH2_CHANNEL* channel) {
    char* exitsignal;
    size_t exitsignal_len;
    char* errmsg;
    size_t errmsg_len;
    char* langtag;
    size_t langtag_len;

    int result = libssh2_channel_get_exit_signal(channel, &exitsignal, &exitsignal_len,
                                               &errmsg, &errmsg_len,
                                               &langtag, &langtag_len);
    return (result == 0) ? exitsignal : NULL;
}

// =====================================
// SFTP Functions
// =====================================

// Initialize SFTP
EMSCRIPTEN_KEEPALIVE
LIBSSH2_SFTP* ssh2_sftp_init(LIBSSH2_SESSION* session) {
    return libssh2_sftp_init(session);
}

// Shutdown SFTP
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_shutdown(LIBSSH2_SFTP* sftp) {
    return libssh2_sftp_shutdown(sftp);
}

// Get SFTP last error
EMSCRIPTEN_KEEPALIVE
unsigned long ssh2_sftp_last_error(LIBSSH2_SFTP* sftp) {
    return libssh2_sftp_last_error(sftp);
}

// Open file
EMSCRIPTEN_KEEPALIVE
LIBSSH2_SFTP_HANDLE* ssh2_sftp_open(LIBSSH2_SFTP* sftp, const char* filename,
                                   unsigned long flags, long mode) {
    return libssh2_sftp_open(sftp, filename, flags, mode);
}

// Open directory
EMSCRIPTEN_KEEPALIVE
LIBSSH2_SFTP_HANDLE* ssh2_sftp_opendir(LIBSSH2_SFTP* sftp, const char* path) {
    return libssh2_sftp_opendir(sftp, path);
}

// Close handle
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_close_handle(LIBSSH2_SFTP_HANDLE* handle) {
    return libssh2_sftp_close_handle(handle);
}

// Read from file
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_read(LIBSSH2_SFTP_HANDLE* handle, char* buffer, size_t buffer_maxlen) {
    return libssh2_sftp_read(handle, buffer, buffer_maxlen);
}

// Write to file
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_write(LIBSSH2_SFTP_HANDLE* handle, const char* buffer, size_t count) {
    return libssh2_sftp_write(handle, buffer, count);
}

// Read directory entry
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_readdir(LIBSSH2_SFTP_HANDLE* handle, char* buffer, size_t buffer_maxlen,
                     char* longentry, size_t longentry_maxlen,
                     LIBSSH2_SFTP_ATTRIBUTES* attrs) {
    return libssh2_sftp_readdir_ex(handle, buffer, buffer_maxlen, longentry, longentry_maxlen, attrs);
}

// Seek in file
EMSCRIPTEN_KEEPALIVE
void ssh2_sftp_seek64(LIBSSH2_SFTP_HANDLE* handle, libssh2_uint64_t offset) {
    libssh2_sftp_seek64(handle, offset);
}

// Tell current position
EMSCRIPTEN_KEEPALIVE
libssh2_uint64_t ssh2_sftp_tell64(LIBSSH2_SFTP_HANDLE* handle) {
    return libssh2_sftp_tell64(handle);
}

// Get file attributes
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_stat(LIBSSH2_SFTP* sftp, const char* path, LIBSSH2_SFTP_ATTRIBUTES* attrs) {
    return libssh2_sftp_stat(sftp, path, attrs);
}

// Set file attributes
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_setstat(LIBSSH2_SFTP* sftp, const char* path, LIBSSH2_SFTP_ATTRIBUTES* attrs) {
    return libssh2_sftp_setstat(sftp, path, attrs);
}

// Create directory
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_mkdir(LIBSSH2_SFTP* sftp, const char* path, long mode) {
    return libssh2_sftp_mkdir(sftp, path, mode);
}

// Remove directory
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_rmdir(LIBSSH2_SFTP* sftp, const char* path) {
    return libssh2_sftp_rmdir(sftp, path);
}

// Remove file
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_unlink(LIBSSH2_SFTP* sftp, const char* filename) {
    return libssh2_sftp_unlink(sftp, filename);
}

// Rename file
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_rename(LIBSSH2_SFTP* sftp, const char* source_filename,
                    const char* dest_filename) {
    return libssh2_sftp_rename(sftp, source_filename, dest_filename);
}

// Create symlink
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_symlink(LIBSSH2_SFTP* sftp, const char* path, char* target) {
    return libssh2_sftp_symlink(sftp, path, target);
}

// Read symlink
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_readlink(LIBSSH2_SFTP* sftp, const char* path, char* target,
                      unsigned int maxlen) {
    return libssh2_sftp_readlink(sftp, path, target, maxlen);
}

// Get real path
EMSCRIPTEN_KEEPALIVE
int ssh2_sftp_realpath(LIBSSH2_SFTP* sftp, const char* path, char* target,
                      unsigned int maxlen) {
    return libssh2_sftp_realpath(sftp, path, target, maxlen);
}

// =====================================
// SCP Functions
// =====================================

// Receive file via SCP
EMSCRIPTEN_KEEPALIVE
LIBSSH2_CHANNEL* ssh2_scp_recv2(LIBSSH2_SESSION* session, const char* path,
                               libssh2_struct_stat* sb) {
    return libssh2_scp_recv2(session, path, sb);
}

// Send file via SCP
EMSCRIPTEN_KEEPALIVE
LIBSSH2_CHANNEL* ssh2_scp_send64(LIBSSH2_SESSION* session, const char* path, int mode,
                                libssh2_uint64_t size, time_t mtime, time_t atime) {
    return libssh2_scp_send64(session, path, mode, size, mtime, atime);
}

// =====================================
// Port Forwarding
// =====================================

// Listen for connections (local port forwarding)
EMSCRIPTEN_KEEPALIVE
LIBSSH2_LISTENER* ssh2_channel_forward_listen(LIBSSH2_SESSION* session, int port) {
    int bound_port;
    return libssh2_channel_forward_listen_ex(session, NULL, port, &bound_port, 16);
}

// Accept forwarded connection
EMSCRIPTEN_KEEPALIVE
LIBSSH2_CHANNEL* ssh2_channel_forward_accept(LIBSSH2_LISTENER* listener) {
    return libssh2_channel_forward_accept(listener);
}

// Cancel port forwarding
EMSCRIPTEN_KEEPALIVE
int ssh2_channel_forward_cancel(LIBSSH2_LISTENER* listener) {
    return libssh2_channel_forward_cancel(listener);
}

// =====================================
// Memory Management Helpers
// =====================================

// Allocate memory (for use with JS)
EMSCRIPTEN_KEEPALIVE
void* ssh2_malloc(size_t size) {
    return malloc(size);
}

// Free memory (for use with JS)
EMSCRIPTEN_KEEPALIVE
void ssh2_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

// Copy string to allocated memory
EMSCRIPTEN_KEEPALIVE
char* ssh2_strdup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char* copy = malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}
