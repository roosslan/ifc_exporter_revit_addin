#pragma once

#include "stdafx.h"

namespace ifc_exporter {

    [Transaction(TransactionMode::Manual)]
	/* Класс-обработчик нажати€ кнопки запуска ifc exporter.exe */
    public ref class btn_click sealed : IExternalCommand {
    public:
        virtual Result Execute(ExternalCommandData^ command_data, string% message, ElementSet^ elements);
    };
	
	public ref class btn_availability : IExternalCommandAvailability {
    public:
    	virtual bool IsCommandAvailable(UIApplication^ uiapp, CategorySet^ cat_set);
    };
};