// Derp

#include "application.hxx"
#include <curl/curl.h>
#include <libxml/parser.h>
#include <iostream>

int main (int argc, char *argv[])
{
  CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
  if (code != CURLE_OK) {
    std::cerr << "Error: While initializing curl: " << curl_easy_strerror(code) << std::endl;
    return EXIT_FAILURE;
  }

  LIBXML_TEST_VERSION

  Derp::Application app(argc, argv);
  app.run();

  xmlCleanupParser();
  if ( xmlMemUsed() > 0 ) {
    std::cerr << "Warning: libxml2 is using " << xmlMemUsed() << " bytes of memory before exit." << std::endl;
  }

  curl_global_cleanup();
  return EXIT_SUCCESS;
}
