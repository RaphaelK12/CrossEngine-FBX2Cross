/**
 * Copyright (c) 2014-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <vector>
#include <unordered_map>
#include <map>
#include <iostream>
#include <fstream>

#if defined( __unix__ ) || defined( __APPLE__ )

#include <sys/stat.h>

#define _stricmp strcasecmp
#endif

#include <cxxopts.hpp>

#include "FBX2glTF.h"
#include "utils/String_Utils.h"
#include "utils/File_Utils.h"
#include "Fbx2Raw.h"
#include "Raw2Cross.h"

bool verboseOutput = true;

int main(int argc, char *argv[])
{
	std::string inputPath;
	std::string outputPath;
	std::vector<std::function<Vec2f(Vec2f)>> texturesTransforms;

	cxxopts::Options options(
		"FBX2Mesh",
		"FBX2Mesh 1.0: Generate a Mesh representation of an FBX model.");

	options.add_options()
		("i,input", "The FBX model to convert.", cxxopts::value<std::string>(inputPath))
		("o,output", "Where to generate the output, without suffix.", cxxopts::value<std::string>(outputPath))
		("flip-u", "Flip all U texture coordinates.")
		("flip-v", "Flip all V texture coordinates (default behaviour!)")
		("h,help", "Show this help.");

	options.parse_positional("input");
	options.parse(argc, argv);

	if (options.count("help")) {
		fmt::printf(options.help());
		return 0;
	}

	if (options.count("input") == false) {
		fmt::printf("You must supply a FBX file to convert.\n");
		fmt::printf(options.help());
		return 1;
	}

	if (options.count("output") == 0) {
		outputPath = StringUtils::GetFolderString(inputPath);

		if (outputPath.empty()) {
			outputPath = "./";
		}
	}

	if (options.count("flip-u") > 0) {
		texturesTransforms.emplace_back([](Vec2f uv) { return Vec2f(1.0f - uv[0], uv[1]); });
	}

	if (options.count("flip-v") > 0) {
		texturesTransforms.emplace_back([](Vec2f uv) { return Vec2f(uv[0], 1.0f - uv[1]); });
	}

	RawModel rawModel;

	if (verboseOutput) {
		fmt::printf("Loading FBX File: %s\n", inputPath);
	}

    if (LoadFBXFile(rawModel, inputPath.c_str(), "tga;bmp;png;jpg;jpeg") == false) {
		fmt::fprintf(stderr, "ERROR:: Failed to parse FBX: %s\n", inputPath.c_str());
        return 1;
    }

	if (texturesTransforms.empty() == false) {
		rawModel.TransformTextures(texturesTransforms);
	}

	rawModel.Condense();
	rawModel.TransformGeometry(ComputeNormalsOption::NEVER);

	std::vector<RawModel> rawMaterialModels;
	rawModel.CreateMaterialModels(rawMaterialModels, false, -1, true);

	char szFName[_MAX_PATH] = { 0 };
	char szFileName[_MAX_PATH] = { 0 };
	char szMeshBinFileName[_MAX_PATH] = { 0 };
	char szMeshXMLFileName[_MAX_PATH] = { 0 };
	splitfilename(inputPath.c_str(), szFName, NULL);
	sprintf(szFileName, "%s.mesh", szFName);
	sprintf(szMeshBinFileName, "%s/%s.mesh", outputPath.c_str(), szFName);
	sprintf(szMeshXMLFileName, "%s/%s.xml", outputPath.c_str(), szFName);

	ExportMesh(szMeshBinFileName, rawModel, rawMaterialModels);
	ExportMaterial(outputPath.c_str(), rawModel);
	ExportMeshXML(szMeshXMLFileName, szFileName, rawModel, rawMaterialModels);

    return 0;
}
