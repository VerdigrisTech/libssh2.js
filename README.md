# libssh2.js

A WebAssembly port of [libssh2](https://www.libssh2.org/) that enables SSH client functionality in JavaScript environments, including browsers and Node.js.

## Overview

libssh2.js provides a complete SSH client implementation compiled to WebAssembly using Emscripten. It supports:

- **SSH Connections**: Establish SSH connections to remote servers
- **Authentication**: Password and public key authentication
- **Terminal Sessions**: Interactive shell sessions with PTY support
- **Custom Transport**: WebSocket bridge support for browser environments
- **Cross-Platform**: Works in browsers, Node.js, and other JavaScript runtimes

## Features

- 🔐 **SSH Protocol Support**: Full SSH2 protocol implementation
- 🌐 **WebAssembly**: Native performance in JavaScript environments
- 🔌 **Custom Transport**: WebSocket bridge for browser compatibility
- 🖥️ **Terminal Support**: Interactive shell sessions with PTY
- 🔑 **Multiple Auth Methods**: Password and public key authentication
- 📱 **Browser Compatible**: Works in modern browsers via WebSocket
- 🚀 **Node.js Ready**: Native performance in Node.js environments
- 📝 **TypeScript Support**: Full type definitions included

## Prerequisites

- **Docker**: For building the WebAssembly module
- **Node.js**: Version 22.14.0 or later
- **Modern Browser**: For browser-based usage (Chrome, Firefox, Safari, Edge)

## Building

The project includes a Docker-based build system that compiles libssh2 and OpenSSL to WebAssembly.

### Quick Build

```bash
# Build the WebAssembly module
docker build -t libssh2-wasm .

# Extract the built files
docker create --name temp-container libssh2-wasm
docker cp temp-container:/workspace/libssh2.js ./
docker cp temp-container:/workspace/libssh2.wasm ./
docker rm temp-container
```

### Build Details

The build process:
1. Sets up Ubuntu 22.04 with Node.js 22.14.0
2. Installs Emscripten SDK
3. Compiles OpenSSL 1.1.1w for WebAssembly
4. Compiles libssh2 with OpenSSL backend
5. Compiles the custom bindings to WebAssembly

## TypeScript Support

This package includes full TypeScript support with comprehensive type definitions. The types are automatically included when you install the package and will be resolved by TypeScript automatically.

### Package.json Types Field

The package includes a `types` field in `package.json` that points to `dist/index.d.ts`, ensuring TypeScript automatically finds the type definitions:

```json
{
  "types": "dist/index.d.ts"
}
```

### TypeScript Usage

```typescript
import { LibSSH2, SSHConnectionOptions } from '@verdigris/libssh2.js';

// Initialize the library
await LibSSH2.init();

// Create a new session
const session = await LibSSH2.createSession();

// Connect to SSH server
const connectionResult = await LibSSH2.connect(session, {
  host: 'example.com',
  port: 22,
  username: 'user'
});

if (connectionResult.success) {
  // Authenticate
  const authResult = await LibSSH2.authenticatePassword(
    session,
    'user',
    'password'
  );

  if (authResult) {
    // Execute command
    const result = await LibSSH2.executeCommand(session, 'ls -la');
    console.log(result.stdout);
  }
}

// Cleanup
LibSSH2.closeSession(session);
LibSSH2.exit();
```

## Usage

### Basic SSH Connection

```javascript
import SSH2Module from './libssh2.js';

// Initialize the module
const SSH2 = await SSH2Module();

// Initialize libssh2
SSH2.ccall('ssh2_init', 'number', [], []);

// Create session
const session = SSH2.ccall('ssh2_session_init_custom', 'number', [], []);

// Set custom transport callbacks
SSH2.customSend = (socket, buffer, length) => {
    // Implement your transport logic here
    // For WebSocket: send data via WebSocket
    return length;
};

SSH2.customRecv = (socket, buffer, length) => {
    // Implement your transport logic here
    // For WebSocket: receive data from WebSocket
    return receivedLength;
};

// Perform handshake
const handshakeResult = SSH2.ccall('ssh2_session_handshake_custom', 'number', ['number'], [session]);

// Authenticate
const authResult = SSH2.ccall('ssh2_userauth_password_custom', 'number',
    ['number', 'string', 'string'], [session, 'username', 'password']);
```

### Terminal Session

```javascript
// Open channel
const channel = SSH2.ccall('ssh2_channel_open_session_custom', 'number', ['number'], [session]);

// Request PTY
SSH2.ccall('ssh2_channel_request_pty_custom', 'number', ['number', 'string'], [channel, 'xterm']);

// Set PTY size
SSH2.ccall('ssh2_channel_request_pty_size', 'number', ['number', 'number', 'number'], [channel, 80, 24]);

// Start shell
SSH2.ccall('ssh2_channel_shell_custom', 'number', ['number'], [channel]);

// Read from channel
const buffer = SSH2._malloc(1024);
const bytesRead = SSH2.ccall('ssh2_channel_read_custom', 'number', ['number', 'number', 'number'], [channel, buffer, 1024]);

// Write to channel
SSH2.ccall('ssh2_channel_write_custom', 'number', ['number', 'string', 'number'], [channel, 'ls -la\n', 8]);
```

### WebSocket Bridge Example

```javascript
// Set up WebSocket connection
const ws = new WebSocket('ws://your-ssh-proxy-server');

// Custom transport implementation
SSH2.customSend = (socket, buffer, length) => {
    const data = new Uint8Array(SSH2.HEAPU8.buffer, buffer, length);
    ws.send(data);
    return length;
};

SSH2.customRecv = (socket, buffer, length) => {
    // Handle incoming WebSocket data
    // Copy to the provided buffer
    return receivedLength;
};
```

## API Reference

### Core Functions

| Function | Description |
|----------|-------------|
| `ssh2_init()` | Initialize libssh2 library |
| `ssh2_exit()` | Cleanup libssh2 library |
| `ssh2_version()` | Get libssh2 version string |

### Session Management

| Function | Description |
|----------|-------------|
| `ssh2_session_init_custom()` | Create new SSH session with custom transport |
| `ssh2_session_handshake_custom()` | Perform SSH handshake |
| `ssh2_session_set_blocking()` | Set session blocking mode |
| `ssh2_session_free()` | Free session resources |

### Authentication

| Function | Description |
|----------|-------------|
| `ssh2_userauth_password_custom()` | Authenticate with username/password |
| `ssh2_userauth_publickey_fromfile()` | Authenticate with public key |

### Channel Operations

| Function | Description |
|----------|-------------|
| `ssh2_channel_open_session_custom()` | Open new session channel |
| `ssh2_channel_request_pty_custom()` | Request PTY allocation |
| `ssh2_channel_shell_custom()` | Start interactive shell |
| `ssh2_channel_read_custom()` | Read data from channel |
| `ssh2_channel_write_custom()` | Write data to channel |
| `ssh2_channel_close()` | Close channel |
| `ssh2_channel_free()` | Free channel resources |

## Architecture

The project consists of three main components:

1. **ssh2_bindings.c**: C bindings that expose libssh2 functionality to JavaScript
2. **Dockerfile**: Build environment for compiling to WebAssembly
3. **Generated Files**: `libssh2.js` and `libssh2.wasm` (built output)

### Custom Transport

The bindings implement a custom transport layer that allows:
- WebSocket bridging for browser environments
- Custom I/O handling for different network protocols
- Integration with existing JavaScript networking code

## Browser Compatibility

- **Chrome**: 67+ (WebAssembly support)
- **Firefox**: 52+ (WebAssembly support)
- **Safari**: 11+ (WebAssembly support)
- **Edge**: 79+ (WebAssembly support)

## Node.js Support

- **Node.js**: 22.14.0+ (for building)
- **Runtime**: Any Node.js version with WebAssembly support

## Security Considerations

- **Transport Security**: Ensure your WebSocket or transport layer is secure (WSS, etc.)
- **Key Management**: Store private keys securely
- **Authentication**: Use strong passwords and key-based authentication when possible
- **Network Security**: SSH connections should be over secure networks

## Development

### Project Structure

```
libssh2.js/
├── ssh2_bindings.c      # C bindings for libssh2
├── Dockerfile           # Build environment
├── README.md            # This file
└── .circleci/           # CI configuration
```

### Building from Source

1. Clone the repository
2. Ensure Docker is running
3. Run the build command
4. Extract the generated files

### Customization

The `ssh2_bindings.c` file can be modified to:
- Add new SSH functionality
- Customize transport behavior
- Expose additional libssh2 features

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This project is based on libssh2, which is licensed under the [BSD 3-Clause License](https://github.com/libssh2/libssh2/blob/master/COPYING).

## Acknowledgments

- [libssh2](https://www.libssh2.org/) - The original C library
- [Emscripten](https://emscripten.org/) - WebAssembly compilation toolchain
- [OpenSSL](https://www.openssl.org/) - Cryptographic library

## Support

For issues and questions:
- Check the [libssh2 documentation](https://www.libssh2.org/docs.html)
- Review the Emscripten [WebAssembly guide](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/WebAssembly.html)
- Open an issue in this repository

---

**Note**: This is a WebAssembly port of libssh2. For production use, ensure your transport layer (WebSocket, etc.) provides appropriate security measures.
