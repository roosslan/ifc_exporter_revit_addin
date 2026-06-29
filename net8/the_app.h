#pragma once

#include "stdafx.h"
#include "sensitive_data.h"
#include "ini_file.h"

namespace ifc_exporter {
    [Transaction(TransactionMode::Manual)]
    public ref class ILog {
    public:
        void info(string text);        
        void error(string text);
    };

    public ref class ext_app : IExternalApplication {
        static string assembly_location_;
        static IExternalApplication^ external_application_;        
        ExternalEvent^ external_export_event_;
        static ILog^ logger_;
        static UIControlledApplication^ uic_application_;
        UIApplication^ ext_ui_application_;
        void try_connect_to_exports_pipe_server(UIApplication^ uiapp);        
        Assembly^ delegate_assembly_resolve(object sender, ResolveEventArgs^ e);
        void delegate_component_manager_ui_element_activated(object sender, UIElementActivatedEventArgs^ e);
        void delegate_on_application_initialized(object sender, ApplicationInitializedEventArgs^ e);
        string set_up_log_config();
        void create_ribbon_buttons();
        void pipe_connect(object pipe_parameter);
        void pipe_handler(object pipe_parameter);
        void on_export_button_click();        
    public:
        ext_app();
        ini_simple^ ini_file = nullptr;
        string appdata_directory = Environment::GetFolderPath(Environment::SpecialFolder::ApplicationData);
        string vendor_directory = appdata_directory + gcnew String(wapp_directory);
        ui::Result DllMain(UIControlledApplication^ hinst_dll);
        virtual ui::Result OnStartup(UIControlledApplication^ uiapp);
        virtual ui::Result OnShutdown(UIControlledApplication^ uiapp);
    };
}
