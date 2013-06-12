#ifndef HASHER_HXX
#define HASHER_HXX

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <mutex>
#include <giomm/file.h>
#include "active.hpp"
#include "callback_dispatcher.hpp"

namespace Derp {
    class Request;

    class Hasher {
    public:
        Hasher();
        ~Hasher();

        /** Hashes files in the directories given by Request. The
         * callback is given on the GMainLoop when all files have been
         * hashed.
         */
        void hash_async(const Derp::Request& request, const std::function<void ()>& cb);

        /** Returns true if there is a file on disk with the given
         * checksum.
         */ 
        bool has_md5(const std::string& md5sum) const;

        /** Saves the known checksums to disk.
         *
         * Blocking. 
         */
        void save_to_disk() const;

    private:
        static constexpr goffset MAXIMUM_FILESIZE = 5100 * 1000;
        Hasher& operator=(const Hasher&) = delete; // Evil func
        Hasher(const Hasher&) = delete; // Evil func

        CallbackDispatcher signal_hashed;

        /** Hashes files in the directories given by Request. The
         * callback is given on the GMainLoop when all files have been
         * hashed.
         */
        void hash(const Derp::Request& request, const std::function<void ()>& cb);
        void hash_directory(const Glib::RefPtr<Gio::File>& dir);
        void hash_file(const Glib::RefPtr<Gio::File>& file);


        /** Inserts the hash and path into the hash table and filepath
         * table.
         */
        void insert_md5(const std::string& md5sum, const std::string& path);

        /** Returns true if the given path has been hashed.
         */ 
        bool has_file_path(const std::string& path) const;

        /** Loads the known checksums from disk
         *
         * Blocking.
         */
        void load_from_disk();

        mutable std::mutex m_table_mutex;
        /* md5sum -> filepath map */
        std::unordered_map<std::string, std::string> m_hash_table;
        std::unordered_set<std::string> m_filepath_table;

        Active active;
    };
}

#endif
