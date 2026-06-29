/* last upd: rusibragimov 22.6.2026 */

#include "start_export.hpp"

namespace ifc_exporter {

    void CExport::Export(export_to_file_format^ record_entity) {

        string current_open_path = nullptr;

        for each (views_n_sites^ row in record_entity->list_views_n_sites) {
            /* Проверяем, нужно ли нам "переключаться" на следующий файл */
            if (row->rvt_file_path->Trim() != current_open_path) {
                /* Закрываем предыдущий файл, если он был открыт */
                if (m_rvt_doc_ != nullptr) {
                    m_rvt_doc_->Close(false);
                    m_rvt_doc_ = nullptr;
                }

                /* Открываем файл */
                if (open_file(row->rvt_file_path->Trim())) {
                    current_open_path = row->rvt_file_path->Trim();
                }
                else {
                    current_open_path = nullptr; /* Если не удалось открыть */
                    continue;
                }
            }
        /* Теперь приступаем непосредственно к экспортам (файл открыт) */
            if (m_rvt_doc_ != nullptr) {
                pipe_toBg_n_qLog->Write("sav line: " + row->rvt_file_path->Trim() + " = " + row->view_name + " = " + row->site_ + " = " + row->output_f_name + " = " + row->json_path);
                try {
                    view3d_name^ export_view_n_id = nullptr;
                    try {
                        export_view_n_id = get_export_view_id(row->view_name);
                    }
                    catch (const exception e) {
                        const string exc = Regex::Replace(e->ToString(), "\t|\n|\r", " ");
                        pipe_toBg_n_qLog->Write("ElementId exportViewId: " + exc + "\n");
                    }

                    auto nwc_export_options = create_options_nwc();
                    auto ifc_export_configuration = create_config_ifc();
                    auto ifc_export_options = gcnew IFCExportOptions();

                    /* Использовать JSON с настройками */
                    if (row->json_path != "") {
                        deserialize_json_into_configuration(row->json_path, ifc_export_configuration, ifc_export_options);
                    }
                    if (row->site_ != "") {
                        ifc_export_configuration->SelectedSite = get_location_to_export(row->site_);
                    }
                    /* Передаем все данные из IFCExportConfiguration вовнутрь IFCExportOptions'a: */
                    ifc_export_configuration->UpdateOptions(ifc_export_options, export_view_n_id->view_id);

                    /* TODO: What for?   Get the SiteLocation instance                  */
                    SiteLocation^ site_location = m_rvt_doc_->SiteLocation;

                    auto output_file_name = sanitize_filename(m_rvt_doc_->Title + "_s" + row->site_ + "_q" + row->view_name + "_v" + export_view_n_id->view_name);
                    if (row->output_f_name != "") {
                        output_file_name = row->output_f_name;
                    }
/* === IFC Export ================================================================================================================ */

                    /* Экспорт в IFC будет выполняться, только, если параметр recordEntity->IsIFC_On */
                    export_to_ifc(record_entity->rec_export_path, export_view_n_id->view_id, output_file_name, ifc_export_options, record_entity->is_ifc_on);

/* === NWC Export ================================================================================================================ */
                    nwc_export_options->ViewId = export_view_n_id->view_id;
                    /* Экспорт в NWC будет выполняться, только, если параметр recordEntity->IsOn */
                    export_to_nwc(record_entity->rec_export_path, output_file_name, nwc_export_options, record_entity->is_nwc_on);

/* =============================================================================================================================== */
                }
                catch (const exception e) {
                    const string exc = Regex::Replace(e->ToString(), "\t|\n|\r", " ");
                    pipe_toBg_n_qLog->Write("ExportTo IFC/NWC: " + exc);
                }
                finally {
                    auto relinquish_options = gcnew RelinquishOptions(true);
                    auto twc_options = gcnew TransactWithCentralOptions();

                    try {
                        WorksharingUtils::RelinquishOwnership(m_rvt_doc_, relinquish_options, twc_options);
                    }
                    catch (exception e) {
                        pipe_toBg_n_qLog->Write("RelinquishOwnership: " + e->ToString());
                    }
                }
            }
        } /* foreach scope + if (current_file_path == nullptr || current_file_path != row->rvt_file_path->Trim()) { */

        /* Закрываем последний документ из sav-файла */
    if (m_rvt_doc_ != nullptr) {
        m_rvt_doc_->Close(false);
    }
        pipe_toBg_n_qLog->Write("End of export\n"); /* Сигнал для bgHelper, что экспорт Revit'ом завершен */
    }

    void CExport::export_to_nwc(const string path_to_export, const string file_name, nwc_export_options navisworks_export_options, const bool do_export) {
        if (do_export) {
            try {
                pipe_toBg_n_qLog->Write("Export view to NWC " + navisworks_export_options->ViewId);
                m_rvt_doc_->Export(path_to_export, file_name, navisworks_export_options);
            }
            catch (std::exception e) {
                pipe_toBg_n_qLog->Write("CExport::ExportToNWC: " + gcnew String(e.what()));
            }
        }
    }

    void CExport::export_to_ifc(const string path_to_export, ElementId^ view_3d, const string file_name, IFCExportOptions^ ifc_export_options, const bool do_export) {
        if (do_export) {
            auto transaction = gcnew Transaction(m_rvt_doc_, "ifc_exporter.IFC_Export");
            transaction->Start();
            try {
                /* Выгружаем указанную 3D-вьюху в отдельный файл */
                pipe_toBg_n_qLog->Write("Export view to IFC " + view_3d + " \n");
                
                m_rvt_doc_->Export(path_to_export, file_name, ifc_export_options);               
            }
            /* B режиме отладки 0xc0000005 Memory access violation */
            catch (std::exception e) {
                pipe_toBg_n_qLog->Write("CExport::ExportToIFC: " + gcnew String(e.what()));
            }
            transaction->Commit();
        }
    }

    bool CExport::open_file(const string file_path)
        try {
            auto model_path = ModelPathUtils::ConvertUserVisiblePathToModelPath(file_path);
            pipe_toBg_n_qLog->Write("modelPath " + model_path);

            auto open_options = gcnew OpenOptions();
            IList<WorksetPreview^>^ worksets_list;
            try {
                worksets_list = WorksharingUtils::GetUserWorksetInfo(model_path);
            }
            catch (Autodesk::Revit::Exceptions::CentralModelException^ e) {
                (void)e;
                /*  "The model is not workshared" exception. В файле нет рабочих наоборов. В этом случае пропускаем их итерацию. */
            }

            if (worksets_list) {
                pipe_toBg_n_qLog->Write("Worksets found.");
                auto workset_ids = gcnew List<WorksetId^>();
                for each (WorksetPreview^ workset_preview in worksets_list) {
                    /* нужны 00 и 02; 01* - это связи, они нам не нужны при экспорте */
                    if (workset_preview->Name->StartsWith("00_") || workset_preview->Name->StartsWith("02_")){
                        pipe_toBg_n_qLog->Write("Workset added: " + workset_preview->Id);
                        workset_ids->Add(workset_preview->Id);
                    }
                }

                auto workset_configuration = gcnew WorksetConfiguration(WorksetConfigurationOption::CloseAllWorksets);
                workset_configuration->Open(workset_ids);
                open_options->SetOpenWorksetsConfiguration(workset_configuration);

                /* Отсоединить и сохранить рабочие наборы */
                open_options->DetachFromCentralOption = DetachFromCentralOption::DetachAndPreserveWorksets;
            }

            try {
                m_rvt_doc_ = m_rvt_app_->OpenDocumentFile(model_path, open_options);
            }
            catch (Autodesk::Revit::Exceptions::OperationCanceledException^ e) {
                if (!m_rvt_doc_)
                    pipe_toBg_n_qLog->Write("Operation ganzel! - RVT_Document is null " + e->Message);
            }            

            return true;
        }
        catch (exception e) {
            const string exc = Regex::Replace(e->ToString(), "\t|\n|\r", " ");
            pipe_toBg_n_qLog->Write("OpenFile " + file_path + " - " + exc);
            return false;
        }
    

    view3d_name^ CExport::get_export_view_id(const string export_this_view_3d) {
        auto ret_view3d_names = gcnew List<view3d_name^>;
        auto collector = gcnew FilteredElementCollector(m_rvt_doc_);

        auto fe_views3d = collector->OfClass(View3D::typeid);

        auto views3d = gcnew List<View3D^>;

        try {
            View3D^ v_view_3d = nullptr;

            for each(Element^ fe_elem in fe_views3d->ToElements()) {
                v_view_3d = dynamic_cast<View3D^>(fe_elem);
                if (v_view_3d) {
                    if (!v_view_3d->IsTemplate)
                        views3d->Add(v_view_3d);
                }
            }
        }
        catch (const std::exception&) {
            pipe_toBg_n_qLog->Write("Thrown an exception! GetExportViewId\n");
        }

        /* 12.2.25 Список для множества 3D-вьюх Navisworks
         * auto exportViewIds = gcnew List<ElementId^>;
         */

        for each(View3D^ v in views3d) {
            if (export_this_view_3d != "") {
                    if (export_this_view_3d->ToLower() == v->Name->ToLower()) {
/*                      exportViewIds->Add(v->Id);          */
                        auto view3d_name_line = gcnew view3d_name(v->Id, v->Name);
                        ret_view3d_names->Add(view3d_name_line);
                        view3d_name_line = {};
                        pipe_toBg_n_qLog->Write("3D-view '" + v->Name + "' добавлена в список выбора для экспорта\n");
                    }
            }
            else /* передали пустое значение вместо имени 3D-вьюхи, экспортируем первое попавшееся, которое содержит в имени navisworks */
                if (v->Name->ToLower()->Contains("navisworks")) {
/*                  exportViewIds->Add(v->Id);              */
                    auto view3d_name_line = gcnew view3d_name(v->Id, v->Name);
                    ret_view3d_names->Add(view3d_name_line);
                    view3d_name_line = {};
                    pipe_toBg_n_qLog->Write("Единственная 3D-view '" + v->Name + "' добавлена в список экспорта\n");
                }
        }

        /* Правка от dbor: всегда выгружаем только первую с названием Navisworks */
        if (ret_view3d_names->Count != 0) {
            return ret_view3d_names[0];
        }
        else {
            ret_view3d_names->Add(gcnew view3d_name(Enumerable::First(views3d)->Id, Enumerable::First(views3d)->Name));
        }
        return ret_view3d_names[0];
    }

    void CExport::deserialize_json_into_configuration(const string pre_setup_file_path, IFCExportConfiguration^ from_json_ifc_export_configuration, IFCExportOptions^ ifc_export_options) {
        pipe_toBg_n_qLog->Write("Object Notation file (.JSON): " + pre_setup_file_path + "\n");

        if (!File::Exists(pre_setup_file_path)) return;
        const string stringified_json = File::ReadAllText(pre_setup_file_path);

        auto ifc_project_addr = JsonConvert::DeserializeObject<IFCProjectAddress^>(stringified_json);
        auto ifc_classification_settings = JsonConvert::DeserializeObject<IFCClassification^>(stringified_json);

        auto json_serial = gcnew JavaScriptSerializer();

        const string proj_addr_json_str = json_serial->Serialize(ifc_project_addr);
        ifc_export_options->AddOption("ProjectAddress", proj_addr_json_str);

        const string classification_json_str = json_serial->Serialize(ifc_classification_settings);
        ifc_export_options->AddOption("ClassificationSettings", classification_json_str);

        auto jobject = JsonConvert::DeserializeObject<JObject^>(stringified_json);

        for each (auto sub_obj in jobject) {
        try {
            auto json_pair = (KeyValuePair<string, JToken^>^)sub_obj;

            /* Эти не нашлись в типе IFCExportConfiguration, добавляю Опциями?... */
            if (json_pair->Key == "ExchangeRequirement")
                ifc_export_options->AddOption("ExchangeRequirement", json_pair->Value->ToString());
            if (json_pair->Key == "ExportHostAsSingleEntity")
                ifc_export_options->AddOption("ExportHostAsSingleEntity", json_pair->Value->ToString());
            if (json_pair->Key == "ExportMaterialPsets")
                ifc_export_options->AddOption("ExportMaterialPsets", json_pair->Value->ToString());
            if (json_pair->Key == "IFCFileType")
                ifc_export_options->AddOption("IFCFileType", json_pair->Value->ToString());
            if (json_pair->Key == "OwnerHistoryLastModified")
                ifc_export_options->AddOption("OwnerHistoryLastModified", json_pair->Value->ToString());
            if (json_pair->Key == "UseTypePropertiesInInstacePSets")
                ifc_export_options->AddOption("UseTypePropertiesInInstacePSets", json_pair->Value->ToString());

/*         if (json_pair->Key == "SitePlacement")
                параметр SiteTransformBasis ("SitePlacement") будет обработан дополнительно
*/
            /* Если поле из Json имеется в типе IFCExportConfiguration, то присваиваем: IFCExportConfiguration->ИмяПоля = Json->ЗначениеПоля */
            Type^ conf = from_json_ifc_export_configuration->GetType();
                    if (json_pair->Key != "ActivePhaseId" && json_pair->Key != "ClassificationSettings" && json_pair->Key != "ProjectAddress") { // ActivePhaseId начинает искать BIM::IFC::Export::UI::IFCPhaseAttributes::Validate(int phaseId) и падает
                        PropertyInfo^ property_info = conf->GetProperty(json_pair->Key);                        
                        if (property_info) {
                            auto parameter_type = property_info->GetMethod->ReturnParameter->ParameterType;
                            if (parameter_type->FullName->Contains("Boolean"))
                                property_info->SetValue(from_json_ifc_export_configuration, json_pair->Value->ToObject<bool>());

                            if ( parameter_type->Name->Contains("String"))
                                property_info->SetValue(from_json_ifc_export_configuration, json_pair->Value->ToString());

                            if (parameter_type->Name->Contains("Double"))
                                property_info->SetValue(from_json_ifc_export_configuration, json_pair->Value->ToObject<double>());

                            if (parameter_type->Name->Contains("Int32") || parameter_type->BaseType->Name == "Enum")
                                property_info->SetValue(from_json_ifc_export_configuration, json_pair->Value->ToObject<int>());
                        }
                    }
        }
        catch (const std::exception& e) {
            pipe_toBg_n_qLog->Write("Parsing Json exception: " + gcnew String(e.what()) + "\n");  }
        }
    }
}