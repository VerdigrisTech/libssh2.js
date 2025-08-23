declare module '@verdigris/libssh2.js' {
  /**
   * SSH session handle
   */
  export type SSHSession = number;

  /**
   * SSH channel handle
   */
  export type SSHChannel = number;

  /**
   * SSH socket handle
   */
  export type SSHSocket = number;

  /**
   * Custom transport callback functions
   */
  export interface CustomTransport {
    customSend: (socket: SSHSocket, buffer: number, length: number) => number;
    customRecv: (socket: SSHSocket, buffer: number, length: number) => number;
  }

  /**
   * SSH2 WASM module interface
   */
  export interface SSH2Module extends CustomTransport {
    /**
     * Initialize libssh2 library
     * @param funcName Function name to call
     * @param returnType Return type
     * @param argTypes Array of argument types
     * @param args Array of arguments
     */
    ccall(
      funcName: string,
      returnType: string,
      argTypes: string[],
      args: any[]
    ): number;

    /**
     * Allocate memory in WASM heap
     * @param size Size in bytes to allocate
     */
    _malloc(size: number): number;

    /**
     * Free memory from WASM heap
     * @param ptr Pointer to free
     */
    _free(ptr: number): void;

    /**
     * Access to WASM heap memory
     */
    HEAPU8: Uint8Array;

    /**
     * Custom send callback for transport
     */
    customSend: (socket: SSHSocket, buffer: number, length: number) => number;

    /**
     * Custom receive callback for transport
     */
    customRecv: (socket: SSHSocket, buffer: number, length: number) => number;
  }

  /**
   * SSH2 module factory function
   */
  export default function SSH2Module(): Promise<SSH2Module>;
}
