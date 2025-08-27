// @ts-ignore
import createModule from '../../../wasm/libssh2.js';

export function initLibSSH2(options?: any): Promise<any> {
  // This would initialize the WASM module when available
  return new Promise((resolve) => {
    // Module will be available globally after WASM loads
    if (typeof (globalThis as any).Module !== 'undefined') {
      resolve((globalThis as any).Module);
    } else {
      // Wait for module to be ready
      (globalThis as any).onModuleReady = resolve;
    }
  });
}

export * from './types.js';
export * from './websocket-transport.js';
