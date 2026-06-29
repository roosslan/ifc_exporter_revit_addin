#pragma once

#include "stdafx.h"

[DllImport("kernel32")]
extern int GetPrivateProfileSectionW(string section, IntPtr ret_val, int size, string file_path);

[DllImport("kernel32")]
extern int GetPrivateProfileString(string section, string key, string default_val, StringBuilder^ ret_val, int size, string file_path);

[DllImport("kernel32")]
extern long WritePrivateProfileString(string section, string key, string value, string file_path);


namespace ifc_exporter {
    ref class ini_simple sealed {
        string ext_path_;        
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