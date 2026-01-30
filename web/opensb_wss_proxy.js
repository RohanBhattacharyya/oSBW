#!/usr/bin/env node
"use strict";

const http = require("http");
const net = require("net");
const { URL } = require("url");
const WebSocket = require("ws");

function parseArgs(argv) {
  const args = {
    listenHost: "0.0.0.0",
    listenPort: 8080,
    targetHost: "127.0.0.1",
    targetPort: 21025,
    path: "/opensb",
    token: "",
  };

  for (let i = 2; i < argv.length; i++) {
    const arg = argv[i];
    const next = argv[i + 1];
    if (arg === "--listen" && next) {
      i++;
      const [host, port] = next.split(":");
      args.listenHost = host || "0.0.0.0";
      args.listenPort = Number(port) || args.listenPort;
    } else if (arg === "--target" && next) {
      i++;
      const [host, port] = next.split(":");
      args.targetHost = host || "127.0.0.1";
      args.targetPort = Number(port) || args.targetPort;
    } else if (arg === "--path" && next) {
      i++;
      args.path = next.startsWith("/") ? next : `/${next}`;
    } else if (arg === "--token" && next) {
      i++;
      args.token = next;
    }
  }

  return args;
}

const options = parseArgs(process.argv);

const server = http.createServer((req, res) => {
  res.writeHead(200, { "content-type": "text/plain" });
  res.end("OpenStarbound WSS proxy\n");
});

const wss = new WebSocket.Server({
  server,
  path: options.path,
  perMessageDeflate: false,
});

wss.on("connection", (ws, req) => {
  const url = new URL(req.url, `http://${req.headers.host}`);
  if (options.token && url.searchParams.get("token") !== options.token) {
    ws.close(1008, "Invalid token");
    return;
  }

  const tcp = net.connect(options.targetPort, options.targetHost);
  tcp.setNoDelay(true);

  ws.on("message", (data) => {
    if (tcp.destroyed) return;
    tcp.write(Buffer.from(data));
  });

  tcp.on("data", (data) => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(data, { binary: true });
    }
  });

  const closeBoth = () => {
    if (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CLOSING) {
      ws.close();
    }
    if (!tcp.destroyed) {
      tcp.destroy();
    }
  };

  ws.on("close", closeBoth);
  ws.on("error", closeBoth);
  tcp.on("close", closeBoth);
  tcp.on("error", closeBoth);
});

server.listen(options.listenPort, options.listenHost, () => {
  console.log(`[OpenSB] WSS proxy listening on ${options.listenHost}:${options.listenPort}${options.path}`);
  console.log(`[OpenSB] Forws serverarding to ${options.targetHost}:${options.targetPort}`);
  if (options.token) {
    console.log(`[OpenSB] Token auth enabled`);
  }
});
