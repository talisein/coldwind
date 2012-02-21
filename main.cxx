// Derp

#include <glibmm/thread.h>
#include "application.hxx"
#include <curl/curl.h>
#include <iostream>

int main (int argc, char *argv[])
{
  CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
  if (code != CURLE_OK) {
    std::cerr << "Error: While initializing curl: " << curl_easy_strerror(code) << std::endl;
    return EXIT_FAILURE;
  }

  if(!Glib::thread_supported()) Glib::thread_init();

  Derp::Application app(argc, argv);
  app.run();

  return EXIT_SUCCESS;
}
