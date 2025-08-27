export interface ModuleOptions {
  socket: {
    send: (socket: number, bufferPointer: number, length: number) => number;
    recv: (socket: number, bufferPointer: number, length: number) => number;
  }
}

export interface SessionOptions {
  username?: string;
  password?: string;
  privateKey?: string;
  publicKey?: string;
  passphrase?: string;
}
