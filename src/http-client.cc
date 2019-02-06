#include "../http.h"

namespace http {

  int Client::complete(http_parser* parser) {
    Context* context = reinterpret_cast<Context*>(parser->data);

    Response res;
    res.body = context->body.str();
    res.parser = *parser;

    listener(res);
    return 0;
  }

  void Client::on_connect (uv_connect_t* req, int status) {
    // @TODO
    // Populate address and time info for logging / stats etc.

    Context* context = reinterpret_cast<Context*>(req->handle->data);
    Client* client = static_cast<Client*>(context->instance);

    //attachEvents(parser_settings);

    if (status == -1) {
      // @TODO
      // Error Callback
      uv_close((uv_handle_t*)req->handle, free_context);
      return;
    }

    uv_buf_t reqbuf;
    std::string reqstr =
      client->opts.method + " " + client->opts.url + " HTTP/1.1" + CRLF +
      //
      // @TODO
      // Add user's headers here
      //
      "Connection: keep-alive" + CRLF + CRLF;

    reqbuf.base = (char*) reqstr.c_str();
    reqbuf.len = reqstr.size();

    uv_read_start(
      req->handle,
      Client::read_allocator, 
      Client::read);

    uv_write(
      &context->write_req,
      req->handle,
      &reqbuf,
      1,
      NULL);
  }

  void Client::on_resolved(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
    Client* client = reinterpret_cast<Client*>(req->data);

    char addr[17] = { '\0' };

    uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
    uv_freeaddrinfo(res);

    struct sockaddr_in dest;
    uv_ip4_addr(addr, client->opts.port, &dest);

    Context* context = new Context();
    context->instance = client;

    context->handle.data = context;
    http_parser_init(&context->parser, HTTP_RESPONSE);
    context->parser.data = context;

    uv_tcp_init(client->UV_LOOP, &context->handle);
    //uv_tcp_keepalive(&context->handle, 1, 60);
    
    context->connect_req.data = client;

    uv_tcp_connect(
      &context->connect_req,
      &context->handle,
      (const struct sockaddr*) &dest,
      Client::on_connect);
  };

  void Client::connect(uv_loop_t* loop) {

    struct addrinfo ai;
    ai.ai_family = PF_INET;
    ai.ai_socktype = SOCK_STREAM;
    ai.ai_protocol = IPPROTO_TCP;
    ai.ai_flags = 0;

    bool managesLoop;
    if (loop) {
      UV_LOOP = loop;
      managesLoop = false;
    } else {
      UV_LOOP = uv_default_loop();
      managesLoop = true;
    }

    addr_req.data = this;
    uv_getaddrinfo(UV_LOOP, &addr_req, Client::on_resolved, opts.host.c_str(), to_string(opts.port).c_str(), &ai);
    if (managesLoop) {
      uv_run(UV_LOOP, UV_RUN_DEFAULT);
    }
  }

} // namespace http

