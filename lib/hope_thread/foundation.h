/* Copyright (C) 2023 - 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

// useful macro section

#pragma once

/**
 * \brief Explicitly deletes move - assign operator and move constructor
 * \param Name Class name to be declared not move constructable
 */
#define HOPE_THREADING_NON_MOVABLE(Name) Name ( Name &&) = delete; \
    Name & operator=( Name &&) = delete

/**
 * \brief Explicitly deletes copy - assign operator and copy constructor
 * \param Name Class name to be declared not copy constructable
 */
#define HOPE_THREADING_NON_COPYABLE(Name) Name(const Name &) = delete; \
    Name & operator=(const Name &) = delete

/**
 * \brief Explicitly deletes copy/move - assign operator and copy/move constructor
 * \param Name Class name to be declared not copy constructable
 */
#define HOPE_THREADING_CONSTRUCTABLE_ONLY(Name) \
    HOPE_THREADING_NON_COPYABLE(Name); \
    HOPE_THREADING_NON_MOVABLE(Name); \

/**
 * \brief Explicitly declares default move - constructor and move - assign operator
 * \param Name Class name to be declared
 */
#define HOPE_THREADING_EXPLICIT_DEFAULT_MOVABLE(Name) Name ( Name &&) noexcept = default; \
    Name & operator=( Name &&) noexcept = default

/**
 * \brief Explicitly declares default copy - constructor and copy - assign operator
 * \param Name Class name to be declared
 */
#define HOPE_THREADING_EXPLICIT_DEFAULT_COPYABLE(Name) Name ( const Name &) noexcept = default; \
    Name & operator=( const Name &) noexcept = default

#define HOPE_THREADING_EXPLICIT_DEFAULT_CONSTRUCTABLE(Name) \
    Name() = default; \
    ~Name() = default;
