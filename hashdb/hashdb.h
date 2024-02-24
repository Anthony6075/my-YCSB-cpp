#pragma once

#include <string>

namespace hashdb {

class HashDB {
public:
    HashDB() {}
    virtual ~HashDB() {}

    virtual int Get(const std::string& key, std::string* value) = 0;
    virtual void Set(const std::string& key, const std::string& vlaue,
                        bool async = true) = 0;
    virtual void Delete(const std::string& key, bool async = true) = 0;
};

void OpenHashDB(HashDB** dbptr, const std::string& dbname =  "");
void DestoryHashDB(const std::string& dbname);

}