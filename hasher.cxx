#ifndef HASHER_CXX
#define HASHER_CXX

#include "hasher.hxx"
#include <glibmm/thread.h>
#include <glibmm/convert.h>
#include <glibmm/checksum.h>
#include <glibmm.h>
#include <iostream>
#include "request.hxx"
#include "config.h"
#include "xml_writer.hpp"
#include "xml_reader.hpp"

namespace Derp {
    Hasher::Hasher()
    {
        active.send([this]{ load_from_disk(); });
    }

    Hasher::~Hasher()
    {
        active.send([this]{ save_to_disk(); });
    }

    void Hasher::hash_async(const Request& request, const HasherCallback& cb)
    {
        active.send([this, request, cb]{ hash(request, cb); });
    }

    void Hasher::hash(const Request& request, const HasherCallback& cb)
    {
        auto base = request.getHashDirectory();
        hash_directory(base);

        auto enumerator = base->enumerate_children("standard::type,standard::name");
        for ( auto info = enumerator->next_file(); info; info = enumerator->next_file()) {
            if ( info->get_file_type() == Gio::FileType::FILE_TYPE_DIRECTORY ) {
                auto dir = base->get_child(info->get_name());
                hash_directory(dir);
            }
        }
        enumerator->close();

        auto board_dir = base->get_child(Glib::filename_from_utf8(request.getBoard()));
        if (board_dir->query_exists()) {
            auto board_enumerator = board_dir->enumerate_children("standard::type,standard::name");
            for ( auto info = board_enumerator->next_file(); info; info = board_enumerator->next_file()) {
                if ( info->get_file_type() == Gio::FileType::FILE_TYPE_DIRECTORY ) {
                    auto dir = board_dir->get_child(info->get_name());
                    hash_directory(dir);
                }
            }
            board_enumerator->close();
        }

        active.send([this, cb]{ m_dispatcher(cb); });
    }

    void Hasher::hash_directory(const Glib::RefPtr<Gio::File>& dir)
    {
        auto enumerator = dir->enumerate_children("standard::type,standard::name,standard::size");
        for(auto info = enumerator->next_file(); info; info = enumerator->next_file()) {
            if ( info->get_file_type() != Gio::FileType::FILE_TYPE_REGULAR )
                continue;
            if ( info->get_size() > MAXIMUM_FILESIZE )
                continue;

            auto file = dir->get_child(info->get_name());
            if (!has_file_path(file->get_path())) {
                auto path = file->get_path();
                active.send([this, file]{ hash_file(file); });
            }
        }
        enumerator->close();
    }

    void Hasher::hash_file(const Glib::RefPtr<Gio::File>& file)
    {
        char* contents;
        gsize length;

        try {
            file->load_contents(contents, length);
            auto md5hex = Glib::Checksum::compute_checksum(Glib::Checksum::ChecksumType::CHECKSUM_MD5,
                                                           reinterpret_cast<guchar*>(contents),
                                                           length);
            insert_md5(md5hex, file->get_path());
            g_free(contents);
        } catch (Gio::Error e) {
            std::cerr << "Error: While trying to load and hash " << file->get_path()
                      << ": " << e.what() << " Code: " << e.code() <<std::endl;
        }
    }

    void Hasher::insert_md5(const std::string& md5sum, const std::string& path)
    {
        std::lock_guard<std::mutex> lock(m_table_mutex);
        m_hash_table.insert(std::make_pair(md5sum, path));
        m_filepath_table.insert(path);
    }

    bool Hasher::has_md5(const std::string& md5sum) const
    {
        std::lock_guard<std::mutex> lock(m_table_mutex);
        return m_hash_table.find(md5sum) != m_hash_table.end();
    }

    bool Hasher::has_file_path(const std::string& path) const
    {
        std::lock_guard<std::mutex> lock(m_table_mutex);
        return m_filepath_table.find(path) != m_filepath_table.end();
    }

    namespace {
        static Glib::RefPtr<Gio::File>
        get_local_file()
        {
            auto data_dir = Glib::get_user_data_dir();
            auto dir = Glib::build_filename(data_dir, "coldwind");
            auto dirfile = Gio::File::create_for_path(dir);
            if (!dirfile->query_exists()) {
                try {
                    dirfile->make_directory_with_parents();
                } catch (Gio::Error e) {
                    std::cerr << "Hasher Error: Unable to create directory "
                              << dirfile->get_path() << ": " << e.what()
                              << std::endl;
                }
            }

            auto info = dirfile->query_info(G_FILE_ATTRIBUTE_STANDARD_TYPE);
            if (info->get_file_type() != Gio::FILE_TYPE_DIRECTORY) {
                std::cerr << "Hasher Error: " << dirfile->get_path()
                          << " is not a directory. Will be unable to cache hashed file"
                          << " list to disk." << std::endl;
            }
            
            return dirfile->get_child_for_display_name("hashed_files.xml");
        }
    }

    void Hasher::load_from_disk()
    {
        auto file = get_local_file();
        char* contents;
        gsize length;
        try {
            file->load_contents(contents, length);
        } catch (Gio::Error e) {
            if (e.code() == Gio::Error::NOT_FOUND)
                return;
            std::cerr << "Hasher Error: Could not load " << file->get_path()
                      << ": " << e.what() << std::endl;
            return;
        }
        
        XmlReader reader(std::string(contents, length));
        std::lock_guard<std::mutex> lock(m_table_mutex);
        try {
            std::string hash;
            enum { HASH_NODE, PATH_NODE, SIZE_NODE, OTHER_NODE } node;
            while(reader.read() == 1) {
                auto const type = reader.get_type();
                if (type == XML_READER_TYPE_ELEMENT) {
                    auto const name = reader.get_name();
                    if (name == "hash") {
                        node = HASH_NODE;
                    } else if (name == "path") {
                        node = PATH_NODE;
                    } else if (name == "size") {
                        node = SIZE_NODE;
                    } else {
                        node = OTHER_NODE;
                    }
                } else if (type == XML_READER_TYPE_END_ELEMENT) {
                    node = OTHER_NODE;
                } else if (type == XML_READER_TYPE_TEXT) {
                    if (node == HASH_NODE) {
                        hash = reader.get_value();
                    } else if (node == PATH_NODE) {
                        auto path = reader.get_value();
                        auto file = Gio::File::create_for_path(path);
                        if (file->query_exists()) {
                            m_hash_table.insert(std::make_pair(hash, path));
                            m_filepath_table.insert(path);
                        }
                    } else if (node == SIZE_NODE) {
                        auto size = std::stoull(reader.get_value());
                        m_hash_table.reserve(size);
                        m_filepath_table.reserve(size);
                    }
                }
            }
            std::cerr << "Info: Hash Table contains " << m_hash_table.size() << " entries" << std::endl;
        } catch (std::exception e) {
            std::cerr << "Hasher Error: While reading XML on node "
                      << reader.get_name() << " value " << reader.get_value()
                      << ": " << e.what();
        }
        g_free(contents);
    }

    void Hasher::save_to_disk() const
    {
        std::lock_guard<std::mutex> lock(m_table_mutex);
        XmlWriter writer;
        writer.startDoc();
        writer.startElement("hashed_files");
        writer.writeElement("size", std::to_string(m_hash_table.size()));
        std::for_each(m_hash_table.cbegin(),
                      m_hash_table.cend(),
                      [&writer](const std::pair<std::string, std::string>& pair) {
                          writer.startElement("hashed_file");
                          writer.writeElement("hash", pair.first);
                          writer.writeElement("path", pair.second);
                          writer.endElement();
                      });
        writer.endDoc();

        auto file = get_local_file();
        try {
            std::string etag;
            file->replace_contents(writer.getString(), std::string(), etag, true, Gio::FILE_CREATE_PRIVATE);
        } catch (Gio::Error e) {
            std::cerr << "Hasher error: Unable to save hashed file cache to disk: "
                      << e.what() << std::endl;
        }
    }
}
#endif
