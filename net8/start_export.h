#pragma once

#include "stdafx.h"
#include "structures.h"
#include "ini_file.h"
#include "sensitive_data.h"

namespace ifc_exporter {
    bool is_reserved_name(string filename);
    string sanitize_filename(string filename);

    ref class export_to_file_format {
    public:
        export_to_file_format(bool ifc_on, bool nwc_on);

        DateTime^ rec_time;
        string rec_revit_version;
        List<views_n_sites^>^ list_views_n_sites;

        bool is_ifc_on;
        bool is_nwc_on;
        string rec_export_path;
    };

    ref class CExport {
        static List <IFCVersion>^ ifc_versions_ = gcnew List<IFCVersion>(System::Linq::Enumerable::ToList(Enumerable::Repeat(IFCVersion::Default, 30)));
        static StreamWriter^ pipe_toBg_n_qLog;        
        static Autodesk::Revit::ApplicationServices::Application^ m_rvt_app_;
        static Document^ m_rvt_doc_;
        static ini_simple^ m_ini_file_;
        static bool m_is_service_on_;
        static ObservableCollection<views_n_sites^>^ m_views_n_sites_;
        static DateTime^ m_selected_time_;
        static bool m_is_ifc_on_;
        static bool m_is_nwc_on_;
        static string m_revit_version_;
        static string m_export_path_;        
        static void Export(export_to_file_format^ record_entity);
        static void deserialize_json_into_configuration(string pre_setup_file_path, IFCExportConfiguration^ from_json_ifc_export_configuration, IFCExportOptions^ ifc_export_options);
        static void get_warn_dialog(object sender, DialogBoxShowingEventArgs^ e);
        static void get_failure_dialog(object sender, FailuresProcessingEventArgs^ e);
        static bool open_file(string file_path);
        static export_to_file_format^ load_exports_config();
        static nwc_export_options create_options_nwc();
        static IFCExportConfiguration^ create_config_ifc();
        static view3d_name^ get_export_view_id(string export_this_view_3d);        
        static void export_to_ifc(string path_to_export, ElementId^ view_3d, string file_name, IFCExportOptions^ ifc_export_options, bool do_export);
        static void export_to_nwc(string path_to_export, string file_name, nwc_export_options navisworks_export_options, bool do_export);
        static export_to_file_format^ serialize_inf(List<views_n_sites^>^ files_to_processing);
        static string get_location_to_export(string site_name);
        static const bool str_to_bool(string bool_as_str);
        /* 22.10.2025 static List <ElementId^>^ GetExportViewIds(string Views3d);  */
    public:
        CExport(UIApplication^ ui_application);
        static string appdata_directory = Environment::GetFolderPath(Environment::SpecialFolder::ApplicationData);
        static string vendor_directory = appdata_directory + gcnew String(wapp_directory);
        static string views_sites_file_path = vendor_directory + "\\views_sites.sav";
    };
}