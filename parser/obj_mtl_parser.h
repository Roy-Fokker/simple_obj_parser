#pragma once

#include <vector>
#include <array>
#include <string_view>

class obj_parser
{
public:
	static const std::string_view version;

public:
	obj_parser() = delete;
	obj_parser(const std::vector<uint8_t> &file_data);
	~obj_parser();

	using float3 = std::array<float, 3>;
	using float2 = std::array<float, 2>;
	using uint3 = std::array<uint32_t, 3>;
	struct group
	{
		std::string name;
		std::vector<uint3> indicies;
		std::string mtl_name;
	};

	auto vertices() const -> std::vector<float3>;
	auto normals() const -> std::vector<float3>;
	auto uv() const -> std::vector<float2>;
	auto sub_mesh_list() const -> std::vector<group>;
	auto mtl_files() const->std::vector<std::string>;

private:
	void parse_obj(const std::vector<uint8_t> &file_data);

private:
	std::vector<float3> o_v;
	std::vector<float3> o_vn;
	std::vector<float2> o_vt;
	std::vector<group> o_g;
	std::vector<std::string> mtllib;
};

class mtl_parser
{
public:
	mtl_parser() = delete;
	mtl_parser(const std::vector<uint8_t> &file_data);
	~mtl_parser();

	using float3 = std::array<float, 3>;
	struct material
	{
		std::string name;
		
		float3 color_ambient;
		float3 color_diffuse;
		float3 color_specular;

		float shininess;
		float transparency;
		uint8_t illumination_type;

		std::string tex_ambient;
		std::string tex_diffuse;
		std::string tex_specular;
		std::string tex_shininess;
		std::string tex_transparency;
		std::string tex_bump;
	};

	auto materials() const -> std::vector<material>;

private:
	void parse_mtl(const std::vector<uint8_t> &file_data);

private:
	std::vector<material> mtls;
};