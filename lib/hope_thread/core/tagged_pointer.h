/* Copyright (C) 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope_threading
 */

#pragma once

#include <new>

namespace hope::threading {

    template<typename TPtr = void*>
    struct alignas(8) tagged_ptr final {
        // x64 ptr has 48 significant bits for address relosving, and the rest of the bits we can use here

        static constexpr std::size_t TagBitsCount{ 26 };
        static constexpr std::size_t TagMask{ (1 << TagBitsCount) - 1 };

        TPtr get_ptr() const noexcept {
            return (Ptr >> TagBitsCount);
        }

        std::size_t get_tag() const noexcept {
            return Ptr & TagMask;
        }

        void set_ptr(TPtr in_ptr) noexcept {
            ptr = get_tag() | (in_ptr << TagBitsCount);
        }

        void set_tag(std::size_t tag) noexcept {
            ptr = tag | (get_ptr() << TagBitsCount);
        }

        void set_all(TPtr in_ptr, std::size_t tag) {
            ptr = (uint64_t(tag) | (in_ptr << TagBitsCount));
        }

        uint64_t ptr{ 0u };
    };
}