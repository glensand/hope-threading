/* Copyright (C) 2026 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#pragma once

#include <cstddef>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace hope::threading::platform {

    /**
     * Mapped shared memory segment (named POSIX shm or Windows file mapping).
     * Call close_shared_memory when done in each process.
     */
    struct shared_memory_segment {
        void* data{ nullptr };
        std::size_t size{ 0 };
#if defined(_WIN32)
        HANDLE mapping{ nullptr };
#else
        int fd{ -1 };
#endif
        /** True if this process created the object (best-effort; see implementation). */
        bool created_new{ false };
    };

#if defined(_WIN32)

    namespace detail {
        inline void close_handles(shared_memory_segment& s) noexcept {
            if (s.data) {
                UnmapViewOfFile(s.data);
                s.data = nullptr;
            }
            if (s.mapping) {
                CloseHandle(s.mapping);
                s.mapping = nullptr;
            }
        }
    } // namespace detail

    /**
     * Creates a named file mapping or opens an existing one, then maps the whole segment.
     * \param name Object name (ASCII). Use a distinct prefix per application (e.g. "Local\\myapp_shm").
     * \param size_bytes Size when creating; if the mapping already exists, \p size_bytes must match its size.
     * \param out Filled on success; unchanged on failure.
     * \return true on success.
     */
    inline bool create_or_open_shared_memory(const char* name, std::size_t size_bytes, shared_memory_segment* out) noexcept {
        if (!out || !name || size_bytes == 0) {
            return false;
        }

        const DWORD size_high = static_cast<DWORD>(size_bytes >> 32);
        const DWORD size_low = static_cast<DWORD>(size_bytes & 0xffffffffu);

        SetLastError(0);
        HANDLE mapping = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            size_high,
            size_low,
            name);

        if (!mapping) {
            return false;
        }

        const DWORD last_err = GetLastError();
        const bool created_new = (last_err != ERROR_ALREADY_EXISTS);

        void* view = nullptr;
        std::size_t mapped_size = size_bytes;

        if (created_new) {
            view = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, size_bytes);
        } else {
            // Size arguments to CreateFileMapping are ignored when the object already exists; map the whole section.
            view = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
            if (view) {
                MEMORY_BASIC_INFORMATION mbi{};
                if (VirtualQuery(view, &mbi, sizeof(mbi)) == 0) {
                    UnmapViewOfFile(view);
                    view = nullptr;
                } else {
                    mapped_size = mbi.RegionSize;
                    if (mapped_size != size_bytes) {
                        UnmapViewOfFile(view);
                        view = nullptr;
                        SetLastError(ERROR_INVALID_PARAMETER);
                    }
                }
            }
        }

        if (!view) {
            CloseHandle(mapping);
            return false;
        }

        out->data = view;
        out->size = mapped_size;
        out->mapping = mapping;
        out->created_new = created_new;
        return true;
    }

    inline void close_shared_memory(shared_memory_segment& seg) noexcept {
        detail::close_handles(seg);
        seg.size = 0;
        seg.created_new = false;
    }

    /**
     * Windows has no POSIX-style unlink; the name is released when the last handle closes.
     * This function is a no-op that returns true so callers can share the same cleanup path as POSIX.
     */
    inline bool unlink_shared_memory(const char* /*name*/) noexcept {
        return true;
    }

#else

    namespace detail {
        inline bool map_fd(int fd, std::size_t map_size, shared_memory_segment* out) {
            void* p = ::mmap(nullptr, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (p == MAP_FAILED) {
                return false;
            }
            out->data = p;
            out->size = map_size;
            out->fd = fd;
            return true;
        }
    } // namespace detail

    /**
     * Creates a POSIX shared memory object or opens an existing one, sizes it if new, then mmap.
     * \param name Must start with '/' (e.g. "/myapp_segment") for portable shm_open.
     * \param size_bytes Size for a new segment; if the object already exists, the existing size is used
     *        and must match \p size_bytes (otherwise the call fails).
     * \param out Filled on success.
     */
    inline bool create_or_open_shared_memory(const char* name, std::size_t size_bytes, shared_memory_segment* out) noexcept {
        if (!out || !name || size_bytes == 0) {
            return false;
        }

        const int fd = ::shm_open(name, O_RDWR | O_CREAT, 0600);
        if (fd < 0) {
            return false;
        }

        struct stat st {};
        if (::fstat(fd, &st) != 0) {
            ::close(fd);
            return false;
        }

        bool created_new = false;
        if (st.st_size == 0) {
            if (::ftruncate(fd, static_cast<off_t>(size_bytes)) != 0) {
                ::close(fd);
                return false;
            }
            created_new = true;
        } else {
            if (static_cast<std::size_t>(st.st_size) != size_bytes) {
                ::close(fd);
                errno = EINVAL;
                return false;
            }
        }

        if (!detail::map_fd(fd, size_bytes, out)) {
            ::close(fd);
            return false;
        }

        out->created_new = created_new;
        return true;
    }

    inline void close_shared_memory(shared_memory_segment& seg) noexcept {
        if (seg.data && seg.size > 0) {
            ::munmap(seg.data, seg.size);
        }
        if (seg.fd >= 0) {
            ::close(seg.fd);
        }
        seg.data = nullptr;
        seg.size = 0;
        seg.fd = -1;
        seg.created_new = false;
    }

    inline bool unlink_shared_memory(const char* name) noexcept {
        if (!name) {
            return false;
        }
        return ::shm_unlink(name) == 0;
    }

#endif

} // namespace hope::threading::platform
