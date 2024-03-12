#include "hashdb/hashdb_db.h"
#include "core/db_factory.h"

namespace {
const std::string PROP_NAME = "hashdb.dbname";
const std::string PROP_NAME_DEFAULT = "";

const std::string PROP_FORMAT = "hashdb.format";
const std::string PROP_FORMAT_DEFAULT = "single";

const std::string PROP_DESTROY = "hashdb.destroy";
const std::string PROP_DESTROY_DEFAULT = "false";

//   const std::string PROP_COMPRESSION = "hashdb.compression";
//   const std::string PROP_COMPRESSION_DEFAULT = "no";

//   const std::string PROP_WRITE_BUFFER_SIZE = "hashdb.write_buffer_size";
//   const std::string PROP_WRITE_BUFFER_SIZE_DEFAULT = "0";

//   const std::string PROP_MAX_FILE_SIZE = "hashdb.max_file_size";
//   const std::string PROP_MAX_FILE_SIZE_DEFAULT = "0";

//   const std::string PROP_MAX_OPEN_FILES = "hashdb.max_open_files";
//   const std::string PROP_MAX_OPEN_FILES_DEFAULT = "0";

//   const std::string PROP_CACHE_SIZE = "hashdb.cache_size";
//   const std::string PROP_CACHE_SIZE_DEFAULT = "0";

//   const std::string PROP_FILTER_BITS = "hashdb.filter_bits";
//   const std::string PROP_FILTER_BITS_DEFAULT = "0";

//   const std::string PROP_BLOCK_SIZE = "hashdb.block_size";
//   const std::string PROP_BLOCK_SIZE_DEFAULT = "0";

//   const std::string PROP_BLOCK_RESTART_INTERVAL =
//   "hashdb.block_restart_interval"; const std::string
//   PROP_BLOCK_RESTART_INTERVAL_DEFAULT = "0";

const std::string PROP_hashdb_files_directory = "hashdb.hashdb_files_directory";
const std::string PROP_hashdb_data_files_directory = "hashdb.hashdb_data_files_directory";
const std::string PROP_hashdb_index_files_directory = "hashdb.hashdb_index_files_directory";
const std::string PROP_hashdb_slots_map_size = "hashdb.hashdb_slots_map_size";
const std::string PROP_hashdb_foreground_threads_num = "hashdb.hashdb_foreground_threads_num";
const std::string PROP_hashdb_background_threads_num = "hashdb.hashdb_background_threads_num";

const std::string PROP_blob_approximate_size = "hashdb.blob_approximate_size";
const std::string PROP_blob_write_buffer_size = "hashdb.blob_write_buffer_size";
const std::string PROP_blob_gc_min_utility_threshold = "hashdb.blob_gc_min_utility_threshold";

const std::string PROP_gc_enable = "hashdb.gc_enable";
const std::string PROP_gc_check_every_some_writes = "hashdb.gc_check_every_some_writes";
const std::string PROP_gc_enable_data_files_gc = "hashdb.gc_enable_data_files_gc";
const std::string PROP_gc_trigger_min_blob_num = "hashdb.gc_trigger_min_blob_num";
const std::string PROP_gc_enable_cache_evict = "hashdb.gc_enable_cache_evict";
const std::string PROP_gc_cache_max_threshold = "hashdb.gc_cache_max_threshold";
const std::string PROP_gc_max_evict_slot_num_per_round = "hashdb.gc_max_evict_slot_num_per_round";
const std::string PROP_gc_enable_index_colddown = "hashdb.gc_enable_index_colddown";
const std::string PROP_gc_max_colddown_index_slot_num_per_round = "hashdb.gc_max_colddown_index_slot_num_per_round";

const std::string PROP_bloom_filters_num = "hashdb.bloom_filters_num";
const std::string PROP_bloom_filters_false_positive_rate = "hashdb.bloom_filters_false_positive_rate";
const std::string PROP_bloom_filters_elements_num = "hashdb.bloom_filters_elements_num";

const std::string PROP_main_key_range = "hashdb.main_key_range";
const std::string PROP_main_write_key_times = "hashdb.main_write_key_times";
const std::string PROP_main_record_size = "hashdb.main_record_size";
const std::string PROP_main_key_size = "hashdb.main_key_size";
const std::string PROP_main_threads_num = "hashdb.main_threads_num";

} // namespace

namespace ycsbc {

hashdb::HashDB *HashdbDB::db_ = nullptr;
int HashdbDB::ref_cnt_ = 0;
std::mutex HashdbDB::mu_;

void HashdbDB::Init() {
    const std::lock_guard<std::mutex> lock(mu_);

    const utils::Properties &props = *props_;
    const std::string &format =
        props.GetProperty(PROP_FORMAT, PROP_FORMAT_DEFAULT);
    if (format == "single") {
        format_ = kSingleEntry;
        method_read_ = &HashdbDB::ReadSingleEntry;
        method_scan_ = &HashdbDB::ScanSingleEntry;
        method_update_ = &HashdbDB::UpdateSingleEntry;
        method_insert_ = &HashdbDB::InsertSingleEntry;
        method_delete_ = &HashdbDB::DeleteSingleEntry;
    } else if (format == "row") {
        format_ = kRowMajor;
        method_read_ = &HashdbDB::ReadCompKeyRM;
        method_scan_ = &HashdbDB::ScanCompKeyRM;
        method_update_ = &HashdbDB::InsertCompKey;
        method_insert_ = &HashdbDB::InsertCompKey;
        method_delete_ = &HashdbDB::DeleteCompKey;
    } else if (format == "column") {
        format_ = kColumnMajor;
        method_read_ = &HashdbDB::ReadCompKeyCM;
        method_scan_ = &HashdbDB::ScanCompKeyCM;
        method_update_ = &HashdbDB::InsertCompKey;
        method_insert_ = &HashdbDB::InsertCompKey;
        method_delete_ = &HashdbDB::DeleteCompKey;
    } else {
        throw utils::Exception("unknown format");
    }
    fieldcount_ = std::stoi(props.GetProperty(
        CoreWorkload::FIELD_COUNT_PROPERTY, CoreWorkload::FIELD_COUNT_DEFAULT));
    field_prefix_ = props.GetProperty(CoreWorkload::FIELD_NAME_PREFIX,
                                      CoreWorkload::FIELD_NAME_PREFIX_DEFAULT);

    ref_cnt_++;
    if (db_) {
        return;
    }

    const std::string &db_path =
        props.GetProperty(PROP_NAME, PROP_NAME_DEFAULT);
    if (db_path == "") {
        throw utils::Exception("HashDB db path is missing");
    }

    SetGflags(props);

    if (props.GetProperty(PROP_DESTROY, PROP_DESTROY_DEFAULT) == "true") {
        hashdb::DestoryHashDB(db_path);
    }

    hashdb::OpenHashDB(&db_);
}

void HashdbDB::SetGflags(const utils::Properties& props) {
    std::string temp;
    
    temp = props.GetProperty(PROP_hashdb_files_directory, "");
    if (!temp.empty()) {
        hashdb::FLAGS_hashdb_files_directory = temp;
    }

    temp = props.GetProperty(PROP_hashdb_data_files_directory, "");
    if (!temp.empty()) {
        hashdb::FLAGS_hashdb_data_files_directory = temp;
    }

    temp = props.GetProperty(PROP_hashdb_index_files_directory, "");
    if (!temp.empty()) {
        hashdb::FLAGS_hashdb_index_files_directory = temp;
    }

    temp = props.GetProperty(PROP_hashdb_slots_map_size, "");
    if (!temp.empty()) {
        hashdb::FLAGS_hashdb_slots_map_size = std::stoull(temp);
    }

    temp = props.GetProperty(PROP_hashdb_foreground_threads_num, "");
    if (!temp.empty()) {
        hashdb::FLAGS_hashdb_foreground_threads_num = std::stoull(temp);
    }

    temp = props.GetProperty(PROP_hashdb_background_threads_num, "");
    if (!temp.empty()) {
        hashdb::FLAGS_hashdb_background_threads_num = std::stoull(temp);
    }

    temp = props.GetProperty(PROP_blob_approximate_size, "");
    if (!temp.empty()) {
        hashdb::FLAGS_blob_approximate_size = std::stoull(temp);
    }

    temp = props.GetProperty(PROP_blob_write_buffer_size, "");
    if (!temp.empty()) {
        hashdb::FLAGS_blob_write_buffer_size = std::stoull(temp);
    }

    temp = props.GetProperty(PROP_blob_gc_min_utility_threshold, "");
    if (!temp.empty()) {
        hashdb::FLAGS_blob_gc_min_utility_threshold = std::stod(temp);
    }

    temp = props.GetProperty(PROP_gc_enable, "");
    if (!temp.empty()) {
        if (temp == "true") {
            hashdb::FLAGS_gc_enable = true;
        } else if (temp == "false") {
            hashdb::FLAGS_gc_enable = false;
        }
    }

    temp = props.GetProperty(PROP_gc_check_every_some_writes, "");
    if (!temp.empty()) {
        hashdb::FLAGS_gc_check_every_some_writes = std::stoull(temp);
    }

    temp = props.GetProperty(PROP_gc_enable_data_files_gc, "");
    if (!temp.empty()) {
        if (temp == "true") {
            hashdb::FLAGS_gc_enable_data_files_gc = true;
        } else if (temp == "false") {
            hashdb::FLAGS_gc_enable_data_files_gc = false;
        }
    }

    temp = props.GetProperty(PROP_gc_trigger_min_blob_num, "");
    if (!temp.empty()) {
        hashdb::FLAGS_gc_trigger_min_blob_num = std::stoull(temp);
    }

    temp = props.GetProperty(PROP_gc_enable_cache_evict, "");
    if (!temp.empty()) {
        if (temp == "true") {
            hashdb::FLAGS_gc_enable_cache_evict = true;
        } else if (temp == "false") {
            hashdb::FLAGS_gc_enable_cache_evict = false;
        }
    }

    temp = props.GetProperty(PROP_gc_cache_max_threshold, "");
    if (!temp.empty()) {
        hashdb::FLAGS_gc_cache_max_threshold = std::stoull(temp);
    }

    temp = props.GetProperty(PROP_gc_max_evict_slot_num_per_round, "");
    if (!temp.empty()) {
        hashdb::FLAGS_gc_max_evict_slot_num_per_round = std::stoull(temp);
    }

    temp = props.GetProperty(PROP_gc_enable_index_colddown, "");
    if (!temp.empty()) {
        if (temp == "true") {
            hashdb::FLAGS_gc_enable_index_colddown = true;
        } else if (temp == "false") {
            hashdb::FLAGS_gc_enable_index_colddown = false;
        }
    }

    temp = props.GetProperty(PROP_gc_max_colddown_index_slot_num_per_round, "");
    if (!temp.empty()) {
        hashdb::FLAGS_gc_max_colddown_index_slot_num_per_round = std::stoull(temp);
    }

    temp = props.GetProperty(PROP_bloom_filters_num, "");
    if (!temp.empty()) {
        hashdb::FLAGS_bloom_filters_num = std::stoull(temp);
    }

    temp = props.GetProperty(PROP_bloom_filters_false_positive_rate, "");
    if (!temp.empty()) {
        hashdb::FLAGS_bloom_filters_false_positive_rate = std::stod(temp);
    }

    temp = props.GetProperty(PROP_bloom_filters_elements_num, "");
    if (!temp.empty()) {
        hashdb::FLAGS_bloom_filters_elements_num = std::stoull(temp);
    }
}

void HashdbDB::Cleanup() {
    const std::lock_guard<std::mutex> lock(mu_);
    if (--ref_cnt_) {
        return;
    }
    delete db_;
}

void HashdbDB::SerializeRow(const std::vector<Field> &values,
                            std::string *data) {
    for (const Field &field : values) {
        uint32_t len = field.name.size();
        data->append(reinterpret_cast<char *>(&len), sizeof(uint32_t));
        data->append(field.name.data(), field.name.size());
        len = field.value.size();
        data->append(reinterpret_cast<char *>(&len), sizeof(uint32_t));
        data->append(field.value.data(), field.value.size());
    }
}

void HashdbDB::DeserializeRowFilter(std::vector<Field> *values,
                                    const std::string &data,
                                    const std::vector<std::string> &fields) {
    const char *p = data.data();
    const char *lim = p + data.size();

    std::vector<std::string>::const_iterator filter_iter = fields.begin();
    while (p != lim && filter_iter != fields.end()) {
        assert(p < lim);
        uint32_t len = *reinterpret_cast<const uint32_t *>(p);
        p += sizeof(uint32_t);
        std::string field(p, static_cast<const size_t>(len));
        p += len;
        len = *reinterpret_cast<const uint32_t *>(p);
        p += sizeof(uint32_t);
        std::string value(p, static_cast<const size_t>(len));
        p += len;
        if (*filter_iter == field) {
            values->push_back({field, value});
            filter_iter++;
        }
    }
    assert(values->size() == fields.size());
}

void HashdbDB::DeserializeRow(std::vector<Field> *values,
                              const std::string &data) {
    const char *p = data.data();
    const char *lim = p + data.size();
    while (p != lim) {
        assert(p < lim);
        uint32_t len = *reinterpret_cast<const uint32_t *>(p);
        p += sizeof(uint32_t);
        std::string field(p, static_cast<const size_t>(len));
        p += len;
        len = *reinterpret_cast<const uint32_t *>(p);
        p += sizeof(uint32_t);
        std::string value(p, static_cast<const size_t>(len));
        p += len;
        values->push_back({field, value});
    }
    assert(values->size() == fieldcount_);
}

std::string HashdbDB::BuildCompKey(const std::string &key,
                                   const std::string &field_name) {
    switch (format_) {
    case kRowMajor:
        return key + ":" + field_name;
        break;
    case kColumnMajor:
        return field_name + ":" + key;
        break;
    default:
        throw utils::Exception("wrong format");
    }
}

std::string HashdbDB::KeyFromCompKey(const std::string &comp_key) {
    size_t idx = comp_key.find(":");
    assert(idx != std::string::npos);
    return comp_key.substr(0, idx);
}

std::string HashdbDB::FieldFromCompKey(const std::string &comp_key) {
    size_t idx = comp_key.find(":");
    assert(idx != std::string::npos);
    return comp_key.substr(idx + 1);
}

DB::Status HashdbDB::ReadSingleEntry(const std::string &table,
                                     const std::string &key,
                                     const std::vector<std::string> *fields,
                                     std::vector<Field> &result) {
    std::string data;

    int res = db_->Get(key, &data);
    if (res == -1) {
        return kNotFound;
    }

    if (fields != nullptr) {
        DeserializeRowFilter(&result, data, *fields);
    } else {
        DeserializeRow(&result, data);
    }

    return kOK;
}

DB::Status HashdbDB::ScanSingleEntry(const std::string &table,
                                     const std::string &key, int len,
                                     const std::vector<std::string> *fields,
                                     std::vector<std::vector<Field>> &result) {
    return kNotImplemented;
}

DB::Status HashdbDB::UpdateSingleEntry(const std::string &table,
                                       const std::string &key,
                                       std::vector<Field> &values) {
    std::string data;
    int res = db_->Get(key, &data);
    if (res == -1) {
        return kNotFound;
    }

    std::vector<Field> current_values;
    DeserializeRow(&current_values, data);
    for (Field &new_field : values) {
        bool found MAYBE_UNUSED = false;
        for (Field &cur_field : current_values) {
            if (cur_field.name == new_field.name) {
                found = true;
                cur_field.value = new_field.value;
                break;
            }
        }
        assert(found);
    }

    data.clear();
    SerializeRow(current_values, &data);
    db_->Set(key, data);

    return kOK;
}

DB::Status HashdbDB::InsertSingleEntry(const std::string &table,
                                       const std::string &key,
                                       std::vector<Field> &values) {
    std::string data;
    SerializeRow(values, &data);
    db_->Set(key, std::move(data));
    return kOK;
}

DB::Status HashdbDB::DeleteSingleEntry(const std::string &table,
                                       const std::string &key) {
    db_->Delete(key);
    return kOK;
}

DB::Status HashdbDB::ReadCompKeyRM(const std::string &table,
                                   const std::string &key,
                                   const std::vector<std::string> *fields,
                                   std::vector<Field> &result) {
    // todo: hashdb不会将同一个key的多个field连续存储，可能有性能劣势
    if (fields != nullptr) {
        std::vector<std::string>::const_iterator filter_iter = fields->begin();
        for (int i = 0; i < fieldcount_ && filter_iter != fields->end(); i++) {
            std::string comp_key = key + ":" + *filter_iter;
            std::string value;
            int res = db_->Get(comp_key, &value);
            if (res == -1) {
                return kNotFound;
            }
            result.push_back({std::move(comp_key), std::move(value)});
            ++filter_iter;
        }
    } else {
        for (int i = 0; i < fieldcount_; i++) {
            std::string comp_key =
                key + ":" + field_prefix_ + std::to_string(i);
            std::string value;
            int res = db_->Get(comp_key, &value);
            if (res == -1) {
                return kNotFound;
            }
            result.push_back({std::move(comp_key), std::move(value)});
        }
    }

    return kOK;
}

DB::Status HashdbDB::ScanCompKeyRM(const std::string &table,
                                   const std::string &key, int len,
                                   const std::vector<std::string> *fields,
                                   std::vector<std::vector<Field>> &result) {
    return kNotImplemented;
}

DB::Status HashdbDB::ReadCompKeyCM(const std::string &table,
                                   const std::string &key,
                                   const std::vector<std::string> *fields,
                                   std::vector<Field> &result) {
    if (fields != nullptr) {
        std::vector<std::string>::const_iterator filter_iter = fields->begin();
        for (int i = 0; i < fieldcount_ && filter_iter != fields->end(); i++) {
            std::string comp_key = *filter_iter + ":" + key;
            std::string value;
            int res = db_->Get(comp_key, &value);
            if (res == -1) {
                return kNotFound;
            }
            result.push_back({std::move(comp_key), std::move(value)});
            ++filter_iter;
        }
    } else {
        for (int i = 0; i < fieldcount_; i++) {
            std::string comp_key =
                field_prefix_ + std::to_string(i) + ":" + key;
            std::string value;
            int res = db_->Get(comp_key, &value);
            if (res == -1) {
                return kNotFound;
            }
            result.push_back({std::move(comp_key), std::move(value)});
        }
    }

    return kOK;
}

DB::Status HashdbDB::ScanCompKeyCM(const std::string &table,
                                   const std::string &key, int len,
                                   const std::vector<std::string> *fields,
                                   std::vector<std::vector<Field>> &result) {
    return kNotImplemented;
}

DB::Status HashdbDB::InsertCompKey(const std::string &table,
                                   const std::string &key,
                                   std::vector<Field> &values) {
    std::string comp_key;
    for (Field &field : values) {
        comp_key = BuildCompKey(key, field.name);
        db_->Set(comp_key, field.value);
    }
    return kOK;
}

DB::Status HashdbDB::DeleteCompKey(const std::string &table,
                                   const std::string &key) {

    std::string comp_key;
    for (int i = 0; i < fieldcount_; i++) {
        comp_key = BuildCompKey(key, field_prefix_ + std::to_string(i));
        db_->Delete(comp_key);
    }
    return kOK;
}

DB *NewHashdbDB() { return new HashdbDB; }

const bool registered = DBFactory::RegisterDB("hashdb", NewHashdbDB);

} // namespace ycsbc