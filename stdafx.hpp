/*
 *  stdafx.hpp
*/

#ifndef STDAFX_H
#define STDAFX_H

#include <iostream>
#include <sstream>
#include <string>
#include <msclr\marshal_cppstd.h>
#include <cliext/vector>

typedef System::String^ string;
typedef System::Object^ object;
typedef System::Exception^ exception;
typedef Autodesk::Revit::DB::NavisworksExportOptions^ nwc_export_options;

using namespace log4net;
using namespace Newtonsoft::Json;
using namespace Newtonsoft::Json::Linq;

using namespace System;
using namespace System::IO;
using namespace System::Xml;
using namespace System::Linq;
using namespace System::Text;
using namespace System::Windows;
using namespace System::Threading;
using namespace System::IO::Pipes;
using namespace System::Web::Script::Serialization;
using namespace System::Reflection;
using namespace System::Resources;
using namespace System::Diagnostics;
using namespace System::Text::Json;
using namespace System::Windows::Interop;
using namespace System::Windows::Threading;
using namespace System::Threading::Tasks;
using namespace System::Collections::Generic;
using namespace System::Windows::Media::Imaging;
using namespace System::Runtime::InteropServices;
using namespace System::Collections::ObjectModel;
using namespace System::Text::RegularExpressions;

using namespace Autodesk::Revit::DB;
using namespace Autodesk::Revit;
using namespace Autodesk::Windows;
using namespace Autodesk::Revit::DB;
using namespace Autodesk::Revit::UI;
using namespace BIM::IFC::Export::UI;
using namespace Autodesk::Revit::UI::Events;
using namespace Autodesk::Revit::DB::Events;
using namespace Autodesk::Revit::Attributes;
using namespace Autodesk::Revit::ApplicationServices;
using namespace Autodesk::Revit::DB::ExtensibleStorage;

namespace img = System::Windows::Media::Imaging;
namespace ui = Autodesk::Revit::UI;
namespace ref = System::Reflection;

#endif