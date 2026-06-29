#include "btn_availability.h"

namespace ifc_exporter {
	Result btn_click::Execute(ExternalCommandData^ commandData, string% message, ElementSet^ elements){
		/* ‘юда ссылается ф-ция ext_app::create_ribbon_buttons() ...  gcnew PushButtonData("ID_EXPORT_BUTTON", "Выгрузка", assembly_location_, "ifc_exporter.btn_click"); */
        return Result::Succeeded;
	}
	bool btn_availability::IsCommandAvailable(UIApplication^ uiapp, CategorySet^ cat_set){
			return true;
        }
}