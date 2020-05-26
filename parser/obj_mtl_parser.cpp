#include "obj_mtl_parser.h"

#include <locale>
#include <iostream>
#include <sstream>
#include <string>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cassert>

const std::string_view obj_parser::version = "0.0.1";

namespace
{
	using rule_function = std::function<void()>;
	using parsing_rules = std::unordered_map<std::string, rule_function>;

	struct membuf : std::streambuf
	{
		membuf(char const *base, size_t size)
		{
			char *p(const_cast<char *>(base));
			this->setg(p, p, p + size);
		}
	};

	struct imemstream : virtual membuf, std::istream
	{
		imemstream(char const *base, size_t size)
			: membuf(base, size)
			, std::istream(static_cast<std::streambuf *>(this))
		{}
	};

	template <typename T>
	std::basic_string<T> &ltrim(std::basic_string<T> &str)
	{
		auto it2 = std::find_if(str.begin(), str.end(), [](T ch)
		{
			return !std::isspace<T>(ch, std::locale::classic());
		});
		str.erase(str.begin(), it2);
		return str;
	}

	template <typename T>
	std::basic_string<T> &rtrim(std::basic_string<T> &str)
	{
		auto it1 = std::find_if(str.rbegin(), str.rend(), [](T ch)
		{
			return !std::isspace<T>(ch, std::locale::classic());
		});
		str.erase(it1.base(), str.end());
		return str;
	}

	template <typename T>
	auto trim(std::basic_string<T> &str) -> std::basic_string<T> &
	{
		return ltrim(rtrim(str));
	}
}

obj_parser::obj_parser(const std::vector<uint8_t> &file_data)
{
	parse_obj(file_data);
}

obj_parser::~obj_parser() = default;

auto obj_parser::vertices() const -> std::vector<float3>
{
	return o_v;
}

auto obj_parser::normals() const -> std::vector<float3>
{
	return o_vn;
}

auto obj_parser::uv() const -> std::vector<float2>
{
	return o_vt;
}

auto obj_parser::sub_mesh_list() const -> std::vector<obj_parser::group>
{
	return o_g;
}

auto obj_parser::mtl_files() const -> std::vector<std::string>
{
	return mtllib;
}

void obj_parser::parse_obj(const std::vector<uint8_t> &file_data)
{
	auto data_stream = imemstream(reinterpret_cast<const char *>(file_data.data()), file_data.size());

	auto g_idx = -1;

	auto parse_vert_str = [](const std::string &vert_str) -> uint3
	{
		auto return_val = uint3{};

		auto it_start = vert_str.begin();
		for (auto &val : return_val)
		{
			auto it_next = std::find(it_start, vert_str.end(), '/');
			auto number = std::string(it_start, it_next);

			val = std::stoi(number) - 1;

			it_start = (it_next != vert_str.end()) ? ++it_next : vert_str.end();
		}
		return return_val;
	};

	auto obj_rules = parsing_rules {
	{ "mtllib", [&]() {
		auto mtllib_name = std::string{};
		std::getline(data_stream, mtllib_name);
		mtllib.push_back(trim(mtllib_name));
	}},
	{ "g", [&]() {
		g_idx++;
		auto &grp = o_g.emplace_back();
		std::getline(data_stream, grp.name);
		trim(grp.name);
	}},
	{ "usemtl", [&]() {
		auto &grp = o_g.at(g_idx);
		std::getline(data_stream, grp.mtl_name);
		trim(grp.mtl_name);
	}},
	{ "f", [&]() {
		auto &grp = o_g.at(g_idx);
		auto face_str = std::string{};
		std::getline(data_stream, face_str);
			
		auto face_stream = std::stringstream(face_str);
		auto elem_idx_str = std::string{};
		while (face_stream >> elem_idx_str)
		{
			grp.indicies.push_back(parse_vert_str(elem_idx_str));
		}
	}},
	{ "v", [&]() {
		auto &pos = o_v.emplace_back();
		data_stream >> pos[0] >> pos[1] >> pos[2];
	}},
	{ "vn", [&]() {
		auto &nor = o_vn.emplace_back();
		data_stream >> nor[0] >> nor[1] >> nor[2];
	}},
	{ "vt", [&]() {
		auto &txc = o_vt.emplace_back();
		data_stream >> txc[0] >> txc[1];
	}}, };

	while (not data_stream.eof())
	{
		auto token = std::string{};
		data_stream >> token;

		auto rule = obj_rules.find(token);
		if (rule == obj_rules.end())
		{
			data_stream.ignore(file_data.size(), '\n');
			continue;
		}

		rule->second();
	}
}

mtl_parser::mtl_parser(const std::vector<uint8_t> &file_data)
{
	parse_mtl(file_data);
}

mtl_parser::~mtl_parser() = default;

auto mtl_parser::materials() const -> std::vector<material>
{
	return mtls;
}

void mtl_parser::parse_mtl(const std::vector<uint8_t> &file_data)
{
	auto data_stream = imemstream(reinterpret_cast<const char *>(file_data.data()), file_data.size());

	auto mtl_idx = -1;

	auto read_color_into = [&](float3 &dest)
	{
		data_stream >> dest[0] >> dest[1] >> dest[2];
	};

	auto read_texture_into = [&](std::string &dest)
	{
		std::getline(data_stream, dest);
		trim(dest);
	};

	auto mtl_rules = parsing_rules{
	{ "newmtl", [&]() {
		mtl_idx++;
		auto &mtl = mtls.emplace_back();
		std::getline(data_stream, mtl.name);
		trim(mtl.name);
	}},
	{ "Ka", [&]() {
		read_color_into(mtls.at(mtl_idx).color_ambient);
	}},
	{ "Kd", [&]() {
		read_color_into(mtls.at(mtl_idx).color_diffuse);
	}},
	{ "Ks", [&]() {
		read_color_into(mtls.at(mtl_idx).color_specular);
	}},
	{ "Ns", [&]() {
		auto &Ns = mtls.at(mtl_idx).shininess;
		data_stream >> Ns;
	}},
	{ "Tr", [&]() {
		auto &Tr = mtls.at(mtl_idx).transparency;
		data_stream >> Tr;
	}},
	{ "d", [&]() {
		auto &Tr = mtls.at(mtl_idx).transparency;
		data_stream >> Tr;
	}},
	{ "illum", [&]() {
		auto &il = mtls.at(mtl_idx).illumination_type;
		data_stream >> il;
	}},
	{ "map_Ka", [&]() {
		read_texture_into(mtls.at(mtl_idx).tex_ambient);
		
	}},
	{ "map_Kd", [&]() {
		read_texture_into(mtls.at(mtl_idx).tex_diffuse);
	}},
	{ "map_Ks", [&]() {
		read_texture_into(mtls.at(mtl_idx).tex_specular);
	}},
	{ "map_Ns", [&]() {
		read_texture_into(mtls.at(mtl_idx).tex_shininess);
	}},
	{ "map_Tr", [&]() {
		read_texture_into(mtls.at(mtl_idx).tex_transparency);
	}},
	{ "map_d", [&]() {
		read_texture_into(mtls.at(mtl_idx).tex_transparency);
	}},
	{ "map_bump", [&]() {
		read_texture_into(mtls.at(mtl_idx).tex_bump);
	}}, };

	while (not data_stream.eof())
	{
		auto token = std::string{};
		data_stream >> token;

		auto rule = mtl_rules.find(token);
		if (rule == mtl_rules.end())
		{
			data_stream.ignore(file_data.size(), '\n');
			continue;
		}

		rule->second();
	}
}
