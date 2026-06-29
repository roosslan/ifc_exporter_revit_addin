#pragma once

#include "stdafx.h"

namespace ifc_exporter {
	public ref class api_wrapper : IExternalEventHandler {
	public:
		api_wrapper();
		IExternalApplication^ addin_;
		UIControlledApplication^ uic_app_addin_;

		/* Унаследовано через IExternalEventHandler */
		virtual string GetName();
		virtual void Execute(UIApplication^ app);
		void task_run_async_in_context(UIApplication^ app);
	};
}