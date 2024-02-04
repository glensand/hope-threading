/* Copyright (C) 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope_threading
 */

#pragma once

#include "hope_thread/foundation.h"

namespace hope::threading {

    template<typename T>
    class linked_list final {
    public:

        HOPE_THREADING_CONSTRUCTABLE_ONLY(linked_list)

        void push_head(const T& val) {
            auto* new_node = new node{ val, nullptr };
            new_node->next = head;
            head = new_node;
        }

        T pop_head() {
            if (head == nullptr) {
                return {};
            }

            auto* node = head;
            head = head->next;
            auto val = std::move(node->value);
            delete node;
            return node;
        }

    private:

        struct node {
            T value;
            node* next;
        };

        node* head;
    };

}