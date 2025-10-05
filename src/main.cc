#include <fstream>
#include <iostream>

#include "converter.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
// enable utf8 on Windows console
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    char* input_aarc;
    char* output_rc;
    char* config_json;

    if ((argc != 5 && argc != 3 && argc != 1) || (argc == 5 && std::string(argv[3]) != "--config")) {
        std::cerr << "Usage: " << argv[0] << " <input.json> <output.json> [--config <config.json>]" << std::endl;
        return 1;
    }
    if (argc == 1) {
        std::cout << "Railchess AARC to RC Converter" << std::endl;
        std::string line;
        std::cout << "Enter input AARC file path: ";
        std::getline(std::cin, line);
        input_aarc = new char[line.size() + 1];
        std::strcpy(input_aarc, line.c_str());
        std::cout << "Enter output RC file path:  ";
        std::getline(std::cin, line);
        output_rc = new char[line.size() + 1];
        std::strcpy(output_rc, line.c_str());
        std::cout << "Enter config JSON file path (or leave empty for default): ";
        std::getline(std::cin, line);
        if (line.empty()) {
            config_json = new char[1];
            config_json[0] = '\0';
        } else {
            config_json = new char[line.size() + 1];
            std::strcpy(config_json, line.c_str());
        }
    } else {
        input_aarc = argv[1];
        output_rc = argv[2];
        config_json = (argc == 5) ? argv[4] : (char*)"";
    }

    try {
        nlohmann::json aarc_json;
        nlohmann::json config_json_data;

        std::ifstream aarc_file(input_aarc);
        if (!aarc_file.is_open()) {
            std::cerr << "Failed to open input file: " << input_aarc << std::endl;
            return 1;
        }
        aarc_file >> aarc_json;
        aarc_file.close();

        if (!std::string(config_json).empty()) {
            std::ifstream config_file(config_json);
            if (!config_file.is_open()) {
                std::cerr << "Failed to open config file: " << config_json << std::endl;
                return 1;
            }
            config_file >> config_json_data;
            config_file.close();
        }

        geometry::Map map(aarc_json, config_json_data);
        rc::Map rcmap = converter::convert_to_rc(map);
        nlohmann::json rc_json = rcmap.to_json();
        std::ofstream rc_file(output_rc);
        if (!rc_file.is_open()) {
            std::cerr << "Failed to open output file: " << output_rc << std::endl;
            return 1;
        }
        rc_file << rc_json.dump(2);
        rc_file.close();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}