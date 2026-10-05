#include "ini_file.h"

namespace ifc_exporter {
    ini_simple::ini_simple(const string ini_path) {
        ext_path_ = (gcnew FileInfo(ini_path))->FullName;
    }

    List<string>^ ini_simple::read_lines() {
        auto lines = gcnew List<string>();
        if (!File::Exists(ext_path_))
            return lines;
        /* StreamReader сам распознаёт BOM (UTF-8/UTF-16); без BOM читает как UTF-8 */
        StreamReader reader(ext_path_, Encoding::UTF8, true);
        string line;
        while ((line = reader.ReadLine()) != nullptr)
            lines->Add(line);
        return lines;
    }

    void ini_simple::write_lines(List<string>^ lines) {
        /* Через временный файл, чтобы при сбое не остался обрезанный INI */
        const string tmp_path = ext_path_ + ".tmp";
        File::WriteAllLines(tmp_path, lines, gcnew UTF8Encoding(false));
        File::Move(tmp_path, ext_path_, true);
    }

    bool ini_simple::try_get_section_name(const string line, string% name) {
        const string t = line->Trim();
        if (t->Length < 2 || t[0] != '[')
            return false;
        const int end = t->IndexOf(']');
        if (end < 0)
            return false;
        name = t->Substring(1, end - 1)->Trim();
        return true;
    }

    bool ini_simple::try_split_pair(const string line, string% key, string% value) {
        const string t = line->Trim();
        if (t->Length == 0 || t[0] == ';' || t[0] == '#')
            return false;
        const int eq = t->IndexOf('=');
        if (eq <= 0)
            return false;
        key = t->Substring(0, eq)->Trim();
        value = t->Substring(eq + 1)->Trim();
        /* Как и GetPrivateProfileString, снимаем обрамляющие кавычки */
        if (value->Length >= 2 && value[0] == '"' && value[value->Length - 1] == '"')
            value = value->Substring(1, value->Length - 2);
        return true;
    }

    /* start — индекс заголовка секции, end — индекс первой строки следующей секции (или Count) */
    bool ini_simple::find_section(List<string>^ lines, const string section, int% start, int% end) {
        start = -1;
        end = lines->Count;
        for (int i = 0; i < lines->Count; ++i) {
            string name;
            if (!try_get_section_name(lines[i], name))
                continue;
            if (start >= 0) {
                end = i;
                break;
            }
            if (name->Equals(section, StringComparison::OrdinalIgnoreCase))
                start = i;
        }
        return start >= 0;
    }

    array<string>^ ini_simple::read_section(const string section) {
        auto lines = read_lines();
        int start, end;
        if (!find_section(lines, section, start, end))
            return nullptr;
        auto result = gcnew List<string>();
        for (int i = start + 1; i < end; ++i) {
            string key, value;
            if (try_split_pair(lines[i], key, value))
                result->Add(key + "=" + value);
        }
        return result->ToArray();
    }

    string ini_simple::read_string(const string section, const string key) {
        auto lines = read_lines();
        int start, end;
        if (find_section(lines, section, start, end)) {
            for (int i = start + 1; i < end; ++i) {
                string k, v;
                if (try_split_pair(lines[i], k, v) && k->Equals(key, StringComparison::OrdinalIgnoreCase))
                    return v;
            }
        }
        return String::Empty;
    }

    void ini_simple::write_string(const string section, const string key, const string value) {
        auto lines = read_lines();
        const string new_line = key + "=" + value;
        int start, end;
        if (!find_section(lines, section, start, end)) {
            lines->Add("[" + section + "]");
            lines->Add(new_line);
            write_lines(lines);
            return;
        }
        int insert_at = start + 1;
        for (int i = start + 1; i < end; ++i) {
            if (lines[i]->Trim()->Length == 0)
                continue;
            insert_at = i + 1;
            string k, v;
            if (try_split_pair(lines[i], k, v) && k->Equals(key, StringComparison::OrdinalIgnoreCase)) {
                lines[i] = new_line;
                write_lines(lines);
                return;
            }
        }
        lines->Insert(insert_at, new_line);
        write_lines(lines);
    }

    void ini_simple::delete_key(const string section, const string key) {
        auto lines = read_lines();
        int start, end;
        if (!find_section(lines, section, start, end))
            return;
        for (int i = end - 1; i > start; --i) {
            string k, v;
            if (try_split_pair(lines[i], k, v) && k->Equals(key, StringComparison::OrdinalIgnoreCase))
                lines->RemoveAt(i);
        }
        write_lines(lines);
    }

    void ini_simple::delete_section(const string section) {
        auto lines = read_lines();
        int start, end;
        if (!find_section(lines, section, start, end))
            return;
        lines->RemoveRange(start, end - start);
        write_lines(lines);
    }

    bool ini_simple::key_exists(const string section, const string key) {
        return read_string(section, key)->Length > 0;
    }
}
