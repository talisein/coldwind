project('coldwind', ['c', 'cpp'],
        license : 'GPLv3+',
        version : '0.5.0',
        default_options : ['c_std=c11',
                           'cpp_std=c++2a',
                           'buildtype=release'])

gtk_dep    = dependency('gtk+-3.0')
cpp_compiler = meson.get_compiler('cpp')

glibmm_dep = dependency('glibmm-2.4',  version : '>=2.44.0')
giomm_dep  = dependency('giomm-2.4',  version : '>=2.44.0')
gtkmm_dep  = dependency('gtkmm-3.0',   version : '>=3.4.0')
curl_dep   = dependency('libcurl',     version : '>=7.24.0')
xml_dep    = dependency('libxml-2.0',  version : '>=2.7.8')
json_dep   = dependency('json-glib-1.0')
thread_dep = dependency('threads')

mutest_dep = dependency('mutest-1',
  fallback: [ 'mutest', 'mutest_dep' ],
  default_options: ['static=true'],
  required: false,
  disabler: true,
)

cpp_compiler.has_header('tuple', required: true)
cpp_compiler.has_header_symbol('memory', 'std::make_unique', required: true)
cpp_compiler.has_header_symbol('charconv', 'std::to_chars', required: true)
cpp_compiler.has_header_symbol('source_location', '__cpp_lib_source_location')
cpp_compiler.has_header_symbol('version', '__cpp_lib_constexpr_string')
cpp_compiler.has_header_symbol('version', '__cpp_lib_coroutine')
cpp_compiler.has_header_symbol('version', '__cpp_lib_bit_cast')
cpp_compiler.has_header_symbol('version', '__cpp_lib_format')
cpp_compiler.has_header_symbol('version', '__cpp_lib_string_view')
cpp_compiler.has_header_symbol('version', '__cpp_lib_filesystem')

subdir('res')
subdir('src')
subdir('test')
