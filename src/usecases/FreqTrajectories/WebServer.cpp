#include <algorithm>
#include <fstream>
#include <vector>
#include <memory>
 
#include <boost/filesystem.hpp>

#include "SimpleWeb/server_http.hpp"
#include "SimpleWeb/utility.hpp"

#include "WebServer.hpp"

std::shared_ptr<std::thread> runWebServer(HttpServer& server, const std::string& webRoot) {
 server.default_resource["GET"] = [&](std::shared_ptr<HttpServer::Response> response,
     std::shared_ptr<HttpServer::Request> request) {
    try {
      auto web_root_path = boost::filesystem::canonical(webRoot);
      auto path = boost::filesystem::canonical(web_root_path / request->path);
      // Check if path is within web_root_path
      if(std::distance(web_root_path.begin(), web_root_path.end()) > std::distance(path.begin(), path.end()) ||
         !std::equal(web_root_path.begin(), web_root_path.end(), path.begin())) 
        throw std::invalid_argument("path must be within root path");
      if(boost::filesystem::is_directory(path))
        path /= "index.html";

      SimpleWeb::CaseInsensitiveMultimap header;

      auto ifs = std::make_shared<std::ifstream>();
      ifs->open(path.string(), std::ifstream::in | std::ios::binary | std::ios::ate);

      if(*ifs) {
        auto length = ifs->tellg();
        ifs->seekg(0, std::ios::beg);

        header.emplace("Content-Length", to_string(length));
        response->write(header);

        // Trick to define a recursive function within this scope (for example purposes)
        class FileServer {
        public:
          static void read_and_send(const std::shared_ptr<HttpServer::Response> &response, 
              const std::shared_ptr<std::ifstream> &ifs) {
            // Read and send 128 KB at a time
            static std::vector<char> buffer(131072); // Safe when server is running on one thread
            std::streamsize read_length;
            if((read_length = ifs->read(&buffer[0], static_cast<std::streamsize>(buffer.size())).gcount()) > 0) {
              response->write(&buffer[0], read_length);
              if(read_length == static_cast<std::streamsize>(buffer.size())) {
                response->send([response, ifs](const SimpleWeb::error_code &ec) {
                  if(!ec)
                    read_and_send(response, ifs);
                  else
                    std::cerr << "Connection interrupted" << std::endl;
                });
              }
            }
          }
        };
        FileServer::read_and_send(response, ifs);
      }
      else
        throw std::invalid_argument("could not read file");
    }
    catch(const std::exception &e) {
      response->write(SimpleWeb::StatusCode::client_error_bad_request, "Could not open path " + request->path + ": " + e.what());
    }
  };
  server.on_error = [](std::shared_ptr<HttpServer::Request> /*request*/, const SimpleWeb::error_code & /*ec*/) {
    // Handle errors here
  };

  return std::make_shared<std::thread>([&server]() {
    // Start server
    std::cout << "starting internal web server at port 8080 ..." << std::endl;
    server.start();
});  
}
