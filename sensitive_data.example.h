#pragma once

/*
 * Шаблон sensitive_data.h.
 * Для сборки файл необходимо скопировать в sensitive_data.h (в корень и в net8\) и подставить реальные значения.
 * sensitive_data.h в репозиторий не добавляется (см. .gitignore).
 */

/* Поддиректория в %APPDATA% с настройками и логами (начинается с обратной косой черты) */
constexpr auto wapp_directory = L"\\ifc_exporter";

/* Вкладка и панель на ленте Revit */
constexpr auto wtab_name = L"BIM";
constexpr auto wpanel_name = L"IFC";

/* Имя поставщика: ifc_exporter.exe ищется в <wapp_directory>\<wvendor_name>\ifc_exporter */
constexpr auto wvendor_name = L"vendor";
