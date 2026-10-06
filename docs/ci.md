# Сборка, тесты и выпуск релизов

Workflow `.github/workflows/ci.yml` выполняется на self-hosted runner'е с установленным Revit.

| Событие | Что выполняется |
|---|---|
| PR, push в `trunk`, ручной запуск | сборка для Revit 2023 и 2026, модульные тесты, раскладка надстройки (артефакт `addins`) |
| тег `v*` | то же, затем zip-архивы надстройки, MSI и GitHub Release |

PR из форков на runner'е не запускаются (условие `if` у задания `build`).

## Требования к машине runner'а

- Windows x64.
- Visual Studio 2022 (или Build Tools) с рабочей нагрузкой «Разработка классических приложений на C++» и компонентом «Поддержка C++/CLI» (`Microsoft.VisualStudio.Component.VC.CLI.Support`).
- .NET Framework 4.8 Developer Pack.
- Revit 2023 и Revit 2026 с IFC-экспортёром (`AddIns\IFCExporterUI\Autodesk.IFC.Export.UI.dll`). Если Revit установлен не в `C:\Program Files\Autodesk\Revit <год>\`, путь задаётся свойством MSBuild `RevitApiDir`.
- WiX Toolset той версии, с которой работает WixSharp из `wix_msi_installer` (как при ручной сборке MSI).
- Политика выполнения PowerShell не строже `RemoteSigned`:

  ```powershell
  Set-ExecutionPolicy RemoteSigned -Scope LocalMachine
  ```

.NET 8 SDK и `nuget.exe` устанавливаются самим workflow.

## Регистрация runner'а

1. В репозитории открыть Settings → Actions → Runners → New self-hosted runner и выполнить показанные команды на машине сборки.
2. При настройке указать дополнительную метку `revit` (метки `self-hosted` и `windows` добавляются автоматически).
3. Установить runner как службу, чтобы он работал без входа пользователя.

Репозиторий публичный, поэтому в Settings → Actions → General необходимо включить «Require approval for all outside collaborators»: workflow из чужих PR не должен выполняться без проверки.

## Секреты и переменные

Задаются в Settings → Secrets and variables → Actions.

| Имя | Тип | Назначение |
|---|---|---|
| `SENSITIVE_DATA_H` | secret | Полное содержимое `sensitive_data.h`. Без него сборка PR и `trunk` идёт с `sensitive_data.example.h`, а релиз завершается ошибкой. |
| `MSI_BUILD_DIR` | variable | Путь на машине runner'а к подготовленной директории `Build` для MSI (см. ниже). Нужна только для релиза. |
| `WIX_INSTALLER_REF` | variable | Ветка или тег `roosslan/wix_msi_installer`, из которого собирается MSI. По умолчанию `trunk`. |

### Содержимое `MSI_BUILD_DIR`

То же, что лежит в `..\Build` при ручной сборке установщика: `bgHelper.exe`, `dbchecker.exe`, `ifc_exporter.exe` с зависимостями и поддиректория `Files\` (`bi_loader.addin`, `ext_plugin.addin`, `app.config`, `ifcxeprt.inf`).

Поддиректорию `Addins\` workflow добавляет сам из свежей сборки:

```
Build\Addins\2023\ifc_exporter.addin
Build\Addins\2023\ifc_exporter\ifc_exporter.dll, зависимости, resources\
Build\Addins\2026\...
```

MSI раскладывает её в `%APPDATA%\Autodesk\Revit\Addins\<год>\`. `license.rtf` для диалога лицензии генерируется из `LICENSE` скриптом `tools\license_to_rtf.ps1`.

## Выпуск релиза

```bash
git tag v1.0.0
```

```bash
git push origin v1.0.0
```

В релиз попадают `ifc_exporter_revit2023_<тег>.zip`, `ifc_exporter_revit2026_<тег>.zip` и MSI. Номер сборки MSI (`AUTO_BUILD_NUMBER` в `consts.h`) заменяется на номер запуска workflow.

Содержимое zip-архива необходимо распаковать в `%APPDATA%\Autodesk\Revit\Addins\<год>\`.

## Локальная сборка и тесты

1. Скопировать `sensitive_data.example.h` в `sensitive_data.h` и `net8\sensitive_data.h`, подставить реальные значения.
2. Восстановить пакеты и собрать:

   ```bash
   nuget restore ifc_exporter.sln
   ```

   ```bash
   msbuild ifc_exporter.sln -p:Configuration=Release -p:Platform=x64 -p:OutDir=%CD%\out\build\revit2023\
   ```

   ```bash
   msbuild net8\ifc_exporter.sln -restore -p:Configuration=Release -p:Platform=x64 -p:OutDir=%CD%\out\build\revit2026\
   ```

   Без `OutDir` результат по-прежнему попадает в `..\Build`.

3. Запустить тесты (используют сборку из `out\build\revit2026\`):

   ```bash
   dotnet test tests\ifc_exporter.tests\ifc_exporter.tests.csproj
   ```

Модульные тесты покрывают классы без зависимостей от Revit API: `ini_simple` (разбор INI) и `file_names` (имена выходных файлов).
