#include <iostream>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

json decode_encoded_value(const std::string& encoded_value) {
    if (std::isdigit(encoded_value[0])) {
        // Basically, The Binary Encoding(bencode) of string looks like "<length>:<string>" ("5:pavan", ie it starts with a number).
        // so, we decode it as "5:pavan" -> "pavan"
        size_t colon_index = encoded_value.find(':');
        if (colon_index != std::string::npos) {
            std::string number_string = encoded_value.substr(0, colon_index);
            int64_t number = std::stoll(number_string);
            return json(encoded_value.substr(colon_index + 1, number));
            // RVO - Return Value Optimization: instead of creating a variable to cast
            // to another variable, we can directly pass it to constructor which creates temperory variable.
        } else {
            throw std::runtime_error("Invalid encoded value: " + encoded_value);
        }
    } else if (encoded_value[0] == 'i' && encoded_value.back() == 'e') {
        // Similarlly, the integer value is serialized as "i<number>e" (ie. "i-52e" -Deserialization-> "-52")
        return json(std::stoll(encoded_value.substr(1, encoded_value.size() - 2)));
    } else {
        throw std::runtime_error("Unhandled encoded value: " + encoded_value);
    }
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
        json decoded_value = decode_encoded_value(encoded_value);
        std::cout << decoded_value.dump() << std::endl;
    } else {
        std::cerr << "Unknown Command: " << command << std::endl;
        return 1;
    }

    return 0;
}