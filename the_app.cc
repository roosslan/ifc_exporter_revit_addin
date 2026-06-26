#include "the_app.hpp"
#include "ini_file.hpp"
#include "start_export.hpp"
#include "api_wrapper.hpp"

using namespace ifc_exporter;

/* Точка входа в плагин, запускается при старте Revit */
ui::Result ext_app::DllMain(UIControlledApplication^ hinst_dll) {
    /* TODO: Конструкторы */
    const string app_config_path = set_up_log_config();

    uic_application_ = hinst_dll;
    uic_application_->ControlledApplication->ApplicationInitialized += gcnew EventHandler<ApplicationInitializedEventArgs^>(this, &ext_app::delegate_on_application_initialized);
    AppDomain::CurrentDomain->AssemblyResolve += gcnew ResolveEventHandler(this, &ext_app::delegate_assembly_resolve); // EventHandler<ResolveEventArgs^>(this, &ext_app::delegate_assembly_resolve);
    create_ribbon_buttons();

    auto handler = gcnew api_wrapper();
    external_export_event_ = ExternalEvent::Create(handler);

    return ui::Result::Succeeded;
}

ext_app::ext_app() {
    file_version_info_ = FileVersionInfo::GetVersionInfo(ext_dll_->Location);
}
Assembly^ ext_app::delegate_assembly_resolve(object sender, ResolveEventArgs^ e) {
    auto file_path = e->Name;
    /* Засоряет лог, включать по необходимости
     * logger_->Info("Fail resolving assembly " + filePath);
     */
    if (System::IO::File::Exists(file_path))
        return Assembly::Load(file_path);
    else
        return nullptr;
}

void ext_app::delegate_on_application_initialized(object sender, Autodesk::Revit::DB::Events::ApplicationInitializedEventArgs^ e) {
    auto app = reinterpret_cast<Autodesk::Revit::ApplicationServices::Application^>(sender);
    ext_ui_application_ = gcnew Autodesk::Revit::UI::UIApplication(app);

    logger_->Info("OnApplicationInitialized: Connecting to named pipe");
    try_connect_to_exports_pipe_server(ext_ui_application_);
}

string ext_app::set_up_log_config() {
    Directory::CreateDirectory(vendor_directory);
    try {
        ini_file = gcnew ini_simple(vendor_directory + "\\ifcexprt.inf");
    }
    catch (const std::exception&) {    
        File::AppendAllText(vendor_directory + "\\ext_addin.dev.log", DateTime::Now.ToString("dd.MM.yyyy hh:mm tt") + "ifc_exporter: The inf-file not found!\n");
    }
    string config_file_path = ini_file->read_string("Strings", "Disk1");

    if (config_file_path == "")
        File::AppendAllText(vendor_directory + "\\ext_addin.dev.log", DateTime::Now.ToString("dd.MM.yyyy hh:mm tt") + "'[ifc_exporter] log4net_config= ' key not found!\n");

    config_file_path = vendor_directory + "\\" + config_file_path;

    auto app_config_xml = gcnew XmlDocument();
    app_config_xml->Load(config_file_path);
    XmlNode^ node = app_config_xml->SelectSingleNode("/log4net/appender/file");
    /* */
    node->Attributes["value"]->Value = vendor_directory + "\\ext_addin.dev.log";
    app_config_xml->Save(config_file_path);

    log4net::Config::XmlConfigurator::Configure(gcnew FileInfo(config_file_path));
    return config_file_path;
}

void ext_app::create_ribbon_buttons() {    
    ComponentManager::UIElementActivated += gcnew EventHandler<UIElementActivatedEventArgs^>(this, &ext_app::delegate_component_manager_ui_element_activated);

    RibbonControl^ ribbon = ComponentManager::Ribbon;

    const auto tab_name = gcnew String(wtab_name);
    const auto bim_panel = gcnew String(wpanel_name);

    RibbonTab^ bim_tab = ribbon->FindTab(tab_name);
    if (bim_tab == nullptr)
        uic_application_->CreateRibbonTab(tab_name);
    
    /* Autodesk::Windows::RibbonPanel^ aw_bim_panel = ribbon->FindPanel(bim_panel, true);
     * if (aw_bim_panel == nullptr)
     */
    try {
        uic_application_->CreateRibbonPanel(tab_name, bim_panel);
    }
    catch (Autodesk::Revit::Exceptions::ArgumentException^ e) {
        (void)e; // Explicitly mark 'e' as unused
        /* The panel with the same name already exists! */
    }

    for each (RibbonTab ^ tab in ribbon->Tabs) {
        if (tab->Id == tab_name)
            for each (Autodesk::Windows::RibbonPanel^ panel in tab->Panels)
                if (panel->Source->Id == "CustomCtrl_%" + tab_name + "%" + bim_panel) {
                    panel->Source->Items->Add(create_revits_button("buttonExport", "Выгрузка", "Экспорт в формат IFC / NWC",
                        "\\resources\\export_16px.png", "\\resources\\export_32px.png", "ID_EXPORT_BUTTON"));
                }
    }
}

/* virtual */
ui::Result ext_app::OnShutdown(ui::UIControlledApplication^ uiapp) {
    uiapp = nullptr;
    return (ui::Result::Succeeded);
}

void ext_app::pipe_handler(object pipe_parameter) {
    auto pipe_client = reinterpret_cast<NamedPipeClientStream^>(pipe_parameter);

    logger_->Info("ext_app::pipe_handler: Connecting... ");

    while (!pipe_client->IsConnected) {
        /* std::cout << "pipe_handler not connected!"; */
    }

    logger_->Info("Pipe handler connected.");

    if (pipe_client->CanWrite) {
        auto str_writer = gcnew StreamWriter(pipe_client);
        str_writer->AutoFlush = true;
        
        // При запуске проверяем, не выставила ли фоновая служба флаг приступать к экспорту:
        if (ini_file->read_string("ControlFlags", "Enabled") != "false") {
            if (file_version_info_){
	            ext_version_ = file_version_info_->FileVersion;
	            str_writer->Write("ext_addin version: " + ext_version_ + "\n");
            }

            /* Сигнал для bgHelper, что Revit успешно запустился и начался экспорт */
            str_writer->Write("Begin of export\n");
            ini_file->write_string("ControlFlags", "Enabled", "false");

            try {
                // Оборачиваем весь код по экспорту в контекст Revit:
                external_export_event_->Raise();
            }
            catch (const std::exception& e) {
                str_writer->Write(e.what());

            } // str_writer->Write(L"End of export\n");
        }
    }
    else
        logger_->Info("Pipe handler can not write!");
}

void ext_app::try_connect_to_exports_pipe_server(UIApplication^ uiapp) {
    try {
        auto pipe_client = gcnew NamedPipeClientStream(".", "\\bghelperpipe", PipeDirection::InOut);
        try {
            auto pipe_thread = gcnew Thread(gcnew ParameterizedThreadStart(this, &ext_app::pipe_connect));
            logger_->Info("pipeThread->Start");
            pipe_thread->Start(pipe_client);

            auto conn_handler_thread = gcnew Thread(gcnew ParameterizedThreadStart(this, &ext_app::pipe_handler));
            logger_->Info("connHandlerThread->Start");
            conn_handler_thread->Start(pipe_client);
        }
        catch (TimeoutException^ e) {
            logger_->Info("Received from server: " + e->ToString());
        }  
        
        /* _logger->Info("_logger->Info(\"End of export?\")"); */
    }
    catch (exception e) {
        this->logger_->Error(e->ToString());
    }
}

void ext_app::delegate_component_manager_ui_element_activated(object sender, UIElementActivatedEventArgs^ e) {
    if (e != nullptr && e->Item != nullptr && e->Item->Id != nullptr) {
        if (e->Item->Id == "ID_EXPORT_BUTTON") {
            on_export_button_click();
        }
    }
}

void ext_app::on_export_button_click() {
    const auto vendor_name = gcnew String(wvendor_name);
    string export_to_exe_path = vendor_directory + "\\" + vendor_name + "\\ifc_exporter\\ifc_exporter.exe";

    auto process = gcnew System::Diagnostics::Process();    

    /* Get the current process. */
    auto current_process = System::Diagnostics::Process::GetCurrentProcess();
    /* Передаем pID текущего процесса в QtQML ifc_exporter */
    process->StartInfo->Arguments = "\"" + current_process->Id + "\"";

    process->StartInfo->FileName = export_to_exe_path;
    process->StartInfo->WorkingDirectory = vendor_directory + "\\" + vendor_name + "\\ifc_exporter";
    process->StartInfo->UseShellExecute = true;

    try {
        bool res = process->Start();
    }
    catch (exception e) {
        logger_->Info("Exception " + e->ToString() + "\n");
    }
};

Autodesk::Windows::RibbonButton^ ext_app::create_revits_button(string btn_name, string btn_text,
		string btn_tool_tip,string img_path16, string img_path32, string id)
{
    ref::Assembly^ p_assembly;

    auto revitsButton = gcnew Autodesk::Windows::RibbonButton();
    revitsButton->Name = btn_name;

    p_assembly = ref::Assembly::GetExecutingAssembly();
    Path^ addin_dir_path;
    string pwd = addin_dir_path->GetDirectoryName(p_assembly->Location) + img_path32;
    auto uri_image = gcnew Uri(pwd);
    auto large_image = gcnew img::BitmapImage(uri_image);

    string small_upd_btn_path = addin_dir_path->GetDirectoryName(p_assembly->Location) + img_path16;
    auto uri_upd_btn_small_img = gcnew Uri(small_upd_btn_path);
	auto small_image = gcnew img::BitmapImage(uri_upd_btn_small_img);

    revitsButton->LargeImage = large_image;
    revitsButton->Image = small_image;
    revitsButton->Id = id;
    revitsButton->AllowInStatusBar = true;
    revitsButton->AllowInToolBar = true;
    revitsButton->GroupLocation = Autodesk::Private::Windows::RibbonItemGroupLocation::Middle;
    revitsButton->IsEnabled = true;
    revitsButton->IsToolTipEnabled = true;
    revitsButton->IsVisible = true;
    revitsButton->ShowImage = true;
    revitsButton->ShowText = true;
    revitsButton->ShowToolTipOnDisabled = true;
    revitsButton->Text = btn_text;
    revitsButton->ToolTip = btn_tool_tip;
    revitsButton->MinHeight = 0;
    revitsButton->MinWidth = 0;
    revitsButton->Size = RibbonItemSize::Large;
    revitsButton->ResizeStyle = RibbonItemResizeStyles::HideText;
    revitsButton->IsCheckable = true;
    revitsButton->Orientation = System::Windows::Controls::Orientation::Vertical;
    /*  revitsButton->KeyTip = "";
        revitsButton->Width = revitsButton->Height; */
    return revitsButton;
}

void ext_app::pipe_connect(object pipe_parameter) {
    logger_->Info("extApp::pipe_connect: Connecting... ");
    try {
        reinterpret_cast<NamedPipeClientStream^>(pipe_parameter)->Connect();
    }
    catch (exception ex) {
        logger_->Info("Connecting the pipe throw an exception: " + ex->ToString());
    }
}

/* virtual */
ui::Result ext_app::OnStartup(ui::UIControlledApplication^ uiapp) {
    DllMain(uiapp);
	return (ui::Result::Succeeded);
}