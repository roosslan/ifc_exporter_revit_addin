
#include "start_export.hpp"

namespace ifc_exporter {

    CExport::CExport(UIApplication^ ui_application) {
        ui_application->Application->FailuresProcessing += gcnew EventHandler<FailuresProcessingEventArgs^>(&CExport::get_failure_dialog);
        ui_application->DialogBoxShowing += gcnew EventHandler<DialogBoxShowingEventArgs^>(&CExport::get_warn_dialog);

        m_rvt_app_ = ui_application->Application;

        m_ini_file_ = gcnew ini_simple(vendor_directory + "\\ifcexprt.inf");

        auto sav_fname = m_ini_file_->read_string("Manufacturer", "sav_file_for_export");
        views_sites_file_path = vendor_directory + "\\" + sav_fname;

        auto pipe_client = gcnew NamedPipeClientStream(".", "\\bghelperpipe", PipeDirection::InOut);
        pipe_client->Connect();
        pipe_toBg_n_qLog = gcnew StreamWriter(pipe_client);
        pipe_toBg_n_qLog->AutoFlush = true;

        ui_application->Application->FailuresProcessing -= gcnew EventHandler<FailuresProcessingEventArgs^>(&CExport::get_failure_dialog);
        ui_application->DialogBoxShowing -= gcnew EventHandler<DialogBoxShowingEventArgs^>(&CExport::get_warn_dialog);

/*      ifc_versions_[0]  = IFCVersion::Default;      /* Default = 0      */

        ifc_versions_[8]  = IFCVersion::IFCBCA;       /* IFCBCA = 8       */
        ifc_versions_[9]  = IFCVersion::IFC2x2;       /* IFC2x2 = 9       */
        ifc_versions_[10] = IFCVersion::IFC2x3;       /* IFC2x3 = 10      */
        ifc_versions_[17] = IFCVersion::IFCCOBIE;     /* IFCCOBIE = 17    */
        ifc_versions_[21] = IFCVersion::IFC2x3CV2;    /* IFC2x3CV2 = 21   */
        ifc_versions_[23] = IFCVersion::IFC4;         /* IFC4 = 23        */
        ifc_versions_[24] = IFCVersion::IFC2x3FM;     /* IFC2x3FM = 24    */
        ifc_versions_[25] = IFCVersion::IFC4RV;       /* IFC4RV = 25      */
        ifc_versions_[26] = IFCVersion::IFC4DTV;      /* IFC4DTV = 26     */
        ifc_versions_[27] = IFCVersion::IFC2x3BFM;    /* IFC2x3BFM = 27   */
/*      ifc_versions_[29] = IFCVersion::IFC4x3;       /* IFC4x3 = 29      */

        auto export_to_entity = load_exports_config();
        Export(export_to_entity);
    }

    string CExport::get_location_to_export(string site_name) {
        ProjectLocationSet^ locations = m_rvt_doc_->ProjectLocations;
        auto location_sites = gcnew cliext::vector<ProjectLocation^>;

        for each (ProjectLocation^ site in locations) {
            if (site_name != "") {
                if (site_name->Contains(site->Name)) {
                    location_sites->push_back(site);
                    pipe_toBg_n_qLog->Write("Площадка " + site->Name + " добавлена в список экспорта\n");
                }
            }
            else /* Площадки не указали вообще */
                location_sites->push_back(site);
        }
        return location_sites[0]->Name;
    }

    nwc_export_options CExport::create_options_nwc() {
        auto nwc_export_options = gcnew NavisworksExportOptions();
        nwc_export_options->ConvertElementProperties = true;
        nwc_export_options->Coordinates = NavisworksCoordinates::Shared;
        nwc_export_options->DivideFileIntoLevels = true;
        nwc_export_options->ExportElementIds = true;
        nwc_export_options->ExportLinks = false;
        nwc_export_options->ExportParts = true;
        nwc_export_options->ExportRoomGeometry = false;
        nwc_export_options->ExportScope = NavisworksExportScope::View;

        return nwc_export_options;
    }

    IFCExportConfiguration^ CExport::create_config_ifc() {
        auto ifc_export_configuration = IFCExportConfiguration::CreateDefaultConfiguration();
        ifc_export_configuration->VisibleElementsOfCurrentView = true;
        ifc_export_configuration->ExportInternalRevitPropertySets = true;
        ifc_export_configuration->ExportRoomsInView = true;
        ifc_export_configuration->ExportIFCCommonPropertySets = false;
        ifc_export_configuration->ExportPartsAsBuildingElements = true;
        ifc_export_configuration->TessellationLevelOfDetail = 1;

        return ifc_export_configuration;
    }

    export_to_file_format::export_to_file_format(const bool ifc_on, const bool nwc_on) {
        is_ifc_on = ifc_on;
        is_nwc_on = nwc_on;
    }

    void CExport::get_warn_dialog(object sender, DialogBoxShowingEventArgs^ e) {
        string dial = e->DialogId;
        if (dial == "TaskDialog_Unresolved_References") {
            e->OverrideResult(1);
        }
        /* Элементы были заняты - выгрузка отменилась */
        else if (dial == "Dialog_Revit_DocWarnDialog") {
            e->OverrideResult(1);
        }
        else if (dial == "Dialog_Revit_ExtendedErrorDialog") {
            e->OverrideResult(1);
        }
        else if (dial == "TaskDialog_Detach_Model_From_Central") {
            e->OverrideResult(1);
        }
        else if (dial == "TaskDialog_Macro_Security_Alert") {
            e->OverrideResult(1);
        }
        else {
            /* ifc_exporter->OnLogsUpdated("Выгрузка споткнулась об: ");
               ifc_exporter->OnLogsUpdated(dial);
             */
        }
    }

    export_to_file_format^ CExport::serialize_inf(List<views_n_sites^>^ files_to_processing) {

        string ansi_default_dest_dir = m_ini_file_->read_string("DestinationDirs", "DefaultDestDir");

        /* В отличие от других строк INF файла, в данном случае для GetPrivateProfileString может быть дана папка с русскими символами в имени */        
        /* После чтения из файла, ANSI в памяти сразу превращается в UTF16 (т.е. в Unicode) */
        cli::array<unsigned char>^ windows1252_bytes = Encoding::Default->GetBytes(ansi_default_dest_dir);
        /* У нас всё в UTF8, конвертируем */
        auto default_dest_dir = Encoding::UTF8->GetString(windows1252_bytes);

        DateTime^ export_time = Convert::ToDateTime(m_ini_file_->read_string("ControlFlags", "Time"));
        string revit_version = m_ini_file_->read_string("ControlFlags", "RevitVersion");

        const bool ifc_is_on = Convert::ToBoolean(m_ini_file_->read_string("RVT", "IFC"));
        const bool nwc_is_on = Convert::ToBoolean(m_ini_file_->read_string("RVT", "NWC"));

        auto ret_export_format = gcnew export_to_file_format(ifc_is_on, nwc_is_on);

        ret_export_format->rec_time = export_time;
        ret_export_format->rec_revit_version = revit_version;
        ret_export_format->list_views_n_sites = files_to_processing;
        ret_export_format->rec_export_path = default_dest_dir;

        return ret_export_format;
    }

    const bool CExport::str2bool(string bool_as_str) {
        bool bret;
        std::istringstream(msclr::interop::marshal_as<std::string>(bool_as_str)) >> std::boolalpha >> bret;
        return bret;
    }
    export_to_file_format^ CExport::load_exports_config() {
        string views_sites_file = File::ReadAllText(views_sites_file_path);

        auto v_files_to_process = gcnew List<views_n_sites^>;

        /* Читаем содержимое sav-файла */
        auto str_reader = gcnew StringReader(views_sites_file);
        string sline = nullptr;

        while ((sline = str_reader->ReadLine()) != nullptr)
            if (!sline->IsNullOrWhiteSpace((sline))) {
                array<string>^ line_restore_to = sline->Split('=');

                bool should_be_exported = str2bool(line_restore_to[5]->Trim());
                if (should_be_exported){                    
                    auto fn_3d_site_line = gcnew views_n_sites(line_restore_to[0]->Trim(), line_restore_to[1]->Trim(), line_restore_to[2]->Trim(), line_restore_to[3]->Trim(), line_restore_to[4]->Trim(), true);
                    v_files_to_process->Add(fn_3d_site_line);
                    fn_3d_site_line = {};
                }                
                line_restore_to->Clear;
            }

        string sect_name = "SourceDisksFiles";
        
        const string inf_file_path = vendor_directory + "\\ifcexprt.inf";
        const string inf_file = File::ReadAllText(inf_file_path);

         /* Захватываем содержимое секции [SourceDisksFiles]: */
         auto inf_files_section = gcnew Regex("\\[" + sect_name + "\\]((?:\\r?\\n\\s*[^\\]\\[\\s].*)+)", RegexOptions::Multiline);
         Match^ match_files = inf_files_section->Match(inf_file);

         /* rasa 18.9.25 List<string>^ v_files_to_process = gcnew List<string>;   */
         auto list_source_disks_files = gcnew List<string>;

        string sect_content = nullptr;
        if (match_files->Groups->Count > 1){
            sect_content = match_files->Groups[1]->ToString();
            auto reader = gcnew StringReader(sect_content);

            string line = nullptr;
            while ((line = reader->ReadLine()) != nullptr)
                if (!line->IsNullOrWhiteSpace((line))){

                    /* removing ini-key's sign "=" at the end:  */
                    array<string>^ line_fpath_n_bool = line->Split('=');
                    auto source_disks_file_path = line_fpath_n_bool[0]->Trim();
                    const bool file_should_be_exported = str2bool(line_fpath_n_bool[1]->Trim());

                    int file_already_in_list = 1;
                    /* Добавляем в список обыкновенные RVT, без вьюх и площадок */
                    for each(auto item in v_files_to_process)
                    {
                        if (item->rvt_file_path == source_disks_file_path)
                            file_already_in_list = 0;
                    }
                    if (file_already_in_list && file_should_be_exported)
                        list_source_disks_files->Add(source_disks_file_path);
                }
        }

        for each (auto val in list_source_disks_files) {
            auto wa_views_n_sites = gcnew views_n_sites(val, "", "", "", "", false);
            v_files_to_process->Add(wa_views_n_sites);
            wa_views_n_sites = {};
        }

        export_to_file_format^ records_to_export = serialize_inf(v_files_to_process);

        m_is_service_on_ = Convert::ToBoolean(m_ini_file_->read_string("ControlFlags", "Enabled"));

        m_views_n_sites_ = gcnew ObservableCollection<views_n_sites^>(records_to_export->list_views_n_sites);
        m_revit_version_ = records_to_export->rec_revit_version;
        m_selected_time_ = records_to_export->rec_time;

        m_export_path_ = records_to_export->rec_export_path;

        return records_to_export;
    }

    bool ifc_exporter::is_reserved_name(string filename) {
        /* Windows reserved filenames */
        array<string>^ reserved_names = {
            "CON", "PRN", "AUX", "NUL",
            "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
            "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
        };

        string name_without_ext = Path::GetFileNameWithoutExtension(filename);

        for each (string reserved in reserved_names) {
            if (name_without_ext->Equals(reserved, StringComparison::OrdinalIgnoreCase)) {
                return true;
            }
        }

        return false;
    }

    void CExport::get_failure_dialog(object sender, FailuresProcessingEventArgs^ e) {
        FailuresAccessor^ failure = e->GetFailuresAccessor();
        List<FailureMessageAccessor^>^ fm_accessors = System::Linq::Enumerable::ToList(failure->GetFailureMessages());

        auto del = gcnew List<ElementId^>();
        for each (FailureMessageAccessor ^ fm_accessor in fm_accessors) {
            try {
                for each (ElementId ^ f in fm_accessor->GetAdditionalElementIds()) {
                    if (!del->Contains(f)) {
                        del->Add(f);
                    }
                }

                failure->DeleteWarning(fm_accessor);

                if (del->Count > 0) {
                    failure->DeleteElements(del);
                }
            }
            catch (...) {
                continue;
            }
        }
    }

    string ifc_exporter::sanitize_filename(string filename) {
        if (String::IsNullOrEmpty(filename))
            return String::Empty;

        // Define invalid characters for Windows filesystems
        array<wchar_t>^ invalid_chars = Path::GetInvalidFileNameChars();

        // Create a StringBuilder for efficient string manipulation
        auto sanitized = gcnew StringBuilder();

        // Remove or replace invalid characters
        for each (wchar_t c in filename) {
            if (Array::IndexOf(invalid_chars, c) >= 0) {
                // Replace invalid characters with underscore
                sanitized->Append('_');
            }
            else {
                sanitized->Append(c);
            }
        }

        // Additional Windows-specific restrictions
        String^ result = sanitized->ToString();

        // Remove leading/trailing spaces and dots (Windows restriction)
        result = result->Trim()->Trim('.');

        // Check for reserved names (CON, PRN, AUX, NUL, COM1-9, LPT1-9)
        if (is_reserved_name(result)) {
            result = "_" + result;
        }

        // Ensure filename is not empty
        if (String::IsNullOrEmpty(result)) {
            result = "unnamed_file";
        }

        // Limit length to 255 characters (Windows max filename length)
        if (result->Length > 255) {
            result = result->Substring(0, 255);
        }

        return result;
    }

}