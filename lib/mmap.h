/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2018-2019 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#ifndef MMAP_H
#define MMAP_H

#include "fd.h"
#include "nocopy.h"

#include <practical/slice.h>

#include <sys/mman.h>

#include <type_traits>

enum class MapMode {
    ReadOnly, Writeable, CopyOnWrite
};
template <MapMode mode>
class Mmap : NoCopy {
    void *memory = MAP_FAILED;
    size_t length = 0;

public:
    Mmap(std::string path) : Mmap(path.c_str()) {
    }

    Mmap(const char *path) {
        static constexpr int OPEN_FLAGS = (mode==MapMode::Writeable ? O_RDWR : O_RDONLY);
        FD fd(path, OPEN_FLAGS);

        struct stat stat;
        if( fstat( fd.get(), &stat )==-1 )
            throw std::runtime_error("Mapping failed to get file attributes");

        length = stat.st_size;
        static constexpr int MMAP_PROTECTION = (mode==MapMode::ReadOnly ? PROT_READ : PROT_READ|PROT_WRITE);
        static constexpr int MMAP_FLAGS = (mode==MapMode::CopyOnWrite ? MAP_PRIVATE : MAP_SHARED);
        memory = mmap(nullptr, length, MMAP_PROTECTION, MMAP_FLAGS|MAP_FILE, fd.get(), 0);
        if( memory==MAP_FAILED )
            throw std::runtime_error("Mapping failed");
    }

    ~Mmap() {
        if( memory!=MAP_FAILED )
            munmap(memory, length);
    }

    template <typename T>
    typename std::enable_if<mode!=MapMode::ReadOnly, Slice<T> >::type getSlice() {
        return Slice<T>(static_cast<T*>(memory), length);
    }

    template <typename T>
    Slice<T> getSlice() const {
        return Slice<const T>(static_cast<const T*>(memory), length);
    }
};

#endif // MMAP_H
