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

    /** Performs MD5 checksum on directory trees, storing the
     * checksum and file path pair.
     *
     * Used to prevent downloading of duplicate images.
     */
    class Hasher {
    public:
        Hasher();
        ~Hasher();

        typedef std::function<void ()> HasherCallback;

        /** Hashes files in the directories given by Request. The
         * callback is given on the GMainLoop when all files have been
         * hashed.
         *
         * Threadsafe.
         */
        void hash_async(const Derp::Request& request, const HasherCallback& cb);

        /** Returns true if there is a file on disk with the given
         * checksum.
         *
         * Threadsafe.
         */
        bool has_md5(const std::string& md5sum) const;

        /** Saves the known checksums to disk.
         *
         * Blocking. Threadsafe.
         */
        void save_to_disk() const;

        /** Hash one file
         */
        void hash_one_async(Glib::RefPtr<Gio::File> file);

    private:
        Hasher& operator=(const Hasher&) = delete;
        Hasher(const Hasher&) = delete;

        /** Hashes files in the directories given by Request. The
         * callback is given on the GMainLoop when all files have been
         * hashed.
         */
        void hash(const Derp::Request& request, const HasherCallback& cb);
        void hash_directory(const Glib::RefPtr<Gio::File>& dir);
        void hash_file(const Glib::RefPtr<Gio::File>& file);

        /** Inserts the hash and path into the hash table and filepath
         * table.
         *
         * Threadsafe.
         */
        void insert_md5(const std::string& md5sum, const std::string& path);

        /** Returns true if the given path has been hashed.
         *
         * Threadsafe.
         */
        bool has_file_path(const std::string& path) const;

        /** Loads the known checksums from disk
         *
         * Blocking. Threadsafe.
         */
        void load_from_disk();

        CallbackDispatcher m_dispatcher;

        mutable std::mutex m_table_mutex;
        /* md5sum -> filepath map */
        std::unordered_multimap<std::string, std::string> m_hash_table;
        std::unordered_set<std::string> m_filepath_table;

        Active active;
    };
}

#endif
