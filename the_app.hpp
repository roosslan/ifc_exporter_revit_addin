#pragma once

#include "stdafx.hpp"
#include "ini_file.hpp"
#include "sensitive_data.hpp"

namespace ifc_exporter {

    public ref class ext_app : IExternalApplication {
        ExternalEvent^ external_export_event_;
        static ILog^ logger_ = LogManager::GetLogger("ifc_exporter");
        static UIControlledApplication^ uic_application_;
        UIApplication^ ext_ui_application_;
        void try_connect_to_exports_pipe_server(UIApplication^ uiapp);
        Autodesk::Windows::RibbonButton^ create_revits_button(string btn_name, string btn_text, string btn_tool_tip, string img_path16, string img_path32, string id);
        Assembly^ delegate_assembly_resolve(object sender, ResolveEventArgs^ e);
        void delegate_component_manager_ui_element_activated(object sender, UIElementActivatedEventArgs^ e);
        void delegate_on_application_initialized(object sender, ApplicationInitializedEventArgs^ e);
        string set_up_log_config();
        void create_ribbon_buttons();
        void pipe_connect(object pipe_parameter);
        void pipe_handler(object pipe_parameter);
        void on_export_button_click();
        FileVersionInfo^ file_version_info_;
        Assembly^ ext_dll_ = Assembly::GetExecutingAssembly();
        string ext_version_ = nullptr;
    public:
        ext_app();
        ini_simple^ ini_file = nullptr;
        string appdata_directory = Environment::GetFolderPath(Environment::SpecialFolder::ApplicationData);
        string vendor_directory = appdata_directory + gcnew String(app_directory);
        ui::Result DllMain(UIControlledApplication^ hinst_dll);
        virtual ui::Result OnStartup(UIControlledApplication^ uiapp);
        virtual ui::Result OnShutdown(UIControlledApplication^ uiapp);
    };
}
