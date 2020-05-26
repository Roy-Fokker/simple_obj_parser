#include "obj_mtl_parser.h"

#include <fmt\core.h>

#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdint>

#include <ShlObj.h>
#include <atlbase.h>
#include <cassert>

using namespace std::string_literals;
using namespace std::string_view_literals;

auto open_file_dialog() -> std::filesystem::path;
auto read_file_data(const std::filesystem::path &file_path) -> std::vector<uint8_t>;

auto main() -> int
{
	fmt::print("Wavefront Obj and Mtl Parser\n");

	fmt::print("Choose File:\n");
	auto obj_file = open_file_dialog();
	auto file_data = read_file_data(obj_file);
	auto obj_file_folder = obj_file.parent_path();

	fmt::print("Parsing Obj file...\n");
	auto obj_data = obj_parser(file_data);
	
	auto mtls = obj_data.mtl_files();
	fmt::print("Parsing Mtl files...\n");
	auto mtl_list = std::vector<mtl_parser::material>{};
	for (auto &mtl_file : mtls)
	{
		auto mtl_data = mtl_parser(read_file_data(obj_file_folder / mtl_file));
		auto mtl_to_add = mtl_data.materials();
		mtl_list.insert(mtl_list.end(), mtl_to_add.begin(), mtl_to_add.end());
	}

	fmt::print("File Statistics\n");
	auto grps = obj_data.sub_mesh_list();
	fmt::print("\t{} groups\n", grps.size());
	auto v = obj_data.vertices();
	fmt::print("\t{} vertices\n", v.size());
	auto vn = obj_data.normals();
	fmt::print("\t{} normals\n", vn.size());
	auto vt = obj_data.uv();
	fmt::print("\t{} uv-coordinates\n", vt.size());
	fmt::print("\t{} materials\n", mtl_list.size());

	return 0;
}

auto open_file_dialog() -> std::filesystem::path
{
	::CoInitialize(NULL);

	constexpr auto file_types = std::array<COMDLG_FILTERSPEC, 1>
	{
		{
			L"Wavefront (*.obj)", L"*.obj"
		},
	};

	auto file_name = std::filesystem::path{};

	auto file_dialog = CComPtr<IFileDialog>{};
	auto hr = CoCreateInstance(CLSID_FileOpenDialog,
							   nullptr,
							   CLSCTX_INPROC_SERVER,
							   IID_PPV_ARGS(&file_dialog));
	assert(SUCCEEDED(hr));

	file_dialog->SetTitle(L"Select model to view");
	
	file_dialog->SetFileTypes(static_cast<uint32_t>(file_types.size()),
							  file_types.data());

	hr = file_dialog->Show(NULL);
	assert(SUCCEEDED(hr));

	auto shell_item = CComPtr<IShellItem>{};
	hr = file_dialog->GetResult(&shell_item);
	assert(SUCCEEDED(hr));

	auto pfileName = PWSTR{ NULL };
	hr = shell_item->GetDisplayName(SIGDN_FILESYSPATH, &pfileName);
	assert(SUCCEEDED(hr));

	file_name.assign(pfileName);
	return file_name;
}

auto read_file_data(const std::filesystem::path &file_path) -> std::vector<uint8_t>
{
	fmt::print("Reading File: {}\n", file_path.string());

	auto file = std::ifstream(file_path, std::ios::in | std::ios::ate);
	assert(file.is_open());

	file.seekg(0, std::ios::end);
	auto file_size = file.tellg();
	file.seekg(0, std::ios::beg);

	auto file_data = std::vector<uint8_t>(file_size);
	file.read(reinterpret_cast<char *>(&file_data[0]),
			  file_size);

	return file_data;
}