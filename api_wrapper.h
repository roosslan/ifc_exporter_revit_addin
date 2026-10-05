#pragma once

#include "stdafx.h"

namespace ifc_exporter {
	public ref class api_wrapper : IExternalEventHandler {
	public:
		api_wrapper();
		/* Канал к bgHelper, открытый в ext_app::pipe_worker */
		StreamWriter^ pipe_writer;
		virtual void Execute(UIApplication^ app);
		void task_run_async_in_context(UIApplication^ app);

		/* Унаследовано через IExternalEventHandler */
		virtual string GetName();
	};
}