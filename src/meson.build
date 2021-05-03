conf_data = configuration_data()
conf_data.set('COLDWIND_WIN32', host_machine.system() == 'windows' ? 'true' : 'false')
conf_data.set10('COLDWIND_WINDOWS', host_machine.system() == 'windows')
configure_file(input: 'config.h.in',
               output: 'config.h',
               configuration: conf_data)

libcoldwind_src = ['parser.cxx',
                   'hasher.cxx',
                   'downloader.cxx',
                   'lurker.cxx',
                   'manager.cxx',
                   'request.cxx',
                   'xml_reader.cpp',
                   'xml_writer.cpp',
                   'horizon_post.c',
                   'post.cpp',
                   'config.cpp']

coldwind_src = ['main.cxx',
                'application.cxx',
                'window.cxx',
                'window_gtk3.cxx',
                'windowimpl.cxx']

libcoldwind = library('libcoldwind', libcoldwind_src,
                      dependencies: [glibmm_dep, giomm_dep, curl_dep, xml_dep, json_dep, thread_dep],
                      install : false,
                      cpp_args : ['-D_GNU_SOURCE', '-fanalyzer', '-fanalyzer-verbosity=3', '-DG_ENABLE_DEBUG=true'],
                      c_args : ['-D_GNU_SOURCE', '-fanalyzer',  '-DG_ENABLE_DEBUG=true']
                            )

libcoldwind_dep = declare_dependency(link_with: libcoldwind,
                                     include_directories: include_directories('./'),
                                     dependencies: [glibmm_dep])

coldwind = executable('coldwind', coldwind_src, coldwind_resources,
                      dependencies : [gtkmm_dep, xml_dep, thread_dep, curl_dep],
                      link_with : libcoldwind,
                      install : true,
                      gui_app : true,
                      cpp_args : ['-D_GNU_SOURCE', '-fanalyzer', '-fanalyzer-verbosity=3', '-DG_ENABLE_DEBUG=true'],
                      c_args : ['-D_GNU_SOURCE', '-fanalyzer',  '-DG_ENABLE_DEBUG=true'],
#                      link_args : ['-fsanitize=thread']

                     )
