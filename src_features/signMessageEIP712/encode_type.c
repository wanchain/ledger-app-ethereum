#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "mem.h"
#include "eip712.h"
#include "encode_type.h"

/**
 * Format an unsigned number up to 32-bit into memory into an ASCII string.
 *
 * @param[in] value Value to write in memory
 * @param[in] max_chars Maximum number of characters that could be written
 *
 * @return how many characters have been written in memory, 0 in case of an allocation error
 */
static uint8_t format_uint_into_mem(uint32_t value, const uint8_t max_chars)
{
    char        *ptr;
    uint8_t     written_chars;

    if ((ptr = mem_alloc(sizeof(char) * max_chars)) == NULL)
    {
        return 0;
    }
    written_chars = sprintf(ptr, "%u", value);
    mem_dealloc(max_chars - written_chars); // in case it ended up being less
    return written_chars;
}

/**
 *
 *
 * @param[in] struct_ptr pointer to the structure we want the typestring of
 * @param[in] str_length length of the formatted string in memory
 * @return pointer of the string in memory, \ref NULL in case of an error
 */
static const char *get_struct_type_string(const uint8_t *const struct_ptr, uint16_t *const str_length)
{
    const char *str_start;
    const char *struct_name;
    uint8_t struct_name_length;
    const uint8_t *field_ptr;
    uint8_t fields_count;
    const char *name;
    uint8_t length;
    uint16_t field_size;
    uint8_t lvls_count;
    const uint8_t *lvl_ptr;
    uint8_t array_size;
    char *char_ptr;
    char *name_ptr;

    // add name
    struct_name = get_struct_name(struct_ptr, &struct_name_length);
    if ((name_ptr = mem_alloc(sizeof(char) * struct_name_length)) == NULL)
    {
        return NULL;
    }
    str_start = memmove(name_ptr, struct_name, struct_name_length);

    if ((char_ptr = mem_alloc(sizeof(char))) == NULL)
    {
        return NULL;
    }
    *char_ptr = '(';

    field_ptr = get_struct_fields_array(struct_ptr, &fields_count);
    for (uint8_t idx = 0; idx < fields_count; ++idx)
    {
        if (idx > 0)
        {
            if ((char_ptr = mem_alloc(sizeof(char))) == NULL)
            {
                return NULL;
            }
            *char_ptr = ',';
        }

        name = get_struct_field_typename(field_ptr, &length);

        if ((name_ptr = mem_alloc(sizeof(char) * length)) == NULL)
        {
            return NULL;
        }
        memmove(name_ptr, name, length);

        if (struct_field_has_typesize(field_ptr))
        {
            field_size = get_struct_field_typesize(field_ptr);
            switch (struct_field_type(field_ptr))
            {
                case TYPE_SOL_INT:
                case TYPE_SOL_UINT:
                    field_size *= 8; // bytes -> bits
                    break;
                case TYPE_SOL_BYTES_FIX:
                    break;
                default:
                    // should not be in here :^)
                    break;
            }
            // max value = 256, 3 characters max
            format_uint_into_mem(field_size, 3);
        }

        if (struct_field_is_array(field_ptr))
        {
            lvl_ptr = get_struct_field_array_lvls_array(field_ptr, &lvls_count);
            while (lvls_count-- > 0)
            {
                if ((char_ptr = mem_alloc(sizeof(char))) == NULL)
                {
                    return NULL;
                }
                *char_ptr = '[';
                switch (struct_field_array_depth(lvl_ptr, &array_size))
                {
                    case ARRAY_DYNAMIC:
                        break;
                    case ARRAY_FIXED_SIZE:
                        // max value = 255, 3 characters max
                        format_uint_into_mem(array_size, 3);
                        break;
                    default:
                        // should not be in here :^)
                        break;
                }
                if ((char_ptr = mem_alloc(sizeof(char))) == NULL)
                {
                    return NULL;
                }
                *char_ptr = ']';
                lvl_ptr = get_next_struct_field_array_lvl(lvl_ptr);
            }
        }
        if ((char_ptr = mem_alloc(sizeof(char))) == NULL)
        {
            return NULL;
        }
        *char_ptr = ' ';
        name = get_struct_field_keyname(field_ptr, &length);

        if ((name_ptr = mem_alloc(sizeof(char) * length)) == NULL)
        {
            return NULL;
        }
        memmove(name_ptr, name, length);

        field_ptr = get_next_struct_field(field_ptr);
    }
    if ((char_ptr = mem_alloc(sizeof(char))) == NULL)
    {
        return NULL;
    }
    *char_ptr = ')';

    *str_length = ((char*)mem_alloc(0) - str_start);
    return str_start;
}

/**
 *
 *
 * @param[in] structs_array pointer to structs array
 * @param[in] deps_count count of how many struct dependencies pointers
 * @param[in,out] dep pointer to the first dependency pointer
 */
static void sort_dependencies(const uint8_t *const deps_count,
                          void **dep)
{
    bool changed = false;
    void *tmp_ptr;
    const char *name1, *name2;
    uint8_t namelen1, namelen2;
    int str_cmp_result;

    for (size_t idx = 0; (idx + 1) < *deps_count; ++idx)
    {
        name1 = get_struct_name(*(dep + idx), &namelen1);
        name2 = get_struct_name(*(dep + idx + 1), &namelen2);

        str_cmp_result = strncmp(name1, name2, MIN(namelen1, namelen2));
        if ((str_cmp_result > 0) || ((str_cmp_result == 0) && (namelen1 > namelen2)))
        {
            tmp_ptr = *(dep + idx);
            *(dep + idx) = *(dep + idx + 1);
            *(dep + idx + 1) = tmp_ptr;

            changed = true;
        }
    }
    // recurse until it is sorted
    if (changed)
    {
        sort_dependencies(deps_count, dep);
    }
}

/**
 *
 *
 * @param[in] structs_array pointer to structs array
 * @param[out] deps_count count of how many struct dependencie pointers
 * @param[in] dep pointer to the first dependency pointer
 * @param[in] struct_ptr pointer to the struct we are getting the dependencies of
 * @return \ref false in case of a memory allocation error, \ref true otherwise
 */
static bool get_struct_dependencies(const void *const structs_array,
                                uint8_t *const deps_count,
                                void **dep,
                                const void *const struct_ptr)
{
    uint8_t fields_count;
    const void *field_ptr;
    const char *arg_structname;
    uint8_t arg_structname_length;
    const void *arg_struct_ptr;
    size_t dep_idx;
    const void **new_dep;

    field_ptr = get_struct_fields_array(struct_ptr, &fields_count);
    for (uint8_t idx = 0; idx < fields_count; ++idx)
    {
        if (struct_field_type(field_ptr) == TYPE_CUSTOM)
        {
            // get struct name
            arg_structname = get_struct_field_typename(field_ptr, &arg_structname_length);
            // from its name, get the pointer to its definition
            arg_struct_ptr = get_structn(structs_array, arg_structname, arg_structname_length);

            // check if it is not already present in the dependencies array
            for (dep_idx = 0; dep_idx < *deps_count; ++dep_idx)
            {
                // it's a match!
                if (*(dep + dep_idx) == arg_struct_ptr)
                {
                    break;
                }
            }
            // if it's not present in the array, add it and recurse into it
            if (dep_idx == *deps_count)
            {
                if ((new_dep = mem_alloc(sizeof(void*))) == NULL)
                {
                    return false;
                }
                *new_dep = arg_struct_ptr;
                *deps_count += 1;
                get_struct_dependencies(structs_array, deps_count, dep, arg_struct_ptr);
            }
        }
        field_ptr = get_next_struct_field(field_ptr);
    }
    return true;
}

/**
 *
 *
 * @param[in] structs_array pointer to structs array
 * @param[in] struct_name name of the given struct
 * @param[in] struct_name_length length of the name of the given struct
 * @param[out] encoded_length length of the returned string
 * @return pointer to encoded string or \ref NULL in case of a memory allocation error
 */
const char  *encode_type(const void *const structs_array,
                        const char *const struct_name,
                        const uint8_t struct_name_length,
                        uint16_t *const encoded_length)
{
    const void *const struct_ptr = get_structn(structs_array,
                                               struct_name,
                                               struct_name_length);
    uint8_t *deps_count;
    void **dep;
    uint16_t length;
    const char *typestr;

    *encoded_length = 0;
    if ((deps_count = mem_alloc(sizeof(uint8_t))) == NULL)
    {
        return NULL;//false;
    }
    *deps_count = 0;
    // get list of structs (own + dependencies), properly ordered
    dep = (void**)(deps_count + 1); // get first elem
    if (get_struct_dependencies(structs_array, deps_count, dep, struct_ptr) == false)
    {
        return NULL;//false;
    }
    sort_dependencies(deps_count, dep);
    typestr = get_struct_type_string(struct_ptr, &length);
    *encoded_length += length;
    // loop over each struct and generate string
    for (int idx = 0; idx < *deps_count; ++idx)
    {
        get_struct_type_string(*dep, &length);
        *encoded_length += length;
        dep += 1;
    }

    return typestr;//true;
}