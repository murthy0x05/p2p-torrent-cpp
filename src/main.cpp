#include <iostream>
#include <fstream>
#include <filesystem>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

json decode_bencoded_value(const std::string& encoded_value, size_t& index);

json decode_bencoded_integer(const std::string& encoded_value, size_t& index) {
    // In Binary Encoding(bencode), The integer value is serialized as "i<number>e"
    // (ie. "i-52e" -Deserialization-> "-52")
    index++; // To skip 'i'
    std::string result = "";
    while (encoded_value[index] != 'e') {
        result.push_back(encoded_value[index]);
        index++;
    }
    index++; // To skip 'e'

    return json(std::atoll(result.c_str()));
}

json decode_bencoded_string(const std::string& encoded_value, size_t& index) {
    // The Binary Encoding(bencode) of string looks like "<length>:<string>" ("5:pavan", ie it starts with a number).
    // so, we decode it as "5:pavan" -> "pavan"
    int length = 0;
    while (encoded_value[index] >= '0' && encoded_value[index] <= '9') {
        length = length * 10 + (encoded_value[index] - '0');
        index++;
    }
    index++; // to skip colon ':'
    std::string result("");
    while (length--) {
        result.push_back(encoded_value[index]);
        index++;
    }

    return json(result);
}

json decode_bencoded_list(const std::string& encoded_value, size_t& index) {
    // The Binary Encoding of list looks like:
    // l<item1><item2>...e
    // Note: here each item can be a string or integer.
    index++; // To skip 'l'
    json list = json::array(); // for lists
    while (encoded_value[index] != 'e') {
        list.push_back(decode_bencoded_value(encoded_value, index));
    }
    index++; // To skip 'e'
    
    return json(list);
}

json decode_bencoded_dict(const std::string& encoded_value, size_t& index) {
    // The Binary Encoding of Dictionary looks like:
    // d<key1><value1><key2><value2>...e
    // constraints: 
    // i. keys should be strings (ie. can't be integers, dictionaries)
    // ii. keys should be lexicographilly sorted (ensures everyone encodes same data same way).
    // reason behind sorting keys while encoding is:
    // In systems like BitTorrent, the actual bencoded value is hashed. if the keys are not sorted, the data
    // would be logically equivalent but not equal interms of hash.
    index++; // To skip 'd'
    json dict = json::object(); // for Dictionaries / maps
    while (encoded_value[index] != 'e') {
        json key = decode_bencoded_value(encoded_value, index);
        if (!key.is_string()) {
            throw std::runtime_error("Bencode dictionary key is not a string!");
        }
        json value = decode_bencoded_value(encoded_value, index);
        dict[key] = value;
    }
    index++; // To skip 'e'

    return json(dict);
}

json decode_bencoded_value(const std::string& encoded_value, size_t& index) {
    if (encoded_value[index] == 'i') {
        return decode_bencoded_integer(encoded_value, index);
    } else if (std::isdigit(encoded_value[index])) {
        return decode_bencoded_string(encoded_value, index);
    } else if (encoded_value[index] == 'l') {
        return decode_bencoded_list(encoded_value, index);
    } else if (encoded_value[index] == 'd') {
        return decode_bencoded_dict(encoded_value, index);
    } else {
        throw std::runtime_error("Unhandled encoded value: " + encoded_value);
    }
}

json decode_bencoded_value(const std::string& encoded_value) {
    size_t index = 0;
    json result = decode_bencoded_value(encoded_value, index);
    if (index != encoded_value.size()) {
        throw std::runtime_error("String not fully consumed.");
    }

    return json(result);
}

int main(int argc, char* argv[]) {
    // Enabling the Automatic Flush
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
        return 1;
    }

    std::string command = argv[1];
    if (command == "decode") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }

        std::string encoded_value = argv[2];
        json decoded_value = decode_bencoded_value(encoded_value);
        std::cout << decoded_value.dump() << std::endl;
    } else if (command == "info") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " info <file_name>" << std::endl;
        }

        std::string file_name = argv[2];
        std::ifstream file(file_name, std::ios::binary); // since, the encoding is Binary Encoding(Bencode).
        std::filesystem::path p{argv[2]};
        std::string info(std::filesystem::file_size(p), '?');
        // to allocate buffer of exact size needed to read the file.
        file.read(info.data(), std::filesystem::file_size(p));
        json torrent_info = decode_bencoded_value(info);
        
        std::cout << "Tracker URL: " << torrent_info["announce"].dump().substr(1, torrent_info["announce"].dump().size() - 2) << std::endl;
        std::cout << "Length: " << torrent_info["info"]["length"].dump() << std::endl;
    } else {
        std::cerr << "Unknown Command: " << command << std::endl;
        return 1;
    }

    return 0;
}