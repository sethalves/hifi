//
//  FileCache.h
//  libraries/networking/src
//
//  Created by Zach Pomerantz on 2/21/2017.
//  Copyright 2017 High Fidelity, Inc.  // //  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FileCache_h
#define hifi_FileCache_h

#include <atomic>
#include <memory>
#include <cstddef>
#include <map>
#include <unordered_set>
#include <mutex>
#include <string>
#include <unordered_map>

#include <QObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(file_cache)

namespace cache {

class File;
using FilePointer = std::shared_ptr<File>;

class FileCache : public QObject {
    Q_OBJECT
    Q_PROPERTY(size_t numTotal READ getNumTotalFiles NOTIFY dirty)
    Q_PROPERTY(size_t numCached READ getNumCachedFiles NOTIFY dirty)
    Q_PROPERTY(size_t sizeTotal READ getSizeTotalFiles NOTIFY dirty)
    Q_PROPERTY(size_t sizeCached READ getSizeCachedFiles NOTIFY dirty)

    static const size_t DEFAULT_MAX_SIZE;
    static const size_t MAX_MAX_SIZE;
    static const size_t DEFAULT_MIN_FREE_STORAGE_SPACE;

public:
    // You can initialize the FileCache with a directory name (ex.: "temp_jpgs") that will be relative to the application local data, OR with a full path
    // The file cache will ignore any file that doesn't match the extension provided
    FileCache(const std::string& dirname, const std::string& ext, QObject* parent = nullptr);
    virtual ~FileCache();

    // Remove all unlocked items from the cache
    void wipe();

    size_t getNumTotalFiles() const { return _numTotalFiles; }
    size_t getNumCachedFiles() const { return _numUnusedFiles; }
    size_t getSizeTotalFiles() const { return _totalFilesSize; }
    size_t getSizeCachedFiles() const { return _unusedFilesSize; }

    // Set the maximum amount of disk space to use on disk
    void setMaxSize(size_t maxCacheSize);

    // Set the minumum amount of free disk space to retain.  This supercedes the max size,
    // so if the cache is consuming all but 500 MB of the drive, unused entries will be ejected 
    // to free up more space, regardless of the cache max size
    void setMinFreeSize(size_t size);

    using Key = std::string;
    struct Metadata {
        Metadata(const Key& key, size_t length) :
            key(key), length(length) {}
        Key key;
        size_t length;
    };

    // derived classes should implement a setter/getter, for example, for a FileCache backing a network cache:
    //
    // DerivedFilePointer writeFile(const char* data, DerivedMetadata&& metadata) {
    //  return writeFile(data, std::forward(metadata));
    // }
    //
    // DerivedFilePointer getFile(const QUrl& url) {
    //  auto key = lookup_hash_for(url); // assuming hashing url in create/evictedFile overrides
    //  return getFile(key);
    // }

signals:
    void dirty();

public:
    /// must be called after construction to create the cache on the fs and restore persisted files
    void initialize();

    // Add file to the cache and return the cache entry.  
    FilePointer writeFile(const char* data, Metadata&& metadata, bool overwrite = false);
    FilePointer getFile(const Key& key);

    /// create a file
    virtual std::unique_ptr<File> createFile(Metadata&& metadata, const std::string& filepath) = 0;

private:
    using Mutex = std::recursive_mutex;
    using Lock = std::unique_lock<Mutex>;
    using Map = std::unordered_map<Key, std::weak_ptr<File>>;
    using Set = std::unordered_set<FilePointer>;
    using KeySet = std::unordered_set<Key>;

    friend class File;

    std::string getFilepath(const Key& key);

    FilePointer addFile(Metadata&& metadata, const std::string& filepath);
    void addUnusedFile(const FilePointer& file);
    void removeUnusedFile(const FilePointer& file);
    void clean();
    void clear();
    // Remove a file from the cache
    void eject(const FilePointer& file);

    size_t getOverbudgetAmount() const;

    // FIXME it might be desirable to have the min free space variable be static so it can be
    // shared among multiple instances of FileCache
    std::atomic<size_t> _minFreeSpaceSize { DEFAULT_MIN_FREE_STORAGE_SPACE };
    std::atomic<size_t> _maxSize { DEFAULT_MAX_SIZE };
    std::atomic<size_t> _numTotalFiles { 0 };
    std::atomic<size_t> _numUnusedFiles { 0 };
    std::atomic<size_t> _totalFilesSize { 0 };
    std::atomic<size_t> _unusedFilesSize { 0 };

    std::string _ext;
    std::string _dirname;
    std::string _dirpath;
    bool _initialized { false };

    Map _files;
    Mutex _filesMutex;

    Set _unusedFiles;
    Mutex _unusedFilesMutex;
};

class File : public QObject {
    Q_OBJECT

public:
    using Key = FileCache::Key;
    using Metadata = FileCache::Metadata;

    const Key& getKey() const { return _key; }
    const size_t& getLength() const { return _length; }
    std::string getFilepath() const { return _filepath; }

    virtual ~File();
    /// overrides should call File::deleter to maintain caching behavior
    virtual void deleter();

protected:
    /// when constructed, the file has already been created/written
    File(Metadata&& metadata, const std::string& filepath);

private:
    friend class FileCache;
    friend struct FilePointerComparator;

    const Key _key;
    const size_t _length;
    const std::string _filepath;

    void touch();
    FileCache* _cache { nullptr };
    int64_t _modified { 0 };

    bool _shouldPersist { false };
};

}

#endif // hifi_FileCache_h
