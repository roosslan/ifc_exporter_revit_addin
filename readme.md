Updated 3 Jul 2026
* ifc_exporter.exe
* bgHelper.exe
* rvtversion.exe
* IFC/NWC Export


 \
![Revit's toolbar](https://github.com/roosslan/ifc_exporter_revit_addin/blob/trunk/toolbar.gif?raw=true)

## Сборка

Корень репозитория — сборка для Revit 2023 (.NET Framework 4.8), `net8/` — для Revit 2026 (.NET 8). Перед сборкой необходимо скопировать `sensitive_data.example.h` в `sensitive_data.h` (в корень и в `net8/`) и подставить реальные значения.

Сборка, модульные тесты и выпуск релизов (zip и MSI) выполняются в GitHub Actions на self-hosted runner'е — см. [docs/ci.md](docs/ci.md).
