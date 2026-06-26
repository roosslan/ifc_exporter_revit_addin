#pragma once

#include "stdafx.hpp"

namespace ifc_exporter {
	public ref class api_wrapper : IExternalEventHandler {
	public:
		api_wrapper();
		virtual void Execute(UIApplication^ app);
		void task_run_async_in_context(UIApplication^ app);

		/* Унаследовано через IExternalEventHandler */
		virtual string GetName();
	};
}