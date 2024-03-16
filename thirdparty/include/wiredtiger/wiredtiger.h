/*-
 * Copyright (c) 2014-present MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *  All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#ifndef __WIREDTIGER_H_
#define __WIREDTIGER_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************
 * Version information
 *******************************************/
#define WIREDTIGER_VERSION_MAJOR    11
#define WIREDTIGER_VERSION_MINOR    3
#define WIREDTIGER_VERSION_PATCH    0
#define WIREDTIGER_VERSION_STRING   "WiredTiger 11.3.0: (November 16, 2023)"

/*******************************************
 * Required includes
 *******************************************/
#include <sys/types.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*******************************************
 * Portable type names
 *******************************************/
typedef off_t wt_off_t;



#if defined(DOXYGEN) || defined(SWIG)
#define __F(func) func
#else
/* NOLINTNEXTLINE(misc-macro-parentheses) */
#define __F(func) (*func)
#endif

/*
 * We support configuring WiredTiger with the gcc/clang -fvisibility=hidden
 * flags, but that requires public APIs be specifically marked.
 */
#if defined(DOXYGEN) || defined(SWIG) || !defined(__GNUC__)
#define WT_ATTRIBUTE_LIBRARY_VISIBLE
#else
#define WT_ATTRIBUTE_LIBRARY_VISIBLE    __attribute__((visibility("default")))
#endif

/*!
 * @defgroup wt WiredTiger API
 * The functions, handles and methods applications use to access and manage
 * data with WiredTiger.
 *
 * @{
 */

/*******************************************
 * Public forward structure declarations
 *******************************************/
struct __wt_collator;       typedef struct __wt_collator WT_COLLATOR;
struct __wt_compressor;     typedef struct __wt_compressor WT_COMPRESSOR;
struct __wt_config_item;    typedef struct __wt_config_item WT_CONFIG_ITEM;
struct __wt_config_parser;
    typedef struct __wt_config_parser WT_CONFIG_PARSER;
struct __wt_connection;     typedef struct __wt_connection WT_CONNECTION;
struct __wt_cursor;     typedef struct __wt_cursor WT_CURSOR;
struct __wt_data_source;    typedef struct __wt_data_source WT_DATA_SOURCE;
struct __wt_encryptor;      typedef struct __wt_encryptor WT_ENCRYPTOR;
struct __wt_event_handler;  typedef struct __wt_event_handler WT_EVENT_HANDLER;
struct __wt_extension_api;  typedef struct __wt_extension_api WT_EXTENSION_API;
struct __wt_extractor;      typedef struct __wt_extractor WT_EXTRACTOR;
struct __wt_file_handle;    typedef struct __wt_file_handle WT_FILE_HANDLE;
struct __wt_file_system;    typedef struct __wt_file_system WT_FILE_SYSTEM;
struct __wt_item;       typedef struct __wt_item WT_ITEM;
struct __wt_modify;     typedef struct __wt_modify WT_MODIFY;
struct __wt_session;        typedef struct __wt_session WT_SESSION;
#if !defined(DOXYGEN)
struct __wt_storage_source; typedef struct __wt_storage_source WT_STORAGE_SOURCE;
#endif

/*!
 * A raw item of data to be managed, including a pointer to the data and a
 * length.
 *
 * WT_ITEM structures do not need to be cleared before use.
 */
struct __wt_item {
    /*!
     * The memory reference of the data item.
     *
     * For items returned by a WT_CURSOR, the pointer is only valid until
     * the next operation on that cursor.  Applications that need to keep
     * an item across multiple cursor operations must make a copy.
     */
    const void *data;

    /*!
     * The number of bytes in the data item.
     *
     * The maximum length of a single column stored in a table is not fixed
     * (as it partially depends on the underlying file configuration), but
     * is always a small number of bytes less than 4GB.
     */
    size_t size;

#ifndef DOXYGEN
    /*! Managed memory chunk (internal use). */
    void *mem;

    /*! Managed memory size (internal use). */
    size_t memsize;

    /*! Object flags (internal use). */
/* AUTOMATIC FLAG VALUE GENERATION START 0 */
#define WT_ITEM_ALIGNED 0x1u
#define WT_ITEM_INUSE   0x2u
/* AUTOMATIC FLAG VALUE GENERATION STOP 32 */
    uint32_t flags;
#endif
};

/*!
 * A set of modifications for a value, including a pointer to new data and a
 * length, plus a target offset in the value and an optional length of data
 * in the value to be replaced.
 *
 * WT_MODIFY structures do not need to be cleared before use.
 */
struct __wt_modify {
    /*!
     * New data. The size of the new data may be zero when no new data is
     * provided.
     */
    WT_ITEM data;

    /*!
     * The zero-based byte offset in the value where the new data is placed.
     *
     * If the offset is past the end of the value, padding bytes are
     * appended to the value up to the specified offset. If the value is a
     * string (value format \c S), the padding byte is a space. If the value
     * is a raw byte array accessed using a WT_ITEM structure (value format
     * \c u), the padding byte is a nul.
     */
     size_t offset;

    /*!
     * The number of bytes in the value to be replaced.
     *
     * If the size is zero, no bytes from the value are replaced and the new
     * data is inserted.
     *
     * If the offset is past the end of the value, the size is ignored.
     *
     * If the offset plus the size overlaps the end of the previous value,
     * bytes from the offset to the end of the value are replaced and any
     * remaining new data is appended.
     */
     size_t size;
};

/*!
 * The maximum packed size of a 64-bit integer.  The ::wiredtiger_struct_pack
 * function will pack single long integers into at most this many bytes.
 */
#define WT_INTPACK64_MAXSIZE    ((int)sizeof(int64_t) + 1)

/*!
 * The maximum packed size of a 32-bit integer.  The ::wiredtiger_struct_pack
 * function will pack single integers into at most this many bytes.
 */
#define WT_INTPACK32_MAXSIZE    ((int)sizeof(int32_t) + 1)

/*!
 * A WT_CURSOR handle is the interface to a cursor.
 *
 * Cursors allow data to be searched, iterated and modified, implementing the
 * CRUD (create, read, update and delete) operations.  Cursors are opened in
 * the context of a session.  If a transaction is started, cursors operate in
 * the context of the transaction until the transaction is resolved.
 *
 * Raw data is represented by key/value pairs of WT_ITEM structures, but
 * cursors can also provide access to fields within the key and value if the
 * formats are described in the WT_SESSION::create method.
 *
 * In the common case, a cursor is used to access records in a table.  However,
 * cursors can be used on subsets of tables (such as a single column or a
 * projection of multiple columns), as an interface to statistics, configuration
 * data or application-specific data sources.  See WT_SESSION::open_cursor for
 * more information.
 *
 * <b>Thread safety:</b> A WT_CURSOR handle is not usually shared between
 * threads. See @ref threads for more information.
 */
struct __wt_cursor {
    WT_SESSION *session;    /*!< The session handle for this cursor. */

    /*!
     * The name of the data source for the cursor, matches the \c uri
     * parameter to WT_SESSION::open_cursor used to open the cursor.
     */
    const char *uri;

    /*!
     * The format of the data packed into key items.  See @ref packing for
     * details.  If not set, a default value of "u" is assumed, and
     * applications must use WT_ITEM structures to manipulate untyped byte
     * arrays.
     */
    const char *key_format;

    /*!
     * The format of the data packed into value items.  See @ref packing
     * for details.  If not set, a default value of "u" is assumed, and
     * applications must use WT_ITEM structures to manipulate untyped byte
     * arrays.
     */
    const char *value_format;

    /*!
     * @name Data access
     * @{
     */
    /*!
     * Get the key for the current record.
     *
     * @snippet ex_all.c Get the cursor's string key
     *
     * @snippet ex_all.c Get the cursor's record number key
     *
     * @param cursor the cursor handle
     * @param ... pointers to hold key fields corresponding to
     * WT_CURSOR::key_format.
     * The API does not validate the argument types passed in; the caller is
     * responsible for passing the correct argument types according to
     * WT_CURSOR::key_format.
     * @errors
     */
    int __F(get_key)(WT_CURSOR *cursor, ...);

    /*!
     * Get the value for the current record.
     *
     * @snippet ex_all.c Get the cursor's string value
     *
     * @snippet ex_all.c Get the cursor's raw value
     *
     * @param cursor the cursor handle
     * @param ... pointers to hold value fields corresponding to
     * WT_CURSOR::value_format.
     * The API does not validate the argument types passed in; the caller is
     * responsible for passing the correct argument types according to
     * WT_CURSOR::value_format.
     * @errors
     */
    int __F(get_value)(WT_CURSOR *cursor, ...);

    /*!
     * Get the raw key and value for the current record.
     *
     * @snippet ex_all.c Get the raw key and value for the current record.
     *
     * @snippet ex_all.c Set the cursor's record number key
     *
     * @param cursor the cursor handle
     * @param key pointer to an item that will contains the current record's raw key
     * @param value pointer to an item that will contains the current record's raw value
     *
     * The caller can optionally pass in NULL for either key or value to retrieve only
     * the other of the key or value.
     *
     * If an error occurs during this operation, a flag will be set in the
     * cursor, and the next operation to access the key will fail.  This
     * simplifies error handling in applications.
     * @errors
     */
        int __F(get_raw_key_value)(WT_CURSOR *cursor, WT_ITEM* key, WT_ITEM* value);

    /*!
     * Set the key for the next operation.
     *
     * @snippet ex_all.c Set the cursor's string key
     *
     * @snippet ex_all.c Set the cursor's record number key
     *
     * @param cursor the cursor handle
     * @param ... key fields corresponding to WT_CURSOR::key_format.
     *
     * If an error occurs during this operation, a flag will be set in the
     * cursor, and the next operation to access the key will fail.  This
     * simplifies error handling in applications.
     */
    void __F(set_key)(WT_CURSOR *cursor, ...);

    /*!
     * Set the value for the next operation.
     *
     * @snippet ex_all.c Set the cursor's string value
     *
     * @snippet ex_all.c Set the cursor's raw value
     *
     * @param cursor the cursor handle
     * @param ... value fields corresponding to WT_CURSOR::value_format.
     *
     * If an error occurs during this operation, a flag will be set in the
     * cursor, and the next operation to access the value will fail.  This
     * simplifies error handling in applications.
     */
    void __F(set_value)(WT_CURSOR *cursor, ...);
    /*! @} */

    /*!
     * @name Cursor positioning
     * @{
     */
    /*!
     * Return the ordering relationship between two cursors: both cursors
     * must have the same data source and have valid keys. (When testing
     * only for equality, WT_CURSOR::equals may be faster.)
     *
     * @snippet ex_all.c Cursor comparison
     *
     * @param cursor the cursor handle
     * @param other another cursor handle
     * @param comparep the status of the comparison: < 0 if
     * <code>cursor</code> refers to a key that appears before
     * <code>other</code>, 0 if the cursors refer to the same key,
     * and > 0 if <code>cursor</code> refers to a key that appears after
     * <code>other</code>.
     * @errors
     */
    int __F(compare)(WT_CURSOR *cursor, WT_CURSOR *other, int *comparep);

    /*!
     * Return the ordering relationship between two cursors, testing only
     * for equality: both cursors must have the same data source and have
     * valid keys.
     *
     * @snippet ex_all.c Cursor equality
     *
     * @param cursor the cursor handle
     * @param other another cursor handle
     * @param[out] equalp the status of the comparison: 1 if the cursors
     * refer to the same key, otherwise 0.
     * @errors
     */
    int __F(equals)(WT_CURSOR *cursor, WT_CURSOR *other, int *equalp);

    /*!
     * Return the next record.
     *
     * @snippet ex_all.c Return the next record
     *
     * @param cursor the cursor handle
     * @errors
     */
    int __F(next)(WT_CURSOR *cursor);

    /*!
     * Return the previous record.
     *
     * @snippet ex_all.c Return the previous record
     *
     * @param cursor the cursor handle
     * @errors
     */
    int __F(prev)(WT_CURSOR *cursor);

    /*!
     * Reset the cursor. Any resources held by the cursor are released,
     * and the cursor's key and position are no longer valid. Subsequent
     * iterations with WT_CURSOR::next will move to the first record, or
     * with WT_CURSOR::prev will move to the last record.
     *
     * In the case of a statistics cursor, resetting the cursor refreshes
     * the statistics information returned. Resetting a session statistics
     * cursor resets all the session statistics values to zero.
     *
     * @snippet ex_all.c Reset the cursor
     *
     * @param cursor the cursor handle
     * @errors
     */
    int __F(reset)(WT_CURSOR *cursor);

    /*!
     * Return the record matching the key. The key must first be set.
     *
     * @snippet ex_all.c Search for an exact match
     *
     * On success, the cursor ends positioned at the returned record; to
     * minimize cursor resources, the WT_CURSOR::reset method should be
     * called as soon as the record has been retrieved and the cursor no
     * longer needs that position.
     *
     * @param cursor the cursor handle
     * @errors
     */
    int __F(search)(WT_CURSOR *cursor);

    /*!
     * Return the record matching the key if it exists, or an adjacent
     * record.  An adjacent record is either the smallest record larger
     * than the key or the largest record smaller than the key (in other
     * words, a logically adjacent key).
     *
     * The key must first be set.
     *
     * An example of a search for an exact or adjacent match:
     *
     * @snippet ex_all.c Search for an exact or adjacent match
     *
     * An example of a forward scan through the table, where all keys
     * greater than or equal to a specified prefix are included in the
     * scan:
     *
     * @snippet ex_all.c Forward scan greater than or equal
     *
     * An example of a backward scan through the table, where all keys
     * less than a specified prefix are included in the scan:
     *
     * @snippet ex_all.c Backward scan less than
     *
     * On success, the cursor ends positioned at the returned record; to
     * minimize cursor resources, the WT_CURSOR::reset method should be
     * called as soon as the record has been retrieved and the cursor no
     * longer needs that position.
     *
     * @param cursor the cursor handle
     * @param exactp the status of the search: 0 if an exact match is
     * found, < 0 if a smaller key is returned, > 0 if a larger key is
     * returned
     * @errors
     */
    int __F(search_near)(WT_CURSOR *cursor, int *exactp);
    /*! @} */

    /*!
     * @name Data modification
     * @{
     */
    /*!
     * Insert a record and optionally update an existing record.
     *
     * If the cursor was configured with "overwrite=true" (the default),
     * both the key and value must be set; if the record already exists,
     * the key's value will be updated, otherwise, the record will be
     * inserted.
     *
     * @snippet ex_all.c Insert a new record or overwrite an existing record
     *
     * If the cursor was not configured with "overwrite=true", both the key
     * and value must be set and the record must not already exist; the
     * record will be inserted. If the record already exists, the
     * ::WT_DUPLICATE_KEY error is returned and the value found in the tree
     * can be retrieved using WT_CURSOR::get_value.
     *
     * @snippet ex_all.c Insert a new record and fail if the record exists
     *
     * If a cursor with record number keys was configured with
     * "append=true" (not the default), the value must be set; a new record
     * will be appended and the new record number can be retrieved using
     * WT_CURSOR::get_key.
     *
     * @snippet ex_all.c Insert a new record and assign a record number
     *
     * The cursor ends with no position, and a subsequent call to the
     * WT_CURSOR::next (WT_CURSOR::prev) method will iterate from the
     * beginning (end) of the table.
     *
     * If the cursor does not have record number keys or was not configured
     * with "append=true", the cursor ends with no key set and a subsequent
     * call to the WT_CURSOR::get_key method will fail. The cursor ends with
     * no value set and a subsequent call to the WT_CURSOR::get_value method
     * will fail, except for the ::WT_DUPLICATE_KEY error return, in which
     * case the value currently stored for the key can be retrieved.
     *
     * Inserting a new record after the current maximum record in a
     * fixed-length bit field column-store (that is, a store with an
     * 'r' type key and 't' type value) will implicitly create the missing
     * records as records with a value of 0.
     *
     * When loading a large amount of data into a new object, using
     * a cursor with the \c bulk configuration string enabled and
     * loading the data in sorted order will be much faster than doing
     * out-of-order inserts.  See @ref tune_bulk_load for more information.
     *
     * The maximum length of a single column stored in a table is not fixed
     * (as it partially depends on the underlying file configuration), but
     * is always a small number of bytes less than 4GB.
     *
     * The WT_CURSOR::insert method can only be used at snapshot isolation.
     *
     * @param cursor the cursor handle
     * @errors
     * In particular, if \c overwrite=false is configured and a record with
     * the specified key already exists, ::WT_DUPLICATE_KEY is returned.
     * Also, if \c in_memory is configured for the database and the insert
     * requires more than the configured cache size to complete,
     * ::WT_CACHE_FULL is returned.
     */
    int __F(insert)(WT_CURSOR *cursor);

    /*!
     * Modify an existing record. Both the key and value must be set and the record must
     * already exist.
     *
     * Modifications are specified in WT_MODIFY structures. Modifications
     * are applied in order and later modifications can update earlier ones.
     *
     * The modify method is only supported on strings (value format type
     * \c S), or raw byte arrays accessed using a WT_ITEM structure (value
     * format type \c u).
     *
     * The WT_CURSOR::modify method stores a change record in cache and writes a change record
     * to the log instead of the usual complete values. Using WT_CURSOR::modify will result in
     * slower reads, and slower writes than the WT_CURSOR::insert or WT_CURSOR::update methods,
     * because of the need to assemble the complete value in both the read and write paths. The
     * WT_CURSOR::modify method is intended for applications where memory and log amplification
     * are issues (in other words, applications where there is cache or I/O pressure and the
     * application wants to trade performance for a smaller working set in cache and smaller
     * log records).
     *
     * @snippet ex_all.c Modify an existing record
     *
     * On success, the cursor ends positioned at the modified record; to
     * minimize cursor resources, the WT_CURSOR::reset method should be
     * called as soon as the cursor no longer needs that position.
     *
     * The maximum length of a single column stored in a table is not fixed
     * (as it partially depends on the underlying file configuration), but
     * is always a small number of bytes less than 4GB.
     *
     * The WT_CURSOR::modify method can only be used at snapshot isolation.
     *
     * @param cursor the cursor handle
     * @param entries an array of modification data structures
     * @param nentries the number of modification data structures
     * @errors
     * In particular, if \c in_memory is configured for the database and
     * the modify requires more than the configured cache size to complete,
     * ::WT_CACHE_FULL is returned.
     */
    int __F(modify)(WT_CURSOR *cursor, WT_MODIFY *entries, int nentries);

    /*!
     * Update an existing record and optionally insert a record.
     *
     * If the cursor was configured with "overwrite=true" (the default),
     * both the key and value must be set; if the record already exists, the
     * key's value will be updated, otherwise, the record will be inserted.
     *
     * @snippet ex_all.c Update an existing record or insert a new record
     *
     * If the cursor was not configured with "overwrite=true", both the key
     * and value must be set and the record must already exist; the
     * record will be updated.
     *
     * @snippet ex_all.c Update an existing record and fail if DNE
     *
     * On success, the cursor ends positioned at the modified record; to
     * minimize cursor resources, the WT_CURSOR::reset method should be
     * called as soon as the cursor no longer needs that position. (The
     * WT_CURSOR::insert method never keeps a cursor position and may be
     * more efficient for that reason.)
     *
     * The maximum length of a single column stored in a table is not fixed
     * (as it partially depends on the underlying file configuration), but
     * is always a small number of bytes less than 4GB.
     *
     * The WT_CURSOR::update method can only be used at snapshot isolation.
     *
     * @param cursor the cursor handle
     * @errors
     * In particular, if \c overwrite=false is configured and no record with
     * the specified key exists, ::WT_NOTFOUND is returned.
     * Also, if \c in_memory is configured for the database and the update
     * requires more than the configured cache size to complete,
     * ::WT_CACHE_FULL is returned.
     */
    int __F(update)(WT_CURSOR *cursor);

    /*!
     * Remove a record.
     *
     * The key must be set; the key's record will be removed if it exists.
     *
     * @snippet ex_all.c Remove a record
     *
     * Any cursor position does not change: if the cursor was positioned
     * before the WT_CURSOR::remove call, the cursor remains positioned
     * at the removed record; to minimize cursor resources, the
     * WT_CURSOR::reset method should be called as soon as the cursor no
     * longer needs that position. If the cursor was not positioned before
     * the WT_CURSOR::remove call, the cursor ends with no position, and a
     * subsequent call to the WT_CURSOR::next (WT_CURSOR::prev) method will
     * iterate from the beginning (end) of the table.
     *
     * @snippet ex_all.c Remove a record and fail if DNE
     *
     * Removing a record in a fixed-length bit field column-store
     * (that is, a store with an 'r' type key and 't' type value) is
     * identical to setting the record's value to 0.
     *
     * The WT_CURSOR::remove method can only be used at snapshot isolation.
     *
     * @param cursor the cursor handle
     * @errors
     */
    int __F(remove)(WT_CURSOR *cursor);

    /*!
     * Reserve an existing record so a subsequent write is less likely to
     * fail due to a conflict between concurrent operations.
     *
     * The key must first be set and the record must already exist.
     *
     * Note that reserve works by doing a special update operation that is
     * not logged and does not change the value of the record. This update
     * is aborted when the enclosing transaction ends regardless of whether
     * it commits or rolls back. Given that, reserve can only be used to
     * detect conflicts between transactions that execute concurrently. It
     * cannot detect all logical conflicts between transactions. For that,
     * some update to the record must be committed.
     *
     * @snippet ex_all.c Reserve a record
     *
     * On success, the cursor ends positioned at the specified record; to
     * minimize cursor resources, the WT_CURSOR::reset method should be
     * called as soon as the cursor no longer needs that position.
     *
     * @param cursor the cursor handle
     * @errors
     */
    int __F(reserve)(WT_CURSOR *cursor);
    /*! @} */

#ifndef DOXYGEN
    /*!
     * If the cursor is opened on a checkpoint, return a unique identifier for the checkpoint;
     * otherwise return 0.
     *
     * This allows applications to confirm that checkpoint cursors opened on default checkpoints
     * in different objects reference the same database checkpoint.
     *
     * @param cursor the cursor handle
     * @errors
     */
    uint64_t __F(checkpoint_id)(WT_CURSOR *cursor);
#endif

    /*!
     * Close the cursor.
     *
     * This releases the resources associated with the cursor handle.
     * Cursors are closed implicitly by ending the enclosing connection or
     * closing the session in which they were opened.
     *
     * @snippet ex_all.c Close the cursor
     *
     * @param cursor the cursor handle
     * @errors
     */
    int __F(close)(WT_CURSOR *cursor);

    /*!
     * Get the table's largest key, ignoring visibility. This method is only supported by
     * file: or table: objects. The cursor ends with no position.
     *
     * @snippet ex_all.c Get the table's largest key
     *
     * @param cursor the cursor handle
     * @errors
     */
    int __F(largest_key)(WT_CURSOR *cursor);

    /*!
     * Reconfigure the cursor.
     *
     * The cursor is reset.
     *
     * @snippet ex_all.c Reconfigure a cursor
     *
     * @param cursor the cursor handle
     * @configstart{WT_CURSOR.reconfigure, see dist/api_data.py}
     * @config{append, append written values as new records\, giving each a new record number key;
     * valid only for cursors with record number keys., a boolean flag; default \c false.}
     * @config{overwrite, configures whether the cursor's insert and update methods check the
     * existing state of the record.  If \c overwrite is \c false\, WT_CURSOR::insert fails with
     * ::WT_DUPLICATE_KEY if the record exists\, and WT_CURSOR::update fails with ::WT_NOTFOUND if
     * the record does not exist., a boolean flag; default \c true.}
     * @configend
     * @errors
     */
    int __F(reconfigure)(WT_CURSOR *cursor, const char *config);

    /*!
     * Set range bounds on the cursor.
     *
     * @param cursor the cursor handle
     * @configstart{WT_CURSOR.bound, see dist/api_data.py}
     * @config{action, configures whether this call into the API will set or clear range bounds on
     * the given cursor.  It takes one of two values\, "set" or "clear". If "set" is specified then
     * "bound" must also be specified.  The keys relevant to the given bound must have been set
     * prior to the call using WT_CURSOR::set_key., a string\, chosen from the following options: \c
     * "clear"\, \c "set"; default \c set.}
     * @config{bound, configures which bound is being operated on.  It takes one of two values\,
     * "lower" or "upper"., a string\, chosen from the following options: \c "lower"\, \c "upper";
     * default empty.}
     * @config{inclusive, configures whether the given bound is inclusive or not., a boolean flag;
     * default \c true.}
     * @configend
     * @errors
     */
    int __F(bound)(WT_CURSOR *cursor, const char *config);

    /*
     * Protected fields, only to be used by cursor implementations.
     */
#if !defined(SWIG) && !defined(DOXYGEN)
    int __F(cache)(WT_CURSOR *cursor);  /* Cache the cursor */
                        /* Reopen a cached cursor */
    int __F(reopen)(WT_CURSOR *cursor, bool check_only);

    uint64_t uri_hash;          /* Hash of URI */

    /*
     * !!!
     * Explicit representations of structures from queue.h.
     * TAILQ_ENTRY(wt_cursor) q;
     */
    struct {
        WT_CURSOR *tqe_next;
        WT_CURSOR **tqe_prev;
    } q;                /* Linked list of WT_CURSORs. */

    uint64_t recno;         /* Record number, normal and raw mode */
    uint8_t raw_recno_buf[WT_INTPACK64_MAXSIZE];

    void    *json_private;      /* JSON specific storage */
    void    *lang_private;      /* Language specific private storage */

    WT_ITEM key, value;
    int saved_err;          /* Saved error in set_{key,value}. */
    /*
     * URI used internally, may differ from the URI provided by the
     * user on open.
     */
    const char *internal_uri;

    /*
     * Lower bound and upper bound buffers that is used for the bound API. Store the key set for
     * either the lower bound and upper bound such that cursor operations can limit the returned key
     * to be within the bounded ranges.
     */
    WT_ITEM lower_bound, upper_bound;

/* AUTOMATIC FLAG VALUE GENERATION START 0 */
#define WT_CURSTD_APPEND        0x000000001ull
#define WT_CURSTD_BOUND_LOWER    0x000000002ull       /* Lower bound. */
#define WT_CURSTD_BOUND_LOWER_INCLUSIVE 0x000000004ull /* Inclusive lower bound. */
#define WT_CURSTD_BOUND_UPPER           0x000000008ull /* Upper bound. */
#define WT_CURSTD_BOUND_UPPER_INCLUSIVE 0x000000010ull /* Inclusive upper bound. */
#define WT_CURSTD_BULK          0x000000020ull
#define WT_CURSTD_CACHEABLE     0x000000040ull
#define WT_CURSTD_CACHED        0x000000080ull
#define WT_CURSTD_DEAD          0x000000100ull
#define WT_CURSTD_DEBUG_COPY_KEY    0x000000200ull
#define WT_CURSTD_DEBUG_COPY_VALUE  0x000000400ull
#define WT_CURSTD_DEBUG_RESET_EVICT 0x000000800ull
#define WT_CURSTD_DUMP_HEX      0x000001000ull
#define WT_CURSTD_DUMP_JSON     0x000002000ull
#define WT_CURSTD_DUMP_PRETTY       0x000004000ull
#define WT_CURSTD_DUMP_PRINT        0x000008000ull
#define WT_CURSTD_DUP_NO_VALUE          0x000010000ull
#define WT_CURSTD_EVICT_REPOSITION     0x000020000ull
#define WT_CURSTD_HS_READ_ACROSS_BTREE 0x000040000ull
#define WT_CURSTD_HS_READ_ALL       0x000080000ull
#define WT_CURSTD_HS_READ_COMMITTED 0x000100000ull
#define WT_CURSTD_IGNORE_TOMBSTONE  0x000200000ull
#define WT_CURSTD_JOINED        0x000400000ull
#define WT_CURSTD_KEY_EXT       0x000800000ull /* Key points out of tree. */
#define WT_CURSTD_KEY_INT       0x001000000ull /* Key points into tree. */
#define WT_CURSTD_KEY_ONLY      0x002000000ull
#define WT_CURSTD_META_INUSE        0x004000000ull
#define WT_CURSTD_OPEN          0x008000000ull
#define WT_CURSTD_OVERWRITE     0x010000000ull
#define WT_CURSTD_RAW           0x020000000ull
#define WT_CURSTD_RAW_SEARCH        0x040000000ull
#define WT_CURSTD_VALUE_EXT     0x080000000ull /* Value points out of tree. */
#define WT_CURSTD_VALUE_INT     0x100000000ull /* Value points into tree. */
#define WT_CURSTD_VERSION_CURSOR    0x200000000ull /* Version cursor. */
/* AUTOMATIC FLAG VALUE GENERATION STOP 64 */
#define WT_CURSTD_KEY_SET   (WT_CURSTD_KEY_EXT | WT_CURSTD_KEY_INT)
#define WT_CURSTD_VALUE_SET (WT_CURSTD_VALUE_EXT | WT_CURSTD_VALUE_INT)
#define WT_CURSTD_BOUND_ALL (WT_CURSTD_BOUND_UPPER | WT_CURSTD_BOUND_UPPER_INCLUSIVE \
| WT_CURSTD_BOUND_LOWER | WT_CURSTD_BOUND_LOWER_INCLUSIVE)
    uint64_t flags;
#endif
};

/*! WT_SESSION::timestamp_transaction_uint timestamp types */
typedef enum {
    WT_TS_TXN_TYPE_COMMIT, /*!< Commit timestamp. */
    WT_TS_TXN_TYPE_DURABLE, /*!< Durable timestamp. */
    WT_TS_TXN_TYPE_PREPARE, /*!< Prepare timestamp. */
    WT_TS_TXN_TYPE_READ /*!< Read timestamp. */
} WT_TS_TXN_TYPE;

/*!
 * All data operations are performed in the context of a WT_SESSION.  This
 * encapsulates the thread and transactional context of the operation.
 *
 * <b>Thread safety:</b> A WT_SESSION handle is not usually shared between
 * threads, see @ref threads for more information.
 */
struct __wt_session {
    /*! The connection for this session. */
    WT_CONNECTION *connection;

    /*
     * Don't expose app_private to non-C language bindings - they have
     * their own way to attach data to an operation.
     */
#if !defined(SWIG)
    /*!
     * A location for applications to store information that will be
     * available in callbacks taking a WT_SESSION handle.
     */
    void *app_private;
#endif

    /*!
     * Close the session handle.
     *
     * This will release the resources associated with the session handle,
     * including rolling back any active transactions and closing any
     * cursors that remain open in the session.
     *
     * @snippet ex_all.c Close a session
     *
     * @param session the session handle
     * @configempty{WT_SESSION.close, see dist/api_data.py}
     * @errors
     */
    int __F(close)(WT_SESSION *session, const char *config);

    /*!
     * Reconfigure a session handle.
     *
     * Only configurations listed in the method arguments are modified, other configurations
     * remain in their current state. This method additionally resets the cursors associated
     * with the session. WT_SESSION::reconfigure will fail if a transaction is in progress in
     * the session.
     *
     * @snippet ex_all.c Reconfigure a session
     *
     * @param session the session handle
     * @configstart{WT_SESSION.reconfigure, see dist/api_data.py}
     * @config{cache_cursors, enable caching of cursors for reuse.  Any calls to WT_CURSOR::close
     * for a cursor created in this session will mark the cursor as cached and keep it available to
     * be reused for later calls to WT_SESSION::open_cursor.  Cached cursors may be eventually
     * closed.  This value is inherited from ::wiredtiger_open \c cache_cursors., a boolean flag;
     * default \c true.}
     * @config{cache_max_wait_ms, the maximum number of milliseconds an application thread will wait
     * for space to be available in cache before giving up.  Default value will be the global
     * setting of the connection config., an integer greater than or equal to \c 0; default \c 0.}
     * @config{debug = (, configure debug specific behavior on a session.  Generally only used for
     * internal testing purposes., a set of related configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;checkpoint_fail_before_turtle_update, Fail before writing a
     * turtle file at the end of a checkpoint., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;release_evict_page, Configure the session to evict the page
     * when it is released and no longer needed., a boolean flag; default \c false.}
     * @config{ ),,}
     * @config{ignore_cache_size, when set\, operations performed by this session ignore the cache
     * size and are not blocked when the cache is full.  Note that use of this option for operations
     * that create cache pressure can starve ordinary sessions that obey the cache size., a boolean
     * flag; default \c false.}
     * @config{isolation, the default isolation level for operations in this session., a string\,
     * chosen from the following options: \c "read-uncommitted"\, \c "read-committed"\, \c
     * "snapshot"; default \c snapshot.}
     * @config{prefetch = (, Enable automatic detection of scans by applications\, and attempt to
     * pre-fetch future content into the cache., a set of related configuration options defined as
     * follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, whether pre-fetch is enabled for this
     * session., a boolean flag; default \c false.}
     * @config{ ),,}
     * @configend
     * @errors
     */
    int __F(reconfigure)(WT_SESSION *session, const char *config);

    /*!
     * Return information about an error as a string.
     *
     * @snippet ex_all.c Display an error thread safe
     *
     * @param session the session handle
     * @param error a return value from a WiredTiger, ISO C, or POSIX
     * standard API call
     * @returns a string representation of the error
     */
    const char *__F(strerror)(WT_SESSION *session, int error);

    /*!
     * @name Cursor handles
     * @{
     */

    /*!
     * Open a new cursor on a data source or duplicate an existing cursor.
     *
     * @snippet ex_all.c Open a cursor
     *
     * An existing cursor can be duplicated by passing it as the \c to_dup
     * parameter and setting the \c uri parameter to \c NULL:
     *
     * @snippet ex_all.c Duplicate a cursor
     *
     * Cursors being duplicated must have a key set, and successfully
     * duplicated cursors are positioned at the same place in the data
     * source as the original.
     *
     * Cursor handles should be discarded by calling WT_CURSOR::close.
     *
     * Cursors capable of supporting transactional operations operate in the
     * context of the current transaction, if any.
     *
     * WT_SESSION::rollback_transaction implicitly resets all cursors associated with the
         * session.
     *
     * Cursors are relatively light-weight objects but may hold references
     * to heavier-weight objects; applications should re-use cursors when
     * possible, but instantiating new cursors is not so expensive that
     * applications need to cache cursors at all cost.
     *
     * @param session the session handle
     * @param uri the data source on which the cursor operates; cursors
     *  are usually opened on tables, however, cursors can be opened on
     *  any data source, regardless of whether it is ultimately stored
     *  in a table.  Some cursor types may have limited functionality
     *  (for example, they may be read-only or not support transactional
     *  updates).  See @ref data_sources for more information.
     *  <br>
     *  @copydoc doc_cursor_types
     * @param to_dup a cursor to duplicate or gather statistics on
     * @configstart{WT_SESSION.open_cursor, see dist/api_data.py}
     * @config{append, append written values as new records\, giving each a new record number key;
     * valid only for cursors with record number keys., a boolean flag; default \c false.}
     * @config{bulk, configure the cursor for bulk-loading\, a fast\, initial load path (see @ref
     * tune_bulk_load for more information). Bulk-load may only be used for newly created objects
     * and applications should use the WT_CURSOR::insert method to insert rows.  When bulk-loading\,
     * rows must be loaded in sorted order.  The value is usually a true/false flag; when
     * bulk-loading fixed-length column store objects\, the special value \c bitmap allows chunks of
     * a memory resident bitmap to be loaded directly into a file by passing a \c WT_ITEM to
     * WT_CURSOR::set_value where the \c size field indicates the number of records in the bitmap
     * (as specified by the object's \c value_format configuration). Bulk-loaded bitmap values must
     * end on a byte boundary relative to the bit count (except for the last set of values loaded).,
     * a string; default \c false.}
     * @config{checkpoint, the name of a checkpoint to open.  (The reserved name
     * "WiredTigerCheckpoint" opens the most recent checkpoint taken for the object.) The cursor
     * does not support data modification., a string; default empty.}
     * @config{debug = (, configure debug specific behavior on a cursor.  Generally only used for
     * internal testing purposes., a set of related configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;dump_version, open a version cursor\, which is a debug cursor
     * on a table that enables iteration through the history of values for a given key., a boolean
     * flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;release_evict, Configure the cursor
     * to evict the page positioned on when the reset API call is used., a boolean flag; default \c
     * false.}
     * @config{ ),,}
     * @config{dump, configure the cursor for dump format inputs and outputs: "hex" selects a simple
     * hexadecimal format\, "json" selects a JSON format with each record formatted as fields named
     * by column names if available\, "pretty" selects a human-readable format (making it
     * incompatible with the "load")\, "pretty_hex" is similar to "pretty" (also incompatible with
     * "load") except raw byte data elements will be printed like "hex" format\, and "print" selects
     * a format where only non-printing characters are hexadecimal encoded.  These formats are
     * compatible with the @ref util_dump and @ref util_load commands., a string\, chosen from the
     * following options: \c "hex"\, \c "json"\, \c "pretty"\, \c "pretty_hex"\, \c "print"; default
     * empty.}
     * @config{incremental = (, configure the cursor for block incremental backup usage.  These
     * formats are only compatible with the backup data source; see @ref backup., a set of related
     * configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;consolidate,
     * causes block incremental backup information to be consolidated if adjacent granularity blocks
     * are modified.  If false\, information will be returned in granularity sized blocks only.
     * This must be set on the primary backup cursor and it applies to all files for this backup., a
     * boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, whether to
     * configure this backup as the starting point for a subsequent incremental backup., a boolean
     * flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;file, the file name when opening a
     * duplicate incremental backup cursor.  That duplicate cursor will return the block
     * modifications relevant to the given file name., a string; default empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;force_stop, causes all block incremental backup information
     * to be released.  This is on an open_cursor call and the resources will be released when this
     * cursor is closed.  No other operations should be done on this open cursor., a boolean flag;
     * default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;granularity, this setting manages the
     * granularity of how WiredTiger maintains modification maps internally.  The larger the
     * granularity\, the smaller amount of information WiredTiger need to maintain., an integer
     * between \c 4KB and \c 2GB; default \c 16MB.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;src_id, a string
     * that identifies a previous checkpoint backup source as the source of this incremental backup.
     * This identifier must have already been created by use of the 'this_id' configuration in an
     * earlier backup.  A source id is required to begin an incremental backup., a string; default
     * empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;this_id, a string that identifies the current system
     * state as a future backup source for an incremental backup via \c src_id.  This identifier is
     * required when opening an incremental backup cursor and an error will be returned if one is
     * not provided.  The identifiers can be any text string\, but should be unique., a string;
     * default empty.}
     * @config{ ),,}
     * @config{next_random, configure the cursor to return a pseudo-random record from the object
     * when the WT_CURSOR::next method is called; valid only for row-store cursors.  See @ref
     * cursor_random for details., a boolean flag; default \c false.}
     * @config{next_random_sample_size, cursors configured by \c next_random to return pseudo-random
     * records from the object randomly select from the entire object\, by default.  Setting \c
     * next_random_sample_size to a non-zero value sets the number of samples the application
     * expects to take using the \c next_random cursor.  A cursor configured with both \c
     * next_random and \c next_random_sample_size attempts to divide the object into \c
     * next_random_sample_size equal-sized pieces\, and each retrieval returns a record from one of
     * those pieces.  See @ref cursor_random for details., a string; default \c 0.}
     * @config{next_random_seed, configure the cursor to set an initial random seed when using \c
     * next_random configuration.  This is used for testing purposes only.  See @ref cursor_random
     * for details., a string; default \c 0.}
     * @config{overwrite, configures whether the cursor's insert and update methods check the
     * existing state of the record.  If \c overwrite is \c false\, WT_CURSOR::insert fails with
     * ::WT_DUPLICATE_KEY if the record exists\, and WT_CURSOR::update fails with ::WT_NOTFOUND if
     * the record does not exist., a boolean flag; default \c true.}
     * @config{raw, ignore the encodings for the key and value\, manage data as if the formats were
     * \c "u". See @ref cursor_raw for details., a boolean flag; default \c false.}
     * @config{read_once, results that are brought into cache from disk by this cursor will be given
     * less priority in the cache., a boolean flag; default \c false.}
     * @config{readonly, only query operations are supported by this cursor.  An error is returned
     * if a modification is attempted using the cursor.  The default is false for all cursor types
     * except for metadata cursors and checkpoint cursors., a boolean flag; default \c false.}
     * @config{statistics, Specify the statistics to be gathered.  Choosing "all" gathers statistics
     * regardless of cost and may include traversing on-disk files; "fast" gathers a subset of
     * relatively inexpensive statistics.  The selection must agree with the database \c statistics
     * configuration specified to ::wiredtiger_open or WT_CONNECTION::reconfigure.  For example\,
     * "all" or "fast" can be configured when the database is configured with "all"\, but the cursor
     * open will fail if "all" is specified when the database is configured with "fast"\, and the
     * cursor open will fail in all cases when the database is configured with "none". If "size" is
     * configured\, only the underlying size of the object on disk is filled in and the object is
     * not opened.  If \c statistics is not configured\, the default configuration is the database
     * configuration.  The "clear" configuration resets statistics after gathering them\, where
     * appropriate (for example\, a cache size statistic is not cleared\, while the count of cursor
     * insert operations will be cleared). See @ref statistics for more information., a list\, with
     * values chosen from the following options: \c "all"\, \c "cache_walk"\, \c "fast"\, \c
     * "clear"\, \c "size"\, \c "tree_walk"; default empty.}
     * @config{target, if non-empty\, back up the given list of objects; valid only for a backup
     * data source., a list of strings; default empty.}
     * @configend
     * @param[out] cursorp a pointer to the newly opened cursor
     * @errors
     */
    int __F(open_cursor)(WT_SESSION *session,
        const char *uri, WT_CURSOR *to_dup, const char *config, WT_CURSOR **cursorp);
    /*! @} */

    /*!
     * @name Table operations
     * @{
     */
    /*!
     * Alter a table.
     *
     * This will allow modification of some table settings after
     * creation.
     *
     * @exclusive
     *
     * @snippet ex_all.c Alter a table
     *
     * @param session the session handle
     * @param name the URI of the object to alter, such as \c "table:stock"
     * @configstart{WT_SESSION.alter, see dist/api_data.py}
     * @config{access_pattern_hint, It is recommended that workloads that consist primarily of
     * updates and/or point queries specify \c random.  Workloads that do many cursor scans through
     * large ranges of data should specify \c sequential and other workloads should specify \c none.
     * The option leads to an appropriate operating system advisory call where available., a
     * string\, chosen from the following options: \c "none"\, \c "random"\, \c "sequential";
     * default \c none.}
     * @config{app_metadata, application-owned metadata for this object., a string; default empty.}
     * @config{assert = (, declare timestamp usage., a set of related configuration options defined
     * as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;read_timestamp, if set\, check that timestamps
     * are \c always or \c never used on reads with this table\, writing an error message if the
     * policy is violated.  If the library was built in diagnostic mode\, drop core at the failing
     * check., a string\, chosen from the following options: \c "always"\, \c "never"\, \c "none";
     * default \c none.}
     * @config{ ),,}
     * @config{cache_resident, do not ever evict the object's pages from cache.  Not compatible with
     * LSM tables; see @ref tuning_cache_resident for more information., a boolean flag; default \c
     * false.}
     * @config{log = (, the transaction log configuration for this object.  Only valid if \c log is
     * enabled in ::wiredtiger_open., a set of related configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, if false\, this object has checkpoint-level
     * durability., a boolean flag; default \c true.}
     * @config{ ),,}
     * @config{os_cache_dirty_max, maximum dirty system buffer cache usage\, in bytes.  If
     * non-zero\, schedule writes for dirty blocks belonging to this object in the system buffer
     * cache after that many bytes from this object are written into the buffer cache., an integer
     * greater than or equal to \c 0; default \c 0.}
     * @config{os_cache_max, maximum system buffer cache usage\, in bytes.  If non-zero\, evict
     * object blocks from the system buffer cache after that many bytes from this object are read or
     * written into the buffer cache., an integer greater than or equal to \c 0; default \c 0.}
     * @config{write_timestamp_usage, describe how timestamps are expected to be used on table
     * modifications.  The choices are the default\, which ensures that once timestamps are used for
     * a key\, they are always used\, and also that multiple updates to a key never use decreasing
     * timestamps and \c never which enforces that timestamps are never used for a table.  (The \c
     * always\, \c key_consistent\, \c mixed_mode and \c ordered choices should not be used\, and
     * are retained for backward compatibility.)., a string\, chosen from the following options: \c
     * "always"\, \c "key_consistent"\, \c "mixed_mode"\, \c "never"\, \c "none"\, \c "ordered";
     * default \c none.}
     * @configend
     * @ebusy_errors
     */
    int __F(alter)(WT_SESSION *session,
        const char *name, const char *config);

    /*!
     * Bind values for a compiled configuration.  The bindings hold for API calls in this
     * session that use the compiled string.  Strings passed into this call are not duplicated,
     * the application must ensure that strings remain valid while the bindings are being
     * used.
     *
     * This API may change in future releases.
     *
     * @param session the session handle
     * @param compiled a string returned from WT_CONNECTION::compile_configuration
     * @errors
     */
    int __F(bind_configuration)(WT_SESSION *session, const char *compiled, ...);

    /*!
     * Create a table, column group, index or file.
     *
     * @not_transactional
     *
     * @snippet ex_all.c Create a table
     *
     * @param session the session handle
     * @param name the URI of the object to create, such as
     * \c "table:stock". For a description of URI formats
     * see @ref data_sources.
     * @configstart{WT_SESSION.create, see dist/api_data.py}
     * @config{access_pattern_hint, It is recommended that workloads that consist primarily of
     * updates and/or point queries specify \c random.  Workloads that do many cursor scans through
     * large ranges of data should specify \c sequential and other workloads should specify \c none.
     * The option leads to an appropriate operating system advisory call where available., a
     * string\, chosen from the following options: \c "none"\, \c "random"\, \c "sequential";
     * default \c none.}
     * @config{allocation_size, the file unit allocation size\, in bytes\, must be a power of two;
     * smaller values decrease the file space required by overflow items\, and the default value of
     * 4KB is a good choice absent requirements from the operating system or storage device., an
     * integer between \c 512B and \c 128MB; default \c 4KB.}
     * @config{app_metadata, application-owned metadata for this object., a string; default empty.}
     * @config{assert = (, declare timestamp usage., a set of related configuration options defined
     * as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;read_timestamp, if set\, check that timestamps
     * are \c always or \c never used on reads with this table\, writing an error message if the
     * policy is violated.  If the library was built in diagnostic mode\, drop core at the failing
     * check., a string\, chosen from the following options: \c "always"\, \c "never"\, \c "none";
     * default \c none.}
     * @config{ ),,}
     * @config{block_allocation, configure block allocation.  Permitted values are \c "best" or \c
     * "first"; the \c "best" configuration uses a best-fit algorithm\, the \c "first" configuration
     * uses a first-available algorithm during block allocation., a string\, chosen from the
     * following options: \c "best"\, \c "first"; default \c best.}
     * @config{block_compressor, configure a compressor for file blocks.  Permitted values are \c
     * "none" or a custom compression engine name created with WT_CONNECTION::add_compressor.  If
     * WiredTiger has builtin support for \c "lz4"\, \c "snappy"\, \c "zlib" or \c "zstd"
     * compression\, these names are also available.  See @ref compression for more information., a
     * string; default \c none.}
     * @config{cache_resident, do not ever evict the object's pages from cache.  Not compatible with
     * LSM tables; see @ref tuning_cache_resident for more information., a boolean flag; default \c
     * false.}
     * @config{checksum, configure block checksums; the permitted values are \c on\, \c off\, \c
     * uncompressed and \c unencrypted.  The default is \c on\, in which case all block writes
     * include a checksum subsequently verified when the block is read.  The \c off setting does no
     * checksums\, the \c uncompressed setting only checksums blocks that are not compressed\, and
     * the \c unencrypted setting only checksums blocks that are not encrypted.  See @ref
     * tune_checksum for more information., a string\, chosen from the following options: \c "on"\,
     * \c "off"\, \c "uncompressed"\, \c "unencrypted"; default \c on.}
     * @config{colgroups, comma-separated list of names of column groups.  Each column group is
     * stored separately\, keyed by the primary key of the table.  If no column groups are
     * specified\, all columns are stored together in a single file.  All value columns in the table
     * must appear in at least one column group.  Each column group must be created with a separate
     * call to WT_SESSION::create using a \c colgroup: URI., a list of strings; default empty.}
     * @config{collator, configure custom collation for keys.  Permitted values are \c "none" or a
     * custom collator name created with WT_CONNECTION::add_collator., a string; default \c none.}
     * @config{columns, list of the column names.  Comma-separated list of the form
     * <code>(column[\,...])</code>. For tables\, the number of entries must match the total number
     * of values in \c key_format and \c value_format.  For colgroups and indices\, all column names
     * must appear in the list of columns for the table., a list of strings; default empty.}
     * @config{dictionary, the maximum number of unique values remembered in the Btree row-store
     * leaf page value dictionary; see @ref file_formats_compression for more information., an
     * integer greater than or equal to \c 0; default \c 0.}
     * @config{encryption = (, configure an encryptor for file blocks.  When a table is created\,
     * its encryptor is not implicitly used for any related indices or column groups., a set of
     * related configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;keyid, An
     * identifier that identifies a unique instance of the encryptor.  It is stored in clear text\,
     * and thus is available when the WiredTiger database is reopened.  On the first use of a
     * (name\, keyid) combination\, the WT_ENCRYPTOR::customize function is called with the keyid as
     * an argument., a string; default empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;name, Permitted
     * values are \c "none" or a custom encryption engine name created with
     * WT_CONNECTION::add_encryptor.  See @ref encryption for more information., a string; default
     * \c none.}
     * @config{ ),,}
     * @config{exclusive, fail if the object exists.  When false (the default)\, if the object
     * exists\, check that its settings match the specified configuration., a boolean flag; default
     * \c false.}
     * @config{extractor, configure a custom extractor for indices.  Permitted values are \c "none"
     * or an extractor name created with WT_CONNECTION::add_extractor., a string; default \c none.}
     * @config{format, the file format., a string\, chosen from the following options: \c "btree";
     * default \c btree.}
     * @config{huffman_key, This option is no longer supported\, retained for backward
     * compatibility., a string; default \c none.}
     * @config{huffman_value, configure Huffman encoding for values.  Permitted values are \c
     * "none"\, \c "english"\, \c "utf8<file>" or \c "utf16<file>". See @ref huffman for more
     * information., a string; default \c none.}
     * @config{ignore_in_memory_cache_size, allow update and insert operations to proceed even if
     * the cache is already at capacity.  Only valid in conjunction with in-memory databases.
     * Should be used with caution - this configuration allows WiredTiger to consume memory over the
     * configured cache limit., a boolean flag; default \c false.}
     * @config{immutable, configure the index to be immutable -- that is\, the index is not changed
     * by any update to a record in the table., a boolean flag; default \c false.}
     * @config{import = (, configure import of an existing object into the currently running
     * database., a set of related configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;compare_timestamp, allow importing files with timestamps
     * smaller or equal to the configured global timestamps.  Note the history of the files are not
     * imported together and thus snapshot read of historical data will not work with the option
     * "stable_timestamp". (The \c oldest and \c stable arguments are deprecated short-hand for \c
     * oldest_timestamp and \c stable_timestamp\, respectively)., a string\, chosen from the
     * following options: \c "oldest"\, \c "oldest_timestamp"\, \c "stable"\, \c "stable_timestamp";
     * default \c oldest_timestamp.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, whether to import the
     * input URI from disk., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;
     * file_metadata, the file configuration extracted from the metadata of the export database., a
     * string; default empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;metadata_file, a text file that
     * contains all the relevant metadata information for the URI to import.  The file is generated
     * by backup:export cursor., a string; default empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;repair,
     * whether to reconstruct the metadata from the raw file content., a boolean flag; default \c
     * false.}
     * @config{ ),,}
     * @config{internal_key_max, This option is no longer supported\, retained for backward
     * compatibility., an integer greater than or equal to \c 0; default \c 0.}
     * @config{internal_key_truncate, configure internal key truncation\, discarding unnecessary
     * trailing bytes on internal keys (ignored for custom collators)., a boolean flag; default \c
     * true.}
     * @config{internal_page_max, the maximum page size for internal nodes\, in bytes; the size must
     * be a multiple of the allocation size and is significant for applications wanting to avoid
     * excessive L2 cache misses while searching the tree.  The page maximum is the bytes of
     * uncompressed data\, that is\, the limit is applied before any block compression is done., an
     * integer between \c 512B and \c 512MB; default \c 4KB.}
     * @config{key_format, the format of the data packed into key items.  See @ref
     * schema_format_types for details.  By default\, the key_format is \c 'u' and applications use
     * WT_ITEM structures to manipulate raw byte arrays.  By default\, records are stored in
     * row-store files: keys of type \c 'r' are record numbers and records referenced by record
     * number are stored in column-store files., a format string; default \c u.}
     * @config{key_gap, This option is no longer supported\, retained for backward compatibility.,
     * an integer greater than or equal to \c 0; default \c 10.}
     * @config{leaf_key_max, the largest key stored in a leaf node\, in bytes.  If set\, keys larger
     * than the specified size are stored as overflow items (which may require additional I/O to
     * access). The default value is one-tenth the size of a newly split leaf page., an integer
     * greater than or equal to \c 0; default \c 0.}
     * @config{leaf_page_max, the maximum page size for leaf nodes\, in bytes; the size must be a
     * multiple of the allocation size\, and is significant for applications wanting to maximize
     * sequential data transfer from a storage device.  The page maximum is the bytes of
     * uncompressed data\, that is\, the limit is applied before any block compression is done.  For
     * fixed-length column store\, the size includes only the bitmap data; pages containing
     * timestamp information can be larger\, and the size is limited to 128KB rather than 512MB., an
     * integer between \c 512B and \c 512MB; default \c 32KB.}
     * @config{leaf_value_max, the largest value stored in a leaf node\, in bytes.  If set\, values
     * larger than the specified size are stored as overflow items (which may require additional I/O
     * to access). If the size is larger than the maximum leaf page size\, the page size is
     * temporarily ignored when large values are written.  The default is one-half the size of a
     * newly split leaf page., an integer greater than or equal to \c 0; default \c 0.}
     * @config{log = (, the transaction log configuration for this object.  Only valid if \c log is
     * enabled in ::wiredtiger_open., a set of related configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, if false\, this object has checkpoint-level
     * durability., a boolean flag; default \c true.}
     * @config{ ),,}
     * @config{lsm = (, options only relevant for LSM data sources., a set of related configuration
     * options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;auto_throttle, Throttle inserts
     * into LSM trees if flushing to disk isn't keeping up., a boolean flag; default \c true.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;bloom, create Bloom filters on LSM tree chunks as they are
     * merged., a boolean flag; default \c true.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;bloom_bit_count,
     * the number of bits used per item for LSM Bloom filters., an integer between \c 2 and \c 1000;
     * default \c 16.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;bloom_config, config string used when
     * creating Bloom filter files\, passed to WT_SESSION::create., a string; default empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;bloom_hash_count, the number of hash values per item used for
     * LSM Bloom filters., an integer between \c 2 and \c 100; default \c 8.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;bloom_oldest, create a Bloom filter on the oldest LSM tree
     * chunk.  Only supported if Bloom filters are enabled., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;chunk_count_limit, the maximum number of chunks to allow in
     * an LSM tree.  This option automatically times out old data.  As new chunks are added old
     * chunks will be removed.  Enabling this option disables LSM background merges., an integer;
     * default \c 0.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;chunk_max, the maximum size a single chunk can
     * be.  Chunks larger than this size are not considered for further merges.  This is a soft
     * limit\, and chunks larger than this value can be created.  Must be larger than chunk_size.,
     * an integer between \c 100MB and \c 10TB; default \c 5GB.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;
     * chunk_size, the maximum size of the in-memory chunk of an LSM tree.  This limit is soft\, it
     * is possible for chunks to be temporarily larger than this value.  This overrides the \c
     * memory_page_max setting., an integer between \c 512K and \c 500MB; default \c 10MB.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;merge_custom = (, configure the tree to merge into a custom
     * data source., a set of related configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;prefix, custom data source prefix
     * instead of \c "file"., a string; default empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;start_generation, merge generation at
     * which the custom data source is used (zero indicates no custom data source)., an integer
     * between \c 0 and \c 10; default \c 0.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;suffix, custom data source suffix
     * instead of \c ".lsm"., a string; default empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp; ),,}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;merge_max, the maximum number of chunks to include in a merge
     * operation., an integer between \c 2 and \c 100; default \c 15.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;merge_min, the minimum number of chunks to include in a merge
     * operation.  If set to 0 or 1 half the value of merge_max is used., an integer no more than \c
     * 100; default \c 0.}
     * @config{ ),,}
     * @config{memory_page_image_max, the maximum in-memory page image represented by a single
     * storage block.  Depending on compression efficiency\, compression can create storage blocks
     * which require significant resources to re-instantiate in the cache\, penalizing the
     * performance of future point updates.  The value limits the maximum in-memory page image a
     * storage block will need.  If set to 0\, a default of 4 times \c leaf_page_max is used., an
     * integer greater than or equal to \c 0; default \c 0.}
     * @config{memory_page_max, the maximum size a page can grow to in memory before being
     * reconciled to disk.  The specified size will be adjusted to a lower bound of
     * <code>leaf_page_max</code>\, and an upper bound of <code>cache_size / 10</code>. This limit
     * is soft - it is possible for pages to be temporarily larger than this value.  This setting is
     * ignored for LSM trees\, see \c chunk_size., an integer between \c 512B and \c 10TB; default
     * \c 5MB.}
     * @config{os_cache_dirty_max, maximum dirty system buffer cache usage\, in bytes.  If
     * non-zero\, schedule writes for dirty blocks belonging to this object in the system buffer
     * cache after that many bytes from this object are written into the buffer cache., an integer
     * greater than or equal to \c 0; default \c 0.}
     * @config{os_cache_max, maximum system buffer cache usage\, in bytes.  If non-zero\, evict
     * object blocks from the system buffer cache after that many bytes from this object are read or
     * written into the buffer cache., an integer greater than or equal to \c 0; default \c 0.}
     * @config{prefix_compression, configure prefix compression on row-store leaf pages., a boolean
     * flag; default \c false.}
     * @config{prefix_compression_min, minimum gain before prefix compression will be used on
     * row-store leaf pages., an integer greater than or equal to \c 0; default \c 4.}
     * @config{split_pct, the Btree page split size as a percentage of the maximum Btree page size\,
     * that is\, when a Btree page is split\, it will be split into smaller pages\, where each page
     * is the specified percentage of the maximum Btree page size., an integer between \c 50 and \c
     * 100; default \c 90.}
     * @config{tiered_storage = (, configure a storage source for this table., a set of related
     * configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;auth_token,
     * authentication string identifier., a string; default empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;
     * bucket, the bucket indicating the location for this table., a string; default empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;bucket_prefix, the unique bucket prefix for this table., a
     * string; default empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;cache_directory, a directory to store
     * locally cached versions of files in the storage source.  By default\, it is named with \c
     * "-cache" appended to the bucket name.  A relative directory name is relative to the home
     * directory., a string; default empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;local_retention, time
     * in seconds to retain data on tiered storage on the local tier for faster read access., an
     * integer between \c 0 and \c 10000; default \c 300.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;name,
     * permitted values are \c "none" or a custom storage source name created with
     * WT_CONNECTION::add_storage_source.  See @ref custom_storage_sources for more information., a
     * string; default \c none.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;shared, enable sharing tiered
     * tables across other WiredTiger instances., a boolean flag; default \c false.}
     * @config{ ),,}
     * @config{type, set the type of data source used to store a column group\, index or simple
     * table.  By default\, a \c "file:" URI is derived from the object name.  The \c type
     * configuration can be used to switch to a different data source\, such as LSM or an extension
     * configured by the application., a string; default \c file.}
     * @config{value_format, the format of the data packed into value items.  See @ref
     * schema_format_types for details.  By default\, the value_format is \c 'u' and applications
     * use a WT_ITEM structure to manipulate raw byte arrays.  Value items of type 't' are
     * bitfields\, and when configured with record number type keys\, will be stored using a
     * fixed-length store., a format string; default \c u.}
     * @config{write_timestamp_usage, describe how timestamps are expected to be used on table
     * modifications.  The choices are the default\, which ensures that once timestamps are used for
     * a key\, they are always used\, and also that multiple updates to a key never use decreasing
     * timestamps and \c never which enforces that timestamps are never used for a table.  (The \c
     * always\, \c key_consistent\, \c mixed_mode and \c ordered choices should not be used\, and
     * are retained for backward compatibility.)., a string\, chosen from the following options: \c
     * "always"\, \c "key_consistent"\, \c "mixed_mode"\, \c "never"\, \c "none"\, \c "ordered";
     * default \c none.}
     * @configend
     * @errors
     */
    int __F(create)(WT_SESSION *session,
        const char *name, const char *config);

    /*!
     * Compact a live row- or column-store btree or LSM tree.
     *
     * @snippet ex_all.c Compact a table
     *
     * @param session the session handle
     * @param name the URI of the object to compact, such as
     * \c "table:stock"
     * @configstart{WT_SESSION.compact, see dist/api_data.py}
     * @config{background, enable/disabled the background compaction server., a boolean flag;
     * default empty.}
     * @config{exclude, list of table objects to be excluded from background compaction.  The list
     * is immutable and only applied when the background compaction gets enabled.  The list is not
     * saved between the calls and needs to be reapplied each time the service is enabled.  The
     * individual objects in the list can only be of the \c table: URI type., a list of strings;
     * default empty.}
     * @config{free_space_target, minimum amount of space recoverable for compaction to proceed., an
     * integer greater than or equal to \c 1MB; default \c 20MB.}
     * @config{run_once, configure background compaction server to run once.  In this mode\,
     * compaction is always attempted on each table unless explicitly excluded., a boolean flag;
     * default \c false.}
     * @config{timeout, maximum amount of time to allow for compact in seconds.  The actual amount
     * of time spent in compact may exceed the configured value.  A value of zero disables the
     * timeout., an integer; default \c 1200.}
     * @configend
     * @errors
     */
    int __F(compact)(WT_SESSION *session,
        const char *name, const char *config);

    /*!
     * Drop (delete) a table.
     *
     * @exclusive
     *
     * @not_transactional
     *
     * @snippet ex_all.c Drop a table
     *
     * @param session the session handle
     * @param name the URI of the object to drop, such as \c "table:stock"
     * @configstart{WT_SESSION.drop, see dist/api_data.py}
     * @config{force, return success if the object does not exist., a boolean flag; default \c
     * false.}
     * @config{remove_files, if the underlying files should be removed., a boolean flag; default \c
     * true.}
     * @configend
     * @ebusy_errors
     */
    int __F(drop)(WT_SESSION *session,
        const char *name, const char *config);

    /*!
     * Join a join cursor with a reference cursor.
     *
     * @snippet ex_schema.c Join cursors
     *
     * @param session the session handle
     * @param join_cursor a cursor that was opened using a
     * \c "join:" URI. It may not have been used for any operations
     * other than other join calls.
     * @param ref_cursor an index cursor having the same base table
     * as the join_cursor, or a table cursor open on the same base table,
     * or another join cursor. Unless the ref_cursor is another join
     * cursor, it must be positioned.
     *
     * The ref_cursor limits the results seen by iterating the
     * join_cursor to table items referred to by the key in this
     * index. The set of keys referred to is modified by the compare
     * config option.
     *
     * Multiple join calls builds up a set of ref_cursors, and
     * by default, the results seen by iteration are the intersection
     * of the cursor ranges participating in the join. When configured
     * with \c "operation=or", the results seen are the union of
     * the participating cursor ranges.
     *
     * After the join call completes, the ref_cursor cursor may not be
     * used for any purpose other than get_key and get_value. Any other
     * cursor method (e.g. next, prev,close) will fail. When the
     * join_cursor is closed, the ref_cursor is made available for
     * general use again. The application should close ref_cursor when
     * finished with it, although not before the join_cursor is closed.
     *
     * @configstart{WT_SESSION.join, see dist/api_data.py}
     * @config{bloom_bit_count, the number of bits used per item for the Bloom filter., an integer
     * between \c 2 and \c 1000; default \c 16.}
     * @config{bloom_false_positives, return all values that pass the Bloom filter\, without
     * eliminating any false positives., a boolean flag; default \c false.}
     * @config{bloom_hash_count, the number of hash values per item for the Bloom filter., an
     * integer between \c 2 and \c 100; default \c 8.}
     * @config{compare, modifies the set of items to be returned so that the index key satisfies the
     * given comparison relative to the key set in this cursor., a string\, chosen from the
     * following options: \c "eq"\, \c "ge"\, \c "gt"\, \c "le"\, \c "lt"; default \c "eq".}
     * @config{count, set an approximate count of the elements that would be included in the join.
     * This is used in sizing the Bloom filter\, and also influences evaluation order for cursors in
     * the join.  When the count is equal for multiple Bloom filters in a composition of joins\, the
     * Bloom filter may be shared., an integer; default \c 0.}
     * @config{operation, the operation applied between this and other joined cursors.  When
     * "operation=and" is specified\, all the conditions implied by joins must be satisfied for an
     * entry to be returned by the join cursor; when "operation=or" is specified\, only one must be
     * satisfied.  All cursors joined to a join cursor must have matching operations., a string\,
     * chosen from the following options: \c "and"\, \c "or"; default \c "and".}
     * @config{strategy, when set to \c bloom\, a Bloom filter is created and populated for this
     * index.  This has an up front cost but may reduce the number of accesses to the main table
     * when iterating the joined cursor.  The \c bloom setting requires that \c count be set., a
     * string\, chosen from the following options: \c "bloom"\, \c "default"; default empty.}
     * @configend
     * @errors
     */
    int __F(join)(WT_SESSION *session, WT_CURSOR *join_cursor,
        WT_CURSOR *ref_cursor, const char *config);

    /*!
     * Flush the log.
     *
     * WT_SESSION::log_flush will fail if logging is not enabled.
     *
     * @param session the session handle
     * @configstart{WT_SESSION.log_flush, see dist/api_data.py}
     * @config{sync, forcibly flush the log and wait for it to achieve the synchronization level
     * specified.  The \c off setting forces any buffered log records to be written to the file
     * system.  The \c on setting forces log records to be written to the storage device., a
     * string\, chosen from the following options: \c "off"\, \c "on"; default \c on.}
     * @configend
     * @errors
     */
    int __F(log_flush)(WT_SESSION *session, const char *config);

    /*!
     * Insert a ::WT_LOGREC_MESSAGE type record in the database log files
     * (the database must be configured for logging when this method is
     * called).
     *
     * @param session the session handle
     * @param format a printf format specifier
     * @errors
     */
    int __F(log_printf)(WT_SESSION *session, const char *format, ...);

    /*!
     * Rename an object.
     *
     * @not_transactional
     *
     * @snippet ex_all.c Rename a table
     *
     * @exclusive
     *
     * @param session the session handle
     * @param uri the current URI of the object, such as \c "table:old"
     * @param newuri the new URI of the object, such as \c "table:new"
     * @configempty{WT_SESSION.rename, see dist/api_data.py}
     * @ebusy_errors
     */
    int __F(rename)(WT_SESSION *session,
        const char *uri, const char *newuri, const char *config);

    /*!
     * Reset the session handle.
     *
     * This method resets the cursors associated with the session, clears session statistics and
     * discards cached resources. No session configurations are modified (or reset to their
     * default values). WT_SESSION::reset will fail if a transaction is in progress in the
     * session.
     *
     * @snippet ex_all.c Reset the session
     *
     * @param session the session handle
     * @errors
     */
    int __F(reset)(WT_SESSION *session);

    /*!
     * Salvage a table.
     *
     * Salvage rebuilds the file or files which comprise a table,
     * discarding any corrupted file blocks.
     *
     * When salvage is done, previously deleted records may re-appear, and
     * inserted records may disappear, so salvage should not be run
     * unless it is known to be necessary.  Normally, salvage should be
     * called after a table or file has been corrupted, as reported by the
     * WT_SESSION::verify method.
     *
     * Files are rebuilt in place. The salvage method overwrites the
     * existing files.
     *
     * @exclusive
     *
     * @snippet ex_all.c Salvage a table
     *
     * @param session the session handle
     * @param name the URI of the table or file to salvage
     * @configstart{WT_SESSION.salvage, see dist/api_data.py}
     * @config{force, force salvage even of files that do not appear to be WiredTiger files., a
     * boolean flag; default \c false.}
     * @configend
     * @ebusy_errors
     */
    int __F(salvage)(WT_SESSION *session,
        const char *name, const char *config);

    /*!
     * Truncate a file, table, cursor range, or backup cursor
     *
     * Truncate a table or file.
     * @snippet ex_all.c Truncate a table
     *
     * Truncate a cursor range.  When truncating based on a cursor position,
     * it is not required the cursor reference a record in the object, only
     * that the key be set.  This allows applications to discard portions of
     * the object name space without knowing exactly what records the object
     * contains. The start and stop points are both inclusive; that is, the
     * key set in the start cursor is the first record to be deleted and the
     * key set in the stop cursor is the last.
     *
     * @snippet ex_all.c Truncate a range
     *
     * Range truncate is implemented as a "scan and write" operation, specifically without range
     * locks. Inserts or other operations in the range, as well as operations before or after
     * the range when no explicit starting or ending key is set, are not well defined: conflicts
     * may be detected or both transactions may commit. If both commit, there's a failure and
     * recovery runs, the result may be different than what was in cache before the crash.
     *
     * The WT_CURSOR::truncate range truncate operation can only be used at snapshot isolation.
     *
     * Any specified cursors end with no position, and subsequent calls to
     * the WT_CURSOR::next (WT_CURSOR::prev) method will iterate from the
     * beginning (end) of the table.
     *
     * Example: truncate a backup cursor.  This operation removes all log files that
     * have been returned by the backup cursor.  It can be used to remove log
     * files after copying them during @ref backup_incremental.
     * @snippet ex_backup.c Truncate a backup cursor
     *
     * @param session the session handle
     * @param name the URI of the table or file to truncate, or \c "log:"
     * for a backup cursor
     * @param start optional cursor marking the first record discarded;
     * if <code>NULL</code>, the truncate starts from the beginning of
     * the object; must be provided when truncating a backup cursor
     * @param stop optional cursor marking the last record discarded;
     * if <code>NULL</code>, the truncate continues to the end of the
     * object; ignored when truncating a backup cursor
     * @configempty{WT_SESSION.truncate, see dist/api_data.py}
     * @errors
     */
    int __F(truncate)(WT_SESSION *session,
        const char *name, WT_CURSOR *start, WT_CURSOR *stop, const char *config);

    /*!
     * Upgrade a table.
     *
     * Upgrade upgrades a table or file, if upgrade is required.
     *
     * @exclusive
     *
     * @snippet ex_all.c Upgrade a table
     *
     * @param session the session handle
     * @param name the URI of the table or file to upgrade
     * @configempty{WT_SESSION.upgrade, see dist/api_data.py}
     * @ebusy_errors
     */
    int __F(upgrade)(WT_SESSION *session,
        const char *name, const char *config);

    /*!
     * Verify a table.
     *
     * Verify reports if a file, or the files that comprise a table, have been corrupted.
     * The WT_SESSION::salvage method can be used to repair a corrupted file.
     *
     * @snippet ex_all.c Verify a table
     *
     * @exclusive
     *
     * @param session the session handle
     * @param name the URI of the table or file to verify, optional if verifying the history
     * store
     * @configstart{WT_SESSION.verify, see dist/api_data.py}
     * @config{do_not_clear_txn_id, Turn off transaction id clearing\, intended for debugging and
     * better diagnosis of crashes or failures., a boolean flag; default \c false.}
     * @config{dump_address, Display page addresses\, time windows\, and page types as pages are
     * verified\, using the application's message handler\, intended for debugging., a boolean flag;
     * default \c false.}
     * @config{dump_all_data, Display application data as pages or blocks are verified\, using the
     * application's message handler\, intended for debugging.  Disabling this does not guarantee
     * that no user data will be output., a boolean flag; default \c false.}
     * @config{dump_blocks, Display the contents of on-disk blocks as they are verified\, using the
     * application's message handler\, intended for debugging., a boolean flag; default \c false.}
     * @config{dump_key_data, Display application data keys as pages or blocks are verified\, using
     * the application's message handler\, intended for debugging.  Disabling this does not
     * guarantee that no user data will be output., a boolean flag; default \c false.}
     * @config{dump_layout, Display the layout of the files as they are verified\, using the
     * application's message handler\, intended for debugging; requires optional support from the
     * block manager., a boolean flag; default \c false.}
     * @config{dump_offsets, Display the contents of specific on-disk blocks\, using the
     * application's message handler\, intended for debugging., a list of strings; default empty.}
     * @config{dump_pages, Display the contents of in-memory pages as they are verified\, using the
     * application's message handler\, intended for debugging., a boolean flag; default \c false.}
     * @config{read_corrupt, A mode that allows verify to continue reading after encountering a
     * checksum error.  It will skip past the corrupt block and continue with the verification
     * process., a boolean flag; default \c false.}
     * @config{stable_timestamp, Ensure that no data has a start timestamp after the stable
     * timestamp\, to be run after rollback_to_stable., a boolean flag; default \c false.}
     * @config{strict, Treat any verification problem as an error; by default\, verify will warn\,
     * but not fail\, in the case of errors that won't affect future behavior (for example\, a
     * leaked block)., a boolean flag; default \c false.}
     * @configend
     * @ebusy_errors
     */
    int __F(verify)(WT_SESSION *session,
        const char *name, const char *config);
    /*! @} */

    /*!
     * @name Transactions
     * @{
     */
    /*!
     * Start a transaction in this session.
     *
     * The transaction remains active until ended by
     * WT_SESSION::commit_transaction or WT_SESSION::rollback_transaction.
     * Operations performed on cursors capable of supporting transactional
     * operations that are already open in this session, or which are opened
     * before the transaction ends, will operate in the context of the
     * transaction.
     *
     * @requires_notransaction
     *
     * @snippet ex_all.c transaction commit/rollback
     *
     * @param session the session handle
     * @configstart{WT_SESSION.begin_transaction, see dist/api_data.py}
     * @config{ignore_prepare, whether to ignore updates by other prepared transactions when doing
     * of read operations of this transaction.  When \c true\, forces the transaction to be
     * read-only.  Use \c force to ignore prepared updates and permit writes (see @ref
     * timestamp_prepare_ignore_prepare for more information)., a string\, chosen from the following
     * options: \c "false"\, \c "force"\, \c "true"; default \c false.}
     * @config{isolation, the isolation level for this transaction; defaults to the session's
     * isolation level., a string\, chosen from the following options: \c "read-uncommitted"\, \c
     * "read-committed"\, \c "snapshot"; default empty.}
     * @config{name, name of the transaction for tracing and debugging., a string; default empty.}
     * @config{no_timestamp, allow a commit without a timestamp\, creating values that have "always
     * existed" and are visible regardless of timestamp.  See @ref timestamp_txn_api., a boolean
     * flag; default \c false.}
     * @config{operation_timeout_ms, when non-zero\, a requested limit on the time taken to complete
     * operations in this transaction.  Time is measured in real time milliseconds from the start of
     * each WiredTiger API call.  There is no guarantee any operation will not take longer than this
     * amount of time.  If WiredTiger notices the limit has been exceeded\, an operation may return
     * a WT_ROLLBACK error.  Default is to have no limit., an integer greater than or equal to \c 0;
     * default \c 0.}
     * @config{priority, priority of the transaction for resolving conflicts.  Transactions with
     * higher values are less likely to abort., an integer between \c -100 and \c 100; default \c
     * 0.}
     * @config{read_timestamp, read using the specified timestamp.  The value must not be older than
     * the current oldest timestamp.  See @ref timestamp_txn_api., a string; default empty.}
     * @config{roundup_timestamps = (, round up timestamps of the transaction., a set of related
     * configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;prepared,
     * applicable only for prepared transactions\, and intended only for special-purpose use.  See
     * @ref timestamp_prepare_roundup.  Allows the prepare timestamp and the commit timestamp of
     * this transaction to be rounded up to be no older than the oldest timestamp\, and allows
     * violating the usual restriction that the prepare timestamp must be newer than the stable
     * timestamp.  Specifically: at transaction prepare\, if the prepare timestamp is less than or
     * equal to the oldest timestamp\, the prepare timestamp will be rounded to the oldest
     * timestamp.  Subsequently\, at commit time\, if the commit timestamp is less than the (now
     * rounded) prepare timestamp\, the commit timestamp will be rounded up to it and thus to at
     * least oldest.  Neither timestamp will be checked against the stable timestamp., a boolean
     * flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;read, if the read timestamp is less
     * than the oldest timestamp\, the read timestamp will be rounded up to the oldest timestamp.
     * See @ref timestamp_read_roundup., a boolean flag; default \c false.}
     * @config{ ),,}
     * @config{sync, whether to sync log records when the transaction commits\, inherited from
     * ::wiredtiger_open \c transaction_sync., a boolean flag; default empty.}
     * @configend
     * @errors
     */
    int __F(begin_transaction)(WT_SESSION *session, const char *config);

    /*!
     * Commit the current transaction.
     *
     * A transaction must be in progress when this method is called.
     *
     * If WT_SESSION::commit_transaction returns an error, the transaction
     * was rolled back, not committed, and all cursors associated with the session are reset.
     *
     * @requires_transaction
     *
     * @snippet ex_all.c transaction commit/rollback
     *
     * @param session the session handle
     * @configstart{WT_SESSION.commit_transaction, see dist/api_data.py}
     * @config{commit_timestamp, set the commit timestamp for the current transaction.  For
     * non-prepared transactions\, the value must not be older than the first commit timestamp
     * already set for the current transaction (if any)\, must not be older than the current oldest
     * timestamp\, and must be after the current stable timestamp.  For prepared transactions\, a
     * commit timestamp is required\, must not be older than the prepare timestamp\, and can be set
     * only once.  See @ref timestamp_txn_api and @ref timestamp_prepare., a string; default empty.}
     * @config{durable_timestamp, set the durable timestamp for the current transaction.  Required
     * for the commit of a prepared transaction\, and otherwise not permitted.  The value must also
     * be after the current oldest and stable timestamps and must not be older than the commit
     * timestamp.  See @ref timestamp_prepare., a string; default empty.}
     * @config{operation_timeout_ms, when non-zero\, a requested limit on the time taken to complete
     * operations in this transaction.  Time is measured in real time milliseconds from the start of
     * each WiredTiger API call.  There is no guarantee any operation will not take longer than this
     * amount of time.  If WiredTiger notices the limit has been exceeded\, an operation may return
     * a WT_ROLLBACK error.  Default is to have no limit., an integer greater than or equal to \c 0;
     * default \c 0.}
     * @config{sync, override whether to sync log records when the transaction commits.  The default
     * is inherited from ::wiredtiger_open \c transaction_sync.  The \c off setting does not wait
     * for records to be written or synchronized.  The \c on setting forces log records to be
     * written to the storage device., a string\, chosen from the following options: \c "off"\, \c
     * "on"; default empty.}
     * @configend
     * @errors
     */
    int __F(commit_transaction)(WT_SESSION *session, const char *config);

    /*!
     * Prepare the current transaction.
     *
     * A transaction must be in progress when this method is called.
     *
     * Preparing a transaction will guarantee a subsequent commit will
     * succeed. Only commit and rollback are allowed on a transaction after
     * it has been prepared. The transaction prepare API is designed to
     * support MongoDB exclusively, and guarantees update conflicts have
     * been resolved, but does not guarantee durability.
     *
     * @requires_transaction
     *
     * @snippet ex_all.c transaction prepare
     *
     * @param session the session handle
     * @configstart{WT_SESSION.prepare_transaction, see dist/api_data.py}
     * @config{prepare_timestamp, set the prepare timestamp for the updates of the current
     * transaction.  The value must not be older than any active read timestamps\, and must be newer
     * than the current stable timestamp.  See @ref timestamp_prepare., a string; default empty.}
     * @configend
     * @errors
     */
    int __F(prepare_transaction)(WT_SESSION *session, const char *config);

    /*!
     * Roll back the current transaction.
     *
     * A transaction must be in progress when this method is called.
     *
     * All cursors associated with the session are reset.
     *
     * @requires_transaction
     *
     * @snippet ex_all.c transaction commit/rollback
     *
     * @param session the session handle
     * @configstart{WT_SESSION.rollback_transaction, see dist/api_data.py}
     * @config{operation_timeout_ms, when non-zero\, a requested limit on the time taken to complete
     * operations in this transaction.  Time is measured in real time milliseconds from the start of
     * each WiredTiger API call.  There is no guarantee any operation will not take longer than this
     * amount of time.  If WiredTiger notices the limit has been exceeded\, an operation may return
     * a WT_ROLLBACK error.  Default is to have no limit., an integer greater than or equal to \c 0;
     * default \c 0.}
     * @configend
     * @errors
     */
    int __F(rollback_transaction)(WT_SESSION *session, const char *config);
    /*! @} */

    /*!
     * @name Transaction timestamps
     * @{
     */
    /*!
     * Query the session's transaction timestamp state.
     *
     * The WT_SESSION.query_timestamp method can only be used at snapshot isolation.
     *
     * @param session the session handle
     * @param[out] hex_timestamp a buffer that will be set to the
     * hexadecimal encoding of the timestamp being queried.  Must be large
     * enough to hold a NUL terminated, hex-encoded 8B timestamp (17 bytes).
     * @configstart{WT_SESSION.query_timestamp, see dist/api_data.py}
     * @config{get, specify which timestamp to query: \c commit returns the most recently set
     * commit_timestamp; \c first_commit returns the first set commit_timestamp; \c prepare returns
     * the timestamp used in preparing a transaction; \c read returns the timestamp at which the
     * transaction is reading.  See @ref timestamp_txn_api., a string\, chosen from the following
     * options: \c "commit"\, \c "first_commit"\, \c "prepare"\, \c "read"; default \c read.}
     * @configend
     *
     * A timestamp of 0 is returned if the timestamp is not available or has not been set.
     * @errors
     */
    int __F(query_timestamp)(
        WT_SESSION *session, char *hex_timestamp, const char *config);

    /*!
     * Set a timestamp on a transaction.
     *
     * The WT_SESSION.timestamp_transaction method can only be used at snapshot isolation.
     *
     * @snippet ex_all.c transaction timestamp
     *
     * @requires_transaction
     *
     * @param session the session handle
     * @configstart{WT_SESSION.timestamp_transaction, see dist/api_data.py}
     * @config{commit_timestamp, set the commit timestamp for the current transaction.  For
     * non-prepared transactions\, the value must not be older than the first commit timestamp
     * already set for the current transaction\, if any\, must not be older than the current oldest
     * timestamp and must be after the current stable timestamp.  For prepared transactions\, a
     * commit timestamp is required\, must not be older than the prepare timestamp\, can be set only
     * once\, and must not be set until after the transaction has successfully prepared.  See @ref
     * timestamp_txn_api and @ref timestamp_prepare., a string; default empty.}
     * @config{durable_timestamp, set the durable timestamp for the current transaction.  Required
     * for the commit of a prepared transaction\, and otherwise not permitted.  Can only be set
     * after the transaction has been prepared and a commit timestamp has been set.  The value must
     * be after the current oldest and stable timestamps and must not be older than the commit
     * timestamp.  See @ref timestamp_prepare., a string; default empty.}
     * @config{prepare_timestamp, set the prepare timestamp for the updates of the current
     * transaction.  The value must not be older than any active read timestamps\, and must be newer
     * than the current stable timestamp.  Can be set only once per transaction.  Setting the
     * prepare timestamp does not by itself prepare the transaction\, but does oblige the
     * application to eventually prepare the transaction before committing it.  See @ref
     * timestamp_prepare., a string; default empty.}
     * @config{read_timestamp, read using the specified timestamp.  The value must not be older than
     * the current oldest timestamp.  This can only be set once for a transaction.  See @ref
     * timestamp_txn_api., a string; default empty.}
     * @configend
     * @errors
     */
    int __F(timestamp_transaction)(WT_SESSION *session, const char *config);

    /*!
     * Set a timestamp on a transaction numerically.  Prefer this method over
     * WT_SESSION::timestamp_transaction if the hexadecimal string parsing done in that method
     * becomes a bottleneck.
     *
     * The WT_SESSION.timestamp_transaction_uint method can only be used at snapshot isolation.
     *
     * @snippet ex_all.c transaction timestamp_uint
     *
     * @requires_transaction
     *
     * @param session the session handle
     * @param which the timestamp being set (see ::WT_TS_TXN_TYPE for available options, and
     * WT_SESSION::timestamp_transaction for constraints on the timestamps).
     * @param ts the timestamp.
     * @errors
     */
    int __F(timestamp_transaction_uint)(WT_SESSION *session, WT_TS_TXN_TYPE which,
            uint64_t ts);
    /*! @} */

    /*!
     * @name Transaction support
     * @{
     */
    /*!
     * Write a transactionally consistent snapshot of a database or set of individual objects.
     *
     * When timestamps are not in use, the checkpoint includes all transactions committed
     * before the checkpoint starts. When timestamps are in use and the checkpoint runs with
     * \c use_timestamp=true (the default), updates committed with a timestamp after the
     * \c stable timestamp, in tables configured for checkpoint-level durability, are not
     * included in the checkpoint. Updates committed in tables configured for commit-level
     * durability are always included in the checkpoint. See @ref durability_checkpoint and
     * @ref durability_log for more information.
     *
     * Calling the checkpoint method multiple times serializes the checkpoints; new checkpoint
     * calls wait for running checkpoint calls to complete.
     *
     * Existing named checkpoints may optionally be discarded.
     *
     * @requires_notransaction
     *
     * @snippet ex_all.c Checkpoint examples
     *
     * @param session the session handle
     * @configstart{WT_SESSION.checkpoint, see dist/api_data.py}
     * @config{drop, specify a list of checkpoints to drop.  The list may additionally contain one
     * of the following keys: \c "from=all" to drop all checkpoints\, \c "from=<checkpoint>" to drop
     * all checkpoints after and including the named checkpoint\, or \c "to=<checkpoint>" to drop
     * all checkpoints before and including the named checkpoint.  Checkpoints cannot be dropped if
     * open in a cursor.  While a hot backup is in progress\, checkpoints created prior to the start
     * of the backup cannot be dropped., a list of strings; default empty.}
     * @config{flush_tier = (, configure flushing objects to tiered storage after checkpoint., a set
     * of related configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;
     * enabled, if true and tiered storage is in use\, perform one iteration of object switching and
     * flushing objects to tiered storage., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;force, if false (the default)\, flush_tier of any individual
     * object may be skipped if the underlying object has not been modified since the previous
     * flush_tier.  If true\, this option forces the flush_tier., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;sync, wait for all objects to be flushed to the shared
     * storage to the level specified.  When false\, do not wait for any objects to be written to
     * the tiered storage system but return immediately after generating the objects and work units
     * for an internal thread.  When true\, the caller waits until all work queued for this call to
     * be completely processed before returning., a boolean flag; default \c true.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;timeout, amount of time\, in seconds\, to wait for flushing
     * of objects to complete.  WiredTiger returns EBUSY if the timeout is reached.  A value of zero
     * disables the timeout., an integer; default \c 0.}
     * @config{ ),,}
     * @config{force, if false (the default)\, checkpoints may be skipped if the underlying object
     * has not been modified.  If true\, this option forces the checkpoint., a boolean flag; default
     * \c false.}
     * @config{name, if set\, specify a name for the checkpoint (note that checkpoints including LSM
     * trees may not be named)., a string; default empty.}
     * @config{target, if non-empty\, checkpoint the list of objects.  Checkpointing a list of
     * objects separately from a database-wide checkpoint can lead to data inconsistencies; see @ref
     * checkpoint_target for more information., a list of strings; default empty.}
     * @config{use_timestamp, if true (the default)\, create the checkpoint as of the last stable
     * timestamp if timestamps are in use\, or with all committed updates if there is no stable
     * timestamp set.  If false\, always generate a checkpoint with all committed updates\, ignoring
     * any stable timestamp., a boolean flag; default \c true.}
     * @configend
     * @errors
     */
    int __F(checkpoint)(WT_SESSION *session, const char *config);

    /*!
     * Reset the snapshot used for database visibility.
     *
     * For transactions running with snapshot isolation, this method releases the existing
     * snapshot of the database and gets a new one. This makes newer commits visible. The
     * call can be used to avoid pinning old and no-longer-needed content in the database.
     * Applications not using read timestamps for search may see different results after the
     * snapshot is updated.
     *
     * It is an error to call this method when using an isolation level other than snapshot
     * isolation, or if the current transaction has already written any data.
     *
     * @requires_transaction
     *
     * @snippet ex_all.c reset snapshot
     *
     * @param session the session handle
     * @errors
     */
    int __F(reset_snapshot)(WT_SESSION *session);

    /*!
     * Return the transaction ID range pinned by the session handle.
     *
     * The ID range is an approximate count of transactions and is calculated
     * based on the oldest ID needed for the active transaction in this session,
     * compared to the newest transaction in the system.
     *
     * @snippet ex_all.c transaction pinned range
     *
     * @param session the session handle
     * @param[out] range the range of IDs pinned by this session. Zero if
     * there is no active transaction.
     * @errors
     */
    int __F(transaction_pinned_range)(WT_SESSION* session, uint64_t *range);
    /*! @} */

#ifndef DOXYGEN
    /*!
     * Optionally returns the reason for the most recent rollback error returned from the API.
     *
     * There is no guarantee a rollback reason will be set and thus the caller
     * must check for a NULL pointer.
     *
     * @param session the session handle
     * @returns an optional string indicating the reason for the rollback
     */
    const char * __F(get_rollback_reason)(WT_SESSION *session);

    /*!
     * Call into the library.
     *
     * This method is used for breakpoints and to set other configuration
     * when debugging layers not directly supporting those features.
     *
     * @param session the session handle
     * @errors
     */
    int __F(breakpoint)(WT_SESSION *session);
#endif
};

/*!
 * A connection to a WiredTiger database.  The connection may be opened within
 * the same address space as the caller or accessed over a socket connection.
 *
 * Most applications will open a single connection to a database for each
 * process.  The first process to open a connection to a database will access
 * the database in its own address space.  Subsequent connections (if allowed)
 * will communicate with the first process over a socket connection to perform
 * their operations.
 *
 * <b>Thread safety:</b> A WT_CONNECTION handle may be shared between threads.
 * See @ref threads for more information.
 */
struct __wt_connection {
    /*!
     * Close a connection.
     *
     * Any open sessions will be closed. This will release the resources
     * associated with the session handle, including rolling back any
     * active transactions and closing any cursors that remain open in the
     * session.
     *
     * @snippet ex_all.c Close a connection
     *
     * @param connection the connection handle
     * @configstart{WT_CONNECTION.close, see dist/api_data.py}
     * @config{leak_memory, don't free memory during close., a boolean flag; default \c false.}
     * @config{use_timestamp, by default\, create the close checkpoint as of the last stable
     * timestamp if timestamps are in use\, or all current updates if there is no stable timestamp
     * set.  If false\, this option generates a checkpoint with all updates., a boolean flag;
     * default \c true.}
     * @configend
     * @errors
     */
    int __F(close)(WT_CONNECTION *connection, const char *config);

#ifndef DOXYGEN
    /*!
     * Output debug information for various subsystems. The output format
     * may change over time, gathering the debug information may be
     * invasive, and the information reported may not provide a point in
     * time view of the system.
     *
     * @param connection the connection handle
     * @configstart{WT_CONNECTION.debug_info, see dist/api_data.py}
     * @config{backup, print incremental backup information., a boolean flag; default \c false.}
     * @config{cache, print cache information., a boolean flag; default \c false.}
     * @config{cursors, print all open cursor information., a boolean flag; default \c false.}
     * @config{handles, print open handles information., a boolean flag; default \c false.}
     * @config{log, print log information., a boolean flag; default \c false.}
     * @config{sessions, print open session information., a boolean flag; default \c false.}
     * @config{txn, print global txn information., a boolean flag; default \c false.}
     * @configend
     * @errors
     */
    int __F(debug_info)(WT_CONNECTION *connection, const char *config);
#endif

    /*!
     * Reconfigure a connection handle.
     *
     * @snippet ex_all.c Reconfigure a connection
     *
     * @param connection the connection handle
     * @configstart{WT_CONNECTION.reconfigure, see dist/api_data.py}
     * @config{block_cache = (, block cache configuration options., a set of related configuration
     * options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;blkcache_eviction_aggression,
     * seconds an unused block remains in the cache before it is evicted., an integer between \c 1
     * and \c 7200; default \c 1800.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;cache_on_checkpoint, cache
     * blocks written by a checkpoint., a boolean flag; default \c true.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;cache_on_writes, cache blocks as they are written (other than
     * checkpoint blocks)., a boolean flag; default \c true.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;
     * enabled, enable block cache., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;full_target, the fraction of the block cache that must be
     * full before eviction will remove unused blocks., an integer between \c 30 and \c 100; default
     * \c 95.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;hashsize, number of buckets in the hashtable that
     * keeps track of blocks., an integer between \c 512 and \c 256K; default \c 32768.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;max_percent_overhead, maximum tolerated overhead expressed as
     * the number of blocks added and removed as percent of blocks looked up; cache population and
     * eviction will be suppressed if the overhead exceeds the threshold., an integer between \c 1
     * and \c 500; default \c 10.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;nvram_path, the absolute path to
     * the file system mounted on the NVRAM device., a string; default empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;percent_file_in_dram, bypass cache for a file if the set
     * percentage of the file fits in system DRAM (as specified by block_cache.system_ram)., an
     * integer between \c 0 and \c 100; default \c 50.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;size,
     * maximum memory to allocate for the block cache., an integer between \c 0 and \c 10TB; default
     * \c 0.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;system_ram, the bytes of system DRAM available for
     * caching filesystem blocks., an integer between \c 0 and \c 1024GB; default \c 0.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;type, cache location: DRAM or NVRAM., a string; default
     * empty.}
     * @config{ ),,}
     * @config{cache_max_wait_ms, the maximum number of milliseconds an application thread will wait
     * for space to be available in cache before giving up.  Default will wait forever., an integer
     * greater than or equal to \c 0; default \c 0.}
     * @config{cache_overhead, assume the heap allocator overhead is the specified percentage\, and
     * adjust the cache usage by that amount (for example\, if there is 10GB of data in cache\, a
     * percentage of 10 means WiredTiger treats this as 11GB). This value is configurable because
     * different heap allocators have different overhead and different workloads will have different
     * heap allocation sizes and patterns\, therefore applications may need to adjust this value
     * based on allocator choice and behavior in measured workloads., an integer between \c 0 and \c
     * 30; default \c 8.}
     * @config{cache_size, maximum heap memory to allocate for the cache.  A database should
     * configure either \c cache_size or \c shared_cache but not both., an integer between \c 1MB
     * and \c 10TB; default \c 100MB.}
     * @config{cache_stuck_timeout_ms, the number of milliseconds to wait before a stuck cache times
     * out in diagnostic mode.  Default will wait for 5 minutes\, 0 will wait forever., an integer
     * greater than or equal to \c 0; default \c 300000.}
     * @config{checkpoint = (, periodically checkpoint the database.  Enabling the checkpoint server
     * uses a session from the configured \c session_max., a set of related configuration options
     * defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;log_size, wait for this amount of log
     * record bytes to be written to the log between each checkpoint.  If non-zero\, this value will
     * use a minimum of the log file size.  A database can configure both log_size and wait to set
     * an upper bound for checkpoints; setting this value above 0 configures periodic checkpoints.,
     * an integer between \c 0 and \c 2GB; default \c 0.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;wait,
     * seconds to wait between each checkpoint; setting this value above 0 configures periodic
     * checkpoints., an integer between \c 0 and \c 100000; default \c 0.}
     * @config{ ),,}
     * @config{checkpoint_cleanup, control how aggressively obsolete content is removed when
     * creating checkpoints.  Default to none\, which means no additional work is done to find
     * obsolete content., a string\, chosen from the following options: \c "none"\, \c
     * "reclaim_space"; default \c none.}
     * @config{chunk_cache = (, chunk cache reconfiguration options., a set of related configuration
     * options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;pinned, List of "table:" URIs
     * exempt from cache eviction.  Capacity config overrides this\, tables exceeding capacity will
     * not be fully retained.  Table names can appear in both this and the preload list\, but not in
     * both this and the exclude list.  Duplicate names are allowed., a list of strings; default
     * empty.}
     * @config{ ),,}
     * @config{compatibility = (, set compatibility version of database.  Changing the compatibility
     * version requires that there are no active operations for the duration of the call., a set of
     * related configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;release,
     * compatibility release version string., a string; default empty.}
     * @config{ ),,}
     * @config{debug_mode = (, control the settings of various extended debugging features., a set
     * of related configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;
     * background_compact, if true\, background compact aggressively removes compact statistics for
     * a file and decreases the max amount of time a file can be skipped for., a boolean flag;
     * default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;checkpoint_retention, adjust log removal
     * to retain the log records of this number of checkpoints.  Zero or one means perform normal
     * removal., an integer between \c 0 and \c 1024; default \c 0.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;corruption_abort, if true and built in diagnostic mode\, dump
     * core in the case of data corruption., a boolean flag; default \c true.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;cursor_copy, if true\, use the system allocator to make a
     * copy of any data returned by a cursor operation and return the copy instead.  The copy is
     * freed on the next cursor operation.  This allows memory sanitizers to detect inappropriate
     * references to memory owned by cursors., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;cursor_reposition, if true\, for operations with snapshot
     * isolation the cursor temporarily releases any page that requires force eviction\, then
     * repositions back to the page for further operations.  A page release encourages eviction of
     * hot or large pages\, which is more likely to succeed without a cursor keeping the page
     * pinned., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;eviction, if
     * true\, modify internal algorithms to change skew to force history store eviction to happen
     * more aggressively.  This includes but is not limited to not skewing newest\, not favoring
     * leaf pages\, and modifying the eviction score mechanism., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;log_retention, adjust log removal to retain at least this
     * number of log files.  (Warning: this option can remove log files required for recovery if no
     * checkpoints have yet been done and the number of log files exceeds the configured value.  As
     * WiredTiger cannot detect the difference between a system that has not yet checkpointed and
     * one that will never checkpoint\, it might discard log files before any checkpoint is done.)
     * Ignored if set to 0., an integer between \c 0 and \c 1024; default \c 0.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;realloc_exact, if true\, reallocation of memory will only
     * provide the exact amount requested.  This will help with spotting memory allocation issues
     * more easily., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;
     * realloc_malloc, if true\, every realloc call will force a new memory allocation by using
     * malloc., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;rollback_error,
     * return a WT_ROLLBACK error from a transaction operation about every Nth operation to simulate
     * a collision., an integer between \c 0 and \c 10M; default \c 0.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;slow_checkpoint, if true\, slow down checkpoint creation by
     * slowing down internal page processing., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;stress_skiplist, Configure various internal parameters to
     * encourage race conditions and other issues with internal skip lists\, e.g.  using a more
     * dense representation., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;
     * table_logging, if true\, write transaction related information to the log for all
     * operations\, even operations for tables with logging turned off.  This additional logging
     * information is intended for debugging and is informational only\, that is\, it is ignored
     * during recovery., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;
     * tiered_flush_error_continue, on a write to tiered storage\, continue when an error occurs., a
     * boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;update_restore_evict, if
     * true\, control all dirty page evictions through forcing update restore eviction., a boolean
     * flag; default \c false.}
     * @config{ ),,}
     * @config{error_prefix, prefix string for error messages., a string; default empty.}
     * @config{eviction = (, eviction configuration options., a set of related configuration options
     * defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;threads_max, maximum number of threads
     * WiredTiger will start to help evict pages from cache.  The number of threads started will
     * vary depending on the current eviction load.  Each eviction worker thread uses a session from
     * the configured session_max., an integer between \c 1 and \c 20; default \c 8.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;threads_min, minimum number of threads WiredTiger will start
     * to help evict pages from cache.  The number of threads currently running will vary depending
     * on the current eviction load., an integer between \c 1 and \c 20; default \c 1.}
     * @config{
     * ),,}
     * @config{eviction_checkpoint_target, perform eviction at the beginning of checkpoints to bring
     * the dirty content in cache to this level.  It is a percentage of the cache size if the value
     * is within the range of 0 to 100 or an absolute size when greater than 100. The value is not
     * allowed to exceed the \c cache_size.  Ignored if set to zero., an integer between \c 0 and \c
     * 10TB; default \c 1.}
     * @config{eviction_dirty_target, perform eviction in worker threads when the cache contains at
     * least this much dirty content.  It is a percentage of the cache size if the value is within
     * the range of 1 to 100 or an absolute size when greater than 100. The value is not allowed to
     * exceed the \c cache_size and has to be lower than its counterpart \c eviction_dirty_trigger.,
     * an integer between \c 1 and \c 10TB; default \c 5.}
     * @config{eviction_dirty_trigger, trigger application threads to perform eviction when the
     * cache contains at least this much dirty content.  It is a percentage of the cache size if the
     * value is within the range of 1 to 100 or an absolute size when greater than 100. The value is
     * not allowed to exceed the \c cache_size and has to be greater than its counterpart \c
     * eviction_dirty_target.  This setting only alters behavior if it is lower than
     * eviction_trigger., an integer between \c 1 and \c 10TB; default \c 20.}
     * @config{eviction_target, perform eviction in worker threads when the cache contains at least
     * this much content.  It is a percentage of the cache size if the value is within the range of
     * 10 to 100 or an absolute size when greater than 100. The value is not allowed to exceed the
     * \c cache_size and has to be lower than its counterpart \c eviction_trigger., an integer
     * between \c 10 and \c 10TB; default \c 80.}
     * @config{eviction_trigger, trigger application threads to perform eviction when the cache
     * contains at least this much content.  It is a percentage of the cache size if the value is
     * within the range of 10 to 100 or an absolute size when greater than 100. The value is not
     * allowed to exceed the \c cache_size and has to be greater than its counterpart \c
     * eviction_target., an integer between \c 10 and \c 10TB; default \c 95.}
     * @config{eviction_updates_target, perform eviction in worker threads when the cache contains
     * at least this many bytes of updates.  It is a percentage of the cache size if the value is
     * within the range of 0 to 100 or an absolute size when greater than 100. Calculated as half of
     * \c eviction_dirty_target by default.  The value is not allowed to exceed the \c cache_size
     * and has to be lower than its counterpart \c eviction_updates_trigger., an integer between \c
     * 0 and \c 10TB; default \c 0.}
     * @config{eviction_updates_trigger, trigger application threads to perform eviction when the
     * cache contains at least this many bytes of updates.  It is a percentage of the cache size if
     * the value is within the range of 1 to 100 or an absolute size when greater than 100\.
     * Calculated as half of \c eviction_dirty_trigger by default.  The value is not allowed to
     * exceed the \c cache_size and has to be greater than its counterpart \c
     * eviction_updates_target.  This setting only alters behavior if it is lower than \c
     * eviction_trigger., an integer between \c 0 and \c 10TB; default \c 0.}
     * @config{extra_diagnostics, enable additional diagnostics in WiredTiger.  These additional
     * diagnostics include diagnostic assertions that can cause WiredTiger to abort when an invalid
     * state is detected.  Options are given as a list\, such as
     * <code>"extra_diagnostics=[out_of_order\,visibility]"</code>. Choosing \c all enables all
     * assertions.  When WiredTiger is compiled with \c HAVE_DIAGNOSTIC=1 all assertions are enabled
     * and cannot be reconfigured., a list\, with values chosen from the following options: \c
     * "all"\, \c "checkpoint_validate"\, \c "cursor_check"\, \c "disk_validate"\, \c
     * "eviction_check"\, \c "generation_check"\, \c "hs_validate"\, \c "key_out_of_order"\, \c
     * "log_validate"\, \c "prepared"\, \c "slow_operation"\, \c "txn_visibility"; default \c [].}
     * @config{file_manager = (, control how file handles are managed., a set of related
     * configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;
     * close_handle_minimum, number of handles open before the file manager will look for handles to
     * close., an integer greater than or equal to \c 0; default \c 250.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;close_idle_time, amount of time in seconds a file handle
     * needs to be idle before attempting to close it.  A setting of 0 means that idle handles are
     * not closed., an integer between \c 0 and \c 100000; default \c 30.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;close_scan_interval, interval in seconds at which to check
     * for files that are inactive and close them., an integer between \c 1 and \c 100000; default
     * \c 10.}
     * @config{ ),,}
     * @config{generation_drain_timeout_ms, the number of milliseconds to wait for a resource to
     * drain before timing out in diagnostic mode.  Default will wait for 4 minutes\, 0 will wait
     * forever., an integer greater than or equal to \c 0; default \c 240000.}
     * @config{history_store = (, history store configuration options., a set of related
     * configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;file_max, the
     * maximum number of bytes that WiredTiger is allowed to use for its history store mechanism.
     * If the history store file exceeds this size\, a panic will be triggered.  The default value
     * means that the history store file is unbounded and may use as much space as the filesystem
     * will accommodate.  The minimum non-zero setting is 100MB., an integer greater than or equal
     * to \c 0; default \c 0.}
     * @config{ ),,}
     * @config{io_capacity = (, control how many bytes per second are written and read.  Exceeding
     * the capacity results in throttling., a set of related configuration options defined as
     * follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;chunk_cache, number of bytes per second available
     * to the chunk cache.  The minimum non-zero setting is 1MB., an integer between \c 0 and \c
     * 1TB; default \c 0.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;total, number of bytes per second
     * available to all subsystems in total.  When set\, decisions about what subsystems are
     * throttled\, and in what proportion\, are made internally.  The minimum non-zero setting is
     * 1MB., an integer between \c 0 and \c 1TB; default \c 0.}
     * @config{ ),,}
     * @config{json_output, enable JSON formatted messages on the event handler interface.  Options
     * are given as a list\, where each option specifies an event handler category e.g.  'error'
     * represents the messages from the WT_EVENT_HANDLER::handle_error method., a list\, with values
     * chosen from the following options: \c "error"\, \c "message"; default \c [].}
     * @config{log = (, enable logging.  Enabling logging uses three sessions from the configured
     * session_max., a set of related configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;os_cache_dirty_pct, maximum dirty system buffer cache usage\,
     * as a percentage of the log's \c file_max.  If non-zero\, schedule writes for dirty blocks
     * belonging to the log in the system buffer cache after that percentage of the log has been
     * written into the buffer cache without an intervening file sync., an integer between \c 0 and
     * \c 100; default \c 0.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;prealloc, pre-allocate log files., a
     * boolean flag; default \c true.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;remove, automatically remove
     * unneeded log files., a boolean flag; default \c true.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;
     * zero_fill, manually write zeroes into log files., a boolean flag; default \c false.}
     * @config{
     * ),,}
     * @config{lsm_manager = (, configure database wide options for LSM tree management.  The LSM
     * manager is started automatically the first time an LSM tree is opened.  The LSM manager uses
     * a session from the configured session_max., a set of related configuration options defined as
     * follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;merge, merge LSM chunks where possible., a boolean
     * flag; default \c true.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;worker_thread_max, Configure a set of
     * threads to manage merging LSM trees in the database.  Each worker thread uses a session
     * handle from the configured session_max., an integer between \c 3 and \c 20; default \c 4.}
     * @config{ ),,}
     * @config{operation_timeout_ms, this option is no longer supported\, retained for backward
     * compatibility., an integer greater than or equal to \c 0; default \c 0.}
     * @config{operation_tracking = (, enable tracking of performance-critical functions.  See @ref
     * operation_tracking for more information., a set of related configuration options defined as
     * follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, enable operation tracking subsystem., a
     * boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;path, the name of a
     * directory into which operation tracking files are written.  The directory must already exist.
     * If the value is not an absolute path\, the path is relative to the database home (see @ref
     * absolute_path for more information)., a string; default \c ".".}
     * @config{ ),,}
     * @config{shared_cache = (, shared cache configuration options.  A database should configure
     * either a cache_size or a shared_cache not both.  Enabling a shared cache uses a session from
     * the configured session_max.  A shared cache can not have absolute values configured for cache
     * eviction settings., a set of related configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;chunk, the granularity that a shared cache is redistributed.,
     * an integer between \c 1MB and \c 10TB; default \c 10MB.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;
     * name, the name of a cache that is shared between databases or \c "none" when no shared cache
     * is configured., a string; default \c none.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;quota, maximum
     * size of cache this database can be allocated from the shared cache.  Defaults to the entire
     * shared cache size., an integer; default \c 0.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;reserve,
     * amount of cache this database is guaranteed to have available from the shared cache.  This
     * setting is per database.  Defaults to the chunk size., an integer; default \c 0.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;size, maximum memory to allocate for the shared cache.
     * Setting this will update the value if one is already set., an integer between \c 1MB and \c
     * 10TB; default \c 500MB.}
     * @config{ ),,}
     * @config{statistics, Maintain database statistics\, which may impact performance.  Choosing
     * "all" maintains all statistics regardless of cost\, "fast" maintains a subset of statistics
     * that are relatively inexpensive\, "none" turns off all statistics.  The "clear" configuration
     * resets statistics after they are gathered\, where appropriate (for example\, a cache size
     * statistic is not cleared\, while the count of cursor insert operations will be cleared). When
     * "clear" is configured for the database\, gathered statistics are reset each time a statistics
     * cursor is used to gather statistics\, as well as each time statistics are logged using the \c
     * statistics_log configuration.  See @ref statistics for more information., a list\, with
     * values chosen from the following options: \c "all"\, \c "cache_walk"\, \c "fast"\, \c
     * "none"\, \c "clear"\, \c "tree_walk"; default \c none.}
     * @config{statistics_log = (, log any statistics the database is configured to maintain\, to a
     * file.  See @ref statistics for more information.  Enabling the statistics log server uses a
     * session from the configured session_max., a set of related configuration options defined as
     * follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;json, encode statistics in JSON format., a boolean
     * flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;on_close, log statistics on database
     * close., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;sources, if
     * non-empty\, include statistics for the list of data source URIs\, if they are open at the
     * time of the statistics logging.  The list may include URIs matching a single data source
     * ("table:mytable")\, or a URI matching all data sources of a particular type ("table:")., a
     * list of strings; default empty.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;timestamp, a timestamp
     * prepended to each log record.  May contain \c strftime conversion specifications.  When \c
     * json is configured\, defaults to \c "%Y-%m-%dT%H:%M:%S.000Z"., a string; default \c "%b %d
     * %H:%M:%S".}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;wait, seconds to wait between each write of the
     * log records; setting this value above 0 configures statistics logging., an integer between \c
     * 0 and \c 100000; default \c 0.}
     * @config{ ),,}
     * @config{tiered_storage = (, enable tiered storage.  Enabling tiered storage may use one
     * session from the configured session_max., a set of related configuration options defined as
     * follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;local_retention, time in seconds to retain data on
     * tiered storage on the local tier for faster read access., an integer between \c 0 and \c
     * 10000; default \c 300.}
     * @config{ ),,}
     * @config{verbose, enable messages for various subsystems and operations.  Options are given as
     * a list\, where each message type can optionally define an associated verbosity level\, such
     * as <code>"verbose=[evictserver\,read:1\,rts:0]"</code>. Verbosity levels that can be provided
     * include <code>0</code> (INFO) and <code>1</code> through <code>5</code>\, corresponding to
     * (DEBUG_1) to (DEBUG_5). \c all is a special case that defines the verbosity level for all
     * categories not explicitly set in the config string., a list\, with values chosen from the
     * following options: \c "all"\, \c "api"\, \c "backup"\, \c "block"\, \c "block_cache"\, \c
     * "checkpoint"\, \c "checkpoint_cleanup"\, \c "checkpoint_progress"\, \c "chunkcache"\, \c
     * "compact"\, \c "compact_progress"\, \c "configuration"\, \c "error_returns"\, \c "evict"\, \c
     * "evict_stuck"\, \c "evictserver"\, \c "fileops"\, \c "generation"\, \c "handleops"\, \c
     * "history_store"\, \c "history_store_activity"\, \c "log"\, \c "lsm"\, \c "lsm_manager"\, \c
     * "metadata"\, \c "mutex"\, \c "out_of_order"\, \c "overflow"\, \c "read"\, \c "reconcile"\, \c
     * "recovery"\, \c "recovery_progress"\, \c "rts"\, \c "salvage"\, \c "shared_cache"\, \c
     * "split"\, \c "temporary"\, \c "thread_group"\, \c "tiered"\, \c "timestamp"\, \c
     * "transaction"\, \c "verify"\, \c "version"\, \c "write"; default \c [].}
     * @configend
     * @errors
     */
    int __F(reconfigure)(WT_CONNECTION *connection, const char *config);

    /*!
     * The home directory of the connection.
     *
     * @snippet ex_all.c Get the database home directory
     *
     * @param connection the connection handle
     * @returns a pointer to a string naming the home directory
     */
    const char *__F(get_home)(WT_CONNECTION *connection);

    /*!
     * Compile a configuration string to be used with an API.  The string returned by this
     * method can be used with the indicated API call as its configuration argument.
     * Precompiled strings should be used where configuration parsing has proved to be a
     * performance bottleneck. The lifetime of a configuration string ends when the connection
     * is closed. The number of compilation strings that can be made is limited by
     * the \c compile_configuration_count configuration in ::wiredtiger_open .
     *
     * Configuration strings containing '%d' or '%s' can have values bound, see
     * WT_SESSION::bind_configuration.
     *
     * This API may change in future releases.
     *
     * @param connection the connection handle
     * @param method the API to the configuration string applies to, e.g.
     * \c "WT_SESSION.open_cursor"
     * @param str the configuration string to compile
     * @param compiled the returned configuration string
     * @errors
     */
    int __F(compile_configuration)(WT_CONNECTION *connection, const char *method,
        const char *str, const char **compiled);

    /*!
     * Add configuration options for a method.  See
     * @ref custom_ds_config_add for more information.
     *
     * @snippet ex_all.c Configure method configuration
     *
     * @param connection the connection handle
     * @param method the method being configured
     * @param uri the object type or NULL for all object types
     * @param config the additional configuration's name and default value
     * @param type the additional configuration's type (must be one of
     * \c "boolean"\, \c "int", \c "list" or \c "string")
     * @param check the additional configuration check string, or NULL if
     * none
     * @errors
     */
    int __F(configure_method)(WT_CONNECTION *connection,
        const char *method, const char *uri,
        const char *config, const char *type, const char *check);

    /*!
     * Return if opening this handle created the database.
     *
     * @snippet ex_all.c Check if the database is newly created
     *
     * @param connection the connection handle
     * @returns false (zero) if the connection existed before the call to
     * ::wiredtiger_open, true (non-zero) if it was created by opening this
     * handle.
     */
    int __F(is_new)(WT_CONNECTION *connection);

    /*!
     * @name Session handles
     * @{
     */
    /*!
     * Open a session.
     *
     * @snippet ex_all.c Open a session
     *
     * @param connection the connection handle
     * @param event_handler An event handler. If <code>NULL</code>, the
     * connection's event handler is used. See @ref event_message_handling
     * for more information.
     * @configstart{WT_CONNECTION.open_session, see dist/api_data.py}
     * @config{cache_cursors, enable caching of cursors for reuse.  Any calls to WT_CURSOR::close
     * for a cursor created in this session will mark the cursor as cached and keep it available to
     * be reused for later calls to WT_SESSION::open_cursor.  Cached cursors may be eventually
     * closed.  This value is inherited from ::wiredtiger_open \c cache_cursors., a boolean flag;
     * default \c true.}
     * @config{cache_max_wait_ms, the maximum number of milliseconds an application thread will wait
     * for space to be available in cache before giving up.  Default value will be the global
     * setting of the connection config., an integer greater than or equal to \c 0; default \c 0.}
     * @config{debug = (, configure debug specific behavior on a session.  Generally only used for
     * internal testing purposes., a set of related configuration options defined as follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;checkpoint_fail_before_turtle_update, Fail before writing a
     * turtle file at the end of a checkpoint., a boolean flag; default \c false.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;release_evict_page, Configure the session to evict the page
     * when it is released and no longer needed., a boolean flag; default \c false.}
     * @config{ ),,}
     * @config{ignore_cache_size, when set\, operations performed by this session ignore the cache
     * size and are not blocked when the cache is full.  Note that use of this option for operations
     * that create cache pressure can starve ordinary sessions that obey the cache size., a boolean
     * flag; default \c false.}
     * @config{isolation, the default isolation level for operations in this session., a string\,
     * chosen from the following options: \c "read-uncommitted"\, \c "read-committed"\, \c
     * "snapshot"; default \c snapshot.}
     * @config{prefetch = (, Enable automatic detection of scans by applications\, and attempt to
     * pre-fetch future content into the cache., a set of related configuration options defined as
     * follows.}
     * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, whether pre-fetch is enabled for this
     * session., a boolean flag; default \c false.}
     * @config{ ),,}
     * @configend
     * @param[out] sessionp the new session handle
     * @errors
     */
    int __F(open_session)(WT_CONNECTION *connection,
        WT_EVENT_HANDLER *event_handler, const char *config,
        WT_SESSION **sessionp);
    /*! @} */

    /*!
     * @name Transactions
     * @{
     */
    /*!
     * Query the global transaction timestamp state.
     *
     * @snippet ex_all.c query timestamp
     *
     * @param connection the connection handle
     * @param[out] hex_timestamp a buffer that will be set to the
     * hexadecimal encoding of the timestamp being queried.  Must be large
     * enough to hold a NUL terminated, hex-encoded 8B timestamp (17 bytes).
     * @configstart{WT_CONNECTION.query_timestamp, see dist/api_data.py}
     * @config{get, specify which timestamp to query: \c all_durable returns the largest timestamp
     * such that all timestamps up to and including that value have been committed (possibly bounded
     * by the application-set \c durable timestamp); \c last_checkpoint returns the timestamp of the
     * most recent stable checkpoint; \c oldest_timestamp returns the most recent \c
     * oldest_timestamp set with WT_CONNECTION::set_timestamp; \c oldest_reader returns the minimum
     * of the read timestamps of all active readers; \c pinned returns the minimum of the \c
     * oldest_timestamp and the read timestamps of all active readers; \c recovery returns the
     * timestamp of the most recent stable checkpoint taken prior to a shutdown; \c stable_timestamp
     * returns the most recent \c stable_timestamp set with WT_CONNECTION::set_timestamp.  (The \c
     * oldest and \c stable arguments are deprecated short-hand for \c oldest_timestamp and \c
     * stable_timestamp\, respectively.) See @ref timestamp_global_api., a string\, chosen from the
     * following options: \c "all_durable"\, \c "last_checkpoint"\, \c "oldest"\, \c
     * "oldest_reader"\, \c "oldest_timestamp"\, \c "pinned"\, \c "recovery"\, \c "stable"\, \c
     * "stable_timestamp"; default \c all_durable.}
     * @configend
     *
     * A timestamp of 0 is returned if the timestamp is not available or has not been set.
     * @errors
     */
    int __F(query_timestamp)(
        WT_CONNECTION *connection, char *hex_timestamp, const char *config);

    /*!
     * Set a global transaction timestamp.
     *
     * @snippet ex_all.c set durable timestamp
     *
     * @snippet ex_all.c set oldest timestamp
     *
     * @snippet ex_all.c set stable timestamp
     *
     * @param connection the connection handle
     * @configstart{WT_CONNECTION.set_timestamp, see dist/api_data.py}
     * @config{durable_timestamp, temporarily set the system's maximum durable timestamp\, bounding
     * the timestamp returned by WT_CONNECTION::query_timestamp with the \c all_durable
     * configuration.  Calls to WT_CONNECTION::query_timestamp will ignore durable timestamps
     * greater than the specified value until a subsequent transaction commit advances the maximum
     * durable timestamp\, or rollback-to-stable resets the value.  See @ref timestamp_global_api.,
     * a string; default empty.}
     * @config{oldest_timestamp, future commits and queries will be no earlier than the specified
     * timestamp.  Values must be monotonically increasing; any attempt to set the value to older
     * than the current is silently ignored.  The value must not be newer than the current stable
     * timestamp.  See @ref timestamp_global_api., a string; default empty.}
     * @config{stable_timestamp, checkpoints will not include commits that are newer than the
     * specified timestamp in tables configured with \c "log=(enabled=false)". Values must be
     * monotonically increasing; any attempt to set the value to older than the current is silently
     * ignored.  The value must not be older than the current oldest timestamp.  See @ref
     * timestamp_global_api., a string; default empty.}
     * @configend
     * @errors
     */
    int __F(set_timestamp)(
        WT_CONNECTION *connection, const char *config);

    /*!
     * Rollback tables to an earlier point in time, discarding all updates to checkpoint durable
     * tables that have commit times more recent than the current global stable timestamp.
     *
     * No updates made to logged tables or updates made without an associated commit timestamp
     * will be discarded. See @ref timestamp_misc.
     *
     * Applications must resolve all running transactions and close or reset all open cursors
     * before the call, and no other API calls should be made for the duration of the call.
     *
     * @snippet ex_all.c rollback to stable
     *
     * @param connection the connection handle
     * @configstart{WT_CONNECTION.rollback_to_stable, see dist/api_data.py}
     * @config{dryrun, perform the checks associated with RTS\, but don't modify any data., a
     * boolean flag; default \c false.}
     * @configend
     * @errors
     * An error should occur only in the case of a system problem, and an application typically
     * will retry WT_CONNECTION::rollback_to_stable on error, or fail outright.
     */
    int __F(rollback_to_stable)(
        WT_CONNECTION *connection, const char *config);

    /*! @} */

    /*!
     * @name Extensions
     * @{
     */
    /*!
     * Load an extension.
     *
     * @snippet ex_all.c Load an extension
     *
     * @param connection the connection handle
     * @param path the filename of the extension module, or \c "local" to
     * search the current application binary for the initialization
     * function, see @ref extensions for more details.
     * @configstart{WT_CONNECTION.load_extension, see dist/api_data.py}
     * @config{config, configuration string passed to the entry point of the extension as its
     * WT_CONFIG_ARG argument., a string; default empty.}
     * @config{early_load, whether this extension should be loaded at the beginning of
     * ::wiredtiger_open.  Only applicable to extensions loaded via the wiredtiger_open
     * configurations string., a boolean flag; default \c false.}
     * @config{entry, the entry point of the extension\, called to initialize the extension when it
     * is loaded.  The signature of the function must match ::wiredtiger_extension_init., a string;
     * default \c wiredtiger_extension_init.}
     * @config{terminate, an optional function in the extension that is called before the extension
     * is unloaded during WT_CONNECTION::close.  The signature of the function must match
     * ::wiredtiger_extension_terminate., a string; default \c wiredtiger_extension_terminate.}
     * @configend
     * @errors
     */
    int __F(load_extension)(WT_CONNECTION *connection,
        const char *path, const char *config);

    /*!
     * Add a custom data source.  See @ref custom_data_sources for more
     * information.
     *
     * The application must first implement the WT_DATA_SOURCE interface
     * and then register the implementation with WiredTiger:
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE register
     *
     * @param connection the connection handle
     * @param prefix the URI prefix for this data source, e.g., "file:"
     * @param data_source the application-supplied implementation of
     *  WT_DATA_SOURCE to manage this data source.
     * @configempty{WT_CONNECTION.add_data_source, see dist/api_data.py}
     * @errors
     */
    int __F(add_data_source)(WT_CONNECTION *connection, const char *prefix,
        WT_DATA_SOURCE *data_source, const char *config);

    /*!
     * Add a custom collation function.
     *
     * The application must first implement the WT_COLLATOR interface and
     * then register the implementation with WiredTiger:
     *
     * @snippet ex_all.c WT_COLLATOR register
     *
     * @param connection the connection handle
     * @param name the name of the collation to be used in calls to
     *  WT_SESSION::create, may not be \c "none"
     * @param collator the application-supplied collation handler
     * @configempty{WT_CONNECTION.add_collator, see dist/api_data.py}
     * @errors
     */
    int __F(add_collator)(WT_CONNECTION *connection,
        const char *name, WT_COLLATOR *collator, const char *config);

    /*!
     * Add a compression function.
     *
     * The application must first implement the WT_COMPRESSOR interface
     * and then register the implementation with WiredTiger:
     *
     * @snippet nop_compress.c WT_COMPRESSOR initialization structure
     *
     * @snippet nop_compress.c WT_COMPRESSOR initialization function
     *
     * @param connection the connection handle
     * @param name the name of the compression function to be used in calls
     *  to WT_SESSION::create, may not be \c "none"
     * @param compressor the application-supplied compression handler
     * @configempty{WT_CONNECTION.add_compressor, see dist/api_data.py}
     * @errors
     */
    int __F(add_compressor)(WT_CONNECTION *connection,
        const char *name, WT_COMPRESSOR *compressor, const char *config);

    /*!
     * Add an encryption function.
     *
     * The application must first implement the WT_ENCRYPTOR interface
     * and then register the implementation with WiredTiger:
     *
     * @snippet nop_encrypt.c WT_ENCRYPTOR initialization structure
     *
     * @snippet nop_encrypt.c WT_ENCRYPTOR initialization function
     *
     * @param connection the connection handle
     * @param name the name of the encryption function to be used in calls
     *  to WT_SESSION::create, may not be \c "none"
     * @param encryptor the application-supplied encryption handler
     * @configempty{WT_CONNECTION.add_encryptor, see dist/api_data.py}
     * @errors
     */
    int __F(add_encryptor)(WT_CONNECTION *connection,
        const char *name, WT_ENCRYPTOR *encryptor, const char *config);

    /*!
     * Add a custom extractor for index keys or column groups.
     *
     * The application must first implement the WT_EXTRACTOR interface and
     * then register the implementation with WiredTiger:
     *
     * @snippet ex_all.c WT_EXTRACTOR register
     *
     * @param connection the connection handle
     * @param name the name of the extractor to be used in calls to
     *  WT_SESSION::create, may not be \c "none"
     * @param extractor the application-supplied extractor
     * @configempty{WT_CONNECTION.add_extractor, see dist/api_data.py}
     * @errors
     */
    int __F(add_extractor)(WT_CONNECTION *connection, const char *name,
        WT_EXTRACTOR *extractor, const char *config);

    /*!
     * Configure a custom file system.
     *
     * This method can only be called from an early loaded extension
     * module. The application must first implement the WT_FILE_SYSTEM
     * interface and then register the implementation with WiredTiger:
     *
     * @snippet ex_file_system.c WT_FILE_SYSTEM register
     *
     * @param connection the connection handle
     * @param fs the populated file system structure
     * @configempty{WT_CONNECTION.set_file_system, see dist/api_data.py}
     * @errors
     */
    int __F(set_file_system)(
        WT_CONNECTION *connection, WT_FILE_SYSTEM *fs, const char *config);

#if !defined(DOXYGEN)
#if !defined(SWIG)
    /*!
     * Add a storage source implementation.
     *
     * The application must first implement the WT_STORAGE_SOURCE
     * interface and then register the implementation with WiredTiger:
     *
     * @snippet ex_storage_source.c WT_STORAGE_SOURCE register
     *
     * @param connection the connection handle
     * @param name the name of the storage source implementation
     * @param storage_source the populated storage source structure
     * @configempty{WT_CONNECTION.add_storage_source, see dist/api_data.py}
     * @errors
     */
    int __F(add_storage_source)(WT_CONNECTION *connection, const char *name,
        WT_STORAGE_SOURCE *storage_source, const char *config);
#endif

    /*!
     * Get a storage source implementation.
     *
     * Look up a storage source by name and return it. The returned storage source
     * must be released by calling WT_STORAGE_SOURCE::terminate.
     *
     * @snippet ex_storage_source.c WT_STORAGE_SOURCE register
     *
     * @param connection the connection handle
     * @param name the name of the storage source implementation
     * @param storage_source the storage source structure
     * @errors
     */
    int __F(get_storage_source)(WT_CONNECTION *connection, const char *name,
        WT_STORAGE_SOURCE **storage_sourcep);
#endif

    /*!
     * Return a reference to the WiredTiger extension functions.
     *
     * @snippet ex_data_source.c WT_EXTENSION_API declaration
     *
     * @param wt_conn the WT_CONNECTION handle
     * @returns a reference to a WT_EXTENSION_API structure.
     */
    WT_EXTENSION_API *__F(get_extension_api)(WT_CONNECTION *wt_conn);
    /*! @} */
};

/*!
 * Open a connection to a database.
 *
 * @snippet ex_all.c Open a connection
 *
 * @param home The path to the database home directory.  See @ref home
 * for more information.
 * @param event_handler An event handler. If <code>NULL</code>, a default
 * event handler is installed that writes error messages to stderr. See
 * @ref event_message_handling for more information.
 * @configstart{wiredtiger_open, see dist/api_data.py}
 * @config{backup_restore_target, If non-empty and restoring from a backup\, restore only the table
 * object targets listed.  WiredTiger will remove all the metadata entries for the tables that are
 * not listed in the list from the reconstructed metadata.  The target list must include URIs of
 * type \c table:., a list of strings; default empty.}
 * @config{block_cache = (, block cache configuration options., a set of related configuration
 * options defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;blkcache_eviction_aggression,
 * seconds an unused block remains in the cache before it is evicted., an integer between \c 1 and
 * \c 7200; default \c 1800.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;cache_on_checkpoint, cache blocks
 * written by a checkpoint., a boolean flag; default \c true.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;
 * cache_on_writes, cache blocks as they are written (other than checkpoint blocks)., a boolean
 * flag; default \c true.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, enable block cache., a boolean
 * flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;full_target, the fraction of the block
 * cache that must be full before eviction will remove unused blocks., an integer between \c 30 and
 * \c 100; default \c 95.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;hashsize, number of buckets in the
 * hashtable that keeps track of blocks., an integer between \c 512 and \c 256K; default \c 32768.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;max_percent_overhead, maximum tolerated overhead expressed as the
 * number of blocks added and removed as percent of blocks looked up; cache population and eviction
 * will be suppressed if the overhead exceeds the threshold., an integer between \c 1 and \c 500;
 * default \c 10.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;nvram_path, the absolute path to the file system
 * mounted on the NVRAM device., a string; default empty.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;
 * percent_file_in_dram, bypass cache for a file if the set percentage of the file fits in system
 * DRAM (as specified by block_cache.system_ram)., an integer between \c 0 and \c 100; default \c
 * 50.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;size, maximum memory to allocate for the block cache., an
 * integer between \c 0 and \c 10TB; default \c 0.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;system_ram, the
 * bytes of system DRAM available for caching filesystem blocks., an integer between \c 0 and \c
 * 1024GB; default \c 0.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;type, cache location: DRAM or NVRAM., a
 * string; default empty.}
 * @config{ ),,}
 * @config{buffer_alignment, in-memory alignment (in bytes) for buffers used for I/O. The default
 * value of -1 indicates a platform-specific alignment value should be used (4KB on Linux systems
 * when direct I/O is configured\, zero elsewhere). If the configured alignment is larger than
 * default or configured object page sizes\, file allocation and page sizes are silently increased
 * to the buffer alignment size.  Requires the \c posix_memalign API. See @ref
 * tuning_system_buffer_cache_direct_io., an integer between \c -1 and \c 1MB; default \c -1.}
 * @config{builtin_extension_config, A structure where the keys are the names of builtin extensions
 * and the values are passed to WT_CONNECTION::load_extension as the \c config parameter (for
 * example\, <code>builtin_extension_config={zlib={compression_level=3}}</code>)., a string; default
 * empty.}
 * @config{cache_cursors, enable caching of cursors for reuse.  This is the default value for any
 * sessions created\, and can be overridden in configuring \c cache_cursors in
 * WT_CONNECTION.open_session., a boolean flag; default \c true.}
 * @config{cache_max_wait_ms, the maximum number of milliseconds an application thread will wait for
 * space to be available in cache before giving up.  Default will wait forever., an integer greater
 * than or equal to \c 0; default \c 0.}
 * @config{cache_overhead, assume the heap allocator overhead is the specified percentage\, and
 * adjust the cache usage by that amount (for example\, if there is 10GB of data in cache\, a
 * percentage of 10 means WiredTiger treats this as 11GB). This value is configurable because
 * different heap allocators have different overhead and different workloads will have different
 * heap allocation sizes and patterns\, therefore applications may need to adjust this value based
 * on allocator choice and behavior in measured workloads., an integer between \c 0 and \c 30;
 * default \c 8.}
 * @config{cache_size, maximum heap memory to allocate for the cache.  A database should configure
 * either \c cache_size or \c shared_cache but not both., an integer between \c 1MB and \c 10TB;
 * default \c 100MB.}
 * @config{cache_stuck_timeout_ms, the number of milliseconds to wait before a stuck cache times out
 * in diagnostic mode.  Default will wait for 5 minutes\, 0 will wait forever., an integer greater
 * than or equal to \c 0; default \c 300000.}
 * @config{checkpoint = (, periodically checkpoint the database.  Enabling the checkpoint server
 * uses a session from the configured \c session_max., a set of related configuration options
 * defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;log_size, wait for this amount of log record
 * bytes to be written to the log between each checkpoint.  If non-zero\, this value will use a
 * minimum of the log file size.  A database can configure both log_size and wait to set an upper
 * bound for checkpoints; setting this value above 0 configures periodic checkpoints., an integer
 * between \c 0 and \c 2GB; default \c 0.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;wait, seconds to wait
 * between each checkpoint; setting this value above 0 configures periodic checkpoints., an integer
 * between \c 0 and \c 100000; default \c 0.}
 * @config{ ),,}
 * @config{checkpoint_cleanup, control how aggressively obsolete content is removed when creating
 * checkpoints.  Default to none\, which means no additional work is done to find obsolete content.,
 * a string\, chosen from the following options: \c "none"\, \c "reclaim_space"; default \c none.}
 * @config{checkpoint_sync, flush files to stable storage when closing or writing checkpoints., a
 * boolean flag; default \c true.}
 * @config{chunk_cache = (, chunk cache configuration options., a set of related configuration
 * options defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;capacity, maximum memory or storage
 * to use for the chunk cache., an integer between \c 512KB and \c 100TB; default \c 10GB.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;chunk_cache_evict_trigger, chunk cache percent full that triggers
 * eviction., an integer between \c 0 and \c 100; default \c 90.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;
 * chunk_size, size of cached chunks., an integer between \c 512KB and \c 100GB; default \c 1MB.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, enable chunk cache., a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;hashsize, number of buckets in the hashtable that keeps track of
 * objects., an integer between \c 64 and \c 1048576; default \c 1024.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;pinned, List of "table:" URIs exempt from cache eviction.
 * Capacity config overrides this\, tables exceeding capacity will not be fully retained.  Table
 * names can appear in both this and the preload list\, but not in both this and the exclude list.
 * Duplicate names are allowed., a list of strings; default empty.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;
 * storage_path, the path (absolute or relative) to the file used as cache location.  This should be
 * on a filesystem that supports file truncation.  All filesystems in common use meet this
 * criteria., a string; default empty.}
 * @config{ ),,}
 * @config{compatibility = (, set compatibility version of database.  Changing the compatibility
 * version requires that there are no active operations for the duration of the call., a set of
 * related configuration options defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;release,
 * compatibility release version string., a string; default empty.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;
 * require_max, required maximum compatibility version of existing data files.  Must be greater than
 * or equal to any release version set in the \c release setting.  Has no effect if creating the
 * database., a string; default empty.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;require_min, required
 * minimum compatibility version of existing data files.  Must be less than or equal to any release
 * version set in the \c release setting.  Has no effect if creating the database., a string;
 * default empty.}
 * @config{ ),,}
 * @config{compile_configuration_count, the number of configuration strings that can be precompiled.
 * Some configuration strings are compiled internally when the connection is opened., an integer
 * greater than or equal to \c 500; default \c 1000.}
 * @config{config_base, write the base configuration file if creating the database.  If \c false in
 * the config passed directly to ::wiredtiger_open\, will ignore any existing base configuration
 * file in addition to not creating one.  See @ref config_base for more information., a boolean
 * flag; default \c true.}
 * @config{create, create the database if it does not exist., a boolean flag; default \c false.}
 * @config{debug_mode = (, control the settings of various extended debugging features., a set of
 * related configuration options defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;
 * background_compact, if true\, background compact aggressively removes compact statistics for a
 * file and decreases the max amount of time a file can be skipped for., a boolean flag; default \c
 * false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;checkpoint_retention, adjust log removal to retain the
 * log records of this number of checkpoints.  Zero or one means perform normal removal., an integer
 * between \c 0 and \c 1024; default \c 0.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;corruption_abort, if
 * true and built in diagnostic mode\, dump core in the case of data corruption., a boolean flag;
 * default \c true.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;cursor_copy, if true\, use the system allocator
 * to make a copy of any data returned by a cursor operation and return the copy instead.  The copy
 * is freed on the next cursor operation.  This allows memory sanitizers to detect inappropriate
 * references to memory owned by cursors., a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;cursor_reposition, if true\, for operations with snapshot
 * isolation the cursor temporarily releases any page that requires force eviction\, then
 * repositions back to the page for further operations.  A page release encourages eviction of hot
 * or large pages\, which is more likely to succeed without a cursor keeping the page pinned., a
 * boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;eviction, if true\, modify
 * internal algorithms to change skew to force history store eviction to happen more aggressively.
 * This includes but is not limited to not skewing newest\, not favoring leaf pages\, and modifying
 * the eviction score mechanism., a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;log_retention, adjust log removal to retain at least this number
 * of log files.  (Warning: this option can remove log files required for recovery if no checkpoints
 * have yet been done and the number of log files exceeds the configured value.  As WiredTiger
 * cannot detect the difference between a system that has not yet checkpointed and one that will
 * never checkpoint\, it might discard log files before any checkpoint is done.) Ignored if set to
 * 0., an integer between \c 0 and \c 1024; default \c 0.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;
 * realloc_exact, if true\, reallocation of memory will only provide the exact amount requested.
 * This will help with spotting memory allocation issues more easily., a boolean flag; default \c
 * false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;realloc_malloc, if true\, every realloc call will force a
 * new memory allocation by using malloc., a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;rollback_error, return a WT_ROLLBACK error from a transaction
 * operation about every Nth operation to simulate a collision., an integer between \c 0 and \c 10M;
 * default \c 0.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;slow_checkpoint, if true\, slow down checkpoint
 * creation by slowing down internal page processing., a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;stress_skiplist, Configure various internal parameters to
 * encourage race conditions and other issues with internal skip lists\, e.g.  using a more dense
 * representation., a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;
 * table_logging, if true\, write transaction related information to the log for all operations\,
 * even operations for tables with logging turned off.  This additional logging information is
 * intended for debugging and is informational only\, that is\, it is ignored during recovery., a
 * boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;tiered_flush_error_continue, on
 * a write to tiered storage\, continue when an error occurs., a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;update_restore_evict, if true\, control all dirty page evictions
 * through forcing update restore eviction., a boolean flag; default \c false.}
 * @config{ ),,}
 * @config{direct_io, Use \c O_DIRECT on POSIX systems\, and \c FILE_FLAG_NO_BUFFERING on Windows to
 * access files.  Options are given as a list\, such as <code>"direct_io=[data]"</code>. Configuring
 * \c direct_io requires care; see @ref tuning_system_buffer_cache_direct_io for important warnings.
 * Including \c "data" will cause WiredTiger data files\, including WiredTiger internal data files\,
 * to use direct I/O; including \c "log" will cause WiredTiger log files to use direct I/O;
 * including \c "checkpoint" will cause WiredTiger data files opened using a (read-only) checkpoint
 * cursor to use direct I/O. \c direct_io should be combined with \c write_through to get the
 * equivalent of \c O_DIRECT on Windows., a list\, with values chosen from the following options: \c
 * "checkpoint"\, \c "data"\, \c "log"; default empty.}
 * @config{encryption = (, configure an encryptor for system wide metadata and logs.  If a system
 * wide encryptor is set\, it is also used for encrypting data files and tables\, unless encryption
 * configuration is explicitly set for them when they are created with WT_SESSION::create., a set of
 * related configuration options defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;keyid, An
 * identifier that identifies a unique instance of the encryptor.  It is stored in clear text\, and
 * thus is available when the WiredTiger database is reopened.  On the first use of a (name\, keyid)
 * combination\, the WT_ENCRYPTOR::customize function is called with the keyid as an argument., a
 * string; default empty.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;name, Permitted values are \c "none" or a
 * custom encryption engine name created with WT_CONNECTION::add_encryptor.  See @ref encryption for
 * more information., a string; default \c none.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;secretkey, A
 * string that is passed to the WT_ENCRYPTOR::customize function.  It is never stored in clear
 * text\, so must be given to any subsequent ::wiredtiger_open calls to reopen the database.  It
 * must also be provided to any "wt" commands used with this database., a string; default empty.}
 * @config{ ),,}
 * @config{error_prefix, prefix string for error messages., a string; default empty.}
 * @config{eviction = (, eviction configuration options., a set of related configuration options
 * defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;threads_max, maximum number of threads
 * WiredTiger will start to help evict pages from cache.  The number of threads started will vary
 * depending on the current eviction load.  Each eviction worker thread uses a session from the
 * configured session_max., an integer between \c 1 and \c 20; default \c 8.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;threads_min, minimum number of threads WiredTiger will start to
 * help evict pages from cache.  The number of threads currently running will vary depending on the
 * current eviction load., an integer between \c 1 and \c 20; default \c 1.}
 * @config{ ),,}
 * @config{eviction_checkpoint_target, perform eviction at the beginning of checkpoints to bring the
 * dirty content in cache to this level.  It is a percentage of the cache size if the value is
 * within the range of 0 to 100 or an absolute size when greater than 100. The value is not allowed
 * to exceed the \c cache_size.  Ignored if set to zero., an integer between \c 0 and \c 10TB;
 * default \c 1.}
 * @config{eviction_dirty_target, perform eviction in worker threads when the cache contains at
 * least this much dirty content.  It is a percentage of the cache size if the value is within the
 * range of 1 to 100 or an absolute size when greater than 100. The value is not allowed to exceed
 * the \c cache_size and has to be lower than its counterpart \c eviction_dirty_trigger., an integer
 * between \c 1 and \c 10TB; default \c 5.}
 * @config{eviction_dirty_trigger, trigger application threads to perform eviction when the cache
 * contains at least this much dirty content.  It is a percentage of the cache size if the value is
 * within the range of 1 to 100 or an absolute size when greater than 100. The value is not allowed
 * to exceed the \c cache_size and has to be greater than its counterpart \c eviction_dirty_target.
 * This setting only alters behavior if it is lower than eviction_trigger., an integer between \c 1
 * and \c 10TB; default \c 20.}
 * @config{eviction_target, perform eviction in worker threads when the cache contains at least this
 * much content.  It is a percentage of the cache size if the value is within the range of 10 to 100
 * or an absolute size when greater than 100. The value is not allowed to exceed the \c cache_size
 * and has to be lower than its counterpart \c eviction_trigger., an integer between \c 10 and \c
 * 10TB; default \c 80.}
 * @config{eviction_trigger, trigger application threads to perform eviction when the cache contains
 * at least this much content.  It is a percentage of the cache size if the value is within the
 * range of 10 to 100 or an absolute size when greater than 100. The value is not allowed to exceed
 * the \c cache_size and has to be greater than its counterpart \c eviction_target., an integer
 * between \c 10 and \c 10TB; default \c 95.}
 * @config{eviction_updates_target, perform eviction in worker threads when the cache contains at
 * least this many bytes of updates.  It is a percentage of the cache size if the value is within
 * the range of 0 to 100 or an absolute size when greater than 100. Calculated as half of \c
 * eviction_dirty_target by default.  The value is not allowed to exceed the \c cache_size and has
 * to be lower than its counterpart \c eviction_updates_trigger., an integer between \c 0 and \c
 * 10TB; default \c 0.}
 * @config{eviction_updates_trigger, trigger application threads to perform eviction when the cache
 * contains at least this many bytes of updates.  It is a percentage of the cache size if the value
 * is within the range of 1 to 100 or an absolute size when greater than 100\. Calculated as half of
 * \c eviction_dirty_trigger by default.  The value is not allowed to exceed the \c cache_size and
 * has to be greater than its counterpart \c eviction_updates_target.  This setting only alters
 * behavior if it is lower than \c eviction_trigger., an integer between \c 0 and \c 10TB; default
 * \c 0.}
 * @config{exclusive, fail if the database already exists\, generally used with the \c create
 * option., a boolean flag; default \c false.}
 * @config{extensions, list of shared library extensions to load (using dlopen). Any values
 * specified to a library extension are passed to WT_CONNECTION::load_extension as the \c config
 * parameter (for example\, <code>extensions=(/path/ext.so={entry=my_entry})</code>)., a list of
 * strings; default empty.}
 * @config{extra_diagnostics, enable additional diagnostics in WiredTiger.  These additional
 * diagnostics include diagnostic assertions that can cause WiredTiger to abort when an invalid
 * state is detected.  Options are given as a list\, such as
 * <code>"extra_diagnostics=[out_of_order\,visibility]"</code>. Choosing \c all enables all
 * assertions.  When WiredTiger is compiled with \c HAVE_DIAGNOSTIC=1 all assertions are enabled and
 * cannot be reconfigured., a list\, with values chosen from the following options: \c "all"\, \c
 * "checkpoint_validate"\, \c "cursor_check"\, \c "disk_validate"\, \c "eviction_check"\, \c
 * "generation_check"\, \c "hs_validate"\, \c "key_out_of_order"\, \c "log_validate"\, \c
 * "prepared"\, \c "slow_operation"\, \c "txn_visibility"; default \c [].}
 * @config{file_extend, file size extension configuration.  If set\, extend files of the given type
 * in allocations of the given size\, instead of a block at a time as each new block is written.
 * For example\, <code>file_extend=(data=16MB)</code>. If set to 0\, disable file size extension for
 * the given type.  For log files\, the allowed range is between 100KB and 2GB; values larger than
 * the configured maximum log size and the default config would extend log files in allocations of
 * the maximum log file size., a list\, with values chosen from the following options: \c "data"\,
 * \c "log"; default empty.}
 * @config{file_manager = (, control how file handles are managed., a set of related configuration
 * options defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;close_handle_minimum, number of
 * handles open before the file manager will look for handles to close., an integer greater than or
 * equal to \c 0; default \c 250.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;close_idle_time, amount of time
 * in seconds a file handle needs to be idle before attempting to close it.  A setting of 0 means
 * that idle handles are not closed., an integer between \c 0 and \c 100000; default \c 30.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;close_scan_interval, interval in seconds at which to check for
 * files that are inactive and close them., an integer between \c 1 and \c 100000; default \c 10.}
 * @config{ ),,}
 * @config{generation_drain_timeout_ms, the number of milliseconds to wait for a resource to drain
 * before timing out in diagnostic mode.  Default will wait for 4 minutes\, 0 will wait forever., an
 * integer greater than or equal to \c 0; default \c 240000.}
 * @config{hash = (, manage resources used by hash bucket arrays.  All values must be a power of
 * two.  Note that setting large values can significantly increase memory usage inside WiredTiger.,
 * a set of related configuration options defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;
 * buckets, configure the number of hash buckets for most system hash arrays., an integer between \c
 * 64 and \c 65536; default \c 512.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;dhandle_buckets, configure the
 * number of hash buckets for hash arrays relating to data handles., an integer between \c 64 and \c
 * 65536; default \c 512.}
 * @config{ ),,}
 * @config{history_store = (, history store configuration options., a set of related configuration
 * options defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;file_max, the maximum number of
 * bytes that WiredTiger is allowed to use for its history store mechanism.  If the history store
 * file exceeds this size\, a panic will be triggered.  The default value means that the history
 * store file is unbounded and may use as much space as the filesystem will accommodate.  The
 * minimum non-zero setting is 100MB., an integer greater than or equal to \c 0; default \c 0.}
 * @config{ ),,}
 * @config{in_memory, keep data in memory only.  See @ref in_memory for more information., a boolean
 * flag; default \c false.}
 * @config{io_capacity = (, control how many bytes per second are written and read.  Exceeding the
 * capacity results in throttling., a set of related configuration options defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;chunk_cache, number of bytes per second available to the chunk
 * cache.  The minimum non-zero setting is 1MB., an integer between \c 0 and \c 1TB; default \c 0.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;total, number of bytes per second available to all subsystems in
 * total.  When set\, decisions about what subsystems are throttled\, and in what proportion\, are
 * made internally.  The minimum non-zero setting is 1MB., an integer between \c 0 and \c 1TB;
 * default \c 0.}
 * @config{ ),,}
 * @config{json_output, enable JSON formatted messages on the event handler interface.  Options are
 * given as a list\, where each option specifies an event handler category e.g.  'error' represents
 * the messages from the WT_EVENT_HANDLER::handle_error method., a list\, with values chosen from
 * the following options: \c "error"\, \c "message"; default \c [].}
 * @config{log = (, enable logging.  Enabling logging uses three sessions from the configured
 * session_max., a set of related configuration options defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;compressor, configure a compressor for log records.  Permitted
 * values are \c "none" or a custom compression engine name created with
 * WT_CONNECTION::add_compressor.  If WiredTiger has builtin support for \c "lz4"\, \c "snappy"\, \c
 * "zlib" or \c "zstd" compression\, these names are also available.  See @ref compression for more
 * information., a string; default \c none.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, enable logging
 * subsystem., a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;file_max, the
 * maximum size of log files., an integer between \c 100KB and \c 2GB; default \c 100MB.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;os_cache_dirty_pct, maximum dirty system buffer cache usage\, as
 * a percentage of the log's \c file_max.  If non-zero\, schedule writes for dirty blocks belonging
 * to the log in the system buffer cache after that percentage of the log has been written into the
 * buffer cache without an intervening file sync., an integer between \c 0 and \c 100; default \c
 * 0.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;path, the name of a directory into which log files are
 * written.  The directory must already exist.  If the value is not an absolute path\, the path is
 * relative to the database home (see @ref absolute_path for more information)., a string; default
 * \c ".".}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;prealloc, pre-allocate log files., a boolean flag;
 * default \c true.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;recover, run recovery or fail with an error if
 * recovery needs to run after an unclean shutdown., a string\, chosen from the following options:
 * \c "error"\, \c "on"; default \c on.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;remove, automatically
 * remove unneeded log files., a boolean flag; default \c true.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;
 * zero_fill, manually write zeroes into log files., a boolean flag; default \c false.}
 * @config{
 * ),,}
 * @config{lsm_manager = (, configure database wide options for LSM tree management.  The LSM
 * manager is started automatically the first time an LSM tree is opened.  The LSM manager uses a
 * session from the configured session_max., a set of related configuration options defined as
 * follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;merge, merge LSM chunks where possible., a boolean
 * flag; default \c true.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;worker_thread_max, Configure a set of
 * threads to manage merging LSM trees in the database.  Each worker thread uses a session handle
 * from the configured session_max., an integer between \c 3 and \c 20; default \c 4.}
 * @config{ ),,}
 * @config{mmap, Use memory mapping when accessing files in a read-only mode., a boolean flag;
 * default \c true.}
 * @config{mmap_all, Use memory mapping to read and write all data files.  May not be configured
 * with direct I/O., a boolean flag; default \c false.}
 * @config{multiprocess, permit sharing between processes (will automatically start an RPC server
 * for primary processes and use RPC for secondary processes). <b>Not yet supported in
 * WiredTiger</b>., a boolean flag; default \c false.}
 * @config{operation_timeout_ms, this option is no longer supported\, retained for backward
 * compatibility., an integer greater than or equal to \c 0; default \c 0.}
 * @config{operation_tracking = (, enable tracking of performance-critical functions.  See @ref
 * operation_tracking for more information., a set of related configuration options defined as
 * follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, enable operation tracking subsystem., a
 * boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;path, the name of a directory
 * into which operation tracking files are written.  The directory must already exist.  If the value
 * is not an absolute path\, the path is relative to the database home (see @ref absolute_path for
 * more information)., a string; default \c ".".}
 * @config{ ),,}
 * @config{prefetch = (, Enable automatic detection of scans by applications\, and attempt to
 * pre-fetch future content into the cache., a set of related configuration options defined as
 * follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;available, whether the thread pool for the pre-fetch
 * functionality is started., a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;
 * default, whether pre-fetch is enabled for all sessions by default., a boolean flag; default \c
 * false.}
 * @config{ ),,}
 * @config{readonly, open connection in read-only mode.  The database must exist.  All methods that
 * may modify a database are disabled.  See @ref readonly for more information., a boolean flag;
 * default \c false.}
 * @config{salvage, open connection and salvage any WiredTiger-owned database and log files that it
 * detects as corrupted.  This call should only be used after getting an error return of
 * WT_TRY_SALVAGE. Salvage rebuilds files in place\, overwriting existing files.  We recommend
 * making a backup copy of all files with the WiredTiger prefix prior to passing this flag., a
 * boolean flag; default \c false.}
 * @config{session_max, maximum expected number of sessions (including server threads)., an integer
 * greater than or equal to \c 1; default \c 100.}
 * @config{shared_cache = (, shared cache configuration options.  A database should configure either
 * a cache_size or a shared_cache not both.  Enabling a shared cache uses a session from the
 * configured session_max.  A shared cache can not have absolute values configured for cache
 * eviction settings., a set of related configuration options defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;chunk, the granularity that a shared cache is redistributed., an
 * integer between \c 1MB and \c 10TB; default \c 10MB.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;name, the
 * name of a cache that is shared between databases or \c "none" when no shared cache is
 * configured., a string; default \c none.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;quota, maximum size of
 * cache this database can be allocated from the shared cache.  Defaults to the entire shared cache
 * size., an integer; default \c 0.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;reserve, amount of cache this
 * database is guaranteed to have available from the shared cache.  This setting is per database.
 * Defaults to the chunk size., an integer; default \c 0.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;size,
 * maximum memory to allocate for the shared cache.  Setting this will update the value if one is
 * already set., an integer between \c 1MB and \c 10TB; default \c 500MB.}
 * @config{ ),,}
 * @config{statistics, Maintain database statistics\, which may impact performance.  Choosing "all"
 * maintains all statistics regardless of cost\, "fast" maintains a subset of statistics that are
 * relatively inexpensive\, "none" turns off all statistics.  The "clear" configuration resets
 * statistics after they are gathered\, where appropriate (for example\, a cache size statistic is
 * not cleared\, while the count of cursor insert operations will be cleared). When "clear" is
 * configured for the database\, gathered statistics are reset each time a statistics cursor is used
 * to gather statistics\, as well as each time statistics are logged using the \c statistics_log
 * configuration.  See @ref statistics for more information., a list\, with values chosen from the
 * following options: \c "all"\, \c "cache_walk"\, \c "fast"\, \c "none"\, \c "clear"\, \c
 * "tree_walk"; default \c none.}
 * @config{statistics_log = (, log any statistics the database is configured to maintain\, to a
 * file.  See @ref statistics for more information.  Enabling the statistics log server uses a
 * session from the configured session_max., a set of related configuration options defined as
 * follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;json, encode statistics in JSON format., a boolean
 * flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;on_close, log statistics on database
 * close., a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;path, the name of a
 * directory into which statistics files are written.  The directory must already exist.  If the
 * value is not an absolute path\, the path is relative to the database home (see @ref absolute_path
 * for more information)., a string; default \c ".".}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;sources, if
 * non-empty\, include statistics for the list of data source URIs\, if they are open at the time of
 * the statistics logging.  The list may include URIs matching a single data source
 * ("table:mytable")\, or a URI matching all data sources of a particular type ("table:")., a list
 * of strings; default empty.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;timestamp, a timestamp prepended to
 * each log record.  May contain \c strftime conversion specifications.  When \c json is
 * configured\, defaults to \c "%Y-%m-%dT%H:%M:%S.000Z"., a string; default \c "%b %d %H:%M:%S".}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;wait, seconds to wait between each write of the log records;
 * setting this value above 0 configures statistics logging., an integer between \c 0 and \c 100000;
 * default \c 0.}
 * @config{ ),,}
 * @config{transaction_sync = (, how to sync log records when the transaction commits., a set of
 * related configuration options defined as follows.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled,
 * whether to sync the log on every commit by default\, can be overridden by the \c sync setting to
 * WT_SESSION::commit_transaction., a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;method, the method used to ensure log records are stable on
 * disk\, see @ref tune_durability for more information., a string\, chosen from the following
 * options: \c "dsync"\, \c "fsync"\, \c "none"; default \c fsync.}
 * @config{ ),,}
 * @config{use_environment, use the \c WIREDTIGER_CONFIG and \c WIREDTIGER_HOME environment
 * variables if the process is not running with special privileges.  See @ref home for more
 * information., a boolean flag; default \c true.}
 * @config{use_environment_priv, use the \c WIREDTIGER_CONFIG and \c WIREDTIGER_HOME environment
 * variables even if the process is running with special privileges.  See @ref home for more
 * information., a boolean flag; default \c false.}
 * @config{verbose, enable messages for various subsystems and operations.  Options are given as a
 * list\, where each message type can optionally define an associated verbosity level\, such as
 * <code>"verbose=[evictserver\,read:1\,rts:0]"</code>. Verbosity levels that can be provided
 * include <code>0</code> (INFO) and <code>1</code> through <code>5</code>\, corresponding to
 * (DEBUG_1) to (DEBUG_5). \c all is a special case that defines the verbosity level for all
 * categories not explicitly set in the config string., a list\, with values chosen from the
 * following options: \c "all"\, \c "api"\, \c "backup"\, \c "block"\, \c "block_cache"\, \c
 * "checkpoint"\, \c "checkpoint_cleanup"\, \c "checkpoint_progress"\, \c "chunkcache"\, \c
 * "compact"\, \c "compact_progress"\, \c "configuration"\, \c "error_returns"\, \c "evict"\, \c
 * "evict_stuck"\, \c "evictserver"\, \c "fileops"\, \c "generation"\, \c "handleops"\, \c
 * "history_store"\, \c "history_store_activity"\, \c "log"\, \c "lsm"\, \c "lsm_manager"\, \c
 * "metadata"\, \c "mutex"\, \c "out_of_order"\, \c "overflow"\, \c "read"\, \c "reconcile"\, \c
 * "recovery"\, \c "recovery_progress"\, \c "rts"\, \c "salvage"\, \c "shared_cache"\, \c "split"\,
 * \c "temporary"\, \c "thread_group"\, \c "tiered"\, \c "timestamp"\, \c "transaction"\, \c
 * "verify"\, \c "version"\, \c "write"; default \c [].}
 * @config{verify_metadata, open connection and verify any WiredTiger metadata.  Not supported when
 * opening a connection from a backup.  This API allows verification and detection of corruption in
 * WiredTiger metadata., a boolean flag; default \c false.}
 * @config{write_through, Use \c FILE_FLAG_WRITE_THROUGH on Windows to write to files.  Ignored on
 * non-Windows systems.  Options are given as a list\, such as <code>"write_through=[data]"</code>.
 * Configuring \c write_through requires care; see @ref tuning_system_buffer_cache_direct_io for
 * important warnings.  Including \c "data" will cause WiredTiger data files to write through
 * cache\, including \c "log" will cause WiredTiger log files to write through cache.  \c
 * write_through should be combined with \c direct_io to get the equivalent of POSIX \c O_DIRECT on
 * Windows., a list\, with values chosen from the following options: \c "data"\, \c "log"; default
 * empty.}
 * @configend
 * Additionally, if files named \c WiredTiger.config or \c WiredTiger.basecfg
 * appear in the WiredTiger home directory, they are read for configuration
 * values (see @ref config_file and @ref config_base for details).
 * See @ref config_order for ordering of the configuration mechanisms.
 * @param[out] connectionp A pointer to the newly opened connection handle
 * @errors
 */
int wiredtiger_open(const char *home,
    WT_EVENT_HANDLER *event_handler, const char *config,
    WT_CONNECTION **connectionp) WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Return information about a WiredTiger error as a string (see
 * WT_SESSION::strerror for a thread-safe API).
 *
 * @snippet ex_all.c Display an error
 *
 * @param error a return value from a WiredTiger, ISO C, or POSIX standard API call
 * @returns a string representation of the error
 */
const char *wiredtiger_strerror(int error) WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*! WT_EVENT_HANDLER::special event types */
typedef enum {
    WT_EVENT_COMPACT_CHECK, /*!< Compact check iteration. */
    WT_EVENT_CONN_CLOSE, /*!< Connection closing. */
    WT_EVENT_CONN_READY /*!< Connection is ready. */
} WT_EVENT_TYPE;

/*!
 * The interface implemented by applications to handle error, informational and
 * progress messages.  Entries set to NULL are ignored and the default handlers
 * will continue to be used.
 */
struct __wt_event_handler {
    /*!
     * Callback to handle error messages; by default, error messages are
     * written to the stderr stream. See @ref event_message_handling for
     * more information.
     *
     * Errors that require the application to exit and restart will have
     * their \c error value set to \c WT_PANIC. The application can exit
     * immediately when \c WT_PANIC is passed to an event handler; there
     * is no reason to return into WiredTiger.
     *
     * Event handler returns are not ignored: if the handler returns
     * non-zero, the error may cause the WiredTiger function posting the
     * event to fail, and may even cause operation or library failure.
     *
     * @param session the WiredTiger session handle in use when the error
     * was generated. The handle may have been created by the application
     * or automatically by WiredTiger.
     * @param error a return value from a WiredTiger, ISO C, or
     * POSIX standard API call, which can be converted to a string using
     * WT_SESSION::strerror
     * @param message an error string
     */
    int (*handle_error)(WT_EVENT_HANDLER *handler,
        WT_SESSION *session, int error, const char *message);

    /*!
     * Callback to handle informational messages; by default, informational
     * messages are written to the stdout stream. See
     * @ref event_message_handling for more information.
     *
     * Message handler returns are not ignored: if the handler returns
     * non-zero, the error may cause the WiredTiger function posting the
     * event to fail, and may even cause operation or library failure.
     *
     * @param session the WiredTiger session handle in use when the message
     * was generated. The handle may have been created by the application
     * or automatically by WiredTiger.
     * @param message an informational string
     */
    int (*handle_message)(WT_EVENT_HANDLER *handler,
        WT_SESSION *session, const char *message);

    /*!
     * Callback to handle progress messages; by default, progress messages
     * are not written. See @ref event_message_handling for more
     * information.
     *
     * Progress handler returns are not ignored: if the handler returns
     * non-zero, the error may cause the WiredTiger function posting the
     * event to fail, and may even cause operation or library failure.
     *
     * @param session the WiredTiger session handle in use when the
     * progress message was generated. The handle may have been created by
     * the application or automatically by WiredTiger.
     * @param operation a string representation of the operation
     * @param progress a counter
     */
    int (*handle_progress)(WT_EVENT_HANDLER *handler,
        WT_SESSION *session, const char *operation, uint64_t progress);

    /*!
     * Callback to handle automatic close of a WiredTiger handle.
     *
     * Close handler returns are not ignored: if the handler returns
     * non-zero, the error may cause the WiredTiger function posting the
     * event to fail, and may even cause operation or library failure.
     *
     * @param session The session handle that is being closed if the
     * cursor parameter is NULL.
     * @param cursor The cursor handle that is being closed, or NULL if
     * it is a session handle being closed.
     */
    int (*handle_close)(WT_EVENT_HANDLER *handler,
        WT_SESSION *session, WT_CURSOR *cursor);

    /*!
     * Callback to handle general events. The application may choose to handle
     * only some types of events. An unhandled event should return 0.
     *
     * General event returns are not ignored in most cases. If the handler
     * returns non-zero, the error may cause the WiredTiger function posting
     * the event to fail.
     *
     * @param wt_conn The connection handle for the database.
     * @param session the WiredTiger session handle in use when the
     * progress message was generated. The handle may have been created by
     * the application or automatically by WiredTiger or may be NULL.
     * @param type A type indicator for what special event occurred.
         * @param arg A generic argument that has a specific meaning
         * depending on the event type.
     * (see ::WT_EVENT_TYPE for available options.)
     */
        int (*handle_general)(WT_EVENT_HANDLER *handler,
            WT_CONNECTION *wt_conn, WT_SESSION *session, WT_EVENT_TYPE type, void *arg);
};

/*!
 * @name Data packing and unpacking
 * @{
 */

/*!
 * Pack a structure into a buffer.
 *
 * See @ref packing for a description of the permitted format strings.
 *
 * @section pack_examples Packing Examples
 *
 * For example, the string <code>"iSh"</code> will pack a 32-bit integer
 * followed by a NUL-terminated string, followed by a 16-bit integer.  The
 * default, big-endian encoding will be used, with no alignment.  This could be
 * used in C as follows:
 *
 * @snippet ex_all.c Pack fields into a buffer
 *
 * Then later, the values can be unpacked as follows:
 *
 * @snippet ex_all.c Unpack fields from a buffer
 *
 * @param session the session handle
 * @param buffer a pointer to a packed byte array
 * @param len the number of valid bytes in the buffer
 * @param format the data format, see @ref packing
 * @errors
 */
int wiredtiger_struct_pack(WT_SESSION *session,
    void *buffer, size_t len, const char *format, ...)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Calculate the size required to pack a structure.
 *
 * Note that for variable-sized fields including variable-sized strings and
 * integers, the calculated sized merely reflects the expected sizes specified
 * in the format string itself.
 *
 * @snippet ex_all.c Get the packed size
 *
 * @param session the session handle
 * @param lenp a location where the number of bytes needed for the
 * matching call to ::wiredtiger_struct_pack is returned
 * @param format the data format, see @ref packing
 * @errors
 */
int wiredtiger_struct_size(WT_SESSION *session,
    size_t *lenp, const char *format, ...) WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Unpack a structure from a buffer.
 *
 * Reverse of ::wiredtiger_struct_pack: gets values out of a
 * packed byte string.
 *
 * @snippet ex_all.c Unpack fields from a buffer
 *
 * @param session the session handle
 * @param buffer a pointer to a packed byte array
 * @param len the number of valid bytes in the buffer
 * @param format the data format, see @ref packing
 * @errors
 */
int wiredtiger_struct_unpack(WT_SESSION *session,
    const void *buffer, size_t len, const char *format, ...)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

#if !defined(SWIG)

/*!
 * Streaming interface to packing.
 *
 * This allows applications to pack or unpack records one field at a time.
 * This is an opaque handle returned by ::wiredtiger_pack_start or
 * ::wiredtiger_unpack_start.  It must be closed with ::wiredtiger_pack_close.
 */
typedef struct __wt_pack_stream WT_PACK_STREAM;

/*!
 * Start a packing operation into a buffer with the given format string.  This
 * should be followed by a series of calls to ::wiredtiger_pack_item,
 * ::wiredtiger_pack_int, ::wiredtiger_pack_str or ::wiredtiger_pack_uint
 * to fill in the values.
 *
 * @param session the session handle
 * @param format the data format, see @ref packing
 * @param buffer a pointer to memory to hold the packed data
 * @param size the size of the buffer
 * @param[out] psp the new packing stream handle
 * @errors
 */
int wiredtiger_pack_start(WT_SESSION *session,
    const char *format, void *buffer, size_t size, WT_PACK_STREAM **psp)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Start an unpacking operation from a buffer with the given format string.
 * This should be followed by a series of calls to ::wiredtiger_unpack_item,
 * ::wiredtiger_unpack_int, ::wiredtiger_unpack_str or ::wiredtiger_unpack_uint
 * to retrieve the packed values.
 *
 * @param session the session handle
 * @param format the data format, see @ref packing
 * @param buffer a pointer to memory holding the packed data
 * @param size the size of the buffer
 * @param[out] psp the new packing stream handle
 * @errors
 */
int wiredtiger_unpack_start(WT_SESSION *session,
    const char *format, const void *buffer, size_t size, WT_PACK_STREAM **psp)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Close a packing stream.
 *
 * @param ps the packing stream handle
 * @param[out] usedp the number of bytes in the buffer used by the stream
 * @errors
 */
int wiredtiger_pack_close(WT_PACK_STREAM *ps, size_t *usedp)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Pack an item into a packing stream.
 *
 * @param ps the packing stream handle
 * @param item an item to pack
 * @errors
 */
int wiredtiger_pack_item(WT_PACK_STREAM *ps, WT_ITEM *item)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Pack a signed integer into a packing stream.
 *
 * @param ps the packing stream handle
 * @param i a signed integer to pack
 * @errors
 */
int wiredtiger_pack_int(WT_PACK_STREAM *ps, int64_t i)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Pack a string into a packing stream.
 *
 * @param ps the packing stream handle
 * @param s a string to pack
 * @errors
 */
int wiredtiger_pack_str(WT_PACK_STREAM *ps, const char *s)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Pack an unsigned integer into a packing stream.
 *
 * @param ps the packing stream handle
 * @param u an unsigned integer to pack
 * @errors
 */
int wiredtiger_pack_uint(WT_PACK_STREAM *ps, uint64_t u)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Unpack an item from a packing stream.
 *
 * @param ps the packing stream handle
 * @param item an item to unpack
 * @errors
 */
int wiredtiger_unpack_item(WT_PACK_STREAM *ps, WT_ITEM *item)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Unpack a signed integer from a packing stream.
 *
 * @param ps the packing stream handle
 * @param[out] ip the unpacked signed integer
 * @errors
 */
int wiredtiger_unpack_int(WT_PACK_STREAM *ps, int64_t *ip)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Unpack a string from a packing stream.
 *
 * @param ps the packing stream handle
 * @param[out] sp the unpacked string
 * @errors
 */
int wiredtiger_unpack_str(WT_PACK_STREAM *ps, const char **sp)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Unpack an unsigned integer from a packing stream.
 *
 * @param ps the packing stream handle
 * @param[out] up the unpacked unsigned integer
 * @errors
 */
int wiredtiger_unpack_uint(WT_PACK_STREAM *ps, uint64_t *up)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;
/*! @} */

/*!
 * @name Configuration strings
 * @{
 */

/*!
 * The configuration information returned by the WiredTiger configuration
 * parsing functions in the WT_EXTENSION_API and the public API.
 */
struct __wt_config_item {
    /*!
     * The value of a configuration string.
     *
     * Regardless of the type of the configuration string (boolean, int,
     * list or string), the \c str field will reference the value of the
     * configuration string.
     *
     * The bytes referenced by \c str are <b>not</b> nul-terminated.
     * Use the \c len field instead of a terminating nul byte.
     */
    const char *str;

    /*! The number of bytes in the value referenced by \c str. */
    size_t len;

    /*!
     * The numeric value of a configuration boolean or integer.
     *
     * If the configuration string's value is "true" or "false", the
     * \c val field will be set to 1 (true), or 0 (false).
     *
     * If the configuration string can be legally interpreted as an integer,
     * using the \c strtoll function rules as specified in ISO/IEC 9899:1990
     * ("ISO C90"), that integer will be stored in the \c val field.
     */
    int64_t val;

    /*! Permitted values of the \c type field. */
    enum WT_CONFIG_ITEM_TYPE {
        /*! A string value with quotes stripped. */
        WT_CONFIG_ITEM_STRING,
        /*! A boolean literal ("true" or "false"). */
        WT_CONFIG_ITEM_BOOL,
        /*! An unquoted identifier: a string value without quotes. */
        WT_CONFIG_ITEM_ID,
        /*! A numeric value. */
        WT_CONFIG_ITEM_NUM,
        /*! A nested structure or list, including brackets. */
        WT_CONFIG_ITEM_STRUCT
    }
    /*!
     * The type of value determined by the parser.  In all cases,
     * the \c str and \c len fields are set.
     */
    type;
};

/*
 * This is needed for compatible usage of this embedded enum type.
 */
#if !defined(SWIG) && !defined(DOXYGEN)
#if defined(__cplusplus)
typedef enum __wt_config_item::WT_CONFIG_ITEM_TYPE WT_CONFIG_ITEM_TYPE;
#else
typedef enum WT_CONFIG_ITEM_TYPE WT_CONFIG_ITEM_TYPE;
#endif
#endif

#if !defined(SWIG) && !defined(DOXYGEN)
/*!
 * Validate a configuration string for a WiredTiger API call.
 * This call is outside the scope of a WiredTiger connection handle, since
 * applications may need to validate configuration strings prior to calling
 * ::wiredtiger_open.
 * @param session the session handle (may be \c NULL if the database not yet
 * opened).
 * @param event_handler An event handler (used if \c session is \c NULL; if both
 * \c session and \c event_handler are \c NULL, error messages will be written
 * to stderr).
 * @param name the WiredTiger function or method to validate.
 * @param config the configuration string being parsed.
 * @returns zero for success, non-zero to indicate an error.
 *
 * @snippet ex_all.c Validate a configuration string
 */
int wiredtiger_config_validate(WT_SESSION *session,
    WT_EVENT_HANDLER *event_handler, const char *name, const char *config)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*
 * Validate a configuration string for a WiredTiger test program.
 */
int wiredtiger_test_config_validate(WT_SESSION *session,
    WT_EVENT_HANDLER *event_handler, const char *name, const char *config)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;
#endif

/*!
 * Create a handle that can be used to parse or create configuration strings
 * compatible with the WiredTiger API.
 * This call is outside the scope of a WiredTiger connection handle, since
 * applications may need to generate configuration strings prior to calling
 * ::wiredtiger_open.
 * @param session the session handle to be used for error reporting (if NULL,
 *  error messages will be written to stderr).
 * @param config the configuration string being parsed. The string must
 *  remain valid for the lifetime of the parser handle.
 * @param len the number of valid bytes in \c config
 * @param[out] config_parserp A pointer to the newly opened handle
 * @errors
 *
 * @snippet ex_config_parse.c Create a configuration parser
 */
int wiredtiger_config_parser_open(WT_SESSION *session,
    const char *config, size_t len, WT_CONFIG_PARSER **config_parserp)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * A handle that can be used to search and traverse configuration strings
 * compatible with the WiredTiger API.
 * To parse the contents of a list or nested configuration string use a new
 * configuration parser handle based on the content of the ::WT_CONFIG_ITEM
 * retrieved from the parent configuration string.
 *
 * @section config_parse_examples Configuration String Parsing examples
 *
 * This could be used in C to create a configuration parser as follows:
 *
 * @snippet ex_config_parse.c Create a configuration parser
 *
 * Once the parser has been created the content can be queried directly:
 *
 * @snippet ex_config_parse.c get
 *
 * Or the content can be traversed linearly:
 *
 * @snippet ex_config_parse.c next
 *
 * Nested configuration values can be queried using a shorthand notation:
 *
 * @snippet ex_config_parse.c nested get
 *
 * Nested configuration values can be traversed using multiple
 * ::WT_CONFIG_PARSER handles:
 *
 * @snippet ex_config_parse.c nested traverse
 */
struct __wt_config_parser {

    /*!
     * Close the configuration scanner releasing any resources.
     *
     * @param config_parser the configuration parser handle
     * @errors
     *
     */
    int __F(close)(WT_CONFIG_PARSER *config_parser);

    /*!
     * Return the next key/value pair.
     *
     * If an item has no explicitly assigned value, the item will be
     * returned in \c key and the \c value will be set to the boolean
     * \c "true" value.
     *
     * @param config_parser the configuration parser handle
     * @param key the returned key
     * @param value the returned value
     * @errors
     * When iteration would pass the end of the configuration string
     * ::WT_NOTFOUND will be returned.
     */
    int __F(next)(WT_CONFIG_PARSER *config_parser,
        WT_CONFIG_ITEM *key, WT_CONFIG_ITEM *value);

    /*!
     * Return the value of an item in the configuration string.
     *
     * @param config_parser the configuration parser handle
     * @param key configuration key string
     * @param value the returned value
     * @errors
     *
     */
    int __F(get)(WT_CONFIG_PARSER *config_parser,
        const char *key, WT_CONFIG_ITEM *value);
};

/*! @} */

/*!
 * @name Support functions
 * @anchor support_functions
 * @{
 */

/*!
 * Return a pointer to a function that calculates a CRC32C checksum.
 *
 * The WiredTiger library CRC32C checksum function uses hardware support where available, else it
 * falls back to a software implementation. Selecting a CRC32C checksum function can be slow, so the
 * return value should be cached by the caller for repeated use.
 *
 * @snippet ex_all.c Checksum a buffer
 *
 * @returns a pointer to a function that takes a buffer and length and returns the CRC32C checksum
 */
uint32_t (*wiredtiger_crc32c_func(void))(const void *, size_t)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Return a pointer to a function that calculates a CRC32C checksum given a starting CRC seed.
 *
 * The WiredTiger library CRC32C checksum function uses hardware support where available, else it
 * falls back to a software implementation. Selecting a CRC32C checksum function can be slow, so the
 * return value should be cached by the caller for repeated use. This version returns a function
 * that accepts a starting seed value for the CRC. This version is useful where an application wants
 * to calculate the CRC of a large buffer in smaller incremental pieces. The starting seed to
 * calculate the CRC of a piece is then the cumulative CRC of all the previous pieces.
 *
 * @snippet ex_all.c Checksum a large buffer in smaller pieces
 *
 * @returns a pointer to a function that takes a starting seed, a buffer and length and returns the
 * CRC32C checksum
 */
uint32_t (*wiredtiger_crc32c_with_seed_func(void))(uint32_t seed, const void *, size_t)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

#endif /* !defined(SWIG) */

/*!
 * Calculate a set of WT_MODIFY operations to represent an update.
 * This call will calculate a set of modifications to an old value that produce
 * the new value.  If more modifications are required than fit in the array
 * passed in by the caller, or if more bytes have changed than the \c maxdiff
 * parameter, the call will fail.  The matching algorithm is approximate, so it
 * may fail and return WT_NOTFOUND if a matching set of WT_MODIFY operations
 * is not found.
 *
 * The \c maxdiff parameter bounds how much work will be done searching for a
 * match: to ensure a match is found, it may need to be set larger than actual
 * number of bytes that differ between the old and new values.  In particular,
 * repeated patterns of bytes in the values can lead to suboptimal matching,
 * and matching ranges less than 64 bytes long will not be detected.
 *
 * If the call succeeds, the WT_MODIFY operations will point into \c newv,
 * which must remain valid until WT_CURSOR::modify is called.
 *
 * @snippet ex_all.c Calculate a modify operation
 *
 * @param session the current WiredTiger session (may be NULL)
 * @param oldv old value
 * @param newv new value
 * @param maxdiff maximum bytes difference
 * @param[out] entries array of modifications producing the new value
 * @param[in,out] nentriesp size of \c entries array passed in,
 *  set to the number of entries used
 * @errors
 */
int wiredtiger_calc_modify(WT_SESSION *session,
    const WT_ITEM *oldv, const WT_ITEM *newv,
    size_t maxdiff, WT_MODIFY *entries, int *nentriesp)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*!
 * Get version information.
 *
 * @snippet ex_all.c Get the WiredTiger library version #1
 * @snippet ex_all.c Get the WiredTiger library version #2
 *
 * @param majorp a location where the major version number is returned
 * @param minorp a location where the minor version number is returned
 * @param patchp a location where the patch version number is returned
 * @returns a string representation of the version
 */
const char *wiredtiger_version(int *majorp, int *minorp, int *patchp)
    WT_ATTRIBUTE_LIBRARY_VISIBLE;

/*! @} */

/*******************************************
 * Error returns
 *******************************************/
/*!
 * @name Error returns
 * Most functions and methods in WiredTiger return an integer code indicating
 * whether the operation succeeded or failed.  A return of zero indicates
 * success; all non-zero return values indicate some kind of failure.
 *
 * WiredTiger reserves all values from -31,800 to -31,999 as possible error
 * return values.  WiredTiger may also return C99/POSIX error codes such as
 * \c ENOMEM, \c EINVAL and \c ENOTSUP, with the usual meanings.
 *
 * The following are all of the WiredTiger-specific error returns:
 * @{
 */
/*
 * DO NOT EDIT: automatically built by dist/api_err.py.
 * Error return section: BEGIN
 */
/*!
 * Conflict between concurrent operations.
 * This error is generated when an operation cannot be completed due to a
 * conflict with concurrent operations.  The operation may be retried; if a
 * transaction is in progress, it should be rolled back and the operation
 * retried in a new transaction.
 */
#define	WT_ROLLBACK	(-31800)
/*!
 * Attempt to insert an existing key.
 * This error is generated when the application attempts to insert a record with
 * the same key as an existing record without the 'overwrite' configuration to
 * WT_SESSION::open_cursor.
 */
#define	WT_DUPLICATE_KEY	(-31801)
/*!
 * Non-specific WiredTiger error.
 * This error is returned when an error is not covered by a specific error
 * return. The operation may be retried; if a transaction is in progress, it
 * should be rolled back and the operation retried in a new transaction.
 */
#define	WT_ERROR	(-31802)
/*!
 * Item not found.
 * This error indicates an operation did not find a value to return.  This
 * includes cursor search and other operations where no record matched the
 * cursor's search key such as WT_CURSOR::update or WT_CURSOR::remove.
 */
#define	WT_NOTFOUND	(-31803)
/*!
 * WiredTiger library panic.
 * This error indicates an underlying problem that requires a database restart.
 * The application may exit immediately, no further WiredTiger calls are
 * required (and further calls will themselves immediately fail).
 */
#define	WT_PANIC	(-31804)
/*! @cond internal */
/*! Restart the operation (internal). */
#define	WT_RESTART	(-31805)
/*! @endcond */
/*!
 * Recovery must be run to continue.
 * This error is generated when ::wiredtiger_open is configured to return an
 * error if recovery is required to use the database.
 */
#define	WT_RUN_RECOVERY	(-31806)
/*!
 * Operation would overflow cache.
 * This error is generated when wiredtiger_open is configured to run in-memory,
 * and a data modification operation requires more than the configured cache
 * size to complete. The operation may be retried; if a transaction is in
 * progress, it should be rolled back and the operation retried in a new
 * transaction.
 */
#define	WT_CACHE_FULL	(-31807)
/*!
 * Conflict with a prepared update.
 * This error is generated when the application attempts to read an updated
 * record which is part of a transaction that has been prepared but not yet
 * resolved.
 */
#define	WT_PREPARE_CONFLICT	(-31808)
/*!
 * Database corruption detected.
 * This error is generated when corruption is detected in an on-disk file.
 * During normal operations, this may occur in rare circumstances as a result of
 * a system crash. The application may choose to salvage the file or retry
 * wiredtiger_open with the 'salvage=true' configuration setting.
 */
#define	WT_TRY_SALVAGE	(-31809)
/*
 * Error return section: END
 * DO NOT EDIT: automatically built by dist/api_err.py.
 */
/*! @} */

#ifndef DOXYGEN
#define WT_DEADLOCK WT_ROLLBACK     /* Backward compatibility */
#endif

/*! @} */

/*!
 * @defgroup wt_ext WiredTiger Extension API
 * The functions and interfaces applications use to customize and extend the
 * behavior of WiredTiger.
 * @{
 */

/*******************************************
 * Forward structure declarations for the extension API
 *******************************************/
struct __wt_config_arg; typedef struct __wt_config_arg WT_CONFIG_ARG;

/*!
 * The interface implemented by applications to provide custom ordering of
 * records.
 *
 * Applications register their implementation with WiredTiger by calling
 * WT_CONNECTION::add_collator.  See @ref custom_collators for more
 * information.
 *
 * @snippet ex_extending.c add collator nocase
 *
 * @snippet ex_extending.c add collator prefix10
 */
struct __wt_collator {
    /*!
     * Callback to compare keys.
     *
     * @param[out] cmp set to -1 if <code>key1 < key2</code>,
     *  0 if <code>key1 == key2</code>,
     *  1 if <code>key1 > key2</code>.
     * @returns zero for success, non-zero to indicate an error.
     *
     * @snippet ex_all.c Implement WT_COLLATOR
     *
     * @snippet ex_extending.c case insensitive comparator
     *
     * @snippet ex_extending.c n character comparator
     */
    int (*compare)(WT_COLLATOR *collator, WT_SESSION *session,
        const WT_ITEM *key1, const WT_ITEM *key2, int *cmp);

    /*!
     * If non-NULL, this callback is called to customize the collator
     * for each data source.  If the callback returns a non-NULL
     * collator, that instance is used instead of this one for all
     * comparisons.
     */
    int (*customize)(WT_COLLATOR *collator, WT_SESSION *session,
        const char *uri, WT_CONFIG_ITEM *passcfg, WT_COLLATOR **customp);

    /*!
     * If non-NULL a callback performed when the data source is closed
     * for customized extractors otherwise when the database is closed.
     *
     * The WT_COLLATOR::terminate callback is intended to allow cleanup;
     * the handle will not be subsequently accessed by WiredTiger.
     */
    int (*terminate)(WT_COLLATOR *collator, WT_SESSION *session);
};

/*!
 * The interface implemented by applications to provide custom compression.
 *
 * Compressors must implement the WT_COMPRESSOR interface: the
 * WT_COMPRESSOR::compress and WT_COMPRESSOR::decompress callbacks must be
 * specified, and WT_COMPRESSOR::pre_size is optional.  To build your own
 * compressor, use one of the compressors in \c ext/compressors as a template:
 * \c ext/nop_compress is a simple compressor that passes through data
 * unchanged, and is a reasonable starting point.
 *
 * Applications register their implementation with WiredTiger by calling
 * WT_CONNECTION::add_compressor.
 *
 * @snippet nop_compress.c WT_COMPRESSOR initialization structure
 * @snippet nop_compress.c WT_COMPRESSOR initialization function
 */
struct __wt_compressor {
    /*!
     * Callback to compress a chunk of data.
     *
     * WT_COMPRESSOR::compress takes a source buffer and a destination
     * buffer, by default of the same size.  If the callback can compress
     * the buffer to a smaller size in the destination, it does so, sets
     * the \c compression_failed return to 0 and returns 0.  If compression
     * does not produce a smaller result, the callback sets the
     * \c compression_failed return to 1 and returns 0. If another
     * error occurs, it returns an errno or WiredTiger error code.
     *
     * On entry, \c src will point to memory, with the length of the memory
     * in \c src_len.  After successful completion, the callback should
     * return \c 0 and set \c result_lenp to the number of bytes required
     * for the compressed representation.
     *
     * On entry, \c dst points to the destination buffer with a length
     * of \c dst_len.  If the WT_COMPRESSOR::pre_size method is specified,
     * the destination buffer will be at least the size returned by that
     * method; otherwise, the destination buffer will be at least as large
     * as the length of the data to compress.
     *
     * If compression would not shrink the data or the \c dst buffer is not
     * large enough to hold the compressed data, the callback should set
     * \c compression_failed to a non-zero value and return 0.
     *
     * @param[in] src the data to compress
     * @param[in] src_len the length of the data to compress
     * @param[in] dst the destination buffer
     * @param[in] dst_len the length of the destination buffer
     * @param[out] result_lenp the length of the compressed data
     * @param[out] compression_failed non-zero if compression did not
     * decrease the length of the data (compression may not have completed)
     * @returns zero for success, non-zero to indicate an error.
     *
     * @snippet nop_compress.c WT_COMPRESSOR compress
     */
    int (*compress)(WT_COMPRESSOR *compressor, WT_SESSION *session,
        uint8_t *src, size_t src_len,
        uint8_t *dst, size_t dst_len,
        size_t *result_lenp, int *compression_failed);

    /*!
     * Callback to decompress a chunk of data.
     *
     * WT_COMPRESSOR::decompress takes a source buffer and a destination
     * buffer.  The contents are switched from \c compress: the
     * source buffer is the compressed value, and the destination buffer is
     * sized to be the original size.  If the callback successfully
     * decompresses the source buffer to the destination buffer, it returns
     * 0.  If an error occurs, it returns an errno or WiredTiger error code.
     * The source buffer that WT_COMPRESSOR::decompress takes may have a
     * size that is rounded up from the size originally produced by
     * WT_COMPRESSOR::compress, with the remainder of the buffer set to
     * zeroes. Most compressors do not care about this difference if the
     * size to be decompressed can be implicitly discovered from the
     * compressed data.  If your compressor cares, you may need to allocate
     * space for, and store, the actual size in the compressed buffer.  See
     * the source code for the included snappy compressor for an example.
     *
     * On entry, \c src will point to memory, with the length of the memory
     * in \c src_len.  After successful completion, the callback should
     * return \c 0 and set \c result_lenp to the number of bytes required
     * for the decompressed representation.
     *
     * If the \c dst buffer is not big enough to hold the decompressed
     * data, the callback should return an error.
     *
     * @param[in] src the data to decompress
     * @param[in] src_len the length of the data to decompress
     * @param[in] dst the destination buffer
     * @param[in] dst_len the length of the destination buffer
     * @param[out] result_lenp the length of the decompressed data
     * @returns zero for success, non-zero to indicate an error.
     *
     * @snippet nop_compress.c WT_COMPRESSOR decompress
     */
    int (*decompress)(WT_COMPRESSOR *compressor, WT_SESSION *session,
        uint8_t *src, size_t src_len,
        uint8_t *dst, size_t dst_len,
        size_t *result_lenp);

    /*!
     * Callback to size a destination buffer for compression
     *
     * WT_COMPRESSOR::pre_size is an optional callback that, given the
     * source buffer and size, produces the size of the destination buffer
     * to be given to WT_COMPRESSOR::compress.  This is useful for
     * compressors that assume that the output buffer is sized for the
     * worst case and thus no overrun checks are made.  If your compressor
     * works like this, WT_COMPRESSOR::pre_size will need to be defined.
     * See the source code for the snappy compressor for an example.
     * However, if your compressor detects and avoids overruns against its
     * target buffer, you will not need to define WT_COMPRESSOR::pre_size.
     * When WT_COMPRESSOR::pre_size is set to NULL, the destination buffer
     * is sized the same as the source buffer.  This is always sufficient,
     * since a compression result that is larger than the source buffer is
     * discarded by WiredTiger.
     *
     * If not NULL, this callback is called before each call to
     * WT_COMPRESSOR::compress to determine the size of the destination
     * buffer to provide.  If the callback is NULL, the destination
     * buffer will be the same size as the source buffer.
     *
     * The callback should set \c result_lenp to a suitable buffer size
     * for compression, typically the maximum length required by
     * WT_COMPRESSOR::compress.
     *
     * This callback function is for compressors that require an output
     * buffer larger than the source buffer (for example, that do not
     * check for buffer overflow during compression).
     *
     * @param[in] src the data to compress
     * @param[in] src_len the length of the data to compress
     * @param[out] result_lenp the required destination buffer size
     * @returns zero for success, non-zero to indicate an error.
     *
     * @snippet nop_compress.c WT_COMPRESSOR presize
     */
    int (*pre_size)(WT_COMPRESSOR *compressor, WT_SESSION *session,
        uint8_t *src, size_t src_len, size_t *result_lenp);

    /*!
     * If non-NULL, a callback performed when the database is closed.
     *
     * The WT_COMPRESSOR::terminate callback is intended to allow cleanup;
     * the handle will not be subsequently accessed by WiredTiger.
     *
     * @snippet nop_compress.c WT_COMPRESSOR terminate
     */
    int (*terminate)(WT_COMPRESSOR *compressor, WT_SESSION *session);
};

/*!
 * Applications can extend WiredTiger by providing new implementations of the
 * WT_DATA_SOURCE class.  Each data source supports a different URI scheme for
 * data sources to WT_SESSION::create, WT_SESSION::open_cursor and related
 * methods.  See @ref custom_data_sources for more information.
 *
 * <b>Thread safety:</b> WiredTiger may invoke methods on the WT_DATA_SOURCE
 * interface from multiple threads concurrently.  It is the responsibility of
 * the implementation to protect any shared data.
 *
 * Applications register their implementation with WiredTiger by calling
 * WT_CONNECTION::add_data_source.
 *
 * @snippet ex_data_source.c WT_DATA_SOURCE register
 */
struct __wt_data_source {
    /*!
     * Callback to alter an object.
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE alter
     */
    int (*alter)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
        const char *uri, WT_CONFIG_ARG *config);

    /*!
     * Callback to create a new object.
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE create
     */
    int (*create)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
        const char *uri, WT_CONFIG_ARG *config);

    /*!
     * Callback to compact an object.
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE compact
     */
    int (*compact)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
        const char *uri, WT_CONFIG_ARG *config);

    /*!
     * Callback to drop an object.
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE drop
     */
    int (*drop)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
        const char *uri, WT_CONFIG_ARG *config);

    /*!
     * Callback to initialize a cursor.
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE open_cursor
     */
    int (*open_cursor)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
        const char *uri, WT_CONFIG_ARG *config, WT_CURSOR **new_cursor);

    /*!
     * Callback to rename an object.
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE rename
     */
    int (*rename)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
        const char *uri, const char *newuri, WT_CONFIG_ARG *config);

    /*!
     * Callback to salvage an object.
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE salvage
     */
    int (*salvage)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
        const char *uri, WT_CONFIG_ARG *config);

    /*!
     * Callback to get the size of an object.
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE size
     */
    int (*size)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
        const char *uri, wt_off_t *size);

    /*!
     * Callback to truncate an object.
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE truncate
     */
    int (*truncate)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
        const char *uri, WT_CONFIG_ARG *config);

    /*!
     * Callback to truncate a range of an object.
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE range truncate
     */
    int (*range_truncate)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
        WT_CURSOR *start, WT_CURSOR *stop);

    /*!
     * Callback to verify an object.
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE verify
     */
    int (*verify)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
        const char *uri, WT_CONFIG_ARG *config);

    /*!
     * Callback to checkpoint the database.
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE checkpoint
     */
    int (*checkpoint)(
        WT_DATA_SOURCE *dsrc, WT_SESSION *session, WT_CONFIG_ARG *config);

    /*!
     * If non-NULL, a callback performed when the database is closed.
     *
     * The WT_DATA_SOURCE::terminate callback is intended to allow cleanup;
     * the handle will not be subsequently accessed by WiredTiger.
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE terminate
     */
    int (*terminate)(WT_DATA_SOURCE *dsrc, WT_SESSION *session);

    /*!
     * If non-NULL, a callback performed before an LSM merge.
     *
     * @param[in] source a cursor configured with the data being merged
     * @param[in] dest a cursor on the new object being filled by the merge
     *
     * @snippet ex_data_source.c WT_DATA_SOURCE lsm_pre_merge
     */
    int (*lsm_pre_merge)(
        WT_DATA_SOURCE *dsrc, WT_CURSOR *source, WT_CURSOR *dest);
};

/*!
 * The interface implemented by applications to provide custom encryption.
 *
 * Encryptors must implement the WT_ENCRYPTOR interface: the WT_ENCRYPTOR::encrypt,
 * WT_ENCRYPTOR::decrypt and WT_ENCRYPTOR::sizing callbacks must be specified,
 * WT_ENCRYPTOR::customize and WT_ENCRYPTOR::terminate are optional.  To build your own
 * encryptor, use one of the encryptors in \c ext/encryptors as a template: \c
 * ext/encryptors/sodium_encrypt uses the open-source libsodium cryptographic library, and
 * \c ext/encryptors/nop_encrypt is a simple template that passes through data unchanged,
 * and is a reasonable starting point.  \c ext/encryptors/rotn_encrypt is an encryptor
 * implementing a simple (insecure) rotation cipher meant for testing.  See @ref
 * encryption "the encryptors page" for further information.
 *
 * Applications register their implementation with WiredTiger by calling
 * WT_CONNECTION::add_encryptor.
 *
 * @snippet nop_encrypt.c WT_ENCRYPTOR initialization structure
 * @snippet nop_encrypt.c WT_ENCRYPTOR initialization function
 */
struct __wt_encryptor {
    /*!
     * Callback to encrypt a chunk of data.
     *
     * WT_ENCRYPTOR::encrypt takes a source buffer and a destination buffer. The
     * callback encrypts the source buffer (plain text) into the destination buffer.
     *
     * On entry, \c src will point to a block of memory to encrypt, with the length of
     * the block in \c src_len.
     *
     * On entry, \c dst points to the destination buffer with a length of \c dst_len.
     * The destination buffer will be at least src_len plus the size returned by that
     * WT_ENCRYPT::sizing.
     *
     * After successful completion, the callback should return \c 0 and set \c
     * result_lenp to the number of bytes required for the encrypted representation,
     * which should be less than or equal to \c dst_len.
     *
     * This callback cannot be NULL.
     *
     * @param[in] src the data to encrypt
     * @param[in] src_len the length of the data to encrypt
     * @param[in] dst the destination buffer
     * @param[in] dst_len the length of the destination buffer
     * @param[out] result_lenp the length of the encrypted data
     * @returns zero for success, non-zero to indicate an error.
     *
     * @snippet nop_encrypt.c WT_ENCRYPTOR encrypt
     */
    int (*encrypt)(WT_ENCRYPTOR *encryptor, WT_SESSION *session,
        uint8_t *src, size_t src_len,
        uint8_t *dst, size_t dst_len,
        size_t *result_lenp);

    /*!
     * Callback to decrypt a chunk of data.
     *
     * WT_ENCRYPTOR::decrypt takes a source buffer and a destination buffer. The
     * contents are switched from \c encrypt: the source buffer is the encrypted
     * value, and the destination buffer is sized to be the original size of the
     * decrypted data. If the callback successfully decrypts the source buffer to the
     * destination buffer, it returns 0. If an error occurs, it returns an errno or
     * WiredTiger error code.
     *
     * On entry, \c src will point to memory, with the length of the memory in \c
     * src_len. After successful completion, the callback should return \c 0 and set
     * \c result_lenp to the number of bytes required for the decrypted
     * representation.
     *
     * If the \c dst buffer is not big enough to hold the decrypted data, the callback
     * should return an error.
     *
     * This callback cannot be NULL.
     *
     * @param[in] src the data to decrypt
     * @param[in] src_len the length of the data to decrypt
     * @param[in] dst the destination buffer
     * @param[in] dst_len the length of the destination buffer
     * @param[out] result_lenp the length of the decrypted data
     * @returns zero for success, non-zero to indicate an error.
     *
     * @snippet nop_encrypt.c WT_ENCRYPTOR decrypt
     */
    int (*decrypt)(WT_ENCRYPTOR *encryptor, WT_SESSION *session,
        uint8_t *src, size_t src_len,
        uint8_t *dst, size_t dst_len,
        size_t *result_lenp);

    /*!
     * Callback to size a destination buffer for encryption.
     *
     * WT_ENCRYPTOR::sizing is an callback that returns the number of additional bytes
     * that is needed when encrypting a data block. This is always necessary, since
     * encryptors should always generate some sort of cryptographic checksum as well
     * as the ciphertext. Without such a call, WiredTiger would have no way to know
     * the worst case for the encrypted buffer size.
     *
     * The WiredTiger encryption infrastructure assumes that buffer sizing is not
     * dependent on the number of bytes of input, that there is a one-to-one
     * relationship in number of bytes needed between input and output. This means
     * that if the encryption uses a block cipher in such a way that the input size
     * needs to be padded to the cipher block size, the sizing method should return
     * the worst case to ensure enough space is available.
     *
     * This callback cannot be NULL.
     *
     * The callback should set \c expansion_constantp to the additional number of
     * bytes needed.
     *
     * @param[out] expansion_constantp the additional number of bytes needed when
     *    encrypting.
     * @returns zero for success, non-zero to indicate an error.
     *
     * @snippet nop_encrypt.c WT_ENCRYPTOR sizing
     */
    int (*sizing)(WT_ENCRYPTOR *encryptor, WT_SESSION *session,
        size_t *expansion_constantp);

    /*!
     * If non-NULL, this callback is called to load keys into the encryptor. (That
     * is, "customize" it for a given key.) The customize function is called whenever
     * a new keyid is used for the first time with this encryptor, whether it be in
     * the ::wiredtiger_open call or the WT_SESSION::create call. This should create a
     * new encryptor instance and insert the requested key in it.
     *
     * The key may be specified either via \c keyid or \c secretkey in the \c
     * encrypt_config parameter. In the former case, the encryptor should look up the
     * requested key ID with whatever key management service is in use and install it
     * in the new encryptor. In the latter case, the encryptor should save the
     * provided secret key (or some transformation of it) in the new
     * encryptor. Further encryption with the same \c keyid will use this new
     * encryptor instance. (In the case of \c secretkey, only one key can be
     * configured, for the system encryption, and the new encryptor will be used for
     * all encryption involving it.) See @ref encryption for more information.
     *
     * This callback may return NULL as the new encryptor, in which case the original
     * encryptor will be used for further operations on the selected key. Unless this
     * happens, the original encryptor structure created during extension
     * initialization will never be used for encryption or decryption.
     *
     * This callback may itself be NULL, in which case it is not called, but in that
     * case there is no way to configure a key. This may be suitable for an
     * environment where a key management service returns a single key under a
     * well-known name that can be compiled in, but in a more general environment is
     * not a useful approach. One should of course never compile in actual keys!
     *
     * @param[in] encrypt_config the "encryption" portion of the configuration from
     *    the wiredtiger_open or WT_SESSION::create call, containing the \c keyid or
     *    \c secretkey setting.
     * @param[out] customp the new modified encryptor, or NULL.
     * @returns zero for success, non-zero to indicate an error.
     */
    int (*customize)(WT_ENCRYPTOR *encryptor, WT_SESSION *session,
        WT_CONFIG_ARG *encrypt_config, WT_ENCRYPTOR **customp);

    /*!
     * If non-NULL, a callback performed when the database is closed. It is called for
     * each encryptor that was added using WT_CONNECTION::add_encryptor or returned by
     * the WT_ENCRYPTOR::customize callback.
     *
     * The WT_ENCRYPTOR::terminate callback is intended to allow cleanup; the handle
     * will not be subsequently accessed by WiredTiger.
     *
     * @snippet nop_encrypt.c WT_ENCRYPTOR terminate
     */
    int (*terminate)(WT_ENCRYPTOR *encryptor, WT_SESSION *session);
};

/*!
 * The interface implemented by applications to provide custom extraction of
 * index keys or column group values.
 *
 * Applications register implementations with WiredTiger by calling
 * WT_CONNECTION::add_extractor.  See @ref custom_extractors for more
 * information.
 *
 * @snippet ex_all.c WT_EXTRACTOR register
 */
struct __wt_extractor {
    /*!
     * Callback to extract a value for an index or column group.
     *
     * @errors
     *
     * @snippet ex_all.c WT_EXTRACTOR
     *
     * @param extractor the WT_EXTRACTOR implementation
     * @param session the current WiredTiger session
     * @param key the table key in raw format, see @ref cursor_raw for
     *  details
     * @param value the table value in raw format, see @ref cursor_raw for
     *  details
     * @param[out] result_cursor the method should call WT_CURSOR::set_key
     *  and WT_CURSOR::insert on this cursor to return a key.  The \c
     *  key_format of the cursor will match that passed to
     *  WT_SESSION::create for the index.  Multiple index keys can be
     *  created for each record by calling WT_CURSOR::insert multiple
     *  times.
     */
    int (*extract)(WT_EXTRACTOR *extractor, WT_SESSION *session,
        const WT_ITEM *key, const WT_ITEM *value,
        WT_CURSOR *result_cursor);

    /*!
     * If non-NULL, this callback is called to customize the extractor for
     * each index.  If the callback returns a non-NULL extractor, that
     * instance is used instead of this one for all comparisons.
     */
    int (*customize)(WT_EXTRACTOR *extractor, WT_SESSION *session,
        const char *uri, WT_CONFIG_ITEM *appcfg, WT_EXTRACTOR **customp);

    /*!
     * If non-NULL a callback performed when the index or column group
     * is closed for customized extractors otherwise when the database
     * is closed.
     *
     * The WT_EXTRACTOR::terminate callback is intended to allow cleanup;
     * the handle will not be subsequently accessed by WiredTiger.
     */
    int (*terminate)(WT_EXTRACTOR *extractor, WT_SESSION *session);
};

/*! WT_FILE_SYSTEM::open_file file types */
typedef enum {
    WT_FS_OPEN_FILE_TYPE_CHECKPOINT,/*!< open a data file checkpoint */
    WT_FS_OPEN_FILE_TYPE_DATA,  /*!< open a data file */
    WT_FS_OPEN_FILE_TYPE_DIRECTORY, /*!< open a directory */
    WT_FS_OPEN_FILE_TYPE_LOG,   /*!< open a log file */
    WT_FS_OPEN_FILE_TYPE_REGULAR    /*!< open a regular file */
} WT_FS_OPEN_FILE_TYPE;

#ifdef DOXYGEN
/*! WT_FILE_SYSTEM::open_file flags: random access pattern */
#define WT_FS_OPEN_ACCESS_RAND  0x0
/*! WT_FILE_SYSTEM::open_file flags: sequential access pattern */
#define WT_FS_OPEN_ACCESS_SEQ   0x0
/*! WT_FILE_SYSTEM::open_file flags: create if does not exist */
#define WT_FS_OPEN_CREATE   0x0
/*! WT_FILE_SYSTEM::open_file flags: direct I/O requested */
#define WT_FS_OPEN_DIRECTIO 0x0
/*! WT_FILE_SYSTEM::open_file flags: file creation must be durable */
#define WT_FS_OPEN_DURABLE  0x0
/*!
 * WT_FILE_SYSTEM::open_file flags: return EBUSY if exclusive use not available
 */
#define WT_FS_OPEN_EXCLUSIVE    0x0
/*! WT_FILE_SYSTEM::open_file flags: open is read-only */
#define WT_FS_OPEN_READONLY 0x0

/*!
 * WT_FILE_SYSTEM::remove or WT_FILE_SYSTEM::rename flags: the remove or rename
 * operation must be durable
 */
#define WT_FS_DURABLE       0x0
#else
/* AUTOMATIC FLAG VALUE GENERATION START 0 */
#define WT_FS_OPEN_ACCESS_RAND  0x001u
#define WT_FS_OPEN_ACCESS_SEQ   0x002u
#define WT_FS_OPEN_CREATE   0x004u
#define WT_FS_OPEN_DIRECTIO 0x008u
#define WT_FS_OPEN_DURABLE  0x010u
#define WT_FS_OPEN_EXCLUSIVE    0x020u
#define WT_FS_OPEN_FIXED    0x040u   /* Path not home relative (internal) */
#define WT_FS_OPEN_FORCE_MMAP 0x080u
#define WT_FS_OPEN_READONLY 0x100u
/* AUTOMATIC FLAG VALUE GENERATION STOP 32 */

/* AUTOMATIC FLAG VALUE GENERATION START 0 */
#define WT_FS_DURABLE       0x1u
/* AUTOMATIC FLAG VALUE GENERATION STOP 32 */
#endif

/*!
 * The interface implemented by applications to provide a custom file system
 * implementation.
 *
 * <b>Thread safety:</b> WiredTiger may invoke methods on the WT_FILE_SYSTEM
 * interface from multiple threads concurrently. It is the responsibility of
 * the implementation to protect any shared data.
 *
 * Applications register implementations with WiredTiger by calling
 * WT_CONNECTION::set_file_system.  See @ref custom_file_systems for more
 * information.
 *
 * @snippet ex_file_system.c WT_FILE_SYSTEM register
 */
struct __wt_file_system {
    /*!
     * Return a list of file names for the named directory.
     *
     * @errors
     *
     * @param file_system the WT_FILE_SYSTEM
     * @param session the current WiredTiger session
     * @param directory the name of the directory
     * @param prefix if not NULL, only files with names matching the prefix
     *    are returned
     * @param[out] dirlist the method returns an allocated array of
     *    individually allocated strings, one for each entry in the
     *    directory.
     * @param[out] countp the number of entries returned
     */
    int (*fs_directory_list)(WT_FILE_SYSTEM *file_system,
        WT_SESSION *session, const char *directory, const char *prefix,
        char ***dirlist, uint32_t *countp);

#if !defined(DOXYGEN)
    /*
     * Return a single file name for the named directory.
     */
    int (*fs_directory_list_single)(WT_FILE_SYSTEM *file_system,
        WT_SESSION *session, const char *directory, const char *prefix,
        char ***dirlist, uint32_t *countp);
#endif

    /*!
     * Free memory allocated by WT_FILE_SYSTEM::directory_list.
     *
     * @errors
     *
     * @param file_system the WT_FILE_SYSTEM
     * @param session the current WiredTiger session
     * @param dirlist array returned by WT_FILE_SYSTEM::directory_list
     * @param count count returned by WT_FILE_SYSTEM::directory_list
     */
    int (*fs_directory_list_free)(WT_FILE_SYSTEM *file_system,
        WT_SESSION *session, char **dirlist, uint32_t count);

    /*!
     * Return if the named file system object exists.
     *
     * @errors
     *
     * @param file_system the WT_FILE_SYSTEM
     * @param session the current WiredTiger session
     * @param name the name of the file
     * @param[out] existp If the named file system object exists
     */
    int (*fs_exist)(WT_FILE_SYSTEM *file_system,
        WT_SESSION *session, const char *name, bool *existp);

    /*!
     * Open a handle for a named file system object
     *
     * The method should return ENOENT if the file is not being created and
     * does not exist.
     *
     * The method should return EACCES if the file cannot be opened in the
     * requested mode (for example, a file opened for writing in a readonly
     * file system).
     *
     * The method should return EBUSY if ::WT_FS_OPEN_EXCLUSIVE is set and
     * the file is in use.
     *
     * @errors
     *
     * @param file_system the WT_FILE_SYSTEM
     * @param session the current WiredTiger session
     * @param name the name of the file system object
     * @param file_type the type of the file
     *    The file type is provided to allow optimization for different file
     *    access patterns.
     * @param flags flags indicating how to open the file, one or more of
     *    ::WT_FS_OPEN_CREATE, ::WT_FS_OPEN_DIRECTIO, ::WT_FS_OPEN_DURABLE,
     *    ::WT_FS_OPEN_EXCLUSIVE or ::WT_FS_OPEN_READONLY.
     * @param[out] file_handlep the handle to the newly opened file. File
     *    system implementations must allocate memory for the handle and
     *    the WT_FILE_HANDLE::name field, and fill in the WT_FILE_HANDLE::
     *    fields. Applications wanting to associate private information
     *    with the WT_FILE_HANDLE:: structure should declare and allocate
     *    their own structure as a superset of a WT_FILE_HANDLE:: structure.
     */
    int (*fs_open_file)(WT_FILE_SYSTEM *file_system, WT_SESSION *session,
        const char *name, WT_FS_OPEN_FILE_TYPE file_type, uint32_t flags,
        WT_FILE_HANDLE **file_handlep);

    /*!
     * Remove a named file system object
     *
     * This method is not required for readonly file systems and should be
     * set to NULL when not required by the file system.
     *
     * @errors
     *
     * @param file_system the WT_FILE_SYSTEM
     * @param session the current WiredTiger session
     * @param name the name of the file system object
     * @param flags 0 or ::WT_FS_DURABLE
     */
    int (*fs_remove)(WT_FILE_SYSTEM *file_system,
        WT_SESSION *session, const char *name, uint32_t flags);

    /*!
     * Rename a named file system object
     *
     * This method is not required for readonly file systems and should be
     * set to NULL when not required by the file system.
     *
     * @errors
     *
     * @param file_system the WT_FILE_SYSTEM
     * @param session the current WiredTiger session
     * @param from the original name of the object
     * @param to the new name for the object
     * @param flags 0 or ::WT_FS_DURABLE
     */
    int (*fs_rename)(WT_FILE_SYSTEM *file_system, WT_SESSION *session,
        const char *from, const char *to, uint32_t flags);

    /*!
     * Return the size of a named file system object
     *
     * @errors
     *
     * @param file_system the WT_FILE_SYSTEM
     * @param session the current WiredTiger session
     * @param name the name of the file system object
     * @param[out] sizep the size of the file system entry
     */
    int (*fs_size)(WT_FILE_SYSTEM *file_system,
        WT_SESSION *session, const char *name, wt_off_t *sizep);

    /*!
     * A callback performed when the file system is closed and will no
     * longer be accessed by the WiredTiger database.
     *
     * This method is not required and should be set to NULL when not
     * required by the file system.
     *
     * The WT_FILE_SYSTEM::terminate callback is intended to allow cleanup;
     * the handle will not be subsequently accessed by WiredTiger.
     */
    int (*terminate)(WT_FILE_SYSTEM *file_system, WT_SESSION *session);
};

/*! WT_FILE_HANDLE::fadvise flags: no longer need */
#define WT_FILE_HANDLE_DONTNEED 1
/*! WT_FILE_HANDLE::fadvise flags: will need */
#define WT_FILE_HANDLE_WILLNEED 2

/*!
 * A file handle implementation returned by WT_FILE_SYSTEM::fs_open_file.
 *
 * <b>Thread safety:</b> Unless explicitly stated otherwise, WiredTiger may
 * invoke methods on the WT_FILE_HANDLE interface from multiple threads
 * concurrently. It is the responsibility of the implementation to protect
 * any shared data.
 *
 * See @ref custom_file_systems for more information.
 */
struct __wt_file_handle {
    /*!
     * The enclosing file system, set by WT_FILE_SYSTEM::fs_open_file.
     */
    WT_FILE_SYSTEM *file_system;

    /*!
     * The name of the file, set by WT_FILE_SYSTEM::fs_open_file.
     */
    char *name;

    /*!
     * Close a file handle. The handle will not be further accessed by
     * WiredTiger.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     */
    int (*close)(WT_FILE_HANDLE *file_handle, WT_SESSION *session);

    /*!
     * Indicate expected future use of file ranges, based on the POSIX
     * 1003.1 standard fadvise.
     *
     * This method is not required, and should be set to NULL when not
     * supported by the file.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     * @param offset the file offset
     * @param len the size of the advisory
     * @param advice one of ::WT_FILE_HANDLE_WILLNEED or
     *    ::WT_FILE_HANDLE_DONTNEED.
     */
    int (*fh_advise)(WT_FILE_HANDLE *file_handle,
        WT_SESSION *session, wt_off_t offset, wt_off_t len, int advice);

    /*!
     * Extend the file.
     *
     * This method is not required, and should be set to NULL when not
     * supported by the file.
     *
     * Any allocated disk space must read as 0 bytes, and no existing file
     * data may change. Allocating all necessary underlying storage (not
     * changing just the file's metadata), is likely to result in increased
     * performance.
     *
     * This method is not called by multiple threads concurrently (on the
     * same file handle). If the file handle's extension method supports
     * concurrent calls, set the WT_FILE_HANDLE::fh_extend_nolock method
     * instead. See @ref custom_file_systems for more information.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     * @param offset desired file size after extension
     */
    int (*fh_extend)(
        WT_FILE_HANDLE *file_handle, WT_SESSION *session, wt_off_t offset);

    /*!
     * Extend the file.
     *
     * This method is not required, and should be set to NULL when not
     * supported by the file.
     *
     * Any allocated disk space must read as 0 bytes, and no existing file
     * data may change. Allocating all necessary underlying storage (not
     * only changing the file's metadata), is likely to result in increased
     * performance.
     *
     * This method may be called by multiple threads concurrently (on the
     * same file handle). If the file handle's extension method does not
     * support concurrent calls, set the WT_FILE_HANDLE::fh_extend method
     * instead. See @ref custom_file_systems for more information.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     * @param offset desired file size after extension
     */
    int (*fh_extend_nolock)(
        WT_FILE_HANDLE *file_handle, WT_SESSION *session, wt_off_t offset);

    /*!
     * Lock/unlock a file from the perspective of other processes running
     * in the system, where necessary.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     * @param lock whether to lock or unlock
     */
    int (*fh_lock)(
        WT_FILE_HANDLE *file_handle, WT_SESSION *session, bool lock);

    /*!
     * Map a file into memory, based on the POSIX 1003.1 standard mmap.
     *
     * This method is not required, and should be set to NULL when not
     * supported by the file.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     * @param[out] mapped_regionp a reference to a memory location into
     *    which should be stored a pointer to the start of the mapped region
     * @param[out] lengthp a reference to a memory location into which
     *    should be stored the length of the region
     * @param[out] mapped_cookiep a reference to a memory location into
     *    which can be optionally stored a pointer to an opaque cookie
     *    which is subsequently passed to WT_FILE_HANDLE::unmap.
     */
    int (*fh_map)(WT_FILE_HANDLE *file_handle, WT_SESSION *session,
        void **mapped_regionp, size_t *lengthp, void **mapped_cookiep);

    /*!
     * Unmap part of a memory mapped file, based on the POSIX 1003.1
     * standard madvise.
     *
     * This method is not required, and should be set to NULL when not
     * supported by the file.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     * @param map a location in the mapped region unlikely to be used in the
     *    near future
     * @param length the length of the mapped region to discard
     * @param mapped_cookie any cookie set by the WT_FILE_HANDLE::map method
     */
    int (*fh_map_discard)(WT_FILE_HANDLE *file_handle,
        WT_SESSION *session, void *map, size_t length, void *mapped_cookie);

    /*!
     * Preload part of a memory mapped file, based on the POSIX 1003.1
     * standard madvise.
     *
     * This method is not required, and should be set to NULL when not
     * supported by the file.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     * @param map a location in the mapped region likely to be used in the
     *    near future
     * @param length the size of the mapped region to preload
     * @param mapped_cookie any cookie set by the WT_FILE_HANDLE::map method
     */
    int (*fh_map_preload)(WT_FILE_HANDLE *file_handle, WT_SESSION *session,
        const void *map, size_t length, void *mapped_cookie);

    /*!
     * Unmap a memory mapped file, based on the POSIX 1003.1 standard
     * munmap.
     *
     * This method is only required if a valid implementation of map is
     * provided by the file, and should be set to NULL otherwise.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     * @param mapped_region a pointer to the start of the mapped region
     * @param length the length of the mapped region
     * @param mapped_cookie any cookie set by the WT_FILE_HANDLE::map method
     */
    int (*fh_unmap)(WT_FILE_HANDLE *file_handle, WT_SESSION *session,
        void *mapped_region, size_t length, void *mapped_cookie);

    /*!
     * Read from a file, based on the POSIX 1003.1 standard pread.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     * @param offset the offset in the file to start reading from
     * @param len the amount to read
     * @param[out] buf buffer to hold the content read from file
     */
    int (*fh_read)(WT_FILE_HANDLE *file_handle,
        WT_SESSION *session, wt_off_t offset, size_t len, void *buf);

    /*!
     * Return the size of a file.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     * @param sizep the size of the file
     */
    int (*fh_size)(
        WT_FILE_HANDLE *file_handle, WT_SESSION *session, wt_off_t *sizep);

    /*!
     * Make outstanding file writes durable and do not return until writes
     * are complete.
     *
     * This method is not required for read-only files, and should be set
     * to NULL when not supported by the file.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     */
    int (*fh_sync)(WT_FILE_HANDLE *file_handle, WT_SESSION *session);

    /*!
     * Schedule the outstanding file writes required for durability and
     * return immediately.
     *
     * This method is not required, and should be set to NULL when not
     * supported by the file.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     */
    int (*fh_sync_nowait)(WT_FILE_HANDLE *file_handle, WT_SESSION *session);

    /*!
     * Truncate the file.
     *
     * This method is not required, and should be set to NULL when not
     * supported by the file.
     *
     * This method is not called by multiple threads concurrently (on the
     * same file handle).
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     * @param offset desired file size after truncate
     */
    int (*fh_truncate)(
        WT_FILE_HANDLE *file_handle, WT_SESSION *session, wt_off_t offset);

    /*!
     * Write to a file, based on the POSIX 1003.1 standard pwrite.
     *
     * This method is not required for read-only files, and should be set
     * to NULL when not supported by the file.
     *
     * @errors
     *
     * @param file_handle the WT_FILE_HANDLE
     * @param session the current WiredTiger session
     * @param offset offset at which to start writing
     * @param length amount of data to write
     * @param buf content to be written to the file
     */
    int (*fh_write)(WT_FILE_HANDLE *file_handle, WT_SESSION *session,
        wt_off_t offset, size_t length, const void *buf);
};

#if !defined(DOXYGEN)
/* This interface is not yet public. */

/*!
 * The interface implemented by applications to provide a storage source
 * implementation. This documentation refers to "object" and "bucket"
 * to mean a "file-like object" and a "container of objects", respectively.
 *
 * <b>Thread safety:</b> WiredTiger may invoke methods on the WT_STORAGE_SOURCE
 * interface from multiple threads concurrently. It is the responsibility of
 * the implementation to protect any shared data.
 *
 * Applications register implementations with WiredTiger by calling
 * WT_CONNECTION::add_storage_source.
 *
 * @snippet ex_storage_source.c WT_STORAGE_SOURCE register
 */
struct __wt_storage_source {
    /*!
     * A reference is added to the storage source.  The reference is released by a
     * call to WT_STORAGE_SOURCE::terminate.  A reference is added as a side effect
     * of calling WT_CONNECTION::get_storage_source.
     *
     * @errors
     *
     * @param storage_source the WT_STORAGE_SOURCE
     */
    int (*ss_add_reference)(WT_STORAGE_SOURCE *storage_source);

    /*!
     * Create a customized file system to access the storage source
     * objects.
     *
     * The file system returned behaves as if objects in the specified buckets are
     * files in the file system.  In particular, the fs_open_file method requires
     * its flags argument to include either WT_FS_OPEN_CREATE or WT_FS_OPEN_READONLY.
     * Objects being created are not deemed to "exist" and be visible to
     * WT_FILE_SYSTEM::fs_exist and other file system methods until the new handle has
     * been closed.  Objects once created are immutable. That is, only objects that
     * do not already exist can be opened with the create flag, and objects that
     * already exist can only be opened with the readonly flag.  Only objects that
     * exist can be transferred to the underlying shared object storage.  This can
     * happen at any time after an object is created, and can be forced to happen using
     * WT_STORAGE_SOURCE::ss_flush.
     *
     * Additionally file handles returned by the file system behave as file handles to a
     * local file.  For example, WT_FILE_HANDLE::fh_sync synchronizes writes to the
     * local file, and does not imply any transferring of data to the shared object store.
     *
     * The directory argument to the WT_FILE_SYSTEM::fs_directory_list method is normally
     * the empty string as the cloud equivalent (bucket) has already been given when
     * customizing the file system.  If specified, the directory path is interpreted
     * as another prefix, which is removed from the results.
     *
     * Names used by the file system methods are generally flat.  However, in some
     * implementations of a file system returned by a storage source, "..", ".", "/"
     * may have a particular meaning, as in a POSIX file system.  We suggest that
     * these constructs be avoided when a caller chooses file names within the returned
     * file system; they may be rejected by the implementation.  Within a bucket name,
     * these characters may or may not be acceptable. That is implementation dependent.
     * In the prefix, "/" is specifically allowed, as this may have performance or
     * administrative benefits.  That said, within a prefix, certain combinations
     * involving "/" may be rejected, for example "/../".
     *
     * @errors
     *
     * @param storage_source the WT_STORAGE_SOURCE
     * @param session the current WiredTiger session
     * @param bucket_name the name of the bucket.  Use of '/' is implementation dependent.
     * @param auth_token the authorization identifier.
     * @param config additional configuration. The only allowable value is \c cache_directory,
     *    the name of a directory holding cached objects. Its default is
     *    \c "<home>/cache-<bucket>" with \c <home> replaced by the @ref home, and
     *    \c <bucket> replaced by the bucket_name.
     * @param[out] file_system the customized file system returned
     */
    int (*ss_customize_file_system)(WT_STORAGE_SOURCE *storage_source, WT_SESSION *session,
        const char *bucket_name, const char *auth_token, const char *config,
        WT_FILE_SYSTEM **file_system);

    /*!
     * Copy a file from the default file system to an object name in shared object storage.
     *
     * @errors
     *
     * @param storage_source the WT_STORAGE_SOURCE
     * @param session the current WiredTiger session
     * @param file_system the destination bucket and credentials
     * @param source the name of the source input file
     * @param object the name of the destination object
     * @param config additional configuration, currently must be NULL
     */
    int (*ss_flush)(WT_STORAGE_SOURCE *storage_source, WT_SESSION *session,
        WT_FILE_SYSTEM *file_system, const char *source, const char *object,
            const char *config);

    /*!
     * After a flush, rename the source file from the default file system to be cached in
     * the shared object storage.
     *
     * @errors
     *
     * @param storage_source the WT_STORAGE_SOURCE
     * @param session the current WiredTiger session
     * @param file_system the destination bucket and credentials
     * @param source the name of the source input file
     * @param object the name of the destination object
     * @param config additional configuration, currently must be NULL
     */
    int (*ss_flush_finish)(WT_STORAGE_SOURCE *storage_source, WT_SESSION *session,
        WT_FILE_SYSTEM *file_system, const char *source, const char *object,
        const char *config);

    /*!
     * A callback performed when the storage source or reference is closed
     * and will no longer be used.  The initial creation of the storage source
     * counts as a reference, and each call to WT_STORAGE_SOURCE::add_reference
     * increase the number of references.  When all references are released, the
     * storage source and any resources associated with it are released.
     *
     * This method is not required and should be set to NULL when not
     * required by the storage source implementation.
     *
     * The WT_STORAGE_SOURCE::terminate callback is intended to allow cleanup;
     * the handle will not be subsequently accessed by WiredTiger.
     */
    int (*terminate)(WT_STORAGE_SOURCE *storage_source, WT_SESSION *session);
};
#endif

/*!
 * Entry point to an extension, called when the extension is loaded.
 *
 * @param connection the connection handle
 * @param config the config information passed to WT_CONNECTION::load_extension
 * @errors
 */
extern int wiredtiger_extension_init(
    WT_CONNECTION *connection, WT_CONFIG_ARG *config);

/*!
 * Optional cleanup function for an extension, called during
 * WT_CONNECTION::close.
 *
 * @param connection the connection handle
 * @errors
 */
extern int wiredtiger_extension_terminate(WT_CONNECTION *connection);

/*! @} */

/*!
 * @addtogroup wt
 * @{
 */
/*!
 * @name Incremental backup types
 * @anchor backup_types
 * @{
 */
/*! Invalid backup type. */
#define WT_BACKUP_INVALID   0
/*! Whole file. */
#define WT_BACKUP_FILE      1
/*! File range. */
#define WT_BACKUP_RANGE     2
/*! @} */

/*!
 * @name Log record and operation types
 * @anchor log_types
 * @{
 */
/*
 * NOTE:  The values of these record types and operations must
 * never change because they're written into the log.  Append
 * any new records or operations to the appropriate set.
 */
/*! Checkpoint. */
#define WT_LOGREC_CHECKPOINT    0
/*! Transaction commit. */
#define WT_LOGREC_COMMIT    1
/*! File sync. */
#define WT_LOGREC_FILE_SYNC 2
/*! Message. */
#define WT_LOGREC_MESSAGE   3
/*! System/internal record. */
#define WT_LOGREC_SYSTEM    4
/*! Invalid operation. */
#define WT_LOGOP_INVALID    0
/*! Column-store put. */
#define WT_LOGOP_COL_PUT    1
/*! Column-store remove. */
#define WT_LOGOP_COL_REMOVE 2
/*! Column-store truncate. */
#define WT_LOGOP_COL_TRUNCATE   3
/*! Row-store put. */
#define WT_LOGOP_ROW_PUT    4
/*! Row-store remove. */
#define WT_LOGOP_ROW_REMOVE 5
/*! Row-store truncate. */
#define WT_LOGOP_ROW_TRUNCATE   6
/*! Checkpoint start. */
#define WT_LOGOP_CHECKPOINT_START   7
/*! Previous LSN. */
#define WT_LOGOP_PREV_LSN   8
/*! Column-store modify. */
#define WT_LOGOP_COL_MODIFY 9
/*! Row-store modify. */
#define WT_LOGOP_ROW_MODIFY 10
/*
 * NOTE: Diagnostic-only log operations should have values in
 * the ignore range.
 */
/*! Diagnostic: transaction timestamps */
#define WT_LOGOP_TXN_TIMESTAMP  (WT_LOGOP_IGNORE | 11)
/*! Incremental backup IDs. */
#define WT_LOGOP_BACKUP_ID 12
/*! @} */

/*******************************************
 * Statistic reference.
 *******************************************/
/*
 * DO NOT EDIT: automatically built by dist/stat.py.
 * Statistics section: BEGIN
 */

/*!
 * @name Connection statistics
 * @anchor statistics_keys
 * @anchor statistics_conn
 * Statistics are accessed through cursors with \c "statistics:" URIs.
 * Individual statistics can be queried through the cursor using the following
 * keys.  See @ref data_statistics for more information.
 * @{
 */
/*! LSM: application work units currently queued */
#define	WT_STAT_CONN_LSM_WORK_QUEUE_APP			1000
/*! LSM: merge work units currently queued */
#define	WT_STAT_CONN_LSM_WORK_QUEUE_MANAGER		1001
/*! LSM: rows merged in an LSM tree */
#define	WT_STAT_CONN_LSM_ROWS_MERGED			1002
/*! LSM: sleep for LSM checkpoint throttle */
#define	WT_STAT_CONN_LSM_CHECKPOINT_THROTTLE		1003
/*! LSM: sleep for LSM merge throttle */
#define	WT_STAT_CONN_LSM_MERGE_THROTTLE			1004
/*! LSM: switch work units currently queued */
#define	WT_STAT_CONN_LSM_WORK_QUEUE_SWITCH		1005
/*! LSM: tree maintenance operations discarded */
#define	WT_STAT_CONN_LSM_WORK_UNITS_DISCARDED		1006
/*! LSM: tree maintenance operations executed */
#define	WT_STAT_CONN_LSM_WORK_UNITS_DONE		1007
/*! LSM: tree maintenance operations scheduled */
#define	WT_STAT_CONN_LSM_WORK_UNITS_CREATED		1008
/*! LSM: tree queue hit maximum */
#define	WT_STAT_CONN_LSM_WORK_QUEUE_MAX			1009
/*! autocommit: retries for readonly operations */
#define	WT_STAT_CONN_AUTOCOMMIT_READONLY_RETRY		1010
/*! autocommit: retries for update operations */
#define	WT_STAT_CONN_AUTOCOMMIT_UPDATE_RETRY		1011
/*! background-compact: background compact failed calls */
#define	WT_STAT_CONN_BACKGROUND_COMPACT_FAIL		1012
/*!
 * background-compact: background compact failed calls due to cache
 * pressure
 */
#define	WT_STAT_CONN_BACKGROUND_COMPACT_FAIL_CACHE_PRESSURE	1013
/*! background-compact: background compact interrupted */
#define	WT_STAT_CONN_BACKGROUND_COMPACT_INTERRUPTED	1014
/*!
 * background-compact: background compact moving average of bytes
 * rewritten
 */
#define	WT_STAT_CONN_BACKGROUND_COMPACT_EMA		1015
/*! background-compact: background compact recovered bytes */
#define	WT_STAT_CONN_BACKGROUND_COMPACT_BYTES_RECOVERED	1016
/*! background-compact: background compact running */
#define	WT_STAT_CONN_BACKGROUND_COMPACT_RUNNING		1017
/*!
 * background-compact: background compact skipped file as it is part of
 * the exclude list
 */
#define	WT_STAT_CONN_BACKGROUND_COMPACT_EXCLUDE		1018
/*!
 * background-compact: background compact skipped file as not meeting
 * requirements for compaction
 */
#define	WT_STAT_CONN_BACKGROUND_COMPACT_SKIPPED		1019
/*! background-compact: background compact successful calls */
#define	WT_STAT_CONN_BACKGROUND_COMPACT_SUCCESS		1020
/*! background-compact: background compact timeout */
#define	WT_STAT_CONN_BACKGROUND_COMPACT_TIMEOUT		1021
/*! background-compact: number of files tracked by background compaction */
#define	WT_STAT_CONN_BACKGROUND_COMPACT_FILES_TRACKED	1022
/*! block-cache: cached blocks updated */
#define	WT_STAT_CONN_BLOCK_CACHE_BLOCKS_UPDATE		1023
/*! block-cache: cached bytes updated */
#define	WT_STAT_CONN_BLOCK_CACHE_BYTES_UPDATE		1024
/*! block-cache: could not perform pre-fetch on internal page */
#define	WT_STAT_CONN_BLOCK_PREFETCH_SKIPPED_INTERNAL_PAGE	1025
/*!
 * block-cache: could not perform pre-fetch on ref without the pre-fetch
 * flag set
 */
#define	WT_STAT_CONN_BLOCK_PREFETCH_SKIPPED_NO_FLAG_SET	1026
/*! block-cache: evicted blocks */
#define	WT_STAT_CONN_BLOCK_CACHE_BLOCKS_EVICTED		1027
/*! block-cache: file size causing bypass */
#define	WT_STAT_CONN_BLOCK_CACHE_BYPASS_FILESIZE	1028
/*! block-cache: lookups */
#define	WT_STAT_CONN_BLOCK_CACHE_LOOKUPS		1029
/*! block-cache: number of blocks not evicted due to overhead */
#define	WT_STAT_CONN_BLOCK_CACHE_NOT_EVICTED_OVERHEAD	1030
/*!
 * block-cache: number of bypasses because no-write-allocate setting was
 * on
 */
#define	WT_STAT_CONN_BLOCK_CACHE_BYPASS_WRITEALLOC	1031
/*! block-cache: number of bypasses due to overhead on put */
#define	WT_STAT_CONN_BLOCK_CACHE_BYPASS_OVERHEAD_PUT	1032
/*! block-cache: number of bypasses on get */
#define	WT_STAT_CONN_BLOCK_CACHE_BYPASS_GET		1033
/*! block-cache: number of bypasses on put because file is too small */
#define	WT_STAT_CONN_BLOCK_CACHE_BYPASS_PUT		1034
/*! block-cache: number of eviction passes */
#define	WT_STAT_CONN_BLOCK_CACHE_EVICTION_PASSES	1035
/*! block-cache: number of hits */
#define	WT_STAT_CONN_BLOCK_CACHE_HITS			1036
/*! block-cache: number of misses */
#define	WT_STAT_CONN_BLOCK_CACHE_MISSES			1037
/*! block-cache: number of put bypasses on checkpoint I/O */
#define	WT_STAT_CONN_BLOCK_CACHE_BYPASS_CHKPT		1038
/*! block-cache: number of times pre-fetch failed to start */
#define	WT_STAT_CONN_BLOCK_PREFETCH_FAILED_START	1039
/*! block-cache: pre-fetch not repeating for recently pre-fetched ref */
#define	WT_STAT_CONN_BLOCK_PREFETCH_SKIPPED_SAME_REF	1040
/*! block-cache: pre-fetch not triggered after single disk read */
#define	WT_STAT_CONN_BLOCK_PREFETCH_DISK_ONE		1041
/*! block-cache: pre-fetch not triggered as there is no valid dhandle */
#define	WT_STAT_CONN_BLOCK_PREFETCH_SKIPPED_NO_VALID_DHANDLE	1042
/*! block-cache: pre-fetch not triggered by page read */
#define	WT_STAT_CONN_BLOCK_PREFETCH_SKIPPED		1043
/*! block-cache: pre-fetch not triggered due to disk read count */
#define	WT_STAT_CONN_BLOCK_PREFETCH_SKIPPED_DISK_READ_COUNT	1044
/*! block-cache: pre-fetch not triggered due to internal session */
#define	WT_STAT_CONN_BLOCK_PREFETCH_SKIPPED_INTERNAL_SESSION	1045
/*! block-cache: pre-fetch not triggered due to special btree handle */
#define	WT_STAT_CONN_BLOCK_PREFETCH_SKIPPED_SPECIAL_HANDLE	1046
/*! block-cache: pre-fetch page not on disk when reading */
#define	WT_STAT_CONN_BLOCK_PREFETCH_PAGES_FAIL		1047
/*! block-cache: pre-fetch pages queued */
#define	WT_STAT_CONN_BLOCK_PREFETCH_PAGES_QUEUED	1048
/*! block-cache: pre-fetch pages read in background */
#define	WT_STAT_CONN_BLOCK_PREFETCH_PAGES_READ		1049
/*! block-cache: pre-fetch triggered by page read */
#define	WT_STAT_CONN_BLOCK_PREFETCH_ATTEMPTS		1050
/*! block-cache: removed blocks */
#define	WT_STAT_CONN_BLOCK_CACHE_BLOCKS_REMOVED		1051
/*! block-cache: time sleeping to remove block (usecs) */
#define	WT_STAT_CONN_BLOCK_CACHE_BLOCKS_REMOVED_BLOCKED	1052
/*! block-cache: total blocks */
#define	WT_STAT_CONN_BLOCK_CACHE_BLOCKS			1053
/*! block-cache: total blocks inserted on read path */
#define	WT_STAT_CONN_BLOCK_CACHE_BLOCKS_INSERT_READ	1054
/*! block-cache: total blocks inserted on write path */
#define	WT_STAT_CONN_BLOCK_CACHE_BLOCKS_INSERT_WRITE	1055
/*! block-cache: total bytes */
#define	WT_STAT_CONN_BLOCK_CACHE_BYTES			1056
/*! block-cache: total bytes inserted on read path */
#define	WT_STAT_CONN_BLOCK_CACHE_BYTES_INSERT_READ	1057
/*! block-cache: total bytes inserted on write path */
#define	WT_STAT_CONN_BLOCK_CACHE_BYTES_INSERT_WRITE	1058
/*! block-manager: blocks pre-loaded */
#define	WT_STAT_CONN_BLOCK_PRELOAD			1059
/*! block-manager: blocks read */
#define	WT_STAT_CONN_BLOCK_READ				1060
/*! block-manager: blocks written */
#define	WT_STAT_CONN_BLOCK_WRITE			1061
/*! block-manager: bytes read */
#define	WT_STAT_CONN_BLOCK_BYTE_READ			1062
/*! block-manager: bytes read via memory map API */
#define	WT_STAT_CONN_BLOCK_BYTE_READ_MMAP		1063
/*! block-manager: bytes read via system call API */
#define	WT_STAT_CONN_BLOCK_BYTE_READ_SYSCALL		1064
/*! block-manager: bytes written */
#define	WT_STAT_CONN_BLOCK_BYTE_WRITE			1065
/*! block-manager: bytes written by compaction */
#define	WT_STAT_CONN_BLOCK_BYTE_WRITE_COMPACT		1066
/*! block-manager: bytes written for checkpoint */
#define	WT_STAT_CONN_BLOCK_BYTE_WRITE_CHECKPOINT	1067
/*! block-manager: bytes written via memory map API */
#define	WT_STAT_CONN_BLOCK_BYTE_WRITE_MMAP		1068
/*! block-manager: bytes written via system call API */
#define	WT_STAT_CONN_BLOCK_BYTE_WRITE_SYSCALL		1069
/*! block-manager: mapped blocks read */
#define	WT_STAT_CONN_BLOCK_MAP_READ			1070
/*! block-manager: mapped bytes read */
#define	WT_STAT_CONN_BLOCK_BYTE_MAP_READ		1071
/*!
 * block-manager: number of times the file was remapped because it
 * changed size via fallocate or truncate
 */
#define	WT_STAT_CONN_BLOCK_REMAP_FILE_RESIZE		1072
/*! block-manager: number of times the region was remapped via write */
#define	WT_STAT_CONN_BLOCK_REMAP_FILE_WRITE		1073
/*! cache: application threads page read from disk to cache count */
#define	WT_STAT_CONN_CACHE_READ_APP_COUNT		1074
/*! cache: application threads page read from disk to cache time (usecs) */
#define	WT_STAT_CONN_CACHE_READ_APP_TIME		1075
/*! cache: application threads page write from cache to disk count */
#define	WT_STAT_CONN_CACHE_WRITE_APP_COUNT		1076
/*! cache: application threads page write from cache to disk time (usecs) */
#define	WT_STAT_CONN_CACHE_WRITE_APP_TIME		1077
/*! cache: bytes allocated for updates */
#define	WT_STAT_CONN_CACHE_BYTES_UPDATES		1078
/*! cache: bytes belonging to page images in the cache */
#define	WT_STAT_CONN_CACHE_BYTES_IMAGE			1079
/*! cache: bytes belonging to the history store table in the cache */
#define	WT_STAT_CONN_CACHE_BYTES_HS			1080
/*! cache: bytes currently in the cache */
#define	WT_STAT_CONN_CACHE_BYTES_INUSE			1081
/*! cache: bytes dirty in the cache cumulative */
#define	WT_STAT_CONN_CACHE_BYTES_DIRTY_TOTAL		1082
/*! cache: bytes not belonging to page images in the cache */
#define	WT_STAT_CONN_CACHE_BYTES_OTHER			1083
/*! cache: bytes read into cache */
#define	WT_STAT_CONN_CACHE_BYTES_READ			1084
/*! cache: bytes written from cache */
#define	WT_STAT_CONN_CACHE_BYTES_WRITE			1085
/*! cache: checkpoint blocked page eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_CHECKPOINT	1086
/*!
 * cache: checkpoint of history store file blocked non-history store page
 * eviction
 */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_CHECKPOINT_HS	1087
/*! cache: eviction calls to get a page */
#define	WT_STAT_CONN_CACHE_EVICTION_GET_REF		1088
/*! cache: eviction calls to get a page found queue empty */
#define	WT_STAT_CONN_CACHE_EVICTION_GET_REF_EMPTY	1089
/*! cache: eviction calls to get a page found queue empty after locking */
#define	WT_STAT_CONN_CACHE_EVICTION_GET_REF_EMPTY2	1090
/*! cache: eviction currently operating in aggressive mode */
#define	WT_STAT_CONN_CACHE_EVICTION_AGGRESSIVE_SET	1091
/*! cache: eviction empty score */
#define	WT_STAT_CONN_CACHE_EVICTION_EMPTY_SCORE		1092
/*!
 * cache: eviction gave up due to detecting a disk value without a
 * timestamp behind the last update on the chain
 */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_NO_TS_CHECKPOINT_RACE_1	1093
/*!
 * cache: eviction gave up due to detecting a tombstone without a
 * timestamp ahead of the selected on disk update
 */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_NO_TS_CHECKPOINT_RACE_2	1094
/*!
 * cache: eviction gave up due to detecting a tombstone without a
 * timestamp ahead of the selected on disk update after validating the
 * update chain
 */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_NO_TS_CHECKPOINT_RACE_3	1095
/*!
 * cache: eviction gave up due to detecting update chain entries without
 * timestamps after the selected on disk update
 */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_NO_TS_CHECKPOINT_RACE_4	1096
/*!
 * cache: eviction gave up due to needing to remove a record from the
 * history store but checkpoint is running
 */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_REMOVE_HS_RACE_WITH_CHECKPOINT	1097
/*! cache: eviction gave up due to no progress being made */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_NO_PROGRESS	1098
/*! cache: eviction passes of a file */
#define	WT_STAT_CONN_CACHE_EVICTION_WALK_PASSES		1099
/*! cache: eviction server candidate queue empty when topping up */
#define	WT_STAT_CONN_CACHE_EVICTION_QUEUE_EMPTY		1100
/*! cache: eviction server candidate queue not empty when topping up */
#define	WT_STAT_CONN_CACHE_EVICTION_QUEUE_NOT_EMPTY	1101
/*! cache: eviction server evicting pages */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_EVICTING	1102
/*! cache: eviction server skips dirty pages during a running checkpoint */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_SKIP_DIRTY_PAGES_DURING_CHECKPOINT	1103
/*! cache: eviction server skips metadata pages with history */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_SKIP_METATDATA_WITH_HISTORY	1104
/*!
 * cache: eviction server skips pages that are written with transactions
 * greater than the last running
 */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_SKIP_PAGES_LAST_RUNNING	1105
/*!
 * cache: eviction server skips pages that previously failed eviction and
 * likely will again
 */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_SKIP_PAGES_RETRY	1106
/*! cache: eviction server skips pages that we do not want to evict */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_SKIP_UNWANTED_PAGES	1107
/*!
 * cache: eviction server skips trees because there are too many active
 * walks
 */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_SKIP_TREES_TOO_MANY_ACTIVE_WALKS	1108
/*! cache: eviction server skips trees that are being checkpointed */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_SKIP_CHECKPOINTING_TREES	1109
/*!
 * cache: eviction server skips trees that are configured to stick in
 * cache
 */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_SKIP_TREES_STICK_IN_CACHE	1110
/*! cache: eviction server skips trees that disable eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_SKIP_TREES_EVICTION_DISABLED	1111
/*! cache: eviction server skips trees that were not useful before */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_SKIP_TREES_NOT_USEFUL_BEFORE	1112
/*!
 * cache: eviction server slept, because we did not make progress with
 * eviction
 */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_SLEPT	1113
/*! cache: eviction server unable to reach eviction goal */
#define	WT_STAT_CONN_CACHE_EVICTION_SLOW		1114
/*! cache: eviction server waiting for a leaf page */
#define	WT_STAT_CONN_CACHE_EVICTION_WALK_LEAF_NOTFOUND	1115
/*! cache: eviction state */
#define	WT_STAT_CONN_CACHE_EVICTION_STATE		1116
/*!
 * cache: eviction walk most recent sleeps for checkpoint handle
 * gathering
 */
#define	WT_STAT_CONN_CACHE_EVICTION_WALK_SLEEPS		1117
/*! cache: eviction walk target pages histogram - 0-9 */
#define	WT_STAT_CONN_CACHE_EVICTION_TARGET_PAGE_LT10	1118
/*! cache: eviction walk target pages histogram - 10-31 */
#define	WT_STAT_CONN_CACHE_EVICTION_TARGET_PAGE_LT32	1119
/*! cache: eviction walk target pages histogram - 128 and higher */
#define	WT_STAT_CONN_CACHE_EVICTION_TARGET_PAGE_GE128	1120
/*! cache: eviction walk target pages histogram - 32-63 */
#define	WT_STAT_CONN_CACHE_EVICTION_TARGET_PAGE_LT64	1121
/*! cache: eviction walk target pages histogram - 64-128 */
#define	WT_STAT_CONN_CACHE_EVICTION_TARGET_PAGE_LT128	1122
/*!
 * cache: eviction walk target pages reduced due to history store cache
 * pressure
 */
#define	WT_STAT_CONN_CACHE_EVICTION_TARGET_PAGE_REDUCED	1123
/*! cache: eviction walk target strategy both clean and dirty pages */
#define	WT_STAT_CONN_CACHE_EVICTION_TARGET_STRATEGY_BOTH_CLEAN_AND_DIRTY	1124
/*! cache: eviction walk target strategy only clean pages */
#define	WT_STAT_CONN_CACHE_EVICTION_TARGET_STRATEGY_CLEAN	1125
/*! cache: eviction walk target strategy only dirty pages */
#define	WT_STAT_CONN_CACHE_EVICTION_TARGET_STRATEGY_DIRTY	1126
/*! cache: eviction walks abandoned */
#define	WT_STAT_CONN_CACHE_EVICTION_WALKS_ABANDONED	1127
/*! cache: eviction walks gave up because they restarted their walk twice */
#define	WT_STAT_CONN_CACHE_EVICTION_WALKS_STOPPED	1128
/*!
 * cache: eviction walks gave up because they saw too many pages and
 * found no candidates
 */
#define	WT_STAT_CONN_CACHE_EVICTION_WALKS_GAVE_UP_NO_TARGETS	1129
/*!
 * cache: eviction walks gave up because they saw too many pages and
 * found too few candidates
 */
#define	WT_STAT_CONN_CACHE_EVICTION_WALKS_GAVE_UP_RATIO	1130
/*! cache: eviction walks reached end of tree */
#define	WT_STAT_CONN_CACHE_EVICTION_WALKS_ENDED		1131
/*! cache: eviction walks restarted */
#define	WT_STAT_CONN_CACHE_EVICTION_WALK_RESTART	1132
/*! cache: eviction walks started from root of tree */
#define	WT_STAT_CONN_CACHE_EVICTION_WALK_FROM_ROOT	1133
/*! cache: eviction walks started from saved location in tree */
#define	WT_STAT_CONN_CACHE_EVICTION_WALK_SAVED_POS	1134
/*! cache: eviction worker thread active */
#define	WT_STAT_CONN_CACHE_EVICTION_ACTIVE_WORKERS	1135
/*! cache: eviction worker thread created */
#define	WT_STAT_CONN_CACHE_EVICTION_WORKER_CREATED	1136
/*! cache: eviction worker thread evicting pages */
#define	WT_STAT_CONN_CACHE_EVICTION_WORKER_EVICTING	1137
/*! cache: eviction worker thread removed */
#define	WT_STAT_CONN_CACHE_EVICTION_WORKER_REMOVED	1138
/*! cache: eviction worker thread stable number */
#define	WT_STAT_CONN_CACHE_EVICTION_STABLE_STATE_WORKERS	1139
/*! cache: files with active eviction walks */
#define	WT_STAT_CONN_CACHE_EVICTION_WALKS_ACTIVE	1140
/*! cache: files with new eviction walks started */
#define	WT_STAT_CONN_CACHE_EVICTION_WALKS_STARTED	1141
/*! cache: force re-tuning of eviction workers once in a while */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_RETUNE	1142
/*!
 * cache: forced eviction - do not retry count to evict pages selected to
 * evict during reconciliation
 */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_NO_RETRY	1143
/*!
 * cache: forced eviction - history store pages failed to evict while
 * session has history store cursor open
 */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_HS_FAIL	1144
/*!
 * cache: forced eviction - history store pages selected while session
 * has history store cursor open
 */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_HS		1145
/*!
 * cache: forced eviction - history store pages successfully evicted
 * while session has history store cursor open
 */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_HS_SUCCESS	1146
/*! cache: forced eviction - pages evicted that were clean count */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_CLEAN		1147
/*! cache: forced eviction - pages evicted that were clean time (usecs) */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_CLEAN_TIME	1148
/*! cache: forced eviction - pages evicted that were dirty count */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_DIRTY		1149
/*! cache: forced eviction - pages evicted that were dirty time (usecs) */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_DIRTY_TIME	1150
/*!
 * cache: forced eviction - pages selected because of a large number of
 * updates to a single item
 */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_LONG_UPDATE_LIST	1151
/*!
 * cache: forced eviction - pages selected because of too many deleted
 * items count
 */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_DELETE	1152
/*! cache: forced eviction - pages selected count */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE		1153
/*! cache: forced eviction - pages selected unable to be evicted count */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_FAIL		1154
/*! cache: forced eviction - pages selected unable to be evicted time */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_FAIL_TIME	1155
/*! cache: hazard pointer blocked page eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_HAZARD	1156
/*! cache: hazard pointer check calls */
#define	WT_STAT_CONN_CACHE_HAZARD_CHECKS		1157
/*! cache: hazard pointer check entries walked */
#define	WT_STAT_CONN_CACHE_HAZARD_WALKS			1158
/*! cache: hazard pointer maximum array length */
#define	WT_STAT_CONN_CACHE_HAZARD_MAX			1159
/*! cache: history store table insert calls */
#define	WT_STAT_CONN_CACHE_HS_INSERT			1160
/*! cache: history store table insert calls that returned restart */
#define	WT_STAT_CONN_CACHE_HS_INSERT_RESTART		1161
/*! cache: history store table max on-disk size */
#define	WT_STAT_CONN_CACHE_HS_ONDISK_MAX		1162
/*! cache: history store table on-disk size */
#define	WT_STAT_CONN_CACHE_HS_ONDISK			1163
/*! cache: history store table reads */
#define	WT_STAT_CONN_CACHE_HS_READ			1164
/*! cache: history store table reads missed */
#define	WT_STAT_CONN_CACHE_HS_READ_MISS			1165
/*! cache: history store table reads requiring squashed modifies */
#define	WT_STAT_CONN_CACHE_HS_READ_SQUASH		1166
/*!
 * cache: history store table resolved updates without timestamps that
 * lose their durable timestamp
 */
#define	WT_STAT_CONN_CACHE_HS_ORDER_LOSE_DURABLE_TIMESTAMP	1167
/*!
 * cache: history store table truncation by rollback to stable to remove
 * an unstable update
 */
#define	WT_STAT_CONN_CACHE_HS_KEY_TRUNCATE_RTS_UNSTABLE	1168
/*!
 * cache: history store table truncation by rollback to stable to remove
 * an update
 */
#define	WT_STAT_CONN_CACHE_HS_KEY_TRUNCATE_RTS		1169
/*!
 * cache: history store table truncation to remove all the keys of a
 * btree
 */
#define	WT_STAT_CONN_CACHE_HS_BTREE_TRUNCATE		1170
/*! cache: history store table truncation to remove an update */
#define	WT_STAT_CONN_CACHE_HS_KEY_TRUNCATE		1171
/*!
 * cache: history store table truncation to remove range of updates due
 * to an update without a timestamp on data page
 */
#define	WT_STAT_CONN_CACHE_HS_ORDER_REMOVE		1172
/*!
 * cache: history store table truncation to remove range of updates due
 * to key being removed from the data page during reconciliation
 */
#define	WT_STAT_CONN_CACHE_HS_KEY_TRUNCATE_ONPAGE_REMOVAL	1173
/*!
 * cache: history store table truncations that would have happened in
 * non-dryrun mode
 */
#define	WT_STAT_CONN_CACHE_HS_BTREE_TRUNCATE_DRYRUN	1174
/*!
 * cache: history store table truncations to remove an unstable update
 * that would have happened in non-dryrun mode
 */
#define	WT_STAT_CONN_CACHE_HS_KEY_TRUNCATE_RTS_UNSTABLE_DRYRUN	1175
/*!
 * cache: history store table truncations to remove an update that would
 * have happened in non-dryrun mode
 */
#define	WT_STAT_CONN_CACHE_HS_KEY_TRUNCATE_RTS_DRYRUN	1176
/*!
 * cache: history store table updates without timestamps fixed up by
 * reinserting with the fixed timestamp
 */
#define	WT_STAT_CONN_CACHE_HS_ORDER_REINSERT		1177
/*! cache: history store table writes requiring squashed modifies */
#define	WT_STAT_CONN_CACHE_HS_WRITE_SQUASH		1178
/*! cache: in-memory page passed criteria to be split */
#define	WT_STAT_CONN_CACHE_INMEM_SPLITTABLE		1179
/*! cache: in-memory page splits */
#define	WT_STAT_CONN_CACHE_INMEM_SPLIT			1180
/*! cache: internal page split blocked its eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_INTERNAL_PAGE_SPLIT	1181
/*! cache: internal pages evicted */
#define	WT_STAT_CONN_CACHE_EVICTION_INTERNAL		1182
/*! cache: internal pages queued for eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_INTERNAL_PAGES_QUEUED	1183
/*! cache: internal pages seen by eviction walk */
#define	WT_STAT_CONN_CACHE_EVICTION_INTERNAL_PAGES_SEEN	1184
/*! cache: internal pages seen by eviction walk that are already queued */
#define	WT_STAT_CONN_CACHE_EVICTION_INTERNAL_PAGES_ALREADY_QUEUED	1185
/*! cache: internal pages split during eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_SPLIT_INTERNAL	1186
/*! cache: leaf pages split during eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_SPLIT_LEAF		1187
/*! cache: maximum bytes configured */
#define	WT_STAT_CONN_CACHE_BYTES_MAX			1188
/*! cache: maximum milliseconds spent at a single eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_MAXIMUM_MILLISECONDS	1189
/*! cache: maximum page size seen at eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_MAXIMUM_PAGE_SIZE	1190
/*! cache: modified pages evicted */
#define	WT_STAT_CONN_CACHE_EVICTION_DIRTY		1191
/*! cache: modified pages evicted by application threads */
#define	WT_STAT_CONN_CACHE_EVICTION_APP_DIRTY		1192
/*! cache: multi-block reconciliation blocked whilst checkpoint is running */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_MULTI_BLOCK_RECONCILATION_DURING_CHECKPOINT	1193
/*! cache: operations timed out waiting for space in cache */
#define	WT_STAT_CONN_CACHE_TIMED_OUT_OPS		1194
/*!
 * cache: overflow keys on a multiblock row-store page blocked its
 * eviction
 */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_OVERFLOW_KEYS	1195
/*! cache: overflow pages read into cache */
#define	WT_STAT_CONN_CACHE_READ_OVERFLOW		1196
/*! cache: page split during eviction deepened the tree */
#define	WT_STAT_CONN_CACHE_EVICTION_DEEPEN		1197
/*! cache: page written requiring history store records */
#define	WT_STAT_CONN_CACHE_WRITE_HS			1198
/*! cache: pages considered for eviction that were brought in by pre-fetch */
#define	WT_STAT_CONN_CACHE_EVICTION_CONSIDER_PREFETCH	1199
/*! cache: pages currently held in the cache */
#define	WT_STAT_CONN_CACHE_PAGES_INUSE			1200
/*! cache: pages evicted by application threads */
#define	WT_STAT_CONN_CACHE_EVICTION_APP			1201
/*! cache: pages evicted in parallel with checkpoint */
#define	WT_STAT_CONN_CACHE_EVICTION_PAGES_IN_PARALLEL_WITH_CHECKPOINT	1202
/*! cache: pages queued for eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_PAGES_QUEUED	1203
/*! cache: pages queued for eviction post lru sorting */
#define	WT_STAT_CONN_CACHE_EVICTION_PAGES_QUEUED_POST_LRU	1204
/*! cache: pages queued for urgent eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_PAGES_QUEUED_URGENT	1205
/*! cache: pages queued for urgent eviction during walk */
#define	WT_STAT_CONN_CACHE_EVICTION_PAGES_QUEUED_OLDEST	1206
/*!
 * cache: pages queued for urgent eviction from history store due to high
 * dirty content
 */
#define	WT_STAT_CONN_CACHE_EVICTION_PAGES_QUEUED_URGENT_HS_DIRTY	1207
/*! cache: pages read into cache */
#define	WT_STAT_CONN_CACHE_READ				1208
/*! cache: pages read into cache after truncate */
#define	WT_STAT_CONN_CACHE_READ_DELETED			1209
/*! cache: pages read into cache after truncate in prepare state */
#define	WT_STAT_CONN_CACHE_READ_DELETED_PREPARED	1210
/*!
 * cache: pages removed from the ordinary queue to be queued for urgent
 * eviction
 */
#define	WT_STAT_CONN_CACHE_EVICTION_CLEAR_ORDINARY	1211
/*! cache: pages requested from the cache */
#define	WT_STAT_CONN_CACHE_PAGES_REQUESTED		1212
/*! cache: pages requested from the cache due to pre-fetch */
#define	WT_STAT_CONN_CACHE_PAGES_PREFETCH		1213
/*! cache: pages seen by eviction walk */
#define	WT_STAT_CONN_CACHE_EVICTION_PAGES_SEEN		1214
/*! cache: pages seen by eviction walk that are already queued */
#define	WT_STAT_CONN_CACHE_EVICTION_PAGES_ALREADY_QUEUED	1215
/*! cache: pages selected for eviction unable to be evicted */
#define	WT_STAT_CONN_CACHE_EVICTION_FAIL		1216
/*!
 * cache: pages selected for eviction unable to be evicted because of
 * active children on an internal page
 */
#define	WT_STAT_CONN_CACHE_EVICTION_FAIL_ACTIVE_CHILDREN_ON_AN_INTERNAL_PAGE	1217
/*!
 * cache: pages selected for eviction unable to be evicted because of
 * failure in reconciliation
 */
#define	WT_STAT_CONN_CACHE_EVICTION_FAIL_IN_RECONCILIATION	1218
/*!
 * cache: pages selected for eviction unable to be evicted because of
 * race between checkpoint and updates without timestamps
 */
#define	WT_STAT_CONN_CACHE_EVICTION_FAIL_CHECKPOINT_NO_TS	1219
/*! cache: pages walked for eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_WALK		1220
/*! cache: pages written from cache */
#define	WT_STAT_CONN_CACHE_WRITE			1221
/*! cache: pages written requiring in-memory restoration */
#define	WT_STAT_CONN_CACHE_WRITE_RESTORE		1222
/*! cache: percentage overhead */
#define	WT_STAT_CONN_CACHE_OVERHEAD			1223
/*! cache: recent modification of a page blocked its eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_RECENTLY_MODIFIED	1224
/*! cache: reverse splits performed */
#define	WT_STAT_CONN_CACHE_REVERSE_SPLITS		1225
/*!
 * cache: reverse splits skipped because of VLCS namespace gap
 * restrictions
 */
#define	WT_STAT_CONN_CACHE_REVERSE_SPLITS_SKIPPED_VLCS	1226
/*! cache: the number of times full update inserted to history store */
#define	WT_STAT_CONN_CACHE_HS_INSERT_FULL_UPDATE	1227
/*! cache: the number of times reverse modify inserted to history store */
#define	WT_STAT_CONN_CACHE_HS_INSERT_REVERSE_MODIFY	1228
/*!
 * cache: total milliseconds spent inside reentrant history store
 * evictions in a reconciliation
 */
#define	WT_STAT_CONN_CACHE_REENTRY_HS_EVICTION_MILLISECONDS	1229
/*! cache: tracked bytes belonging to internal pages in the cache */
#define	WT_STAT_CONN_CACHE_BYTES_INTERNAL		1230
/*! cache: tracked bytes belonging to leaf pages in the cache */
#define	WT_STAT_CONN_CACHE_BYTES_LEAF			1231
/*! cache: tracked dirty bytes in the cache */
#define	WT_STAT_CONN_CACHE_BYTES_DIRTY			1232
/*! cache: tracked dirty pages in the cache */
#define	WT_STAT_CONN_CACHE_PAGES_DIRTY			1233
/*! cache: uncommitted truncate blocked page eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_BLOCKED_UNCOMMITTED_TRUNCATE	1234
/*! cache: unmodified pages evicted */
#define	WT_STAT_CONN_CACHE_EVICTION_CLEAN		1235
/*! capacity: background fsync file handles considered */
#define	WT_STAT_CONN_FSYNC_ALL_FH_TOTAL			1236
/*! capacity: background fsync file handles synced */
#define	WT_STAT_CONN_FSYNC_ALL_FH			1237
/*! capacity: background fsync time (msecs) */
#define	WT_STAT_CONN_FSYNC_ALL_TIME			1238
/*! capacity: bytes read */
#define	WT_STAT_CONN_CAPACITY_BYTES_READ		1239
/*! capacity: bytes written for checkpoint */
#define	WT_STAT_CONN_CAPACITY_BYTES_CKPT		1240
/*! capacity: bytes written for chunk cache */
#define	WT_STAT_CONN_CAPACITY_BYTES_CHUNKCACHE		1241
/*! capacity: bytes written for eviction */
#define	WT_STAT_CONN_CAPACITY_BYTES_EVICT		1242
/*! capacity: bytes written for log */
#define	WT_STAT_CONN_CAPACITY_BYTES_LOG			1243
/*! capacity: bytes written total */
#define	WT_STAT_CONN_CAPACITY_BYTES_WRITTEN		1244
/*! capacity: threshold to call fsync */
#define	WT_STAT_CONN_CAPACITY_THRESHOLD			1245
/*! capacity: time waiting due to total capacity (usecs) */
#define	WT_STAT_CONN_CAPACITY_TIME_TOTAL		1246
/*! capacity: time waiting during checkpoint (usecs) */
#define	WT_STAT_CONN_CAPACITY_TIME_CKPT			1247
/*! capacity: time waiting during eviction (usecs) */
#define	WT_STAT_CONN_CAPACITY_TIME_EVICT		1248
/*! capacity: time waiting during logging (usecs) */
#define	WT_STAT_CONN_CAPACITY_TIME_LOG			1249
/*! capacity: time waiting during read (usecs) */
#define	WT_STAT_CONN_CAPACITY_TIME_READ			1250
/*! capacity: time waiting for chunk cache IO bandwidth (usecs) */
#define	WT_STAT_CONN_CAPACITY_TIME_CHUNKCACHE		1251
/*! checkpoint: checkpoint has acquired a snapshot for its transaction */
#define	WT_STAT_CONN_CHECKPOINT_SNAPSHOT_ACQUIRED	1252
/*! checkpoint: checkpoints skipped because database was clean */
#define	WT_STAT_CONN_CHECKPOINT_SKIPPED			1253
/*! checkpoint: fsync calls after allocating the transaction ID */
#define	WT_STAT_CONN_CHECKPOINT_FSYNC_POST		1254
/*! checkpoint: fsync duration after allocating the transaction ID (usecs) */
#define	WT_STAT_CONN_CHECKPOINT_FSYNC_POST_DURATION	1255
/*! checkpoint: generation */
#define	WT_STAT_CONN_CHECKPOINT_GENERATION		1256
/*! checkpoint: max time (msecs) */
#define	WT_STAT_CONN_CHECKPOINT_TIME_MAX		1257
/*! checkpoint: min time (msecs) */
#define	WT_STAT_CONN_CHECKPOINT_TIME_MIN		1258
/*!
 * checkpoint: most recent duration for checkpoint dropping all handles
 * (usecs)
 */
#define	WT_STAT_CONN_CHECKPOINT_HANDLE_DROP_DURATION	1259
/*! checkpoint: most recent duration for gathering all handles (usecs) */
#define	WT_STAT_CONN_CHECKPOINT_HANDLE_DURATION		1260
/*! checkpoint: most recent duration for gathering applied handles (usecs) */
#define	WT_STAT_CONN_CHECKPOINT_HANDLE_APPLY_DURATION	1261
/*! checkpoint: most recent duration for gathering skipped handles (usecs) */
#define	WT_STAT_CONN_CHECKPOINT_HANDLE_SKIP_DURATION	1262
/*! checkpoint: most recent duration for handles metadata checked (usecs) */
#define	WT_STAT_CONN_CHECKPOINT_HANDLE_META_CHECK_DURATION	1263
/*! checkpoint: most recent duration for locking the handles (usecs) */
#define	WT_STAT_CONN_CHECKPOINT_HANDLE_LOCK_DURATION	1264
/*! checkpoint: most recent handles applied */
#define	WT_STAT_CONN_CHECKPOINT_HANDLE_APPLIED		1265
/*! checkpoint: most recent handles checkpoint dropped */
#define	WT_STAT_CONN_CHECKPOINT_HANDLE_DROPPED		1266
/*! checkpoint: most recent handles metadata checked */
#define	WT_STAT_CONN_CHECKPOINT_HANDLE_META_CHECKED	1267
/*! checkpoint: most recent handles metadata locked */
#define	WT_STAT_CONN_CHECKPOINT_HANDLE_LOCKED		1268
/*! checkpoint: most recent handles skipped */
#define	WT_STAT_CONN_CHECKPOINT_HANDLE_SKIPPED		1269
/*! checkpoint: most recent handles walked */
#define	WT_STAT_CONN_CHECKPOINT_HANDLE_WALKED		1270
/*! checkpoint: most recent time (msecs) */
#define	WT_STAT_CONN_CHECKPOINT_TIME_RECENT		1271
/*! checkpoint: number of checkpoints started by api */
#define	WT_STAT_CONN_CHECKPOINTS_API			1272
/*! checkpoint: number of checkpoints started by compaction */
#define	WT_STAT_CONN_CHECKPOINTS_COMPACT		1273
/*! checkpoint: number of files synced */
#define	WT_STAT_CONN_CHECKPOINT_SYNC			1274
/*! checkpoint: number of handles visited after writes complete */
#define	WT_STAT_CONN_CHECKPOINT_PRESYNC			1275
/*! checkpoint: number of history store pages caused to be reconciled */
#define	WT_STAT_CONN_CHECKPOINT_HS_PAGES_RECONCILED	1276
/*! checkpoint: number of internal pages visited */
#define	WT_STAT_CONN_CHECKPOINT_PAGES_VISITED_INTERNAL	1277
/*! checkpoint: number of leaf pages visited */
#define	WT_STAT_CONN_CHECKPOINT_PAGES_VISITED_LEAF	1278
/*! checkpoint: number of pages caused to be reconciled */
#define	WT_STAT_CONN_CHECKPOINT_PAGES_RECONCILED	1279
/*! checkpoint: pages added for eviction during checkpoint cleanup */
#define	WT_STAT_CONN_CHECKPOINT_CLEANUP_PAGES_EVICT	1280
/*! checkpoint: pages removed during checkpoint cleanup */
#define	WT_STAT_CONN_CHECKPOINT_CLEANUP_PAGES_REMOVED	1281
/*! checkpoint: pages skipped during checkpoint cleanup tree walk */
#define	WT_STAT_CONN_CHECKPOINT_CLEANUP_PAGES_WALK_SKIPPED	1282
/*! checkpoint: pages visited during checkpoint cleanup */
#define	WT_STAT_CONN_CHECKPOINT_CLEANUP_PAGES_VISITED	1283
/*! checkpoint: prepare currently running */
#define	WT_STAT_CONN_CHECKPOINT_PREP_RUNNING		1284
/*! checkpoint: prepare max time (msecs) */
#define	WT_STAT_CONN_CHECKPOINT_PREP_MAX		1285
/*! checkpoint: prepare min time (msecs) */
#define	WT_STAT_CONN_CHECKPOINT_PREP_MIN		1286
/*! checkpoint: prepare most recent time (msecs) */
#define	WT_STAT_CONN_CHECKPOINT_PREP_RECENT		1287
/*! checkpoint: prepare total time (msecs) */
#define	WT_STAT_CONN_CHECKPOINT_PREP_TOTAL		1288
/*! checkpoint: progress state */
#define	WT_STAT_CONN_CHECKPOINT_STATE			1289
/*! checkpoint: scrub dirty target */
#define	WT_STAT_CONN_CHECKPOINT_SCRUB_TARGET		1290
/*! checkpoint: scrub max time (msecs) */
#define	WT_STAT_CONN_CHECKPOINT_SCRUB_MAX		1291
/*! checkpoint: scrub min time (msecs) */
#define	WT_STAT_CONN_CHECKPOINT_SCRUB_MIN		1292
/*! checkpoint: scrub most recent time (msecs) */
#define	WT_STAT_CONN_CHECKPOINT_SCRUB_RECENT		1293
/*! checkpoint: scrub total time (msecs) */
#define	WT_STAT_CONN_CHECKPOINT_SCRUB_TOTAL		1294
/*! checkpoint: stop timing stress active */
#define	WT_STAT_CONN_CHECKPOINT_STOP_STRESS_ACTIVE	1295
/*! checkpoint: time spent on per-tree checkpoint work (usecs) */
#define	WT_STAT_CONN_CHECKPOINT_TREE_DURATION		1296
/*! checkpoint: total failed number of checkpoints */
#define	WT_STAT_CONN_CHECKPOINTS_TOTAL_FAILED		1297
/*! checkpoint: total succeed number of checkpoints */
#define	WT_STAT_CONN_CHECKPOINTS_TOTAL_SUCCEED		1298
/*! checkpoint: total time (msecs) */
#define	WT_STAT_CONN_CHECKPOINT_TIME_TOTAL		1299
/*! checkpoint: transaction checkpoints due to obsolete pages */
#define	WT_STAT_CONN_CHECKPOINT_OBSOLETE_APPLIED	1300
/*! checkpoint: wait cycles while cache dirty level is decreasing */
#define	WT_STAT_CONN_CHECKPOINT_WAIT_REDUCE_DIRTY	1301
/*! chunk-cache: aggregate number of spanned chunks on read */
#define	WT_STAT_CONN_CHUNKCACHE_SPANS_CHUNKS_READ	1302
/*! chunk-cache: chunks evicted */
#define	WT_STAT_CONN_CHUNKCACHE_CHUNKS_EVICTED		1303
/*! chunk-cache: could not allocate due to exceeding bitmap capacity */
#define	WT_STAT_CONN_CHUNKCACHE_EXCEEDED_BITMAP_CAPACITY	1304
/*! chunk-cache: could not allocate due to exceeding capacity */
#define	WT_STAT_CONN_CHUNKCACHE_EXCEEDED_CAPACITY	1305
/*! chunk-cache: lookups */
#define	WT_STAT_CONN_CHUNKCACHE_LOOKUPS			1306
/*!
 * chunk-cache: number of chunks loaded from flushed tables in chunk
 * cache
 */
#define	WT_STAT_CONN_CHUNKCACHE_CHUNKS_LOADED_FROM_FLUSHED_TABLES	1307
/*! chunk-cache: number of metadata entries inserted */
#define	WT_STAT_CONN_CHUNKCACHE_METADATA_INSERTED	1308
/*! chunk-cache: number of metadata entries removed */
#define	WT_STAT_CONN_CHUNKCACHE_METADATA_REMOVED	1309
/*!
 * chunk-cache: number of metadata inserts/deletes dropped by the worker
 * thread
 */
#define	WT_STAT_CONN_CHUNKCACHE_METADATA_WORK_UNITS_DROPPED	1310
/*!
 * chunk-cache: number of metadata inserts/deletes pushed to the worker
 * thread
 */
#define	WT_STAT_CONN_CHUNKCACHE_METADATA_WORK_UNITS_CREATED	1311
/*!
 * chunk-cache: number of metadata inserts/deletes read by the worker
 * thread
 */
#define	WT_STAT_CONN_CHUNKCACHE_METADATA_WORK_UNITS_DEQUEUED	1312
/*! chunk-cache: number of misses */
#define	WT_STAT_CONN_CHUNKCACHE_MISSES			1313
/*! chunk-cache: number of times a read from storage failed */
#define	WT_STAT_CONN_CHUNKCACHE_IO_FAILED		1314
/*! chunk-cache: retried accessing a chunk while I/O was in progress */
#define	WT_STAT_CONN_CHUNKCACHE_RETRIES			1315
/*! chunk-cache: retries from a chunk cache checksum mismatch */
#define	WT_STAT_CONN_CHUNKCACHE_RETRIES_CHECKSUM_MISMATCH	1316
/*! chunk-cache: timed out due to too many retries */
#define	WT_STAT_CONN_CHUNKCACHE_TOOMANY_RETRIES		1317
/*! chunk-cache: total bytes read from persistent content */
#define	WT_STAT_CONN_CHUNKCACHE_BYTES_READ_PERSISTENT	1318
/*! chunk-cache: total bytes used by the cache */
#define	WT_STAT_CONN_CHUNKCACHE_BYTES_INUSE		1319
/*! chunk-cache: total bytes used by the cache for pinned chunks */
#define	WT_STAT_CONN_CHUNKCACHE_BYTES_INUSE_PINNED	1320
/*! chunk-cache: total chunks held by the chunk cache */
#define	WT_STAT_CONN_CHUNKCACHE_CHUNKS_INUSE		1321
/*!
 * chunk-cache: total number of chunks inserted on startup from persisted
 * metadata.
 */
#define	WT_STAT_CONN_CHUNKCACHE_CREATED_FROM_METADATA	1322
/*! chunk-cache: total pinned chunks held by the chunk cache */
#define	WT_STAT_CONN_CHUNKCACHE_CHUNKS_PINNED		1323
/*! connection: auto adjusting condition resets */
#define	WT_STAT_CONN_COND_AUTO_WAIT_RESET		1324
/*! connection: auto adjusting condition wait calls */
#define	WT_STAT_CONN_COND_AUTO_WAIT			1325
/*!
 * connection: auto adjusting condition wait raced to update timeout and
 * skipped updating
 */
#define	WT_STAT_CONN_COND_AUTO_WAIT_SKIPPED		1326
/*! connection: detected system time went backwards */
#define	WT_STAT_CONN_TIME_TRAVEL			1327
/*! connection: files currently open */
#define	WT_STAT_CONN_FILE_OPEN				1328
/*! connection: hash bucket array size for data handles */
#define	WT_STAT_CONN_BUCKETS_DH				1329
/*! connection: hash bucket array size general */
#define	WT_STAT_CONN_BUCKETS				1330
/*! connection: memory allocations */
#define	WT_STAT_CONN_MEMORY_ALLOCATION			1331
/*! connection: memory frees */
#define	WT_STAT_CONN_MEMORY_FREE			1332
/*! connection: memory re-allocations */
#define	WT_STAT_CONN_MEMORY_GROW			1333
/*! connection: number of sessions without a sweep for 5+ minutes */
#define	WT_STAT_CONN_NO_SESSION_SWEEP_5MIN		1334
/*! connection: number of sessions without a sweep for 60+ minutes */
#define	WT_STAT_CONN_NO_SESSION_SWEEP_60MIN		1335
/*! connection: pthread mutex condition wait calls */
#define	WT_STAT_CONN_COND_WAIT				1336
/*! connection: pthread mutex shared lock read-lock calls */
#define	WT_STAT_CONN_RWLOCK_READ			1337
/*! connection: pthread mutex shared lock write-lock calls */
#define	WT_STAT_CONN_RWLOCK_WRITE			1338
/*! connection: total fsync I/Os */
#define	WT_STAT_CONN_FSYNC_IO				1339
/*! connection: total read I/Os */
#define	WT_STAT_CONN_READ_IO				1340
/*! connection: total write I/Os */
#define	WT_STAT_CONN_WRITE_IO				1341
/*! cursor: Total number of entries skipped by cursor next calls */
#define	WT_STAT_CONN_CURSOR_NEXT_SKIP_TOTAL		1342
/*! cursor: Total number of entries skipped by cursor prev calls */
#define	WT_STAT_CONN_CURSOR_PREV_SKIP_TOTAL		1343
/*!
 * cursor: Total number of entries skipped to position the history store
 * cursor
 */
#define	WT_STAT_CONN_CURSOR_SKIP_HS_CUR_POSITION	1344
/*!
 * cursor: Total number of times a search near has exited due to prefix
 * config
 */
#define	WT_STAT_CONN_CURSOR_SEARCH_NEAR_PREFIX_FAST_PATHS	1345
/*!
 * cursor: Total number of times cursor fails to temporarily release
 * pinned page to encourage eviction of hot or large page
 */
#define	WT_STAT_CONN_CURSOR_REPOSITION_FAILED		1346
/*!
 * cursor: Total number of times cursor temporarily releases pinned page
 * to encourage eviction of hot or large page
 */
#define	WT_STAT_CONN_CURSOR_REPOSITION			1347
/*! cursor: bulk cursor count */
#define	WT_STAT_CONN_CURSOR_BULK_COUNT			1348
/*! cursor: cached cursor count */
#define	WT_STAT_CONN_CURSOR_CACHED_COUNT		1349
/*! cursor: cursor bound calls that return an error */
#define	WT_STAT_CONN_CURSOR_BOUND_ERROR			1350
/*! cursor: cursor bounds cleared from reset */
#define	WT_STAT_CONN_CURSOR_BOUNDS_RESET		1351
/*! cursor: cursor bounds comparisons performed */
#define	WT_STAT_CONN_CURSOR_BOUNDS_COMPARISONS		1352
/*! cursor: cursor bounds next called on an unpositioned cursor */
#define	WT_STAT_CONN_CURSOR_BOUNDS_NEXT_UNPOSITIONED	1353
/*! cursor: cursor bounds next early exit */
#define	WT_STAT_CONN_CURSOR_BOUNDS_NEXT_EARLY_EXIT	1354
/*! cursor: cursor bounds prev called on an unpositioned cursor */
#define	WT_STAT_CONN_CURSOR_BOUNDS_PREV_UNPOSITIONED	1355
/*! cursor: cursor bounds prev early exit */
#define	WT_STAT_CONN_CURSOR_BOUNDS_PREV_EARLY_EXIT	1356
/*! cursor: cursor bounds search early exit */
#define	WT_STAT_CONN_CURSOR_BOUNDS_SEARCH_EARLY_EXIT	1357
/*! cursor: cursor bounds search near call repositioned cursor */
#define	WT_STAT_CONN_CURSOR_BOUNDS_SEARCH_NEAR_REPOSITIONED_CURSOR	1358
/*! cursor: cursor bulk loaded cursor insert calls */
#define	WT_STAT_CONN_CURSOR_INSERT_BULK			1359
/*! cursor: cursor cache calls that return an error */
#define	WT_STAT_CONN_CURSOR_CACHE_ERROR			1360
/*! cursor: cursor close calls that result in cache */
#define	WT_STAT_CONN_CURSOR_CACHE			1361
/*! cursor: cursor close calls that return an error */
#define	WT_STAT_CONN_CURSOR_CLOSE_ERROR			1362
/*! cursor: cursor compare calls that return an error */
#define	WT_STAT_CONN_CURSOR_COMPARE_ERROR		1363
/*! cursor: cursor create calls */
#define	WT_STAT_CONN_CURSOR_CREATE			1364
/*! cursor: cursor equals calls that return an error */
#define	WT_STAT_CONN_CURSOR_EQUALS_ERROR		1365
/*! cursor: cursor get key calls that return an error */
#define	WT_STAT_CONN_CURSOR_GET_KEY_ERROR		1366
/*! cursor: cursor get value calls that return an error */
#define	WT_STAT_CONN_CURSOR_GET_VALUE_ERROR		1367
/*! cursor: cursor insert calls */
#define	WT_STAT_CONN_CURSOR_INSERT			1368
/*! cursor: cursor insert calls that return an error */
#define	WT_STAT_CONN_CURSOR_INSERT_ERROR		1369
/*! cursor: cursor insert check calls that return an error */
#define	WT_STAT_CONN_CURSOR_INSERT_CHECK_ERROR		1370
/*! cursor: cursor insert key and value bytes */
#define	WT_STAT_CONN_CURSOR_INSERT_BYTES		1371
/*! cursor: cursor largest key calls that return an error */
#define	WT_STAT_CONN_CURSOR_LARGEST_KEY_ERROR		1372
/*! cursor: cursor modify calls */
#define	WT_STAT_CONN_CURSOR_MODIFY			1373
/*! cursor: cursor modify calls that return an error */
#define	WT_STAT_CONN_CURSOR_MODIFY_ERROR		1374
/*! cursor: cursor modify key and value bytes affected */
#define	WT_STAT_CONN_CURSOR_MODIFY_BYTES		1375
/*! cursor: cursor modify value bytes modified */
#define	WT_STAT_CONN_CURSOR_MODIFY_BYTES_TOUCH		1376
/*! cursor: cursor next calls */
#define	WT_STAT_CONN_CURSOR_NEXT			1377
/*! cursor: cursor next calls that return an error */
#define	WT_STAT_CONN_CURSOR_NEXT_ERROR			1378
/*!
 * cursor: cursor next calls that skip due to a globally visible history
 * store tombstone
 */
#define	WT_STAT_CONN_CURSOR_NEXT_HS_TOMBSTONE		1379
/*!
 * cursor: cursor next calls that skip greater than 1 and fewer than 100
 * entries
 */
#define	WT_STAT_CONN_CURSOR_NEXT_SKIP_LT_100		1380
/*!
 * cursor: cursor next calls that skip greater than or equal to 100
 * entries
 */
#define	WT_STAT_CONN_CURSOR_NEXT_SKIP_GE_100		1381
/*! cursor: cursor next random calls that return an error */
#define	WT_STAT_CONN_CURSOR_NEXT_RANDOM_ERROR		1382
/*! cursor: cursor operation restarted */
#define	WT_STAT_CONN_CURSOR_RESTART			1383
/*! cursor: cursor prev calls */
#define	WT_STAT_CONN_CURSOR_PREV			1384
/*! cursor: cursor prev calls that return an error */
#define	WT_STAT_CONN_CURSOR_PREV_ERROR			1385
/*!
 * cursor: cursor prev calls that skip due to a globally visible history
 * store tombstone
 */
#define	WT_STAT_CONN_CURSOR_PREV_HS_TOMBSTONE		1386
/*!
 * cursor: cursor prev calls that skip greater than or equal to 100
 * entries
 */
#define	WT_STAT_CONN_CURSOR_PREV_SKIP_GE_100		1387
/*! cursor: cursor prev calls that skip less than 100 entries */
#define	WT_STAT_CONN_CURSOR_PREV_SKIP_LT_100		1388
/*! cursor: cursor reconfigure calls that return an error */
#define	WT_STAT_CONN_CURSOR_RECONFIGURE_ERROR		1389
/*! cursor: cursor remove calls */
#define	WT_STAT_CONN_CURSOR_REMOVE			1390
/*! cursor: cursor remove calls that return an error */
#define	WT_STAT_CONN_CURSOR_REMOVE_ERROR		1391
/*! cursor: cursor remove key bytes removed */
#define	WT_STAT_CONN_CURSOR_REMOVE_BYTES		1392
/*! cursor: cursor reopen calls that return an error */
#define	WT_STAT_CONN_CURSOR_REOPEN_ERROR		1393
/*! cursor: cursor reserve calls */
#define	WT_STAT_CONN_CURSOR_RESERVE			1394
/*! cursor: cursor reserve calls that return an error */
#define	WT_STAT_CONN_CURSOR_RESERVE_ERROR		1395
/*! cursor: cursor reset calls */
#define	WT_STAT_CONN_CURSOR_RESET			1396
/*! cursor: cursor reset calls that return an error */
#define	WT_STAT_CONN_CURSOR_RESET_ERROR			1397
/*! cursor: cursor search calls */
#define	WT_STAT_CONN_CURSOR_SEARCH			1398
/*! cursor: cursor search calls that return an error */
#define	WT_STAT_CONN_CURSOR_SEARCH_ERROR		1399
/*! cursor: cursor search history store calls */
#define	WT_STAT_CONN_CURSOR_SEARCH_HS			1400
/*! cursor: cursor search near calls */
#define	WT_STAT_CONN_CURSOR_SEARCH_NEAR			1401
/*! cursor: cursor search near calls that return an error */
#define	WT_STAT_CONN_CURSOR_SEARCH_NEAR_ERROR		1402
/*! cursor: cursor sweep buckets */
#define	WT_STAT_CONN_CURSOR_SWEEP_BUCKETS		1403
/*! cursor: cursor sweep cursors closed */
#define	WT_STAT_CONN_CURSOR_SWEEP_CLOSED		1404
/*! cursor: cursor sweep cursors examined */
#define	WT_STAT_CONN_CURSOR_SWEEP_EXAMINED		1405
/*! cursor: cursor sweeps */
#define	WT_STAT_CONN_CURSOR_SWEEP			1406
/*! cursor: cursor truncate calls */
#define	WT_STAT_CONN_CURSOR_TRUNCATE			1407
/*! cursor: cursor truncates performed on individual keys */
#define	WT_STAT_CONN_CURSOR_TRUNCATE_KEYS_DELETED	1408
/*! cursor: cursor update calls */
#define	WT_STAT_CONN_CURSOR_UPDATE			1409
/*! cursor: cursor update calls that return an error */
#define	WT_STAT_CONN_CURSOR_UPDATE_ERROR		1410
/*! cursor: cursor update key and value bytes */
#define	WT_STAT_CONN_CURSOR_UPDATE_BYTES		1411
/*! cursor: cursor update value size change */
#define	WT_STAT_CONN_CURSOR_UPDATE_BYTES_CHANGED	1412
/*! cursor: cursors reused from cache */
#define	WT_STAT_CONN_CURSOR_REOPEN			1413
/*! cursor: open cursor count */
#define	WT_STAT_CONN_CURSOR_OPEN_COUNT			1414
/*! data-handle: connection data handle size */
#define	WT_STAT_CONN_DH_CONN_HANDLE_SIZE		1415
/*! data-handle: connection data handles currently active */
#define	WT_STAT_CONN_DH_CONN_HANDLE_COUNT		1416
/*! data-handle: connection sweep candidate became referenced */
#define	WT_STAT_CONN_DH_SWEEP_REF			1417
/*! data-handle: connection sweep dhandles closed */
#define	WT_STAT_CONN_DH_SWEEP_CLOSE			1418
/*! data-handle: connection sweep dhandles removed from hash list */
#define	WT_STAT_CONN_DH_SWEEP_REMOVE			1419
/*! data-handle: connection sweep time-of-death sets */
#define	WT_STAT_CONN_DH_SWEEP_TOD			1420
/*! data-handle: connection sweeps */
#define	WT_STAT_CONN_DH_SWEEPS				1421
/*!
 * data-handle: connection sweeps skipped due to checkpoint gathering
 * handles
 */
#define	WT_STAT_CONN_DH_SWEEP_SKIP_CKPT			1422
/*! data-handle: session dhandles swept */
#define	WT_STAT_CONN_DH_SESSION_HANDLES			1423
/*! data-handle: session sweep attempts */
#define	WT_STAT_CONN_DH_SESSION_SWEEPS			1424
/*! lock: checkpoint lock acquisitions */
#define	WT_STAT_CONN_LOCK_CHECKPOINT_COUNT		1425
/*! lock: checkpoint lock application thread wait time (usecs) */
#define	WT_STAT_CONN_LOCK_CHECKPOINT_WAIT_APPLICATION	1426
/*! lock: checkpoint lock internal thread wait time (usecs) */
#define	WT_STAT_CONN_LOCK_CHECKPOINT_WAIT_INTERNAL	1427
/*! lock: dhandle lock application thread time waiting (usecs) */
#define	WT_STAT_CONN_LOCK_DHANDLE_WAIT_APPLICATION	1428
/*! lock: dhandle lock internal thread time waiting (usecs) */
#define	WT_STAT_CONN_LOCK_DHANDLE_WAIT_INTERNAL		1429
/*! lock: dhandle read lock acquisitions */
#define	WT_STAT_CONN_LOCK_DHANDLE_READ_COUNT		1430
/*! lock: dhandle write lock acquisitions */
#define	WT_STAT_CONN_LOCK_DHANDLE_WRITE_COUNT		1431
/*! lock: metadata lock acquisitions */
#define	WT_STAT_CONN_LOCK_METADATA_COUNT		1432
/*! lock: metadata lock application thread wait time (usecs) */
#define	WT_STAT_CONN_LOCK_METADATA_WAIT_APPLICATION	1433
/*! lock: metadata lock internal thread wait time (usecs) */
#define	WT_STAT_CONN_LOCK_METADATA_WAIT_INTERNAL	1434
/*! lock: schema lock acquisitions */
#define	WT_STAT_CONN_LOCK_SCHEMA_COUNT			1435
/*! lock: schema lock application thread wait time (usecs) */
#define	WT_STAT_CONN_LOCK_SCHEMA_WAIT_APPLICATION	1436
/*! lock: schema lock internal thread wait time (usecs) */
#define	WT_STAT_CONN_LOCK_SCHEMA_WAIT_INTERNAL		1437
/*!
 * lock: table lock application thread time waiting for the table lock
 * (usecs)
 */
#define	WT_STAT_CONN_LOCK_TABLE_WAIT_APPLICATION	1438
/*!
 * lock: table lock internal thread time waiting for the table lock
 * (usecs)
 */
#define	WT_STAT_CONN_LOCK_TABLE_WAIT_INTERNAL		1439
/*! lock: table read lock acquisitions */
#define	WT_STAT_CONN_LOCK_TABLE_READ_COUNT		1440
/*! lock: table write lock acquisitions */
#define	WT_STAT_CONN_LOCK_TABLE_WRITE_COUNT		1441
/*! lock: txn global lock application thread time waiting (usecs) */
#define	WT_STAT_CONN_LOCK_TXN_GLOBAL_WAIT_APPLICATION	1442
/*! lock: txn global lock internal thread time waiting (usecs) */
#define	WT_STAT_CONN_LOCK_TXN_GLOBAL_WAIT_INTERNAL	1443
/*! lock: txn global read lock acquisitions */
#define	WT_STAT_CONN_LOCK_TXN_GLOBAL_READ_COUNT		1444
/*! lock: txn global write lock acquisitions */
#define	WT_STAT_CONN_LOCK_TXN_GLOBAL_WRITE_COUNT	1445
/*! log: busy returns attempting to switch slots */
#define	WT_STAT_CONN_LOG_SLOT_SWITCH_BUSY		1446
/*! log: force log remove time sleeping (usecs) */
#define	WT_STAT_CONN_LOG_FORCE_REMOVE_SLEEP		1447
/*! log: log bytes of payload data */
#define	WT_STAT_CONN_LOG_BYTES_PAYLOAD			1448
/*! log: log bytes written */
#define	WT_STAT_CONN_LOG_BYTES_WRITTEN			1449
/*! log: log files manually zero-filled */
#define	WT_STAT_CONN_LOG_ZERO_FILLS			1450
/*! log: log flush operations */
#define	WT_STAT_CONN_LOG_FLUSH				1451
/*! log: log force write operations */
#define	WT_STAT_CONN_LOG_FORCE_WRITE			1452
/*! log: log force write operations skipped */
#define	WT_STAT_CONN_LOG_FORCE_WRITE_SKIP		1453
/*! log: log records compressed */
#define	WT_STAT_CONN_LOG_COMPRESS_WRITES		1454
/*! log: log records not compressed */
#define	WT_STAT_CONN_LOG_COMPRESS_WRITE_FAILS		1455
/*! log: log records too small to compress */
#define	WT_STAT_CONN_LOG_COMPRESS_SMALL			1456
/*! log: log release advances write LSN */
#define	WT_STAT_CONN_LOG_RELEASE_WRITE_LSN		1457
/*! log: log scan operations */
#define	WT_STAT_CONN_LOG_SCANS				1458
/*! log: log scan records requiring two reads */
#define	WT_STAT_CONN_LOG_SCAN_REREADS			1459
/*! log: log server thread advances write LSN */
#define	WT_STAT_CONN_LOG_WRITE_LSN			1460
/*! log: log server thread write LSN walk skipped */
#define	WT_STAT_CONN_LOG_WRITE_LSN_SKIP			1461
/*! log: log sync operations */
#define	WT_STAT_CONN_LOG_SYNC				1462
/*! log: log sync time duration (usecs) */
#define	WT_STAT_CONN_LOG_SYNC_DURATION			1463
/*! log: log sync_dir operations */
#define	WT_STAT_CONN_LOG_SYNC_DIR			1464
/*! log: log sync_dir time duration (usecs) */
#define	WT_STAT_CONN_LOG_SYNC_DIR_DURATION		1465
/*! log: log write operations */
#define	WT_STAT_CONN_LOG_WRITES				1466
/*! log: logging bytes consolidated */
#define	WT_STAT_CONN_LOG_SLOT_CONSOLIDATED		1467
/*! log: maximum log file size */
#define	WT_STAT_CONN_LOG_MAX_FILESIZE			1468
/*! log: number of pre-allocated log files to create */
#define	WT_STAT_CONN_LOG_PREALLOC_MAX			1469
/*! log: pre-allocated log files not ready and missed */
#define	WT_STAT_CONN_LOG_PREALLOC_MISSED		1470
/*! log: pre-allocated log files prepared */
#define	WT_STAT_CONN_LOG_PREALLOC_FILES			1471
/*! log: pre-allocated log files used */
#define	WT_STAT_CONN_LOG_PREALLOC_USED			1472
/*! log: records processed by log scan */
#define	WT_STAT_CONN_LOG_SCAN_RECORDS			1473
/*! log: slot close lost race */
#define	WT_STAT_CONN_LOG_SLOT_CLOSE_RACE		1474
/*! log: slot close unbuffered waits */
#define	WT_STAT_CONN_LOG_SLOT_CLOSE_UNBUF		1475
/*! log: slot closures */
#define	WT_STAT_CONN_LOG_SLOT_CLOSES			1476
/*! log: slot join atomic update races */
#define	WT_STAT_CONN_LOG_SLOT_RACES			1477
/*! log: slot join calls atomic updates raced */
#define	WT_STAT_CONN_LOG_SLOT_YIELD_RACE		1478
/*! log: slot join calls did not yield */
#define	WT_STAT_CONN_LOG_SLOT_IMMEDIATE			1479
/*! log: slot join calls found active slot closed */
#define	WT_STAT_CONN_LOG_SLOT_YIELD_CLOSE		1480
/*! log: slot join calls slept */
#define	WT_STAT_CONN_LOG_SLOT_YIELD_SLEEP		1481
/*! log: slot join calls yielded */
#define	WT_STAT_CONN_LOG_SLOT_YIELD			1482
/*! log: slot join found active slot closed */
#define	WT_STAT_CONN_LOG_SLOT_ACTIVE_CLOSED		1483
/*! log: slot joins yield time (usecs) */
#define	WT_STAT_CONN_LOG_SLOT_YIELD_DURATION		1484
/*! log: slot transitions unable to find free slot */
#define	WT_STAT_CONN_LOG_SLOT_NO_FREE_SLOTS		1485
/*! log: slot unbuffered writes */
#define	WT_STAT_CONN_LOG_SLOT_UNBUFFERED		1486
/*! log: total in-memory size of compressed records */
#define	WT_STAT_CONN_LOG_COMPRESS_MEM			1487
/*! log: total log buffer size */
#define	WT_STAT_CONN_LOG_BUFFER_SIZE			1488
/*! log: total size of compressed records */
#define	WT_STAT_CONN_LOG_COMPRESS_LEN			1489
/*! log: written slots coalesced */
#define	WT_STAT_CONN_LOG_SLOT_COALESCED			1490
/*! log: yields waiting for previous log file close */
#define	WT_STAT_CONN_LOG_CLOSE_YIELDS			1491
/*! perf: file system read latency histogram (bucket 1) - 0-10ms */
#define	WT_STAT_CONN_PERF_HIST_FSREAD_LATENCY_LT10	1492
/*! perf: file system read latency histogram (bucket 2) - 10-49ms */
#define	WT_STAT_CONN_PERF_HIST_FSREAD_LATENCY_LT50	1493
/*! perf: file system read latency histogram (bucket 3) - 50-99ms */
#define	WT_STAT_CONN_PERF_HIST_FSREAD_LATENCY_LT100	1494
/*! perf: file system read latency histogram (bucket 4) - 100-249ms */
#define	WT_STAT_CONN_PERF_HIST_FSREAD_LATENCY_LT250	1495
/*! perf: file system read latency histogram (bucket 5) - 250-499ms */
#define	WT_STAT_CONN_PERF_HIST_FSREAD_LATENCY_LT500	1496
/*! perf: file system read latency histogram (bucket 6) - 500-999ms */
#define	WT_STAT_CONN_PERF_HIST_FSREAD_LATENCY_LT1000	1497
/*! perf: file system read latency histogram (bucket 7) - 1000ms+ */
#define	WT_STAT_CONN_PERF_HIST_FSREAD_LATENCY_GT1000	1498
/*! perf: file system read latency histogram total (msecs) */
#define	WT_STAT_CONN_PERF_HIST_FSREAD_LATENCY_TOTAL_MSECS	1499
/*! perf: file system write latency histogram (bucket 1) - 0-10ms */
#define	WT_STAT_CONN_PERF_HIST_FSWRITE_LATENCY_LT10	1500
/*! perf: file system write latency histogram (bucket 2) - 10-49ms */
#define	WT_STAT_CONN_PERF_HIST_FSWRITE_LATENCY_LT50	1501
/*! perf: file system write latency histogram (bucket 3) - 50-99ms */
#define	WT_STAT_CONN_PERF_HIST_FSWRITE_LATENCY_LT100	1502
/*! perf: file system write latency histogram (bucket 4) - 100-249ms */
#define	WT_STAT_CONN_PERF_HIST_FSWRITE_LATENCY_LT250	1503
/*! perf: file system write latency histogram (bucket 5) - 250-499ms */
#define	WT_STAT_CONN_PERF_HIST_FSWRITE_LATENCY_LT500	1504
/*! perf: file system write latency histogram (bucket 6) - 500-999ms */
#define	WT_STAT_CONN_PERF_HIST_FSWRITE_LATENCY_LT1000	1505
/*! perf: file system write latency histogram (bucket 7) - 1000ms+ */
#define	WT_STAT_CONN_PERF_HIST_FSWRITE_LATENCY_GT1000	1506
/*! perf: file system write latency histogram total (msecs) */
#define	WT_STAT_CONN_PERF_HIST_FSWRITE_LATENCY_TOTAL_MSECS	1507
/*! perf: operation read latency histogram (bucket 1) - 0-100us */
#define	WT_STAT_CONN_PERF_HIST_OPREAD_LATENCY_LT100	1508
/*! perf: operation read latency histogram (bucket 2) - 100-249us */
#define	WT_STAT_CONN_PERF_HIST_OPREAD_LATENCY_LT250	1509
/*! perf: operation read latency histogram (bucket 3) - 250-499us */
#define	WT_STAT_CONN_PERF_HIST_OPREAD_LATENCY_LT500	1510
/*! perf: operation read latency histogram (bucket 4) - 500-999us */
#define	WT_STAT_CONN_PERF_HIST_OPREAD_LATENCY_LT1000	1511
/*! perf: operation read latency histogram (bucket 5) - 1000-9999us */
#define	WT_STAT_CONN_PERF_HIST_OPREAD_LATENCY_LT10000	1512
/*! perf: operation read latency histogram (bucket 6) - 10000us+ */
#define	WT_STAT_CONN_PERF_HIST_OPREAD_LATENCY_GT10000	1513
/*! perf: operation read latency histogram total (usecs) */
#define	WT_STAT_CONN_PERF_HIST_OPREAD_LATENCY_TOTAL_USECS	1514
/*! perf: operation write latency histogram (bucket 1) - 0-100us */
#define	WT_STAT_CONN_PERF_HIST_OPWRITE_LATENCY_LT100	1515
/*! perf: operation write latency histogram (bucket 2) - 100-249us */
#define	WT_STAT_CONN_PERF_HIST_OPWRITE_LATENCY_LT250	1516
/*! perf: operation write latency histogram (bucket 3) - 250-499us */
#define	WT_STAT_CONN_PERF_HIST_OPWRITE_LATENCY_LT500	1517
/*! perf: operation write latency histogram (bucket 4) - 500-999us */
#define	WT_STAT_CONN_PERF_HIST_OPWRITE_LATENCY_LT1000	1518
/*! perf: operation write latency histogram (bucket 5) - 1000-9999us */
#define	WT_STAT_CONN_PERF_HIST_OPWRITE_LATENCY_LT10000	1519
/*! perf: operation write latency histogram (bucket 6) - 10000us+ */
#define	WT_STAT_CONN_PERF_HIST_OPWRITE_LATENCY_GT10000	1520
/*! perf: operation write latency histogram total (usecs) */
#define	WT_STAT_CONN_PERF_HIST_OPWRITE_LATENCY_TOTAL_USECS	1521
/*! reconciliation: VLCS pages explicitly reconciled as empty */
#define	WT_STAT_CONN_REC_VLCS_EMPTIED_PAGES		1522
/*! reconciliation: approximate byte size of timestamps in pages written */
#define	WT_STAT_CONN_REC_TIME_WINDOW_BYTES_TS		1523
/*!
 * reconciliation: approximate byte size of transaction IDs in pages
 * written
 */
#define	WT_STAT_CONN_REC_TIME_WINDOW_BYTES_TXN		1524
/*! reconciliation: fast-path pages deleted */
#define	WT_STAT_CONN_REC_PAGE_DELETE_FAST		1525
/*! reconciliation: leaf-page overflow keys */
#define	WT_STAT_CONN_REC_OVERFLOW_KEY_LEAF		1526
/*! reconciliation: maximum milliseconds spent in a reconciliation call */
#define	WT_STAT_CONN_REC_MAXIMUM_MILLISECONDS		1527
/*!
 * reconciliation: maximum milliseconds spent in building a disk image in
 * a reconciliation
 */
#define	WT_STAT_CONN_REC_MAXIMUM_IMAGE_BUILD_MILLISECONDS	1528
/*!
 * reconciliation: maximum milliseconds spent in moving updates to the
 * history store in a reconciliation
 */
#define	WT_STAT_CONN_REC_MAXIMUM_HS_WRAPUP_MILLISECONDS	1529
/*! reconciliation: overflow values written */
#define	WT_STAT_CONN_REC_OVERFLOW_VALUE			1530
/*! reconciliation: page reconciliation calls */
#define	WT_STAT_CONN_REC_PAGES				1531
/*! reconciliation: page reconciliation calls for eviction */
#define	WT_STAT_CONN_REC_PAGES_EVICTION			1532
/*!
 * reconciliation: page reconciliation calls that resulted in values with
 * prepared transaction metadata
 */
#define	WT_STAT_CONN_REC_PAGES_WITH_PREPARE		1533
/*!
 * reconciliation: page reconciliation calls that resulted in values with
 * timestamps
 */
#define	WT_STAT_CONN_REC_PAGES_WITH_TS			1534
/*!
 * reconciliation: page reconciliation calls that resulted in values with
 * transaction ids
 */
#define	WT_STAT_CONN_REC_PAGES_WITH_TXN			1535
/*! reconciliation: pages deleted */
#define	WT_STAT_CONN_REC_PAGE_DELETE			1536
/*!
 * reconciliation: pages written including an aggregated newest start
 * durable timestamp
 */
#define	WT_STAT_CONN_REC_TIME_AGGR_NEWEST_START_DURABLE_TS	1537
/*!
 * reconciliation: pages written including an aggregated newest stop
 * durable timestamp
 */
#define	WT_STAT_CONN_REC_TIME_AGGR_NEWEST_STOP_DURABLE_TS	1538
/*!
 * reconciliation: pages written including an aggregated newest stop
 * timestamp
 */
#define	WT_STAT_CONN_REC_TIME_AGGR_NEWEST_STOP_TS	1539
/*!
 * reconciliation: pages written including an aggregated newest stop
 * transaction ID
 */
#define	WT_STAT_CONN_REC_TIME_AGGR_NEWEST_STOP_TXN	1540
/*!
 * reconciliation: pages written including an aggregated newest
 * transaction ID
 */
#define	WT_STAT_CONN_REC_TIME_AGGR_NEWEST_TXN		1541
/*!
 * reconciliation: pages written including an aggregated oldest start
 * timestamp
 */
#define	WT_STAT_CONN_REC_TIME_AGGR_OLDEST_START_TS	1542
/*! reconciliation: pages written including an aggregated prepare */
#define	WT_STAT_CONN_REC_TIME_AGGR_PREPARED		1543
/*! reconciliation: pages written including at least one prepare state */
#define	WT_STAT_CONN_REC_TIME_WINDOW_PAGES_PREPARED	1544
/*!
 * reconciliation: pages written including at least one start durable
 * timestamp
 */
#define	WT_STAT_CONN_REC_TIME_WINDOW_PAGES_DURABLE_START_TS	1545
/*! reconciliation: pages written including at least one start timestamp */
#define	WT_STAT_CONN_REC_TIME_WINDOW_PAGES_START_TS	1546
/*!
 * reconciliation: pages written including at least one start transaction
 * ID
 */
#define	WT_STAT_CONN_REC_TIME_WINDOW_PAGES_START_TXN	1547
/*!
 * reconciliation: pages written including at least one stop durable
 * timestamp
 */
#define	WT_STAT_CONN_REC_TIME_WINDOW_PAGES_DURABLE_STOP_TS	1548
/*! reconciliation: pages written including at least one stop timestamp */
#define	WT_STAT_CONN_REC_TIME_WINDOW_PAGES_STOP_TS	1549
/*!
 * reconciliation: pages written including at least one stop transaction
 * ID
 */
#define	WT_STAT_CONN_REC_TIME_WINDOW_PAGES_STOP_TXN	1550
/*! reconciliation: records written including a prepare state */
#define	WT_STAT_CONN_REC_TIME_WINDOW_PREPARED		1551
/*! reconciliation: records written including a start durable timestamp */
#define	WT_STAT_CONN_REC_TIME_WINDOW_DURABLE_START_TS	1552
/*! reconciliation: records written including a start timestamp */
#define	WT_STAT_CONN_REC_TIME_WINDOW_START_TS		1553
/*! reconciliation: records written including a start transaction ID */
#define	WT_STAT_CONN_REC_TIME_WINDOW_START_TXN		1554
/*! reconciliation: records written including a stop durable timestamp */
#define	WT_STAT_CONN_REC_TIME_WINDOW_DURABLE_STOP_TS	1555
/*! reconciliation: records written including a stop timestamp */
#define	WT_STAT_CONN_REC_TIME_WINDOW_STOP_TS		1556
/*! reconciliation: records written including a stop transaction ID */
#define	WT_STAT_CONN_REC_TIME_WINDOW_STOP_TXN		1557
/*! reconciliation: split bytes currently awaiting free */
#define	WT_STAT_CONN_REC_SPLIT_STASHED_BYTES		1558
/*! reconciliation: split objects currently awaiting free */
#define	WT_STAT_CONN_REC_SPLIT_STASHED_OBJECTS		1559
/*! session: attempts to remove a local object and the object is in use */
#define	WT_STAT_CONN_LOCAL_OBJECTS_INUSE		1560
/*! session: flush_tier failed calls */
#define	WT_STAT_CONN_FLUSH_TIER_FAIL			1561
/*! session: flush_tier operation calls */
#define	WT_STAT_CONN_FLUSH_TIER				1562
/*! session: flush_tier tables skipped due to no checkpoint */
#define	WT_STAT_CONN_FLUSH_TIER_SKIPPED			1563
/*! session: flush_tier tables switched */
#define	WT_STAT_CONN_FLUSH_TIER_SWITCHED		1564
/*! session: local objects removed */
#define	WT_STAT_CONN_LOCAL_OBJECTS_REMOVED		1565
/*! session: open session count */
#define	WT_STAT_CONN_SESSION_OPEN			1566
/*! session: session query timestamp calls */
#define	WT_STAT_CONN_SESSION_QUERY_TS			1567
/*! session: table alter failed calls */
#define	WT_STAT_CONN_SESSION_TABLE_ALTER_FAIL		1568
/*! session: table alter successful calls */
#define	WT_STAT_CONN_SESSION_TABLE_ALTER_SUCCESS	1569
/*! session: table alter triggering checkpoint calls */
#define	WT_STAT_CONN_SESSION_TABLE_ALTER_TRIGGER_CHECKPOINT	1570
/*! session: table alter unchanged and skipped */
#define	WT_STAT_CONN_SESSION_TABLE_ALTER_SKIP		1571
/*! session: table compact conflicted with checkpoint */
#define	WT_STAT_CONN_SESSION_TABLE_COMPACT_CONFLICTING_CHECKPOINT	1572
/*! session: table compact dhandle successful calls */
#define	WT_STAT_CONN_SESSION_TABLE_COMPACT_DHANDLE_SUCCESS	1573
/*! session: table compact failed calls */
#define	WT_STAT_CONN_SESSION_TABLE_COMPACT_FAIL		1574
/*! session: table compact failed calls due to cache pressure */
#define	WT_STAT_CONN_SESSION_TABLE_COMPACT_FAIL_CACHE_PRESSURE	1575
/*! session: table compact passes */
#define	WT_STAT_CONN_SESSION_TABLE_COMPACT_PASSES	1576
/*! session: table compact running */
#define	WT_STAT_CONN_SESSION_TABLE_COMPACT_RUNNING	1577
/*! session: table compact skipped as process would not reduce file size */
#define	WT_STAT_CONN_SESSION_TABLE_COMPACT_SKIPPED	1578
/*! session: table compact successful calls */
#define	WT_STAT_CONN_SESSION_TABLE_COMPACT_SUCCESS	1579
/*! session: table compact timeout */
#define	WT_STAT_CONN_SESSION_TABLE_COMPACT_TIMEOUT	1580
/*! session: table create failed calls */
#define	WT_STAT_CONN_SESSION_TABLE_CREATE_FAIL		1581
/*! session: table create successful calls */
#define	WT_STAT_CONN_SESSION_TABLE_CREATE_SUCCESS	1582
/*! session: table create with import failed calls */
#define	WT_STAT_CONN_SESSION_TABLE_CREATE_IMPORT_FAIL	1583
/*! session: table create with import successful calls */
#define	WT_STAT_CONN_SESSION_TABLE_CREATE_IMPORT_SUCCESS	1584
/*! session: table drop failed calls */
#define	WT_STAT_CONN_SESSION_TABLE_DROP_FAIL		1585
/*! session: table drop successful calls */
#define	WT_STAT_CONN_SESSION_TABLE_DROP_SUCCESS		1586
/*! session: table rename failed calls */
#define	WT_STAT_CONN_SESSION_TABLE_RENAME_FAIL		1587
/*! session: table rename successful calls */
#define	WT_STAT_CONN_SESSION_TABLE_RENAME_SUCCESS	1588
/*! session: table salvage failed calls */
#define	WT_STAT_CONN_SESSION_TABLE_SALVAGE_FAIL		1589
/*! session: table salvage successful calls */
#define	WT_STAT_CONN_SESSION_TABLE_SALVAGE_SUCCESS	1590
/*! session: table truncate failed calls */
#define	WT_STAT_CONN_SESSION_TABLE_TRUNCATE_FAIL	1591
/*! session: table truncate successful calls */
#define	WT_STAT_CONN_SESSION_TABLE_TRUNCATE_SUCCESS	1592
/*! session: table verify failed calls */
#define	WT_STAT_CONN_SESSION_TABLE_VERIFY_FAIL		1593
/*! session: table verify successful calls */
#define	WT_STAT_CONN_SESSION_TABLE_VERIFY_SUCCESS	1594
/*! session: tiered operations dequeued and processed */
#define	WT_STAT_CONN_TIERED_WORK_UNITS_DEQUEUED		1595
/*! session: tiered operations removed without processing */
#define	WT_STAT_CONN_TIERED_WORK_UNITS_REMOVED		1596
/*! session: tiered operations scheduled */
#define	WT_STAT_CONN_TIERED_WORK_UNITS_CREATED		1597
/*! session: tiered storage local retention time (secs) */
#define	WT_STAT_CONN_TIERED_RETENTION			1598
/*! thread-state: active filesystem fsync calls */
#define	WT_STAT_CONN_THREAD_FSYNC_ACTIVE		1599
/*! thread-state: active filesystem read calls */
#define	WT_STAT_CONN_THREAD_READ_ACTIVE			1600
/*! thread-state: active filesystem write calls */
#define	WT_STAT_CONN_THREAD_WRITE_ACTIVE		1601
/*! thread-yield: application thread snapshot refreshed for eviction */
#define	WT_STAT_CONN_APPLICATION_EVICT_SNAPSHOT_REFRESHED	1602
/*! thread-yield: application thread time evicting (usecs) */
#define	WT_STAT_CONN_APPLICATION_EVICT_TIME		1603
/*! thread-yield: application thread time waiting for cache (usecs) */
#define	WT_STAT_CONN_APPLICATION_CACHE_TIME		1604
/*!
 * thread-yield: connection close blocked waiting for transaction state
 * stabilization
 */
#define	WT_STAT_CONN_TXN_RELEASE_BLOCKED		1605
/*! thread-yield: connection close yielded for lsm manager shutdown */
#define	WT_STAT_CONN_CONN_CLOSE_BLOCKED_LSM		1606
/*! thread-yield: data handle lock yielded */
#define	WT_STAT_CONN_DHANDLE_LOCK_BLOCKED		1607
/*!
 * thread-yield: get reference for page index and slot time sleeping
 * (usecs)
 */
#define	WT_STAT_CONN_PAGE_INDEX_SLOT_REF_BLOCKED	1608
/*! thread-yield: page access yielded due to prepare state change */
#define	WT_STAT_CONN_PREPARED_TRANSITION_BLOCKED_PAGE	1609
/*! thread-yield: page acquire busy blocked */
#define	WT_STAT_CONN_PAGE_BUSY_BLOCKED			1610
/*! thread-yield: page acquire eviction blocked */
#define	WT_STAT_CONN_PAGE_FORCIBLE_EVICT_BLOCKED	1611
/*! thread-yield: page acquire locked blocked */
#define	WT_STAT_CONN_PAGE_LOCKED_BLOCKED		1612
/*! thread-yield: page acquire read blocked */
#define	WT_STAT_CONN_PAGE_READ_BLOCKED			1613
/*! thread-yield: page acquire time sleeping (usecs) */
#define	WT_STAT_CONN_PAGE_SLEEP				1614
/*!
 * thread-yield: page delete rollback time sleeping for state change
 * (usecs)
 */
#define	WT_STAT_CONN_PAGE_DEL_ROLLBACK_BLOCKED		1615
/*! thread-yield: page reconciliation yielded due to child modification */
#define	WT_STAT_CONN_CHILD_MODIFY_BLOCKED_PAGE		1616
/*! transaction: Number of prepared updates */
#define	WT_STAT_CONN_TXN_PREPARED_UPDATES		1617
/*! transaction: Number of prepared updates committed */
#define	WT_STAT_CONN_TXN_PREPARED_UPDATES_COMMITTED	1618
/*! transaction: Number of prepared updates repeated on the same key */
#define	WT_STAT_CONN_TXN_PREPARED_UPDATES_KEY_REPEATED	1619
/*! transaction: Number of prepared updates rolled back */
#define	WT_STAT_CONN_TXN_PREPARED_UPDATES_ROLLEDBACK	1620
/*!
 * transaction: a reader raced with a prepared transaction commit and
 * skipped an update or updates
 */
#define	WT_STAT_CONN_TXN_READ_RACE_PREPARE_COMMIT	1621
/*! transaction: number of times overflow removed value is read */
#define	WT_STAT_CONN_TXN_READ_OVERFLOW_REMOVE		1622
/*! transaction: oldest pinned transaction ID rolled back for eviction */
#define	WT_STAT_CONN_TXN_ROLLBACK_OLDEST_PINNED		1623
/*! transaction: prepared transactions */
#define	WT_STAT_CONN_TXN_PREPARE			1624
/*! transaction: prepared transactions committed */
#define	WT_STAT_CONN_TXN_PREPARE_COMMIT			1625
/*! transaction: prepared transactions currently active */
#define	WT_STAT_CONN_TXN_PREPARE_ACTIVE			1626
/*! transaction: prepared transactions rolled back */
#define	WT_STAT_CONN_TXN_PREPARE_ROLLBACK		1627
/*! transaction: query timestamp calls */
#define	WT_STAT_CONN_TXN_QUERY_TS			1628
/*! transaction: race to read prepared update retry */
#define	WT_STAT_CONN_TXN_READ_RACE_PREPARE_UPDATE	1629
/*! transaction: rollback to stable calls */
#define	WT_STAT_CONN_TXN_RTS				1630
/*!
 * transaction: rollback to stable history store keys that would have
 * been swept in non-dryrun mode
 */
#define	WT_STAT_CONN_TXN_RTS_SWEEP_HS_KEYS_DRYRUN	1631
/*!
 * transaction: rollback to stable history store records with stop
 * timestamps older than newer records
 */
#define	WT_STAT_CONN_TXN_RTS_HS_STOP_OLDER_THAN_NEWER_START	1632
/*! transaction: rollback to stable inconsistent checkpoint */
#define	WT_STAT_CONN_TXN_RTS_INCONSISTENT_CKPT		1633
/*! transaction: rollback to stable keys removed */
#define	WT_STAT_CONN_TXN_RTS_KEYS_REMOVED		1634
/*! transaction: rollback to stable keys restored */
#define	WT_STAT_CONN_TXN_RTS_KEYS_RESTORED		1635
/*!
 * transaction: rollback to stable keys that would have been removed in
 * non-dryrun mode
 */
#define	WT_STAT_CONN_TXN_RTS_KEYS_REMOVED_DRYRUN	1636
/*!
 * transaction: rollback to stable keys that would have been restored in
 * non-dryrun mode
 */
#define	WT_STAT_CONN_TXN_RTS_KEYS_RESTORED_DRYRUN	1637
/*! transaction: rollback to stable pages visited */
#define	WT_STAT_CONN_TXN_RTS_PAGES_VISITED		1638
/*! transaction: rollback to stable restored tombstones from history store */
#define	WT_STAT_CONN_TXN_RTS_HS_RESTORE_TOMBSTONES	1639
/*! transaction: rollback to stable restored updates from history store */
#define	WT_STAT_CONN_TXN_RTS_HS_RESTORE_UPDATES		1640
/*! transaction: rollback to stable skipping delete rle */
#define	WT_STAT_CONN_TXN_RTS_DELETE_RLE_SKIPPED		1641
/*! transaction: rollback to stable skipping stable rle */
#define	WT_STAT_CONN_TXN_RTS_STABLE_RLE_SKIPPED		1642
/*! transaction: rollback to stable sweeping history store keys */
#define	WT_STAT_CONN_TXN_RTS_SWEEP_HS_KEYS		1643
/*!
 * transaction: rollback to stable tombstones from history store that
 * would have been restored in non-dryrun mode
 */
#define	WT_STAT_CONN_TXN_RTS_HS_RESTORE_TOMBSTONES_DRYRUN	1644
/*! transaction: rollback to stable tree walk skipping pages */
#define	WT_STAT_CONN_TXN_RTS_TREE_WALK_SKIP_PAGES	1645
/*! transaction: rollback to stable updates aborted */
#define	WT_STAT_CONN_TXN_RTS_UPD_ABORTED		1646
/*!
 * transaction: rollback to stable updates from history store that would
 * have been restored in non-dryrun mode
 */
#define	WT_STAT_CONN_TXN_RTS_HS_RESTORE_UPDATES_DRYRUN	1647
/*! transaction: rollback to stable updates removed from history store */
#define	WT_STAT_CONN_TXN_RTS_HS_REMOVED			1648
/*!
 * transaction: rollback to stable updates that would have been aborted
 * in non-dryrun mode
 */
#define	WT_STAT_CONN_TXN_RTS_UPD_ABORTED_DRYRUN		1649
/*!
 * transaction: rollback to stable updates that would have been removed
 * from history store in non-dryrun mode
 */
#define	WT_STAT_CONN_TXN_RTS_HS_REMOVED_DRYRUN		1650
/*! transaction: sessions scanned in each walk of concurrent sessions */
#define	WT_STAT_CONN_TXN_SESSIONS_WALKED		1651
/*! transaction: set timestamp calls */
#define	WT_STAT_CONN_TXN_SET_TS				1652
/*! transaction: set timestamp durable calls */
#define	WT_STAT_CONN_TXN_SET_TS_DURABLE			1653
/*! transaction: set timestamp durable updates */
#define	WT_STAT_CONN_TXN_SET_TS_DURABLE_UPD		1654
/*! transaction: set timestamp force calls */
#define	WT_STAT_CONN_TXN_SET_TS_FORCE			1655
/*!
 * transaction: set timestamp global oldest timestamp set to be more
 * recent than the global stable timestamp
 */
#define	WT_STAT_CONN_TXN_SET_TS_OUT_OF_ORDER		1656
/*! transaction: set timestamp oldest calls */
#define	WT_STAT_CONN_TXN_SET_TS_OLDEST			1657
/*! transaction: set timestamp oldest updates */
#define	WT_STAT_CONN_TXN_SET_TS_OLDEST_UPD		1658
/*! transaction: set timestamp stable calls */
#define	WT_STAT_CONN_TXN_SET_TS_STABLE			1659
/*! transaction: set timestamp stable updates */
#define	WT_STAT_CONN_TXN_SET_TS_STABLE_UPD		1660
/*! transaction: transaction begins */
#define	WT_STAT_CONN_TXN_BEGIN				1661
/*!
 * transaction: transaction checkpoint history store file duration
 * (usecs)
 */
#define	WT_STAT_CONN_TXN_HS_CKPT_DURATION		1662
/*! transaction: transaction range of IDs currently pinned */
#define	WT_STAT_CONN_TXN_PINNED_RANGE			1663
/*! transaction: transaction range of IDs currently pinned by a checkpoint */
#define	WT_STAT_CONN_TXN_PINNED_CHECKPOINT_RANGE	1664
/*! transaction: transaction range of timestamps currently pinned */
#define	WT_STAT_CONN_TXN_PINNED_TIMESTAMP		1665
/*! transaction: transaction range of timestamps pinned by a checkpoint */
#define	WT_STAT_CONN_TXN_PINNED_TIMESTAMP_CHECKPOINT	1666
/*!
 * transaction: transaction range of timestamps pinned by the oldest
 * active read timestamp
 */
#define	WT_STAT_CONN_TXN_PINNED_TIMESTAMP_READER	1667
/*!
 * transaction: transaction range of timestamps pinned by the oldest
 * timestamp
 */
#define	WT_STAT_CONN_TXN_PINNED_TIMESTAMP_OLDEST	1668
/*! transaction: transaction read timestamp of the oldest active reader */
#define	WT_STAT_CONN_TXN_TIMESTAMP_OLDEST_ACTIVE_READ	1669
/*! transaction: transaction rollback to stable currently running */
#define	WT_STAT_CONN_TXN_ROLLBACK_TO_STABLE_RUNNING	1670
/*! transaction: transaction walk of concurrent sessions */
#define	WT_STAT_CONN_TXN_WALK_SESSIONS			1671
/*! transaction: transactions committed */
#define	WT_STAT_CONN_TXN_COMMIT				1672
/*! transaction: transactions rolled back */
#define	WT_STAT_CONN_TXN_ROLLBACK			1673
/*! transaction: update conflicts */
#define	WT_STAT_CONN_TXN_UPDATE_CONFLICT		1674

/*!
 * @}
 * @name Statistics for data sources
 * @anchor statistics_dsrc
 * @{
 */
/*! LSM: bloom filter false positives */
#define	WT_STAT_DSRC_BLOOM_FALSE_POSITIVE		2000
/*! LSM: bloom filter hits */
#define	WT_STAT_DSRC_BLOOM_HIT				2001
/*! LSM: bloom filter misses */
#define	WT_STAT_DSRC_BLOOM_MISS				2002
/*! LSM: bloom filter pages evicted from cache */
#define	WT_STAT_DSRC_BLOOM_PAGE_EVICT			2003
/*! LSM: bloom filter pages read into cache */
#define	WT_STAT_DSRC_BLOOM_PAGE_READ			2004
/*! LSM: bloom filters in the LSM tree */
#define	WT_STAT_DSRC_BLOOM_COUNT			2005
/*! LSM: chunks in the LSM tree */
#define	WT_STAT_DSRC_LSM_CHUNK_COUNT			2006
/*! LSM: highest merge generation in the LSM tree */
#define	WT_STAT_DSRC_LSM_GENERATION_MAX			2007
/*!
 * LSM: queries that could have benefited from a Bloom filter that did
 * not exist
 */
#define	WT_STAT_DSRC_LSM_LOOKUP_NO_BLOOM		2008
/*! LSM: sleep for LSM checkpoint throttle */
#define	WT_STAT_DSRC_LSM_CHECKPOINT_THROTTLE		2009
/*! LSM: sleep for LSM merge throttle */
#define	WT_STAT_DSRC_LSM_MERGE_THROTTLE			2010
/*! LSM: total size of bloom filters */
#define	WT_STAT_DSRC_BLOOM_SIZE				2011
/*! autocommit: retries for readonly operations */
#define	WT_STAT_DSRC_AUTOCOMMIT_READONLY_RETRY		2012
/*! autocommit: retries for update operations */
#define	WT_STAT_DSRC_AUTOCOMMIT_UPDATE_RETRY		2013
/*! block-manager: allocations requiring file extension */
#define	WT_STAT_DSRC_BLOCK_EXTENSION			2014
/*! block-manager: blocks allocated */
#define	WT_STAT_DSRC_BLOCK_ALLOC			2015
/*! block-manager: blocks freed */
#define	WT_STAT_DSRC_BLOCK_FREE				2016
/*! block-manager: checkpoint size */
#define	WT_STAT_DSRC_BLOCK_CHECKPOINT_SIZE		2017
/*! block-manager: file allocation unit size */
#define	WT_STAT_DSRC_ALLOCATION_SIZE			2018
/*! block-manager: file bytes available for reuse */
#define	WT_STAT_DSRC_BLOCK_REUSE_BYTES			2019
/*! block-manager: file magic number */
#define	WT_STAT_DSRC_BLOCK_MAGIC			2020
/*! block-manager: file major version number */
#define	WT_STAT_DSRC_BLOCK_MAJOR			2021
/*! block-manager: file size in bytes */
#define	WT_STAT_DSRC_BLOCK_SIZE				2022
/*! block-manager: minor version number */
#define	WT_STAT_DSRC_BLOCK_MINOR			2023
/*! btree: btree checkpoint generation */
#define	WT_STAT_DSRC_BTREE_CHECKPOINT_GENERATION	2024
/*! btree: btree clean tree checkpoint expiration time */
#define	WT_STAT_DSRC_BTREE_CLEAN_CHECKPOINT_TIMER	2025
/*! btree: btree compact pages reviewed */
#define	WT_STAT_DSRC_BTREE_COMPACT_PAGES_REVIEWED	2026
/*! btree: btree compact pages rewritten */
#define	WT_STAT_DSRC_BTREE_COMPACT_PAGES_REWRITTEN	2027
/*! btree: btree compact pages skipped */
#define	WT_STAT_DSRC_BTREE_COMPACT_PAGES_SKIPPED	2028
/*! btree: btree expected number of compact pages rewritten */
#define	WT_STAT_DSRC_BTREE_COMPACT_PAGES_REWRITTEN_EXPECTED	2029
/*! btree: btree skipped by compaction as process would not reduce size */
#define	WT_STAT_DSRC_BTREE_COMPACT_SKIPPED		2030
/*!
 * btree: column-store fixed-size leaf pages, only reported if tree_walk
 * or all statistics are enabled
 */
#define	WT_STAT_DSRC_BTREE_COLUMN_FIX			2031
/*!
 * btree: column-store fixed-size time windows, only reported if
 * tree_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_BTREE_COLUMN_TWS			2032
/*!
 * btree: column-store internal pages, only reported if tree_walk or all
 * statistics are enabled
 */
#define	WT_STAT_DSRC_BTREE_COLUMN_INTERNAL		2033
/*!
 * btree: column-store variable-size RLE encoded values, only reported if
 * tree_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_BTREE_COLUMN_RLE			2034
/*!
 * btree: column-store variable-size deleted values, only reported if
 * tree_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_BTREE_COLUMN_DELETED		2035
/*!
 * btree: column-store variable-size leaf pages, only reported if
 * tree_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_BTREE_COLUMN_VARIABLE		2036
/*! btree: fixed-record size */
#define	WT_STAT_DSRC_BTREE_FIXED_LEN			2037
/*! btree: maximum internal page size */
#define	WT_STAT_DSRC_BTREE_MAXINTLPAGE			2038
/*! btree: maximum leaf page key size */
#define	WT_STAT_DSRC_BTREE_MAXLEAFKEY			2039
/*! btree: maximum leaf page size */
#define	WT_STAT_DSRC_BTREE_MAXLEAFPAGE			2040
/*! btree: maximum leaf page value size */
#define	WT_STAT_DSRC_BTREE_MAXLEAFVALUE			2041
/*! btree: maximum tree depth */
#define	WT_STAT_DSRC_BTREE_MAXIMUM_DEPTH		2042
/*!
 * btree: number of key/value pairs, only reported if tree_walk or all
 * statistics are enabled
 */
#define	WT_STAT_DSRC_BTREE_ENTRIES			2043
/*!
 * btree: overflow pages, only reported if tree_walk or all statistics
 * are enabled
 */
#define	WT_STAT_DSRC_BTREE_OVERFLOW			2044
/*!
 * btree: row-store empty values, only reported if tree_walk or all
 * statistics are enabled
 */
#define	WT_STAT_DSRC_BTREE_ROW_EMPTY_VALUES		2045
/*!
 * btree: row-store internal pages, only reported if tree_walk or all
 * statistics are enabled
 */
#define	WT_STAT_DSRC_BTREE_ROW_INTERNAL			2046
/*!
 * btree: row-store leaf pages, only reported if tree_walk or all
 * statistics are enabled
 */
#define	WT_STAT_DSRC_BTREE_ROW_LEAF			2047
/*! cache: bytes currently in the cache */
#define	WT_STAT_DSRC_CACHE_BYTES_INUSE			2048
/*! cache: bytes dirty in the cache cumulative */
#define	WT_STAT_DSRC_CACHE_BYTES_DIRTY_TOTAL		2049
/*! cache: bytes read into cache */
#define	WT_STAT_DSRC_CACHE_BYTES_READ			2050
/*! cache: bytes written from cache */
#define	WT_STAT_DSRC_CACHE_BYTES_WRITE			2051
/*! cache: checkpoint blocked page eviction */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_CHECKPOINT	2052
/*!
 * cache: checkpoint of history store file blocked non-history store page
 * eviction
 */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_CHECKPOINT_HS	2053
/*! cache: data source pages selected for eviction unable to be evicted */
#define	WT_STAT_DSRC_CACHE_EVICTION_FAIL		2054
/*!
 * cache: eviction gave up due to detecting a disk value without a
 * timestamp behind the last update on the chain
 */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_NO_TS_CHECKPOINT_RACE_1	2055
/*!
 * cache: eviction gave up due to detecting a tombstone without a
 * timestamp ahead of the selected on disk update
 */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_NO_TS_CHECKPOINT_RACE_2	2056
/*!
 * cache: eviction gave up due to detecting a tombstone without a
 * timestamp ahead of the selected on disk update after validating the
 * update chain
 */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_NO_TS_CHECKPOINT_RACE_3	2057
/*!
 * cache: eviction gave up due to detecting update chain entries without
 * timestamps after the selected on disk update
 */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_NO_TS_CHECKPOINT_RACE_4	2058
/*!
 * cache: eviction gave up due to needing to remove a record from the
 * history store but checkpoint is running
 */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_REMOVE_HS_RACE_WITH_CHECKPOINT	2059
/*! cache: eviction gave up due to no progress being made */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_NO_PROGRESS	2060
/*! cache: eviction walk passes of a file */
#define	WT_STAT_DSRC_CACHE_EVICTION_WALK_PASSES		2061
/*! cache: eviction walk target pages histogram - 0-9 */
#define	WT_STAT_DSRC_CACHE_EVICTION_TARGET_PAGE_LT10	2062
/*! cache: eviction walk target pages histogram - 10-31 */
#define	WT_STAT_DSRC_CACHE_EVICTION_TARGET_PAGE_LT32	2063
/*! cache: eviction walk target pages histogram - 128 and higher */
#define	WT_STAT_DSRC_CACHE_EVICTION_TARGET_PAGE_GE128	2064
/*! cache: eviction walk target pages histogram - 32-63 */
#define	WT_STAT_DSRC_CACHE_EVICTION_TARGET_PAGE_LT64	2065
/*! cache: eviction walk target pages histogram - 64-128 */
#define	WT_STAT_DSRC_CACHE_EVICTION_TARGET_PAGE_LT128	2066
/*!
 * cache: eviction walk target pages reduced due to history store cache
 * pressure
 */
#define	WT_STAT_DSRC_CACHE_EVICTION_TARGET_PAGE_REDUCED	2067
/*! cache: eviction walks abandoned */
#define	WT_STAT_DSRC_CACHE_EVICTION_WALKS_ABANDONED	2068
/*! cache: eviction walks gave up because they restarted their walk twice */
#define	WT_STAT_DSRC_CACHE_EVICTION_WALKS_STOPPED	2069
/*!
 * cache: eviction walks gave up because they saw too many pages and
 * found no candidates
 */
#define	WT_STAT_DSRC_CACHE_EVICTION_WALKS_GAVE_UP_NO_TARGETS	2070
/*!
 * cache: eviction walks gave up because they saw too many pages and
 * found too few candidates
 */
#define	WT_STAT_DSRC_CACHE_EVICTION_WALKS_GAVE_UP_RATIO	2071
/*! cache: eviction walks reached end of tree */
#define	WT_STAT_DSRC_CACHE_EVICTION_WALKS_ENDED		2072
/*! cache: eviction walks restarted */
#define	WT_STAT_DSRC_CACHE_EVICTION_WALK_RESTART	2073
/*! cache: eviction walks started from root of tree */
#define	WT_STAT_DSRC_CACHE_EVICTION_WALK_FROM_ROOT	2074
/*! cache: eviction walks started from saved location in tree */
#define	WT_STAT_DSRC_CACHE_EVICTION_WALK_SAVED_POS	2075
/*! cache: hazard pointer blocked page eviction */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_HAZARD	2076
/*! cache: history store table insert calls */
#define	WT_STAT_DSRC_CACHE_HS_INSERT			2077
/*! cache: history store table insert calls that returned restart */
#define	WT_STAT_DSRC_CACHE_HS_INSERT_RESTART		2078
/*! cache: history store table reads */
#define	WT_STAT_DSRC_CACHE_HS_READ			2079
/*! cache: history store table reads missed */
#define	WT_STAT_DSRC_CACHE_HS_READ_MISS			2080
/*! cache: history store table reads requiring squashed modifies */
#define	WT_STAT_DSRC_CACHE_HS_READ_SQUASH		2081
/*!
 * cache: history store table resolved updates without timestamps that
 * lose their durable timestamp
 */
#define	WT_STAT_DSRC_CACHE_HS_ORDER_LOSE_DURABLE_TIMESTAMP	2082
/*!
 * cache: history store table truncation by rollback to stable to remove
 * an unstable update
 */
#define	WT_STAT_DSRC_CACHE_HS_KEY_TRUNCATE_RTS_UNSTABLE	2083
/*!
 * cache: history store table truncation by rollback to stable to remove
 * an update
 */
#define	WT_STAT_DSRC_CACHE_HS_KEY_TRUNCATE_RTS		2084
/*!
 * cache: history store table truncation to remove all the keys of a
 * btree
 */
#define	WT_STAT_DSRC_CACHE_HS_BTREE_TRUNCATE		2085
/*! cache: history store table truncation to remove an update */
#define	WT_STAT_DSRC_CACHE_HS_KEY_TRUNCATE		2086
/*!
 * cache: history store table truncation to remove range of updates due
 * to an update without a timestamp on data page
 */
#define	WT_STAT_DSRC_CACHE_HS_ORDER_REMOVE		2087
/*!
 * cache: history store table truncation to remove range of updates due
 * to key being removed from the data page during reconciliation
 */
#define	WT_STAT_DSRC_CACHE_HS_KEY_TRUNCATE_ONPAGE_REMOVAL	2088
/*!
 * cache: history store table truncations that would have happened in
 * non-dryrun mode
 */
#define	WT_STAT_DSRC_CACHE_HS_BTREE_TRUNCATE_DRYRUN	2089
/*!
 * cache: history store table truncations to remove an unstable update
 * that would have happened in non-dryrun mode
 */
#define	WT_STAT_DSRC_CACHE_HS_KEY_TRUNCATE_RTS_UNSTABLE_DRYRUN	2090
/*!
 * cache: history store table truncations to remove an update that would
 * have happened in non-dryrun mode
 */
#define	WT_STAT_DSRC_CACHE_HS_KEY_TRUNCATE_RTS_DRYRUN	2091
/*!
 * cache: history store table updates without timestamps fixed up by
 * reinserting with the fixed timestamp
 */
#define	WT_STAT_DSRC_CACHE_HS_ORDER_REINSERT		2092
/*! cache: history store table writes requiring squashed modifies */
#define	WT_STAT_DSRC_CACHE_HS_WRITE_SQUASH		2093
/*! cache: in-memory page passed criteria to be split */
#define	WT_STAT_DSRC_CACHE_INMEM_SPLITTABLE		2094
/*! cache: in-memory page splits */
#define	WT_STAT_DSRC_CACHE_INMEM_SPLIT			2095
/*! cache: internal page split blocked its eviction */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_INTERNAL_PAGE_SPLIT	2096
/*! cache: internal pages evicted */
#define	WT_STAT_DSRC_CACHE_EVICTION_INTERNAL		2097
/*! cache: internal pages split during eviction */
#define	WT_STAT_DSRC_CACHE_EVICTION_SPLIT_INTERNAL	2098
/*! cache: leaf pages split during eviction */
#define	WT_STAT_DSRC_CACHE_EVICTION_SPLIT_LEAF		2099
/*! cache: modified pages evicted */
#define	WT_STAT_DSRC_CACHE_EVICTION_DIRTY		2100
/*! cache: multi-block reconciliation blocked whilst checkpoint is running */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_MULTI_BLOCK_RECONCILATION_DURING_CHECKPOINT	2101
/*!
 * cache: overflow keys on a multiblock row-store page blocked its
 * eviction
 */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_OVERFLOW_KEYS	2102
/*! cache: overflow pages read into cache */
#define	WT_STAT_DSRC_CACHE_READ_OVERFLOW		2103
/*! cache: page split during eviction deepened the tree */
#define	WT_STAT_DSRC_CACHE_EVICTION_DEEPEN		2104
/*! cache: page written requiring history store records */
#define	WT_STAT_DSRC_CACHE_WRITE_HS			2105
/*! cache: pages read into cache */
#define	WT_STAT_DSRC_CACHE_READ				2106
/*! cache: pages read into cache after truncate */
#define	WT_STAT_DSRC_CACHE_READ_DELETED			2107
/*! cache: pages read into cache after truncate in prepare state */
#define	WT_STAT_DSRC_CACHE_READ_DELETED_PREPARED	2108
/*! cache: pages requested from the cache */
#define	WT_STAT_DSRC_CACHE_PAGES_REQUESTED		2109
/*! cache: pages requested from the cache due to pre-fetch */
#define	WT_STAT_DSRC_CACHE_PAGES_PREFETCH		2110
/*! cache: pages seen by eviction walk */
#define	WT_STAT_DSRC_CACHE_EVICTION_PAGES_SEEN		2111
/*! cache: pages written from cache */
#define	WT_STAT_DSRC_CACHE_WRITE			2112
/*! cache: pages written requiring in-memory restoration */
#define	WT_STAT_DSRC_CACHE_WRITE_RESTORE		2113
/*! cache: recent modification of a page blocked its eviction */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_RECENTLY_MODIFIED	2114
/*! cache: reverse splits performed */
#define	WT_STAT_DSRC_CACHE_REVERSE_SPLITS		2115
/*!
 * cache: reverse splits skipped because of VLCS namespace gap
 * restrictions
 */
#define	WT_STAT_DSRC_CACHE_REVERSE_SPLITS_SKIPPED_VLCS	2116
/*! cache: the number of times full update inserted to history store */
#define	WT_STAT_DSRC_CACHE_HS_INSERT_FULL_UPDATE	2117
/*! cache: the number of times reverse modify inserted to history store */
#define	WT_STAT_DSRC_CACHE_HS_INSERT_REVERSE_MODIFY	2118
/*! cache: tracked dirty bytes in the cache */
#define	WT_STAT_DSRC_CACHE_BYTES_DIRTY			2119
/*! cache: uncommitted truncate blocked page eviction */
#define	WT_STAT_DSRC_CACHE_EVICTION_BLOCKED_UNCOMMITTED_TRUNCATE	2120
/*! cache: unmodified pages evicted */
#define	WT_STAT_DSRC_CACHE_EVICTION_CLEAN		2121
/*!
 * cache_walk: Average difference between current eviction generation
 * when the page was last considered, only reported if cache_walk or all
 * statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_GEN_AVG_GAP		2122
/*!
 * cache_walk: Average on-disk page image size seen, only reported if
 * cache_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_AVG_WRITTEN_SIZE	2123
/*!
 * cache_walk: Average time in cache for pages that have been visited by
 * the eviction server, only reported if cache_walk or all statistics are
 * enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_AVG_VISITED_AGE	2124
/*!
 * cache_walk: Average time in cache for pages that have not been visited
 * by the eviction server, only reported if cache_walk or all statistics
 * are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_AVG_UNVISITED_AGE	2125
/*!
 * cache_walk: Clean pages currently in cache, only reported if
 * cache_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_PAGES_CLEAN		2126
/*!
 * cache_walk: Current eviction generation, only reported if cache_walk
 * or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_GEN_CURRENT		2127
/*!
 * cache_walk: Dirty pages currently in cache, only reported if
 * cache_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_PAGES_DIRTY		2128
/*!
 * cache_walk: Entries in the root page, only reported if cache_walk or
 * all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_ROOT_ENTRIES		2129
/*!
 * cache_walk: Internal pages currently in cache, only reported if
 * cache_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_PAGES_INTERNAL		2130
/*!
 * cache_walk: Leaf pages currently in cache, only reported if cache_walk
 * or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_PAGES_LEAF		2131
/*!
 * cache_walk: Maximum difference between current eviction generation
 * when the page was last considered, only reported if cache_walk or all
 * statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_GEN_MAX_GAP		2132
/*!
 * cache_walk: Maximum page size seen, only reported if cache_walk or all
 * statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_MAX_PAGESIZE		2133
/*!
 * cache_walk: Minimum on-disk page image size seen, only reported if
 * cache_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_MIN_WRITTEN_SIZE	2134
/*!
 * cache_walk: Number of pages never visited by eviction server, only
 * reported if cache_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_UNVISITED_COUNT	2135
/*!
 * cache_walk: On-disk page image sizes smaller than a single allocation
 * unit, only reported if cache_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_SMALLER_ALLOC_SIZE	2136
/*!
 * cache_walk: Pages created in memory and never written, only reported
 * if cache_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_MEMORY			2137
/*!
 * cache_walk: Pages currently queued for eviction, only reported if
 * cache_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_QUEUED			2138
/*!
 * cache_walk: Pages that could not be queued for eviction, only reported
 * if cache_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_NOT_QUEUEABLE		2139
/*!
 * cache_walk: Refs skipped during cache traversal, only reported if
 * cache_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_REFS_SKIPPED		2140
/*!
 * cache_walk: Size of the root page, only reported if cache_walk or all
 * statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_ROOT_SIZE		2141
/*!
 * cache_walk: Total number of pages currently in cache, only reported if
 * cache_walk or all statistics are enabled
 */
#define	WT_STAT_DSRC_CACHE_STATE_PAGES			2142
/*! checkpoint: checkpoint has acquired a snapshot for its transaction */
#define	WT_STAT_DSRC_CHECKPOINT_SNAPSHOT_ACQUIRED	2143
/*! checkpoint: pages added for eviction during checkpoint cleanup */
#define	WT_STAT_DSRC_CHECKPOINT_CLEANUP_PAGES_EVICT	2144
/*! checkpoint: pages removed during checkpoint cleanup */
#define	WT_STAT_DSRC_CHECKPOINT_CLEANUP_PAGES_REMOVED	2145
/*! checkpoint: pages skipped during checkpoint cleanup tree walk */
#define	WT_STAT_DSRC_CHECKPOINT_CLEANUP_PAGES_WALK_SKIPPED	2146
/*! checkpoint: pages visited during checkpoint cleanup */
#define	WT_STAT_DSRC_CHECKPOINT_CLEANUP_PAGES_VISITED	2147
/*! checkpoint: transaction checkpoints due to obsolete pages */
#define	WT_STAT_DSRC_CHECKPOINT_OBSOLETE_APPLIED	2148
/*!
 * compression: compressed page maximum internal page size prior to
 * compression
 */
#define	WT_STAT_DSRC_COMPRESS_PRECOMP_INTL_MAX_PAGE_SIZE	2149
/*!
 * compression: compressed page maximum leaf page size prior to
 * compression
 */
#define	WT_STAT_DSRC_COMPRESS_PRECOMP_LEAF_MAX_PAGE_SIZE	2150
/*! compression: page written to disk failed to compress */
#define	WT_STAT_DSRC_COMPRESS_WRITE_FAIL		2151
/*! compression: page written to disk was too small to compress */
#define	WT_STAT_DSRC_COMPRESS_WRITE_TOO_SMALL		2152
/*! compression: pages read from disk */
#define	WT_STAT_DSRC_COMPRESS_READ			2153
/*!
 * compression: pages read from disk with compression ratio greater than
 * 64
 */
#define	WT_STAT_DSRC_COMPRESS_READ_RATIO_HIST_MAX	2154
/*!
 * compression: pages read from disk with compression ratio smaller than
 * 2
 */
#define	WT_STAT_DSRC_COMPRESS_READ_RATIO_HIST_2		2155
/*!
 * compression: pages read from disk with compression ratio smaller than
 * 4
 */
#define	WT_STAT_DSRC_COMPRESS_READ_RATIO_HIST_4		2156
/*!
 * compression: pages read from disk with compression ratio smaller than
 * 8
 */
#define	WT_STAT_DSRC_COMPRESS_READ_RATIO_HIST_8		2157
/*!
 * compression: pages read from disk with compression ratio smaller than
 * 16
 */
#define	WT_STAT_DSRC_COMPRESS_READ_RATIO_HIST_16	2158
/*!
 * compression: pages read from disk with compression ratio smaller than
 * 32
 */
#define	WT_STAT_DSRC_COMPRESS_READ_RATIO_HIST_32	2159
/*!
 * compression: pages read from disk with compression ratio smaller than
 * 64
 */
#define	WT_STAT_DSRC_COMPRESS_READ_RATIO_HIST_64	2160
/*! compression: pages written to disk */
#define	WT_STAT_DSRC_COMPRESS_WRITE			2161
/*!
 * compression: pages written to disk with compression ratio greater than
 * 64
 */
#define	WT_STAT_DSRC_COMPRESS_WRITE_RATIO_HIST_MAX	2162
/*!
 * compression: pages written to disk with compression ratio smaller than
 * 2
 */
#define	WT_STAT_DSRC_COMPRESS_WRITE_RATIO_HIST_2	2163
/*!
 * compression: pages written to disk with compression ratio smaller than
 * 4
 */
#define	WT_STAT_DSRC_COMPRESS_WRITE_RATIO_HIST_4	2164
/*!
 * compression: pages written to disk with compression ratio smaller than
 * 8
 */
#define	WT_STAT_DSRC_COMPRESS_WRITE_RATIO_HIST_8	2165
/*!
 * compression: pages written to disk with compression ratio smaller than
 * 16
 */
#define	WT_STAT_DSRC_COMPRESS_WRITE_RATIO_HIST_16	2166
/*!
 * compression: pages written to disk with compression ratio smaller than
 * 32
 */
#define	WT_STAT_DSRC_COMPRESS_WRITE_RATIO_HIST_32	2167
/*!
 * compression: pages written to disk with compression ratio smaller than
 * 64
 */
#define	WT_STAT_DSRC_COMPRESS_WRITE_RATIO_HIST_64	2168
/*! cursor: Total number of entries skipped by cursor next calls */
#define	WT_STAT_DSRC_CURSOR_NEXT_SKIP_TOTAL		2169
/*! cursor: Total number of entries skipped by cursor prev calls */
#define	WT_STAT_DSRC_CURSOR_PREV_SKIP_TOTAL		2170
/*!
 * cursor: Total number of entries skipped to position the history store
 * cursor
 */
#define	WT_STAT_DSRC_CURSOR_SKIP_HS_CUR_POSITION	2171
/*!
 * cursor: Total number of times a search near has exited due to prefix
 * config
 */
#define	WT_STAT_DSRC_CURSOR_SEARCH_NEAR_PREFIX_FAST_PATHS	2172
/*!
 * cursor: Total number of times cursor fails to temporarily release
 * pinned page to encourage eviction of hot or large page
 */
#define	WT_STAT_DSRC_CURSOR_REPOSITION_FAILED		2173
/*!
 * cursor: Total number of times cursor temporarily releases pinned page
 * to encourage eviction of hot or large page
 */
#define	WT_STAT_DSRC_CURSOR_REPOSITION			2174
/*! cursor: bulk loaded cursor insert calls */
#define	WT_STAT_DSRC_CURSOR_INSERT_BULK			2175
/*! cursor: cache cursors reuse count */
#define	WT_STAT_DSRC_CURSOR_REOPEN			2176
/*! cursor: close calls that result in cache */
#define	WT_STAT_DSRC_CURSOR_CACHE			2177
/*! cursor: create calls */
#define	WT_STAT_DSRC_CURSOR_CREATE			2178
/*! cursor: cursor bound calls that return an error */
#define	WT_STAT_DSRC_CURSOR_BOUND_ERROR			2179
/*! cursor: cursor bounds cleared from reset */
#define	WT_STAT_DSRC_CURSOR_BOUNDS_RESET		2180
/*! cursor: cursor bounds comparisons performed */
#define	WT_STAT_DSRC_CURSOR_BOUNDS_COMPARISONS		2181
/*! cursor: cursor bounds next called on an unpositioned cursor */
#define	WT_STAT_DSRC_CURSOR_BOUNDS_NEXT_UNPOSITIONED	2182
/*! cursor: cursor bounds next early exit */
#define	WT_STAT_DSRC_CURSOR_BOUNDS_NEXT_EARLY_EXIT	2183
/*! cursor: cursor bounds prev called on an unpositioned cursor */
#define	WT_STAT_DSRC_CURSOR_BOUNDS_PREV_UNPOSITIONED	2184
/*! cursor: cursor bounds prev early exit */
#define	WT_STAT_DSRC_CURSOR_BOUNDS_PREV_EARLY_EXIT	2185
/*! cursor: cursor bounds search early exit */
#define	WT_STAT_DSRC_CURSOR_BOUNDS_SEARCH_EARLY_EXIT	2186
/*! cursor: cursor bounds search near call repositioned cursor */
#define	WT_STAT_DSRC_CURSOR_BOUNDS_SEARCH_NEAR_REPOSITIONED_CURSOR	2187
/*! cursor: cursor cache calls that return an error */
#define	WT_STAT_DSRC_CURSOR_CACHE_ERROR			2188
/*! cursor: cursor close calls that return an error */
#define	WT_STAT_DSRC_CURSOR_CLOSE_ERROR			2189
/*! cursor: cursor compare calls that return an error */
#define	WT_STAT_DSRC_CURSOR_COMPARE_ERROR		2190
/*! cursor: cursor equals calls that return an error */
#define	WT_STAT_DSRC_CURSOR_EQUALS_ERROR		2191
/*! cursor: cursor get key calls that return an error */
#define	WT_STAT_DSRC_CURSOR_GET_KEY_ERROR		2192
/*! cursor: cursor get value calls that return an error */
#define	WT_STAT_DSRC_CURSOR_GET_VALUE_ERROR		2193
/*! cursor: cursor insert calls that return an error */
#define	WT_STAT_DSRC_CURSOR_INSERT_ERROR		2194
/*! cursor: cursor insert check calls that return an error */
#define	WT_STAT_DSRC_CURSOR_INSERT_CHECK_ERROR		2195
/*! cursor: cursor largest key calls that return an error */
#define	WT_STAT_DSRC_CURSOR_LARGEST_KEY_ERROR		2196
/*! cursor: cursor modify calls that return an error */
#define	WT_STAT_DSRC_CURSOR_MODIFY_ERROR		2197
/*! cursor: cursor next calls that return an error */
#define	WT_STAT_DSRC_CURSOR_NEXT_ERROR			2198
/*!
 * cursor: cursor next calls that skip due to a globally visible history
 * store tombstone
 */
#define	WT_STAT_DSRC_CURSOR_NEXT_HS_TOMBSTONE		2199
/*!
 * cursor: cursor next calls that skip greater than 1 and fewer than 100
 * entries
 */
#define	WT_STAT_DSRC_CURSOR_NEXT_SKIP_LT_100		2200
/*!
 * cursor: cursor next calls that skip greater than or equal to 100
 * entries
 */
#define	WT_STAT_DSRC_CURSOR_NEXT_SKIP_GE_100		2201
/*! cursor: cursor next random calls that return an error */
#define	WT_STAT_DSRC_CURSOR_NEXT_RANDOM_ERROR		2202
/*! cursor: cursor prev calls that return an error */
#define	WT_STAT_DSRC_CURSOR_PREV_ERROR			2203
/*!
 * cursor: cursor prev calls that skip due to a globally visible history
 * store tombstone
 */
#define	WT_STAT_DSRC_CURSOR_PREV_HS_TOMBSTONE		2204
/*!
 * cursor: cursor prev calls that skip greater than or equal to 100
 * entries
 */
#define	WT_STAT_DSRC_CURSOR_PREV_SKIP_GE_100		2205
/*! cursor: cursor prev calls that skip less than 100 entries */
#define	WT_STAT_DSRC_CURSOR_PREV_SKIP_LT_100		2206
/*! cursor: cursor reconfigure calls that return an error */
#define	WT_STAT_DSRC_CURSOR_RECONFIGURE_ERROR		2207
/*! cursor: cursor remove calls that return an error */
#define	WT_STAT_DSRC_CURSOR_REMOVE_ERROR		2208
/*! cursor: cursor reopen calls that return an error */
#define	WT_STAT_DSRC_CURSOR_REOPEN_ERROR		2209
/*! cursor: cursor reserve calls that return an error */
#define	WT_STAT_DSRC_CURSOR_RESERVE_ERROR		2210
/*! cursor: cursor reset calls that return an error */
#define	WT_STAT_DSRC_CURSOR_RESET_ERROR			2211
/*! cursor: cursor search calls that return an error */
#define	WT_STAT_DSRC_CURSOR_SEARCH_ERROR		2212
/*! cursor: cursor search near calls that return an error */
#define	WT_STAT_DSRC_CURSOR_SEARCH_NEAR_ERROR		2213
/*! cursor: cursor update calls that return an error */
#define	WT_STAT_DSRC_CURSOR_UPDATE_ERROR		2214
/*! cursor: insert calls */
#define	WT_STAT_DSRC_CURSOR_INSERT			2215
/*! cursor: insert key and value bytes */
#define	WT_STAT_DSRC_CURSOR_INSERT_BYTES		2216
/*! cursor: modify */
#define	WT_STAT_DSRC_CURSOR_MODIFY			2217
/*! cursor: modify key and value bytes affected */
#define	WT_STAT_DSRC_CURSOR_MODIFY_BYTES		2218
/*! cursor: modify value bytes modified */
#define	WT_STAT_DSRC_CURSOR_MODIFY_BYTES_TOUCH		2219
/*! cursor: next calls */
#define	WT_STAT_DSRC_CURSOR_NEXT			2220
/*! cursor: open cursor count */
#define	WT_STAT_DSRC_CURSOR_OPEN_COUNT			2221
/*! cursor: operation restarted */
#define	WT_STAT_DSRC_CURSOR_RESTART			2222
/*! cursor: prev calls */
#define	WT_STAT_DSRC_CURSOR_PREV			2223
/*! cursor: remove calls */
#define	WT_STAT_DSRC_CURSOR_REMOVE			2224
/*! cursor: remove key bytes removed */
#define	WT_STAT_DSRC_CURSOR_REMOVE_BYTES		2225
/*! cursor: reserve calls */
#define	WT_STAT_DSRC_CURSOR_RESERVE			2226
/*! cursor: reset calls */
#define	WT_STAT_DSRC_CURSOR_RESET			2227
/*! cursor: search calls */
#define	WT_STAT_DSRC_CURSOR_SEARCH			2228
/*! cursor: search history store calls */
#define	WT_STAT_DSRC_CURSOR_SEARCH_HS			2229
/*! cursor: search near calls */
#define	WT_STAT_DSRC_CURSOR_SEARCH_NEAR			2230
/*! cursor: truncate calls */
#define	WT_STAT_DSRC_CURSOR_TRUNCATE			2231
/*! cursor: update calls */
#define	WT_STAT_DSRC_CURSOR_UPDATE			2232
/*! cursor: update key and value bytes */
#define	WT_STAT_DSRC_CURSOR_UPDATE_BYTES		2233
/*! cursor: update value size change */
#define	WT_STAT_DSRC_CURSOR_UPDATE_BYTES_CHANGED	2234
/*! reconciliation: VLCS pages explicitly reconciled as empty */
#define	WT_STAT_DSRC_REC_VLCS_EMPTIED_PAGES		2235
/*! reconciliation: approximate byte size of timestamps in pages written */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_BYTES_TS		2236
/*!
 * reconciliation: approximate byte size of transaction IDs in pages
 * written
 */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_BYTES_TXN		2237
/*! reconciliation: dictionary matches */
#define	WT_STAT_DSRC_REC_DICTIONARY			2238
/*! reconciliation: fast-path pages deleted */
#define	WT_STAT_DSRC_REC_PAGE_DELETE_FAST		2239
/*!
 * reconciliation: internal page key bytes discarded using suffix
 * compression
 */
#define	WT_STAT_DSRC_REC_SUFFIX_COMPRESSION		2240
/*! reconciliation: internal page multi-block writes */
#define	WT_STAT_DSRC_REC_MULTIBLOCK_INTERNAL		2241
/*! reconciliation: leaf page key bytes discarded using prefix compression */
#define	WT_STAT_DSRC_REC_PREFIX_COMPRESSION		2242
/*! reconciliation: leaf page multi-block writes */
#define	WT_STAT_DSRC_REC_MULTIBLOCK_LEAF		2243
/*! reconciliation: leaf-page overflow keys */
#define	WT_STAT_DSRC_REC_OVERFLOW_KEY_LEAF		2244
/*! reconciliation: maximum blocks required for a page */
#define	WT_STAT_DSRC_REC_MULTIBLOCK_MAX			2245
/*! reconciliation: overflow values written */
#define	WT_STAT_DSRC_REC_OVERFLOW_VALUE			2246
/*! reconciliation: page reconciliation calls */
#define	WT_STAT_DSRC_REC_PAGES				2247
/*! reconciliation: page reconciliation calls for eviction */
#define	WT_STAT_DSRC_REC_PAGES_EVICTION			2248
/*! reconciliation: pages deleted */
#define	WT_STAT_DSRC_REC_PAGE_DELETE			2249
/*!
 * reconciliation: pages written including an aggregated newest start
 * durable timestamp
 */
#define	WT_STAT_DSRC_REC_TIME_AGGR_NEWEST_START_DURABLE_TS	2250
/*!
 * reconciliation: pages written including an aggregated newest stop
 * durable timestamp
 */
#define	WT_STAT_DSRC_REC_TIME_AGGR_NEWEST_STOP_DURABLE_TS	2251
/*!
 * reconciliation: pages written including an aggregated newest stop
 * timestamp
 */
#define	WT_STAT_DSRC_REC_TIME_AGGR_NEWEST_STOP_TS	2252
/*!
 * reconciliation: pages written including an aggregated newest stop
 * transaction ID
 */
#define	WT_STAT_DSRC_REC_TIME_AGGR_NEWEST_STOP_TXN	2253
/*!
 * reconciliation: pages written including an aggregated newest
 * transaction ID
 */
#define	WT_STAT_DSRC_REC_TIME_AGGR_NEWEST_TXN		2254
/*!
 * reconciliation: pages written including an aggregated oldest start
 * timestamp
 */
#define	WT_STAT_DSRC_REC_TIME_AGGR_OLDEST_START_TS	2255
/*! reconciliation: pages written including an aggregated prepare */
#define	WT_STAT_DSRC_REC_TIME_AGGR_PREPARED		2256
/*! reconciliation: pages written including at least one prepare */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_PAGES_PREPARED	2257
/*!
 * reconciliation: pages written including at least one start durable
 * timestamp
 */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_PAGES_DURABLE_START_TS	2258
/*! reconciliation: pages written including at least one start timestamp */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_PAGES_START_TS	2259
/*!
 * reconciliation: pages written including at least one start transaction
 * ID
 */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_PAGES_START_TXN	2260
/*!
 * reconciliation: pages written including at least one stop durable
 * timestamp
 */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_PAGES_DURABLE_STOP_TS	2261
/*! reconciliation: pages written including at least one stop timestamp */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_PAGES_STOP_TS	2262
/*!
 * reconciliation: pages written including at least one stop transaction
 * ID
 */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_PAGES_STOP_TXN	2263
/*! reconciliation: records written including a prepare */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_PREPARED		2264
/*! reconciliation: records written including a start durable timestamp */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_DURABLE_START_TS	2265
/*! reconciliation: records written including a start timestamp */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_START_TS		2266
/*! reconciliation: records written including a start transaction ID */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_START_TXN		2267
/*! reconciliation: records written including a stop durable timestamp */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_DURABLE_STOP_TS	2268
/*! reconciliation: records written including a stop timestamp */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_STOP_TS		2269
/*! reconciliation: records written including a stop transaction ID */
#define	WT_STAT_DSRC_REC_TIME_WINDOW_STOP_TXN		2270
/*! session: object compaction */
#define	WT_STAT_DSRC_SESSION_COMPACT			2271
/*!
 * transaction: a reader raced with a prepared transaction commit and
 * skipped an update or updates
 */
#define	WT_STAT_DSRC_TXN_READ_RACE_PREPARE_COMMIT	2272
/*! transaction: number of times overflow removed value is read */
#define	WT_STAT_DSRC_TXN_READ_OVERFLOW_REMOVE		2273
/*! transaction: race to read prepared update retry */
#define	WT_STAT_DSRC_TXN_READ_RACE_PREPARE_UPDATE	2274
/*!
 * transaction: rollback to stable history store keys that would have
 * been swept in non-dryrun mode
 */
#define	WT_STAT_DSRC_TXN_RTS_SWEEP_HS_KEYS_DRYRUN	2275
/*!
 * transaction: rollback to stable history store records with stop
 * timestamps older than newer records
 */
#define	WT_STAT_DSRC_TXN_RTS_HS_STOP_OLDER_THAN_NEWER_START	2276
/*! transaction: rollback to stable inconsistent checkpoint */
#define	WT_STAT_DSRC_TXN_RTS_INCONSISTENT_CKPT		2277
/*! transaction: rollback to stable keys removed */
#define	WT_STAT_DSRC_TXN_RTS_KEYS_REMOVED		2278
/*! transaction: rollback to stable keys restored */
#define	WT_STAT_DSRC_TXN_RTS_KEYS_RESTORED		2279
/*!
 * transaction: rollback to stable keys that would have been removed in
 * non-dryrun mode
 */
#define	WT_STAT_DSRC_TXN_RTS_KEYS_REMOVED_DRYRUN	2280
/*!
 * transaction: rollback to stable keys that would have been restored in
 * non-dryrun mode
 */
#define	WT_STAT_DSRC_TXN_RTS_KEYS_RESTORED_DRYRUN	2281
/*! transaction: rollback to stable restored tombstones from history store */
#define	WT_STAT_DSRC_TXN_RTS_HS_RESTORE_TOMBSTONES	2282
/*! transaction: rollback to stable restored updates from history store */
#define	WT_STAT_DSRC_TXN_RTS_HS_RESTORE_UPDATES		2283
/*! transaction: rollback to stable skipping delete rle */
#define	WT_STAT_DSRC_TXN_RTS_DELETE_RLE_SKIPPED		2284
/*! transaction: rollback to stable skipping stable rle */
#define	WT_STAT_DSRC_TXN_RTS_STABLE_RLE_SKIPPED		2285
/*! transaction: rollback to stable sweeping history store keys */
#define	WT_STAT_DSRC_TXN_RTS_SWEEP_HS_KEYS		2286
/*!
 * transaction: rollback to stable tombstones from history store that
 * would have been restored in non-dryrun mode
 */
#define	WT_STAT_DSRC_TXN_RTS_HS_RESTORE_TOMBSTONES_DRYRUN	2287
/*!
 * transaction: rollback to stable updates from history store that would
 * have been restored in non-dryrun mode
 */
#define	WT_STAT_DSRC_TXN_RTS_HS_RESTORE_UPDATES_DRYRUN	2288
/*! transaction: rollback to stable updates removed from history store */
#define	WT_STAT_DSRC_TXN_RTS_HS_REMOVED			2289
/*!
 * transaction: rollback to stable updates that would have been removed
 * from history store in non-dryrun mode
 */
#define	WT_STAT_DSRC_TXN_RTS_HS_REMOVED_DRYRUN		2290
/*! transaction: update conflicts */
#define	WT_STAT_DSRC_TXN_UPDATE_CONFLICT		2291

/*!
 * @}
 * @name Statistics for join cursors
 * @anchor statistics_join
 * @{
 */
/*! join: accesses to the main table */
#define	WT_STAT_JOIN_MAIN_ACCESS			3000
/*! join: bloom filter false positives */
#define	WT_STAT_JOIN_BLOOM_FALSE_POSITIVE		3001
/*! join: checks that conditions of membership are satisfied */
#define	WT_STAT_JOIN_MEMBERSHIP_CHECK			3002
/*! join: items inserted into a bloom filter */
#define	WT_STAT_JOIN_BLOOM_INSERT			3003
/*! join: items iterated */
#define	WT_STAT_JOIN_ITERATED				3004

/*!
 * @}
 * @name Statistics for session
 * @anchor statistics_session
 * @{
 */
/*! session: bytes read into cache */
#define	WT_STAT_SESSION_BYTES_READ			4000
/*! session: bytes written from cache */
#define	WT_STAT_SESSION_BYTES_WRITE			4001
/*! session: dhandle lock wait time (usecs) */
#define	WT_STAT_SESSION_LOCK_DHANDLE_WAIT		4002
/*! session: dirty bytes in this txn */
#define	WT_STAT_SESSION_TXN_BYTES_DIRTY			4003
/*! session: page read from disk to cache time (usecs) */
#define	WT_STAT_SESSION_READ_TIME			4004
/*! session: page write from cache to disk time (usecs) */
#define	WT_STAT_SESSION_WRITE_TIME			4005
/*! session: schema lock wait time (usecs) */
#define	WT_STAT_SESSION_LOCK_SCHEMA_WAIT		4006
/*! session: time waiting for cache (usecs) */
#define	WT_STAT_SESSION_CACHE_TIME			4007
/*! @} */
/*
 * Statistics section: END
 * DO NOT EDIT: automatically built by dist/stat.py.
 */
/*! @} */

/*******************************************
 * Verbose categories
 *******************************************/
/*!
 * @addtogroup wt
 * @{
 */
/*!
 * @name Verbose categories
 * @anchor verbose_categories
 * @{
 */
/*!
 * WiredTiger verbose event categories.
 * Note that the verbose categories cover a wide set of sub-systems and operations
 * within WiredTiger. As such, the categories are subject to change and evolve
 * between different WiredTiger releases.
 */
typedef enum {
/* VERBOSE ENUM START */
    WT_VERB_ALL,                  /*!< ALL module messages. */
    WT_VERB_API,                  /*!< API messages. */
    WT_VERB_BACKUP,               /*!< Backup messages. */
    WT_VERB_BLKCACHE,
    WT_VERB_BLOCK,                /*!< Block manager messages. */
    WT_VERB_CHECKPOINT,           /*!< Checkpoint messages. */
    WT_VERB_CHECKPOINT_CLEANUP,
    WT_VERB_CHECKPOINT_PROGRESS,  /*!< Checkpoint progress messages. */
    WT_VERB_CHUNKCACHE,           /*!< Chunk cache messages. */
    WT_VERB_COMPACT,              /*!< Compact messages. */
    WT_VERB_COMPACT_PROGRESS,     /*!< Compact progress messages. */
    WT_VERB_CONFIGURATION,
    WT_VERB_DEFAULT,
    WT_VERB_ERROR_RETURNS,
    WT_VERB_EVICT,                /*!< Eviction messages. */
    WT_VERB_EVICTSERVER,          /*!< Eviction server messages. */
    WT_VERB_EVICT_STUCK,
    WT_VERB_EXTENSION,            /*!< Extension messages. */
    WT_VERB_FILEOPS,
    WT_VERB_GENERATION,
    WT_VERB_HANDLEOPS,
    WT_VERB_HS,                   /*!< History store messages. */
    WT_VERB_HS_ACTIVITY,          /*!< History store activity messages. */
    WT_VERB_LOG,                  /*!< Log messages. */
    WT_VERB_LSM,                  /*!< LSM messages. */
    WT_VERB_LSM_MANAGER,
    WT_VERB_MUTEX,
    WT_VERB_METADATA,             /*!< Metadata messages. */
    WT_VERB_OUT_OF_ORDER,
    WT_VERB_OVERFLOW,
    WT_VERB_PREFETCH,
    WT_VERB_READ,
    WT_VERB_RECONCILE,            /*!< Reconcile messages. */
    WT_VERB_RECOVERY,             /*!< Recovery messages. */
    WT_VERB_RECOVERY_PROGRESS,    /*!< Recovery progress messages. */
    WT_VERB_RTS,                  /*!< RTS messages. */
    WT_VERB_SALVAGE,              /*!< Salvage messages. */
    WT_VERB_SHARED_CACHE,
    WT_VERB_SPLIT,
    WT_VERB_TEMPORARY,
    WT_VERB_THREAD_GROUP,
    WT_VERB_TIERED,               /*!< Tiered storage messages. */
    WT_VERB_TIMESTAMP,            /*!< Timestamp messages. */
    WT_VERB_TRANSACTION,          /*!< Transaction messages. */
    WT_VERB_VERIFY,               /*!< Verify messages. */
    WT_VERB_VERSION,              /*!< Version messages. */
    WT_VERB_WRITE,
/* VERBOSE ENUM STOP */
    WT_VERB_NUM_CATEGORIES
} WT_VERBOSE_CATEGORY;
/*! @} */

/*******************************************
 * Verbose levels
 *******************************************/
/*!
 * @name Verbose levels
 * @anchor verbose_levels
 * @{
 */
/*!
 * WiredTiger verbosity levels. The levels define a range of severity categories, with
 * \c WT_VERBOSE_ERROR being the lowest, most critical level (used by messages on critical error
 * paths) and \c WT_VERBOSE_DEBUG_5 being the highest verbosity/informational level (mostly adopted
 * for debugging).
 */
typedef enum {
    WT_VERBOSE_ERROR = -3,  /*!< Error conditions triggered in WiredTiger. */
    WT_VERBOSE_WARNING,     /*!< Warning conditions potentially signaling non-imminent errors and
                            behaviors. */
    WT_VERBOSE_NOTICE,      /*!< Messages for significant events in WiredTiger, usually worth
                            noting. */
    WT_VERBOSE_INFO,        /*!< Informational style messages. */
    WT_VERBOSE_DEBUG_1,     /*!< Low severity messages, useful for debugging purposes. This is
                            the default level when debugging. */
    WT_VERBOSE_DEBUG_2,     /*!< Low severity messages, an increase in verbosity from
                            the previous level. */
    WT_VERBOSE_DEBUG_3,     /*!< Low severity messages. */
    WT_VERBOSE_DEBUG_4,     /*!< Low severity messages. */
    WT_VERBOSE_DEBUG_5      /*!< Lowest severity messages. */
} WT_VERBOSE_LEVEL;
/*! @} */
/*
 * Verbose section: END
 */
/*! @} */

#undef __F

#if defined(__cplusplus)
}
#endif
#endif /* __WIREDTIGER_H_ */
