#pragma once

#include <gflags/gflags.h>

namespace hashdb {

DECLARE_string(hashdb_files_directory);
DECLARE_string(hashdb_data_files_directory);
DECLARE_string(hashdb_index_files_directory);

DECLARE_uint32(hashdb_slots_map_size);
DECLARE_uint32(hashdb_foreground_threads_num);
DECLARE_uint32(hashdb_background_threads_num);

DECLARE_uint64(blob_approximate_size);
DECLARE_uint32(blob_write_buffer_size);
DECLARE_double(blob_gc_min_utility_threshold);

DECLARE_bool  (gc_enable);
DECLARE_uint64(gc_check_every_some_writes);
DECLARE_bool  (gc_enable_data_files_gc);
DECLARE_uint32(gc_trigger_min_blob_num);
DECLARE_bool  (gc_enable_cache_evict);
DECLARE_uint64(gc_cache_max_threshold);
DECLARE_uint32(gc_max_evict_slot_num_per_round);
DECLARE_bool  (gc_enable_index_colddown);
DECLARE_uint32(gc_max_colddown_index_slot_num_per_round);

DECLARE_uint32(bloom_filters_num);
DECLARE_double(bloom_filters_false_positive_rate);
DECLARE_uint32(bloom_filters_elements_num);

DECLARE_uint32(main_key_range);
DECLARE_uint32(main_write_key_times);
DECLARE_uint32(main_record_size);
DECLARE_uint32(main_key_size);
DECLARE_uint32(main_threads_num);

}