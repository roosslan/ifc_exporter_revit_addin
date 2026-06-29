#include "stdafx.h"

#include "api_wrapper.h"
#include "start_export.h"

namespace ifc_exporter {
    api_wrapper::api_wrapper() {
    }

    void api_wrapper::Execute(UIApplication^ app)
    try {
        /*
        Document^ doc = app->ActiveUIDocument->Document;
        UIDocument^ ui_doc = app->ActiveUIDocument;

        if (doc == nullptr)
        {
            TaskDialog::Show("External Event api_wrapper", "Не удалось получить активный документ Revit!");
            return;
        }
        */
        task_run_async_in_context(app);
    }
    catch (const std::exception& e) {
        (void)e;
        /* File::AppendAllText("\\ext_addin.dev.log", DateTime::Now.ToString("dd.MM.yyyy hh:mm tt") + "apiWrapper::Execute ");  */
    }

    void api_wrapper::task_run_async_in_context(UIApplication^ app){
        auto ifcexporter = gcnew CExport(app);
    }

    /* virtual */
    string api_wrapper::GetName() {
        /* throw gcnew System::NotImplementedException();   */
        return ("External Event api_wrapper");
    }

}

