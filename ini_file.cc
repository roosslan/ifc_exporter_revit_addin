#include "ini_file.hpp"

namespace ifc_exporter{
        ini_simple::ini_simple(string ini_path) {
            ext_path_ = (gcnew FileInfo(ini_path))->FullName; 
        }

        array<string>^ ini_simple::read_section(string section) {
	        constexpr int max_buffer = 32767;
            array<string>^ ret_array;
            IntPtr pReturnedString = Marshal::AllocCoTaskMem(static_cast<int>(max_buffer) * sizeof(char));
            const int bytes_returned = GetPrivateProfileSectionW(section, pReturnedString, max_buffer, ext_path_);
            if (bytes_returned == max_buffer - 2 || bytes_returned == 0) {
                Marshal::FreeCoTaskMem(pReturnedString);
                return nullptr;
            }
            /* NB: Calling Marshal::PtrToStringAuto(pReturnedString) will  result in only the first pair being returned  */
            string returned_string = Marshal::PtrToStringAuto(pReturnedString, bytes_returned - 1);

            ret_array = returned_string->Split('\0');

            Marshal::FreeCoTaskMem(pReturnedString);
            return ret_array;
        }

        string ini_simple::read_string(string section, string key) {
            auto ret_val = gcnew StringBuilder(255);
            GetPrivateProfileString(section, key, "", ret_val, 255, ext_path_);
            return ret_val->ToString();
        }

        void ini_simple::write_string(string section, string key, string value) {
            WritePrivateProfileString(section, key, value, ext_path_);
        }

        void ini_simple::delete_key(string section, string key) {
            WritePrivateProfileString(section, key, nullptr, ext_path_);
        }

        void ini_simple::delete_section(string section) {
            WritePrivateProfileString(section, nullptr, nullptr, ext_path_);
        }

        bool ini_simple::key_exists(string section, string key) {
            return read_string(section, key)->Length > 0;
        }
}