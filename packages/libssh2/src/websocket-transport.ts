export interface WebSocketTransportOptions {
  url: string;
  protocols?: string[];
  timeout?: number;
}

export class WebSocketTransport {
  private socket: WebSocket | null = null;
  private socketId: number;
  private connected: boolean = false;
  private messageQueue: Uint8Array[] = [];
  private resolveConnection: ((value: void) => void) | null = null;
  private rejectConnection: ((reason: any) => void) | null = null;

  constructor(private options: WebSocketTransportOptions) {
    this.socketId = Math.floor(Math.random() * 1000000);
  }

  async connect(): Promise<void> {
    return new Promise((resolve, reject) => {
      this.resolveConnection = resolve;
      this.rejectConnection = reject;

      try {
        this.socket = new WebSocket(this.options.url, this.options.protocols);
        this.socket.binaryType = 'arraybuffer';

        this.socket.onopen = () => {
          this.connected = true;
          if (this.resolveConnection) {
            this.resolveConnection();
            this.resolveConnection = null;
            this.rejectConnection = null;
          }
        };

        this.socket.onclose = () => {
          this.connected = false;
          if (this.rejectConnection) {
            this.rejectConnection(new Error('WebSocket connection closed'));
            this.resolveConnection = null;
            this.rejectConnection = null;
          }
        };

        this.socket.onerror = (error) => {
          if (this.rejectConnection) {
            this.rejectConnection(error);
            this.resolveConnection = null;
            this.rejectConnection = null;
          }
        };

        this.socket.onmessage = (event) => {
          if (event.data instanceof ArrayBuffer) {
            this.messageQueue.push(new Uint8Array(event.data));
          }
        };

        // Set timeout if specified
        if (this.options.timeout) {
          setTimeout(() => {
            if (!this.connected && this.rejectConnection) {
              this.rejectConnection(new Error('WebSocket connection timeout'));
              this.resolveConnection = null;
              this.rejectConnection = null;
              if (this.socket) {
                this.socket.close();
              }
            }
          }, this.options.timeout);
        }
      } catch (error) {
        if (this.rejectConnection) {
          this.rejectConnection(error);
          this.resolveConnection = null;
          this.rejectConnection = null;
        }
      }
    });
  }

  send(data: Uint8Array): boolean {
    if (!this.socket || !this.connected) {
      return false;
    }

    try {
      this.socket.send(data.buffer);
      return true;
    } catch (error) {
      console.error('WebSocket send error:', error);
      return false;
    }
  }

  receive(): Uint8Array | null {
    if (this.messageQueue.length > 0) {
      return this.messageQueue.shift() || null;
    }
    return null;
  }

  close(): void {
    if (this.socket) {
      this.connected = false;
      this.socket.close();
      this.socket = null;
    }
  }

  get isConnected(): boolean {
    return this.connected;
  }

  get id(): number {
    return this.socketId;
  }
}

// Socket proxy for WASM integration
export class SocketProxy {
  private static transports = new Map<number, WebSocketTransport>();

  static registerTransport(transport: WebSocketTransport): number {
    this.transports.set(transport.id, transport);
    return transport.id;
  }

  static unregisterTransport(socketId: number): void {
    const transport = this.transports.get(socketId);
    if (transport) {
      transport.close();
      this.transports.delete(socketId);
    }
  }

  static send(socketId: number, data: Uint8Array): boolean {
    const transport = this.transports.get(socketId);
    return transport ? transport.send(data) : false;
  }

  static receive(socketId: number): Uint8Array | null {
    const transport = this.transports.get(socketId);
    return transport ? transport.receive() : null;
  }

  static isConnected(socketId: number): boolean {
    const transport = this.transports.get(socketId);
    return transport ? transport.isConnected : false;
  }
}