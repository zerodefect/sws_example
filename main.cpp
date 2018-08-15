// MIT License - edited version of Eidheim's 'Simple Web Server' (test/io_test.cpp)

#include <simple-web-server/client_http.hpp>
#include <simple-web-server/server_http.hpp>

#include <cassert>

using namespace std;

#ifndef USE_STANDALONE_ASIO
namespace asio = boost::asio;
#endif

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

int main()
{
  HttpServer server;
  server.config.port = 8080;

  server.resource["^/string$"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
    auto content = request->content.string();

    *response << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n"
              << content;

    assert(!request->remote_endpoint_address().empty());
    assert(request->remote_endpoint_port() != 0);
  };

  server.resource["^/string/dup$"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
    auto content = request->content.string();

    // Send content twice, before it has a chance to be written to the socket.
    *response << "HTTP/1.1 200 OK\r\nContent-Length: " << (content.length() * 2) << "\r\n\r\n"
              << content;
    response->send();
    *response << content;
    response->send();

    assert(!request->remote_endpoint_address().empty());
    assert(request->remote_endpoint_port() != 0);
  };

  server.resource["^/string2$"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
    response->write(request->content.string());
  };

  server.resource["^/string3$"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
    stringstream stream;
    stream << request->content.rdbuf();
    response->write(stream);
  };

  server.resource["^/string4$"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> /*request*/) {
    response->write(SimpleWeb::StatusCode::client_error_forbidden, {{"Test1", "test2"}, {"tesT3", "test4"}});
  };

  server.resource["^/info$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
    stringstream content_stream;
    content_stream << request->method << " " << request->path << " " << request->http_version << " ";
    content_stream << request->header.find("test parameter")->second;

    content_stream.seekp(0, ios::end);

    *response << "HTTP/1.1 200 OK\r\nContent-Length: " << content_stream.tellp() << "\r\n\r\n"
              << content_stream.rdbuf();
  };

  server.resource["^/work$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> /*request*/) {
    thread work_thread([response] {
      this_thread::sleep_for(chrono::seconds(5));
      response->write("Work done");
    });
    work_thread.detach();
  };

  server.resource["^/match/([0-9]+)$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
    string number = request->path_match[1];
    *response << "HTTP/1.1 200 OK\r\nContent-Length: " << number.length() << "\r\n\r\n"
              << number;
  };

  server.resource["^/header$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
    auto content = request->header.find("test1")->second + request->header.find("test2")->second;

    *response << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n"
              << content;
  };

  server.resource["^/query_string$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
    assert(request->path == "/query_string");
    assert(request->query_string == "testing");
    auto queries = request->parse_query_string();
    auto it = queries.find("Testing");
    assert(it != queries.end() && it->first == "testing" && it->second == "");
    response->write(request->query_string);
  };

  server.resource["^/chunked$"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
    assert(request->path == "/chunked");

    assert(request->content.string() == "SimpleWeb in\r\n\r\nchunks.");

    response->write("6\r\nSimple\r\n3\r\nWeb\r\nE\r\n in\r\n\r\nchunks.\r\n0\r\n\r\n", {{"Transfer-Encoding", "chunked"}});
  };

  thread server_thread([&server]() {
    // Start server
    server.start();
  });

  this_thread::sleep_for(chrono::seconds(1));

  server.stop();
  server_thread.join();

  server_thread = thread([&server]() {
    // Start server
    server.start();
  });

  this_thread::sleep_for(chrono::seconds(1));

  server.stop();
  server_thread.join();

  // Test server destructor
  {
    auto io_service = make_shared<asio::io_service>();
    bool call = false;
    bool client_catch = false;
    {
      HttpServer server;
      server.config.port = 8081;
      server.io_service = io_service;
      server.resource["^/test$"]["GET"] = [&call](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> /*request*/) {
        call = true;
        thread sleep_thread([response] {
          this_thread::sleep_for(chrono::seconds(5));
          response->write(SimpleWeb::StatusCode::success_ok, "test");
          response->send([](const SimpleWeb::error_code & /*ec*/) {
            assert(false);
          });
        });
        sleep_thread.detach();
      };
      server.start();
      thread server_thread([io_service] {
        io_service->run();
      });
      server_thread.detach();
      this_thread::sleep_for(chrono::seconds(1));
      thread client_thread([&client_catch] {
        HttpClient client("localhost:8081");
        try {
          auto r = client.request("GET", "/test");
          assert(false);
        }
        catch(...) {
          client_catch = true;
        }
      });
      client_thread.detach();
      this_thread::sleep_for(chrono::seconds(1));
    }
    this_thread::sleep_for(chrono::seconds(5));
    assert(call);
    assert(client_catch);
    io_service->stop();
  }
}
