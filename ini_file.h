#pragma once

#include "stdafx.h"

namespace ifc_exporter {
    /* Разбор INI-файла в UTF-8 (с BOM или без). Заменяет GetPrivateProfileString, который UTF-8 не поддерживает. public — для модульных тестов */
    public ref class ini_simple sealed {
        string ext_path_;
        List<string>^ read_lines();
        void write_lines(List<string>^ lines);
        static bool try_get_section_name(string line, string% name);
        static bool try_split_pair(string line, string% key, string% value);
        static bool find_section(List<string>^ lines, string section, int% start, int% end);
    public:
        ini_simple(string ini_path);
        array<string>^ read_section(string section);
        string read_string(string section, string key);
        void write_string(string section, string key, string value);
        void delete_key(string section, string key);
        void delete_section(string section);
        bool key_exists(string section, string key);
    };
}
