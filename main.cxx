// Derp

#include <glibmm/thread.h>
#include "application.hxx"
#include <curl/curl.h>


int main (int argc, char *argv[])
{
  curl_global_init(CURL_GLOBAL_ALL);
  if(!Glib::thread_supported()) Glib::thread_init();

  Derp::Application app(argc, argv);
  app.run();

  return EXIT_SUCCESS;
}
