#include "ini_file.hpp"

namespace ifc_exporter{
        ini_simple::ini_simple(const string ini_path) {
            ext_path_ = (gcnew FileInfo(ini_path))->FullName; 
        }

        array<string>^ ini_simple::read_section(const string section) {
	        constexpr int max_buffer = 32767;
	        IntPtr p_returned_string = Marshal::AllocCoTaskMem(static_cast<int>(max_buffer) * sizeof(char));
            const int bytes_returned = GetPrivateProfileSectionW(section, p_returned_string, max_buffer, ext_path_);
            if (bytes_returned == max_buffer - 2 || bytes_returned == 0) {
                Marshal::FreeCoTaskMem(p_returned_string);
                return nullptr;
            }
            /* NB: Calling Marshal::PtrToStringAuto(pReturnedString) will  result in only the first pair being returned  */
            const string returned_string = Marshal::PtrToStringAuto(p_returned_string, bytes_returned - 1);

            array<string>^ ret_array = returned_string->Split('\0');

            Marshal::FreeCoTaskMem(p_returned_string);
            return ret_array;
        }

        string ini_simple::read_string(const string section, const string key) {
            auto ret_val = gcnew StringBuilder(255);
            GetPrivateProfileString(section, key, "", ret_val, 255, ext_path_);
            return ret_val->ToString();
        }

        void ini_simple::write_string(const string section, const string key, const string value) {
            WritePrivateProfileString(section, key, value, ext_path_);
        }

        void ini_simple::delete_key(const string section, const string key) {
            WritePrivateProfileString(section, key, nullptr, ext_path_);
        }

        void ini_simple::delete_section(const string section) {
            WritePrivateProfileString(section, nullptr, nullptr, ext_path_);
        }

        bool ini_simple::key_exists(const string section, const string key) {
            return read_string(section, key)->Length > 0;
        }
}