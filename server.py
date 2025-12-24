#!/usr/bin/env python3
"""
Simple HTTP server with COOP/COEP headers for SharedArrayBuffer support.
Required for FFmpeg.wasm to work properly.

Usage: python3 server.py [port]
Default port: 8080
"""

import http.server
import socketserver
import sys

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8080

class CORSRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Required for SharedArrayBuffer (needed by FFmpeg.wasm)
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        # Cache control
        self.send_header('Cache-Control', 'no-cache, no-store, must-revalidate')
        super().end_headers()
    
    def do_GET(self):
        # Handle .wasm files with correct MIME type
        if self.path.endswith('.wasm'):
            self.send_response(200)
            self.send_header('Content-Type', 'application/wasm')
            self.end_headers()
            with open('.' + self.path, 'rb') as f:
                self.wfile.write(f.read())
            return
        return super().do_GET()

with socketserver.TCPServer(("", PORT), CORSRequestHandler) as httpd:
    print(f"🚀 Server running at http://localhost:{PORT}")
    print(f"   SharedArrayBuffer: enabled")
    print(f"   Press Ctrl+C to stop")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n👋 Server stopped")
