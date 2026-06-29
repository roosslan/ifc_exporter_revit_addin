#pragma once

#include "stdafx.h"

ref class ClassificationSettings {
public:
    string ClassificationName;
    string ClassificationEdition;
    string ClassificationSource;
    DateTime ClassificationEditionDate;
    string ClassificationLocation;
    string ClassificationFieldName;
};
ref class IFCClassification { public: ClassificationSettings ClassificationSettings; };

ref class ProjectAddress {
public:
    bool UpdateProjectInformation;
    bool AssignAddressToSite;
    bool AssignAddressToBuilding;
};
ref class IFCProjectAddress { public: ProjectAddress ProjectAddress; };

namespace ifc_exporter
{
        ref struct views_n_sites {
            string rvt_file_path;
            string view_name;
            string site_;
            string output_f_name;
            string json_path;
            bool   should_be_exported;
            views_n_sites(string fn, string v3d_name, string site, string output_file_name, string json_file_path, bool should_export)
            {
                rvt_file_path = fn;
                view_name = v3d_name;
                site_ = site;
                output_f_name = output_file_name;
                json_path = json_file_path;
                should_be_exported = should_export;
            }
        };

        ref struct view3d_name {
            ElementId^ view_id;
            string view_name;
            view3d_name(ElementId^ element_id, string v_name)
            {
                view_id = element_id;
                view_name = v_name;
            }
        };
}