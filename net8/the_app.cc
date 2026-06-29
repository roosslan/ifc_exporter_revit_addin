#include "the_app.h"
#include "ini_file.h"
#include "start_export.h"
#include "api_wrapper.h"
#include "btn_availability.h"

using namespace ifc_exporter;

void ILog::info(const string text)
{
    File::AppendAllText(CExport::vendor_directory + "\\ext_addin.dev.log", DateTime::Now.ToString("dd.MM.yyyy hh:mm tt") + text);
}
void ILog::error(const string text) {}

ext_app::ext_app() {
    assembly_location_ = Assembly::GetExecutingAssembly()->Location;
};

/* Точка входа в плагин, запускается при старте Revit */
Result ext_app::DllMain(UIControlledApplication^ hinst_dll) {
    const string app_config_path = set_up_log_config();

    uic_application_ = hinst_dll;
    uic_application_->ControlledApplication->ApplicationInitialized += gcnew EventHandler<ApplicationInitializedEventArgs^>(this, &ext_app::delegate_on_application_initialized);
    AppDomain::CurrentDomain->AssemblyResolve += gcnew ResolveEventHandler(this, &ext_app::delegate_assembly_resolve);
    create_ribbon_buttons();

    auto handler = gcnew api_wrapper();
    external_export_event_ = ExternalEvent::Create(handler);

    return ui::Result::Succeeded;
}

Assembly^ ext_app::delegate_assembly_resolve(object sender, ResolveEventArgs^ e) {
    auto file_path = e->Name;
    logger_->info("Fail resolving assembly " + file_path);
    if (System::IO::File::Exists(file_path))
        return Assembly::Load(file_path);
    else
        return nullptr;
}

void ext_app::delegate_on_application_initialized(object sender, Autodesk::Revit::DB::Events::ApplicationInitializedEventArgs^ e) {
    auto app = reinterpret_cast<Autodesk::Revit::ApplicationServices::Application^>(sender);
    ext_ui_application_ = gcnew Autodesk::Revit::UI::UIApplication(app);

    logger_->info("OnApplicationInitialized: Connecting to named pipe");
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

/*  log4net::Config::XmlConfigurator::Configure(gcnew FileInfo(config_file_path)); */
    return config_file_path;
}

void ext_app::create_ribbon_buttons() {

    const auto tab_name = gcnew String(wtab_name);
    const auto bim_panel = gcnew String(wpanel_name);

    ComponentManager::UIElementActivated += gcnew EventHandler<UIElementActivatedEventArgs^>(this, &ext_app::delegate_component_manager_ui_element_activated);

    RibbonControl^ ribbon = ComponentManager::Ribbon;

    RibbonTab^ bim_tab = ribbon->FindTab(tab_name);
    if (bim_tab == nullptr)
        uic_application_->CreateRibbonTab(tab_name);

    auto btn_exporter_data = gcnew PushButtonData("ID_EXPORT_BUTTON", "Выгрузка", assembly_location_, "ifc_exporter.btn_click");
	btn_exporter_data->AvailabilityClassName = "ifc_exporter.btn_availability";
    
    Autodesk::Revit::UI::RibbonPanel^ ribbon_panel = nullptr;
    try {
        ribbon_panel = uic_application_->CreateRibbonPanel(tab_name, bim_panel);
    }
    catch (Autodesk::Revit::Exceptions::ArgumentException^ e) {
        (void)e; // Explicitly mark 'e' as unused
        /* The panel with the same name already exists! */
    }

    btn_exporter_data->LargeImage = gcnew BitmapImage(gcnew Uri(Path::GetDirectoryName(assembly_location_) + "\\resources\\export_32px.png"));
    btn_exporter_data->Image = gcnew BitmapImage(gcnew Uri(Path::GetDirectoryName(assembly_location_) + "\\resources\\export_16px.png"));
    btn_exporter_data->ToolTip = "Экспорт в формат IFC / NWC";

    auto push_button = dynamic_cast<PushButton^>(ribbon_panel->AddItem(btn_exporter_data));
}

Result ext_app::OnShutdown(UIControlledApplication^ uiapp) {
    uiapp = nullptr;
    return (ui::Result::Succeeded);
}

void ext_app::pipe_handler(object pipe_parameter) {
    auto pipe_client = reinterpret_cast<NamedPipeClientStream^>(pipe_parameter);

    auto ssw_options = gcnew StreamPipeWriterOptions(System::Buffers::MemoryPool<byte>::Shared, 4096, true);
    PipeWriter^ pipe_writer;
    pipe_writer->Create(pipe_client, ssw_options);

    logger_->info("ext_app::pipe_handler: Connecting... ");
    
    while (!pipe_client->IsConnected) {
        /* std::cout << "pipeHandler not Connected!"; */
    }

    logger_->info("Pipe handler connected.");

    if (pipe_client->CanWrite) {
        auto ss_writer = gcnew StreamWriter(pipe_client, Encoding::UTF8, 4096, true);
        ss_writer->AutoFlush = true;
        
        // При запуске проверяем, не выставила ли фоновая служба флаг приступать к экспорту:
        if (ini_file->read_string("ControlFlags", "Enabled") != "false") {
            ss_writer->Write("Begin of export\n");  /* Сигнал для bgHelper, что Revit успешно запустился и начался экспорт */
            ini_file->write_string("ControlFlags", "Enabled", "false");

            try {
                // Оборачиваем весь код по экспорту в контекст Revit:
                external_export_event_->Raise();
            }
            catch (const std::exception& e) {
                ss_writer->Write(e.what());

            }
        }
    }
    else
        logger_->info("Pipe handler can not write!");
}

void ext_app::try_connect_to_exports_pipe_server(UIApplication^ uiapp) {
    try {
        auto pipe_client = gcnew NamedPipeClientStream(".", "\\bghelperpipe", PipeDirection::InOut);
        try {
            auto pipe_thread = gcnew Thread(gcnew ParameterizedThreadStart(this, &ext_app::pipe_connect));
            logger_->info("pipeThread->Start");
            pipe_thread->Start(pipe_client);

            auto conn_handler_thread = gcnew Thread(gcnew ParameterizedThreadStart(this, &ext_app::pipe_handler));
            logger_->info("connHandlerThread->Start");
            conn_handler_thread->Start(pipe_client);
        }
        catch (TimeoutException^ e) {
            logger_->info("Received from server: " + e->ToString());
        }  
        
        /* _logger->Info("_logger->Info(\"End of export?\")"); */
    }
    catch (exception e) {
        this->logger_->error(e->ToString());
    }
}

void ext_app::delegate_component_manager_ui_element_activated(object sender, UIElementActivatedEventArgs^ e) {
    const auto tab_name = gcnew String(wtab_name);
    const auto bim_panel = gcnew String(wpanel_name);
    if (e != nullptr && e->Item != nullptr && e->Item->Id != nullptr) {
        if (e->Item->Id == "CustomCtrl_%CustomCtrl_%" + tab_name + "%" + bim_panel + "%ID_EXPORT_BUTTON") {
            on_export_button_click();
        }
    }
}

void ext_app::on_export_button_click() {
    const auto vendor_name = gcnew String(wvendor_name);
    const string export_to_exe_path = vendor_directory + "\\" + vendor_name + "\\ifc_exporter\\ifc_exporter.exe";

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
    catch (exception e){
        File::AppendAllText(vendor_directory + "\\ext_addin.dev.log", DateTime::Now.ToString("dd.MM.yyyy hh:mm tt") + "Exception " + e->ToString() + "\n");
        logger_->info("Exception " + e->ToString() + "\n");
    }
}

void ext_app::pipe_connect(object pipe_parameter) {
    logger_->info("ext_app::pipe_connect: Connecting... ");
    try {
        reinterpret_cast<NamedPipeClientStream^>(pipe_parameter)->Connect();
    }
    catch (exception ex){
        logger_->info("Connecting the pipe throw an exception: " + ex->ToString());
    }
}

Result ext_app::OnStartup(UIControlledApplication^ uiapp) {
    DllMain(uiapp);
	return (ui::Result::Succeeded);
}