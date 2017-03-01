#include <string>
namespace slg { namespace ocl {
std::string KernelSource_pathoclstatebase_funcs = 
"#line 2 \"pathoclstatebase_funcs.cl\"\n"
"\n"
"/***************************************************************************\n"
" * Copyright 1998-2017 by authors (see AUTHORS.txt)                        *\n"
" *                                                                         *\n"
" *   This file is part of LuxRender.                                       *\n"
" *                                                                         *\n"
" * Licensed under the Apache License, Version 2.0 (the \"License\");         *\n"
" * you may not use this file except in compliance with the License.        *\n"
" * You may obtain a copy of the License at                                 *\n"
" *                                                                         *\n"
" *     http://www.apache.org/licenses/LICENSE-2.0                          *\n"
" *                                                                         *\n"
" * Unless required by applicable law or agreed to in writing, software     *\n"
" * distributed under the License is distributed on an \"AS IS\" BASIS,       *\n"
" * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*\n"
" * See the License for the specific language governing permissions and     *\n"
" * limitations under the License.                                          *\n"
" ***************************************************************************/\n"
"\n"
"// List of symbols defined at compile time:\n"
"//  PARAM_MAX_PATH_DEPTH\n"
"//  PARAM_MAX_PATH_DEPTH_DIFFUSE\n"
"//  PARAM_MAX_PATH_DEPTH_GLOSSY\n"
"//  PARAM_MAX_PATH_DEPTH_SPECULAR\n"
"//  PARAM_RR_DEPTH\n"
"//  PARAM_RR_CAP\n"
"\n"
"// (optional)\n"
"//  PARAM_CAMERA_TYPE (0 = Perspective, 1 = Orthographic, 2 = Stereo)\n"
"//  PARAM_CAMERA_ENABLE_CLIPPING_PLANE\n"
"//  PARAM_CAMERA_ENABLE_OCULUSRIFT_BARREL\n"
"\n"
"// (optional)\n"
"//  PARAM_IMAGE_FILTER_TYPE (0 = No filter, 1 = Box, 2 = Gaussian, 3 = Mitchell, 4 = Blackman-Harris)\n"
"//  PARAM_IMAGE_FILTER_WIDTH_X\n"
"//  PARAM_IMAGE_FILTER_WIDTH_Y\n"
"//  PARAM_IMAGE_FILTER_PIXEL_WIDTH_X\n"
"//  PARAM_IMAGE_FILTER_PIXEL_WIDTH_Y\n"
"// (Box filter)\n"
"// (Gaussian filter)\n"
"//  PARAM_IMAGE_FILTER_GAUSSIAN_ALPHA\n"
"// (Mitchell filter)\n"
"//  PARAM_IMAGE_FILTER_MITCHELL_B\n"
"//  PARAM_IMAGE_FILTER_MITCHELL_C\n"
"// (MitchellSS filter)\n"
"//  PARAM_IMAGE_FILTER_MITCHELL_B\n"
"//  PARAM_IMAGE_FILTER_MITCHELL_C\n"
"//  PARAM_IMAGE_FILTER_MITCHELL_A0\n"
"//  PARAM_IMAGE_FILTER_MITCHELL_A1\n"
"\n"
"// (optional)\n"
"//  PARAM_SAMPLER_TYPE (0 = Inlined Random, 1 = Metropolis, 2 = Sobol, 3 = TilePathSampler)\n"
"// (Metropolis)\n"
"//  PARAM_SAMPLER_METROPOLIS_LARGE_STEP_RATE\n"
"//  PARAM_SAMPLER_METROPOLIS_MAX_CONSECUTIVE_REJECT\n"
"//  PARAM_SAMPLER_METROPOLIS_IMAGE_MUTATION_RANGE\n"
"// (Sobol)\n"
"//  PARAM_SAMPLER_SOBOL_STARTOFFSET\n"
"//  PARAM_SAMPLER_SOBOL_MAXDEPTH\n"
"//  PARAM_SAMPLER_SOBOL_RNG0\n"
"//  PARAM_SAMPLER_SOBOL_RNG1\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// PathDepthInfo\n"
"//------------------------------------------------------------------------------\n"
"\n"
"void PathDepthInfo_Init(__global PathDepthInfo *depthInfo) {\n"
"	depthInfo->depth = 0;\n"
"	depthInfo->diffuseDepth = 0;\n"
"	depthInfo->glossyDepth = 0;\n"
"	depthInfo->specularDepth = 0;\n"
"}\n"
"\n"
"void PathDepthInfo_IncDepths(__global PathDepthInfo *depthInfo, const BSDFEvent event) {\n"
"	++(depthInfo->depth);\n"
"	if (event & DIFFUSE)\n"
"		++(depthInfo->diffuseDepth);\n"
"	if (event & GLOSSY)\n"
"		++(depthInfo->glossyDepth);\n"
"	if (event & SPECULAR)\n"
"		++(depthInfo->specularDepth);\n"
"}\n"
"\n"
"bool PathDepthInfo_IsLastPathVertex(__global PathDepthInfo *depthInfo, const BSDFEvent event) {\n"
"	return (depthInfo->depth + 1 >= PARAM_MAX_PATH_DEPTH) ||\n"
"			((event & DIFFUSE) && (depthInfo->diffuseDepth + 1 >= PARAM_MAX_PATH_DEPTH_DIFFUSE)) ||\n"
"			((event & GLOSSY) && (depthInfo->glossyDepth + 1 >= PARAM_MAX_PATH_DEPTH_GLOSSY)) ||\n"
"			((event & SPECULAR) && (depthInfo->specularDepth + 1 >= PARAM_MAX_PATH_DEPTH_SPECULAR));\n"
"}\n"
"\n"
"bool PathDepthInfo_CheckComponentDepths(const BSDFEvent component) {\n"
"	return ((PARAM_MAX_PATH_DEPTH_DIFFUSE > 0) && (component & DIFFUSE)) ||\n"
"			((PARAM_MAX_PATH_DEPTH_GLOSSY > 0) && (component & GLOSSY)) ||\n"
"			((PARAM_MAX_PATH_DEPTH_SPECULAR > 0) && (component & SPECULAR));\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Init Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"#if defined(RENDER_ENGINE_RTPATHOCL)\n"
"\n"
"// Morton decode from https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/\n"
"\n"
"// Inverse of Part1By1 - \"delete\" all odd-indexed bits\n"
"\n"
"uint Compact1By1(uint x) {\n"
"	x &= 0x55555555;					// x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0\n"
"	x = (x ^ (x >> 1)) & 0x33333333;	// x = --fe --dc --ba --98 --76 --54 --32 --10\n"
"	x = (x ^ (x >> 2)) & 0x0f0f0f0f;	// x = ---- fedc ---- ba98 ---- 7654 ---- 3210\n"
"	x = (x ^ (x >> 4)) & 0x00ff00ff;	// x = ---- ---- fedc ba98 ---- ---- 7654 3210\n"
"	x = (x ^ (x >> 8)) & 0x0000ffff;	// x = ---- ---- ---- ---- fedc ba98 7654 3210\n"
"	return x;\n"
"}\n"
"\n"
"// Inverse of Part1By2 - \"delete\" all bits not at positions divisible by 3\n"
"\n"
"uint Compact1By2(uint x) {\n"
"	x &= 0x09249249;					// x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0\n"
"	x = (x ^ (x >> 2)) & 0x030c30c3;	// x = ---- --98 ---- 76-- --54 ---- 32-- --10\n"
"	x = (x ^ (x >> 4)) & 0x0300f00f;	// x = ---- --98 ---- ---- 7654 ---- ---- 3210\n"
"	x = (x ^ (x >> 8)) & 0xff0000ff;	// x = ---- --98 ---- ---- ---- ---- 7654 3210\n"
"	x = (x ^ (x >> 16)) & 0x000003ff;	// x = ---- ---- ---- ---- ---- --98 7654 3210\n"
"	return x;\n"
"}\n"
"\n"
"uint DecodeMorton2X(const uint code) {\n"
"	return Compact1By1(code >> 0);\n"
"}\n"
"\n"
"uint DecodeMorton2Y(const uint code) {\n"
"	return Compact1By1(code >> 1);\n"
"}\n"
"\n"
"#endif\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Init Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"bool InitSampleResult(\n"
"		__global Sample *sample,\n"
"		__global float *sampleDataPathBase,\n"
"		const uint filmWidth, const uint filmHeight,\n"
"		const uint filmSubRegion0, const uint filmSubRegion1,\n"
"		const uint filmSubRegion2, const uint filmSubRegion3,\n"
"		__global float *pixelFilterDistribution,\n"
"		Seed *seed\n"
"#if defined(RENDER_ENGINE_TILEPATHOCL) || defined(RENDER_ENGINE_RTPATHOCL)\n"
"		, const uint tileStartX, const uint tileStartY\n"
"		, const uint tileWidth, const uint tileHeight\n"
"		, const uint tilePass\n"
"		// aaSamples is always 1 in RENDER_ENGINE_RTPATHOCL\n"
"		, const uint aaSamples\n"
"#endif\n"
"		) {\n"
"	SampleResult_Init(&sample->result);\n"
"\n"
"	const float u0 = Sampler_GetSamplePath(seed, sample, sampleDataPathBase, IDX_SCREEN_X);\n"
"	const float u1 = Sampler_GetSamplePath(seed, sample, sampleDataPathBase, IDX_SCREEN_Y);\n"
"\n"
"	uint pixelX, pixelY;\n"
"	float uSubPixelX, uSubPixelY;\n"
"#if defined(RENDER_ENGINE_RTPATHOCL)\n"
"	// Stratified sampling of the pixel\n"
"	const size_t gid = get_global_id(0);\n"
"\n"
"	if (tilePass < PARAM_RTPATHOCL_PREVIEW_RESOLUTION_REDUCTION_STEP) {\n"
"		const uint samplesPerRow = filmWidth / PARAM_RTPATHOCL_PREVIEW_RESOLUTION_REDUCTION;\n"
"		const uint subPixelX = gid % samplesPerRow;\n"
"		const uint subPixelY = gid / samplesPerRow;\n"
"\n"
"		pixelX = subPixelX * PARAM_RTPATHOCL_PREVIEW_RESOLUTION_REDUCTION;\n"
"		pixelY = subPixelY * PARAM_RTPATHOCL_PREVIEW_RESOLUTION_REDUCTION;\n"
"	} else {\n"
"		const uint samplesPerRow = filmWidth / PARAM_RTPATHOCL_RESOLUTION_REDUCTION;\n"
"		const uint subPixelX = gid % samplesPerRow;\n"
"		const uint subPixelY = gid / samplesPerRow;\n"
"\n"
"		pixelX = subPixelX * PARAM_RTPATHOCL_RESOLUTION_REDUCTION;\n"
"		pixelY = subPixelY * PARAM_RTPATHOCL_RESOLUTION_REDUCTION;\n"
"\n"
"		const uint pixelsCount = PARAM_RTPATHOCL_RESOLUTION_REDUCTION;\n"
"		const uint pixelsCount2 = pixelsCount * pixelsCount;\n"
"\n"
"		// Rendering according a Morton curve\n"
"		const uint pixelIndex = tilePass % pixelsCount2;\n"
"		const uint mortonX = DecodeMorton2X(pixelIndex);\n"
"		const uint mortonY = DecodeMorton2Y(pixelIndex);\n"
"\n"
"		pixelX += mortonX;\n"
"		pixelY += mortonY;\n"
"	}\n"
"\n"
"	if ((pixelX >= tileWidth) || (pixelY >= tileHeight))\n"
"		return false;\n"
"\n"
"	uSubPixelX = u0;\n"
"	uSubPixelY = u1;\n"
"#elif defined(RENDER_ENGINE_TILEPATHOCL)\n"
"	// Stratified sampling of the pixel\n"
"	const size_t gid = get_global_id(0);\n"
"\n"
"	const uint samplesPerRow = filmWidth * aaSamples;\n"
"	const uint subPixelX = gid % samplesPerRow;\n"
"	const uint subPixelY = gid / samplesPerRow;\n"
"	\n"
"	pixelX = subPixelX / aaSamples;\n"
"	pixelY = subPixelY / aaSamples;\n"
"\n"
"	if ((pixelX >= tileWidth) || (pixelY >= tileHeight))\n"
"		return false;\n"
"\n"
"	uSubPixelX = u0;\n"
"	uSubPixelY = u1;\n"
"#else\n"
"	float ux, uy;\n"
"	Film_GetSampleXY(u0, u1, &ux, &uy,\n"
"			filmWidth, filmHeight,\n"
"			filmSubRegion0, filmSubRegion1,\n"
"			filmSubRegion2, filmSubRegion3);\n"
"\n"
"	pixelX = Floor2UInt(ux);\n"
"	pixelY = Floor2UInt(uy);	\n"
"	uSubPixelX = ux - pixelX;\n"
"	uSubPixelY = uy - pixelY;\n"
"#endif\n"
"\n"
"	sample->result.pixelX = pixelX;\n"
"	sample->result.pixelY = pixelY;\n"
"\n"
"	// Sample according the pixel filter distribution\n"
"	float distX, distY;\n"
"	FilterDistribution_SampleContinuous(pixelFilterDistribution, uSubPixelX, uSubPixelY, &distX, &distY);\n"
"\n"
"	sample->result.filmX = pixelX + .5f + distX;\n"
"	sample->result.filmY = pixelY + .5f + distY;\n"
"	\n"
"	return true;\n"
"}\n"
"\n"
"bool GenerateEyePath(\n"
"		__global GPUTaskDirectLight *taskDirectLight,\n"
"		__global GPUTaskState *taskState,\n"
"		__global Sample *sample,\n"
"		__global float *sampleDataPathBase,\n"
"		__global const Camera* restrict camera,\n"
"		const uint filmWidth, const uint filmHeight,\n"
"		const uint filmSubRegion0, const uint filmSubRegion1,\n"
"		const uint filmSubRegion2, const uint filmSubRegion3,\n"
"#if defined(RENDER_ENGINE_TILEPATHOCL) || defined(RENDER_ENGINE_RTPATHOCL)\n"
"		// cameraFilmWidth/cameraFilmHeight and filmWidth/filmHeight are usually\n"
"		// the same. They are different when doing tile rendering\n"
"		const uint cameraFilmWidth, const uint cameraFilmHeight,\n"
"		const uint tileStartX, const uint tileStartY,\n"
"		const uint tileWidth, const uint tileHeight,\n"
"		const uint tilePass,\n"
"		// aaSamples is always 1 in RENDER_ENGINE_RTPATHOCL\n"
"		const uint aaSamples,\n"
"#endif\n"
"		__global float *pixelFilterDistribution,\n"
"		__global Ray *ray,\n"
"		Seed *seed) {\n"
"	const bool validPixel = InitSampleResult(sample, sampleDataPathBase,\n"
"		filmWidth, filmHeight,\n"
"		filmSubRegion0, filmSubRegion1,\n"
"		filmSubRegion2, filmSubRegion3\n"
"		, pixelFilterDistribution\n"
"		, seed\n"
"#if defined(RENDER_ENGINE_TILEPATHOCL) || defined(RENDER_ENGINE_RTPATHOCL)\n"
"		, tileStartX, tileStartY, tileWidth, tileHeight, tilePass\n"
"		, aaSamples\n"
"#endif\n"
"		);\n"
"	\n"
"	if (!validPixel)\n"
"		return false;\n"
"\n"
"	// Generate the came ray\n"
"	const float time = Sampler_GetSamplePath(seed, sample, sampleDataPathBase, IDX_EYE_TIME);\n"
"\n"
"	const float dofSampleX = Sampler_GetSamplePath(seed, sample, sampleDataPathBase, IDX_DOF_X);\n"
"	const float dofSampleY = Sampler_GetSamplePath(seed, sample, sampleDataPathBase, IDX_DOF_Y);\n"
"\n"
"#if defined(RENDER_ENGINE_TILEPATHOCL) || defined(RENDER_ENGINE_RTPATHOCL)\n"
"	Camera_GenerateRay(camera, cameraFilmWidth, cameraFilmHeight,\n"
"			ray,\n"
"			sample->result.filmX + tileStartX, sample->result.filmY + tileStartY,\n"
"			time,\n"
"			dofSampleX, dofSampleY);\n"
"#else\n"
"	Camera_GenerateRay(camera, filmWidth, filmHeight,\n"
"			ray,\n"
"			sample->result.filmX, sample->result.filmY,\n"
"			time,\n"
"			dofSampleX, dofSampleY);\n"
"#endif\n"
"\n"
"	// Initialize the path state\n"
"	taskState->state = MK_RT_NEXT_VERTEX;\n"
"	PathDepthInfo_Init(&taskState->depthInfo);\n"
"	VSTORE3F(WHITE, taskState->throughput.c);\n"
"	taskDirectLight->lastBSDFEvent = SPECULAR; // SPECULAR is required to avoid MIS\n"
"	taskDirectLight->lastPdfW = 1.f;\n"
"\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"	// This is a bit tricky. I store the passThroughEvent in the BSDF\n"
"	// before of the initialization because it can be used during the\n"
"	// tracing of next path vertex ray.\n"
"\n"
"	taskState->bsdf.hitPoint.passThroughEvent = Sampler_GetSamplePath(seed, sample, sampleDataPathBase, IDX_EYE_PASSTHROUGH);\n"
"#endif\n"
"\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_SHADOW_MASK)\n"
"	sample->result.directShadowMask = 1.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SHADOW_MASK)\n"
"	sample->result.indirectShadowMask = 1.f;\n"
"#endif\n"
"\n"
"	sample->result.lastPathVertex = (PARAM_MAX_PATH_DEPTH == 1);\n"
"\n"
"	return true;\n"
"}\n"
"\n"
"__kernel __attribute__((work_group_size_hint(64, 1, 1))) void InitSeed(__global GPUTask *tasks,\n"
"		const uint seedBase) {\n"
"	const size_t gid = get_global_id(0);\n"
"\n"
"	// Initialize random number generator\n"
"\n"
"	Seed seed;\n"
"	Rnd_Init(seedBase + gid, &seed);\n"
"\n"
"	// Save the seed\n"
"	__global GPUTask *task = &tasks[gid];\n"
"	task->seed = seed;\n"
"}\n"
"\n"
"__kernel __attribute__((work_group_size_hint(64, 1, 1))) void Init(\n"
"		const uint filmWidth, const uint filmHeight,\n"
"		const uint filmSubRegion0, const uint filmSubRegion1,\n"
"		const uint filmSubRegion2, const uint filmSubRegion3,\n"
"		__global GPUTask *tasks,\n"
"		__global GPUTaskDirectLight *tasksDirectLight,\n"
"		__global GPUTaskState *tasksState,\n"
"		__global GPUTaskStats *taskStats,\n"
"		__global Sample *samples,\n"
"		__global float *samplesData,\n"
"#if defined(PARAM_HAS_VOLUMES)\n"
"		__global PathVolumeInfo *pathVolInfos,\n"
"#endif\n"
"		__global float *pixelFilterDistribution,\n"
"		__global Ray *rays,\n"
"		__global Camera *camera\n"
"#if defined(RENDER_ENGINE_TILEPATHOCL) || defined(RENDER_ENGINE_RTPATHOCL)\n"
"		// cameraFilmWidth/cameraFilmHeight and filmWidth/filmHeight are usually\n"
"		// the same. They are different when doing tile rendering\n"
"		, const uint cameraFilmWidth, const uint cameraFilmHeight\n"
"		, const uint tileStartX, const uint tileStartY\n"
"		, const uint tileWidth, const uint tileHeight\n"
"		, const uint tilePass, const uint aaSamples\n"
"#endif\n"
"		) {\n"
"	const size_t gid = get_global_id(0);\n"
"\n"
"	__global GPUTaskState *taskState = &tasksState[gid];\n"
"\n"
"#if defined(RENDER_ENGINE_TILEPATHOCL) || defined(RENDER_ENGINE_RTPATHOCL)\n"
"	if (gid >= filmWidth * filmHeight * aaSamples * aaSamples) {\n"
"		taskState->state = MK_DONE;\n"
"		return;\n"
"	}\n"
"#endif\n"
"\n"
"	// Initialize the task\n"
"	__global GPUTask *task = &tasks[gid];\n"
"	__global GPUTaskDirectLight *taskDirectLight = &tasksDirectLight[gid];\n"
"\n"
"	// Read the seed\n"
"	Seed seedValue = task->seed;\n"
"	// This trick is required by Sampler_GetSample() macro\n"
"	Seed *seed = &seedValue;\n"
"\n"
"	// Initialize the sample and path\n"
"	__global Sample *sample = &samples[gid];\n"
"	__global float *sampleData = Sampler_GetSampleData(sample, samplesData);\n"
"	Sampler_Init(seed, sample, sampleData);\n"
"#if defined(RENDER_ENGINE_TILEPATHOCL) || defined(RENDER_ENGINE_RTPATHOCL)\n"
"	sample->currentTilePass = tilePass;\n"
"#endif\n"
"	__global float *sampleDataPathBase = Sampler_GetSampleDataPathBase(sample, sampleData);\n"
"\n"
"	// Generate the eye path\n"
"	const bool validPath = GenerateEyePath(taskDirectLight, taskState, sample, sampleDataPathBase, camera,\n"
"			filmWidth, filmHeight,\n"
"			filmSubRegion0, filmSubRegion1, filmSubRegion2, filmSubRegion3,\n"
"#if defined(RENDER_ENGINE_TILEPATHOCL) || defined(RENDER_ENGINE_RTPATHOCL)\n"
"			cameraFilmWidth, cameraFilmHeight,\n"
"			tileStartX, tileStartY, tileWidth, tileHeight, tilePass,\n"
"			aaSamples,\n"
"#endif\n"
"			pixelFilterDistribution,\n"
"			&rays[gid], seed);\n"
"\n"
"	// Save the seed\n"
"	task->seed = seedValue;\n"
"\n"
"	__global GPUTaskStats *taskStat = &taskStats[gid];\n"
"	taskStat->sampleCount = 0;\n"
"\n"
"	if (!validPath) {\n"
"#if defined(RENDER_ENGINE_TILEPATHOCL) || defined(RENDER_ENGINE_RTPATHOCL)\n"
"		taskState->state = MK_DONE;\n"
"#else\n"
"		taskState->state = MK_GENERATE_CAMERA_RAY;\n"
"#endif\n"
"		// Mark the ray like like one to NOT trace\n"
"		rays[gid].flags = RAY_FLAGS_MASKED;\n"
"		return;\n"
"	}\n"
"\n"
"#if defined(PARAM_HAS_VOLUMES)\n"
"	PathVolumeInfo_Init(&pathVolInfos[gid]);\n"
"#endif\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Utility functions\n"
"//------------------------------------------------------------------------------\n"
"\n"
"#if defined(PARAM_HAS_ENVLIGHTS)\n"
"void DirectHitInfiniteLight(\n"
"		const BSDFEvent lastBSDFEvent,\n"
"		__global const Spectrum* restrict pathThroughput,\n"
"		const float3 eyeDir, const float lastPdfW,\n"
"		__global SampleResult *sampleResult\n"
"		LIGHTS_PARAM_DECL) {\n"
"	const float3 throughput = VLOAD3F(pathThroughput->c);\n"
"\n"
"	for (uint i = 0; i < envLightCount; ++i) {\n"
"		__global const LightSource* restrict light = &lights[envLightIndices[i]];\n"
"\n"
"		float directPdfW;\n"
"		const float3 lightRadiance = EnvLight_GetRadiance(light, -eyeDir, &directPdfW\n"
"				LIGHTS_PARAM);\n"
"\n"
"		if (!Spectrum_IsBlack(lightRadiance)) {\n"
"			// MIS between BSDF sampling and direct light sampling\n"
"			const float weight = ((lastBSDFEvent & SPECULAR) ? 1.f : PowerHeuristic(lastPdfW, directPdfW));\n"
"\n"
"			SampleResult_AddEmission(sampleResult, light->lightID, throughput, weight * lightRadiance);\n"
"		}\n"
"	}\n"
"}\n"
"#endif\n"
"\n"
"void DirectHitFiniteLight(\n"
"		const BSDFEvent lastBSDFEvent,\n"
"		__global const Spectrum* restrict pathThroughput, const float distance, __global BSDF *bsdf,\n"
"		const float lastPdfW, __global SampleResult *sampleResult\n"
"		LIGHTS_PARAM_DECL) {\n"
"	float directPdfA;\n"
"	const float3 emittedRadiance = BSDF_GetEmittedRadiance(bsdf, &directPdfA\n"
"			LIGHTS_PARAM);\n"
"\n"
"	if (!Spectrum_IsBlack(emittedRadiance)) {\n"
"		// Add emitted radiance\n"
"		float weight = 1.f;\n"
"		if (!(lastBSDFEvent & SPECULAR)) {\n"
"			const float lightPickProb = Scene_SampleLightPdf(lightsDistribution,\n"
"					lights[bsdf->triangleLightSourceIndex].lightSceneIndex);\n"
"			const float directPdfW = PdfAtoW(directPdfA, distance,\n"
"				fabs(dot(VLOAD3F(&bsdf->hitPoint.fixedDir.x), VLOAD3F(&bsdf->hitPoint.shadeN.x))));\n"
"\n"
"			// MIS between BSDF sampling and direct light sampling\n"
"			weight = PowerHeuristic(lastPdfW, directPdfW * lightPickProb);\n"
"		}\n"
"\n"
"		SampleResult_AddEmission(sampleResult, BSDF_GetLightID(bsdf\n"
"				MATERIALS_PARAM), VLOAD3F(pathThroughput->c), weight * emittedRadiance);\n"
"	}\n"
"}\n"
"\n"
"float RussianRouletteProb(const float3 color) {\n"
"	return clamp(Spectrum_Filter(color), PARAM_RR_CAP, 1.f);\n"
"}\n"
"\n"
"bool DirectLight_Illuminate(\n"
"		__global BSDF *bsdf,\n"
"		const float worldCenterX,\n"
"		const float worldCenterY,\n"
"		const float worldCenterZ,\n"
"		const float worldRadius,\n"
"		__global HitPoint *tmpHitPoint,\n"
"		const float u0, const float u1, const float u2,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"		const float lightPassThroughEvent,\n"
"#endif\n"
"		const float3 point,\n"
"		__global DirectLightIlluminateInfo *info\n"
"		LIGHTS_PARAM_DECL) {\n"
"	// Select the light strategy to use\n"
"	__global const float* restrict lightDist = BSDF_IsShadowCatcherOnlyInfiniteLights(bsdf MATERIALS_PARAM) ?\n"
"		infiniteLightSourcesDistribution : lightsDistribution;\n"
"\n"
"	// Pick a light source to sample\n"
"	float lightPickPdf;\n"
"	const uint lightIndex = Scene_SampleLights(lightDist, u0, &lightPickPdf);\n"
"	if (lightPickPdf <= 0.f)\n"
"		return false;\n"
"\n"
"	__global const LightSource* restrict light = &lights[lightIndex];\n"
"\n"
"	info->lightIndex = lightIndex;\n"
"	info->lightID = light->lightID;\n"
"	info->pickPdf = lightPickPdf;\n"
"\n"
"	// Illuminate the point\n"
"	float3 lightRayDir;\n"
"	float distance, directPdfW;\n"
"	const float3 lightRadiance = Light_Illuminate(\n"
"			&lights[lightIndex],\n"
"			point,\n"
"			u1, u2,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"			lightPassThroughEvent,\n"
"#endif\n"
"			worldCenterX, worldCenterY, worldCenterZ, worldRadius,\n"
"			tmpHitPoint,		\n"
"			&lightRayDir, &distance, &directPdfW\n"
"			LIGHTS_PARAM);\n"
"	\n"
"	if (Spectrum_IsBlack(lightRadiance))\n"
"		return false;\n"
"	else {\n"
"		VSTORE3F(lightRayDir, &info->dir.x);\n"
"		info->distance = distance;\n"
"		info->directPdfW = directPdfW;\n"
"		VSTORE3F(lightRadiance, info->lightRadiance.c);\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_IRRADIANCE)\n"
"		VSTORE3F(lightRadiance, info->lightIrradiance.c);\n"
"#endif\n"
"		return true;\n"
"	}\n"
"}\n"
"\n"
"bool DirectLight_BSDFSampling(\n"
"		__global DirectLightIlluminateInfo *info,\n"
"		const float time,\n"
"		const bool lastPathVertex, const uint pathVertexCount,\n"
"		__global BSDF *bsdf,\n"
"		__global Ray *shadowRay\n"
"		LIGHTS_PARAM_DECL) {\n"
"	const float3 lightRayDir = VLOAD3F(&info->dir.x);\n"
"	\n"
"	// Sample the BSDF\n"
"	BSDFEvent event;\n"
"	float bsdfPdfW;\n"
"	const float3 bsdfEval = BSDF_Evaluate(bsdf,\n"
"			lightRayDir, &event, &bsdfPdfW\n"
"			MATERIALS_PARAM);\n"
"\n"
"	if (Spectrum_IsBlack(bsdfEval))\n"
"		return false;\n"
"\n"
"	const float cosThetaToLight = fabs(dot(lightRayDir, VLOAD3F(&bsdf->hitPoint.shadeN.x)));\n"
"	const float directLightSamplingPdfW = info->directPdfW * info->pickPdf;\n"
"	const float factor = 1.f / directLightSamplingPdfW;\n"
"\n"
"	// Russian Roulette\n"
"	// The +1 is there to account the current path vertex used for DL\n"
"	bsdfPdfW *= (pathVertexCount + 1 >= PARAM_RR_DEPTH) ? RussianRouletteProb(bsdfEval) : 1.f;\n"
"\n"
"	// MIS between direct light sampling and BSDF sampling\n"
"	//\n"
"	// Note: I have to avoiding MIS on the last path vertex\n"
"	__global const LightSource* restrict light = &lights[info->lightIndex];\n"
"	const float weight = (!lastPathVertex && Light_IsEnvOrIntersectable(light)) ?\n"
"		PowerHeuristic(directLightSamplingPdfW, bsdfPdfW) : 1.f;\n"
"\n"
"	const float3 lightRadiance = VLOAD3F(info->lightRadiance.c);\n"
"	VSTORE3F(bsdfEval * (weight * factor) * lightRadiance, info->lightRadiance.c);\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_IRRADIANCE)\n"
"	VSTORE3F(factor * lightRadiance, info->lightIrradiance.c);\n"
"#endif\n"
"\n"
"	// Setup the shadow ray\n"
"	const float3 hitPoint = VLOAD3F(&bsdf->hitPoint.p.x);\n"
"	const float distance = info->distance;\n"
"	Ray_Init4(shadowRay, hitPoint, lightRayDir, 0.f, distance, time);\n"
"\n"
"	return true;\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Kernel parameters\n"
"//------------------------------------------------------------------------------\n"
"\n"
"#if defined(PARAM_HAS_VOLUMES)\n"
"#define KERNEL_ARGS_VOLUMES \\\n"
"		, __global PathVolumeInfo *pathVolInfos \\\n"
"		, __global PathVolumeInfo *directLightVolInfos\n"
"#else\n"
"#define KERNEL_ARGS_VOLUMES\n"
"#endif\n"
"\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_0)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_0 \\\n"
"		, __global float *filmRadianceGroup0\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_0\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_1)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_1 \\\n"
"		, __global float *filmRadianceGroup1\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_1\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_2)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_2 \\\n"
"		, __global float *filmRadianceGroup2\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_2\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_3)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_3 \\\n"
"		, __global float *filmRadianceGroup3\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_3\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_4)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_4 \\\n"
"		, __global float *filmRadianceGroup4\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_4\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_5)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_5 \\\n"
"		, __global float *filmRadianceGroup5\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_5\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_6)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_6 \\\n"
"		, __global float *filmRadianceGroup6\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_6\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_7)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_7 \\\n"
"		, __global float *filmRadianceGroup7\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_7\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_ALPHA)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_ALPHA \\\n"
"		, __global float *filmAlpha\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_ALPHA\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DEPTH)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DEPTH \\\n"
"		, __global float *filmDepth\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DEPTH\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_POSITION)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_POSITION \\\n"
"		, __global float *filmPosition\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_POSITION\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_GEOMETRY_NORMAL)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_GEOMETRY_NORMAL \\\n"
"		, __global float *filmGeometryNormal\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_GEOMETRY_NORMAL\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_SHADING_NORMAL)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_SHADING_NORMAL \\\n"
"		, __global float *filmShadingNormal\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_SHADING_NORMAL\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_MATERIAL_ID)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_MATERIAL_ID \\\n"
"		, __global uint *filmMaterialID\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_MATERIAL_ID\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_DIFFUSE)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DIRECT_DIFFUSE \\\n"
"		, __global float *filmDirectDiffuse\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DIRECT_DIFFUSE\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_GLOSSY)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DIRECT_GLOSSY \\\n"
"		, __global float *filmDirectGlossy\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DIRECT_GLOSSY\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_EMISSION)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_EMISSION \\\n"
"		, __global float *filmEmission\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_EMISSION\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_DIFFUSE)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_DIFFUSE \\\n"
"		, __global float *filmIndirectDiffuse\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_DIFFUSE\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_GLOSSY)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_GLOSSY \\\n"
"		, __global float *filmIndirectGlossy\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_GLOSSY\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SPECULAR)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_SPECULAR \\\n"
"		, __global float *filmIndirectSpecular\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_SPECULAR\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_MATERIAL_ID_MASK)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_MATERIAL_ID_MASK \\\n"
"		, __global float *filmMaterialIDMask\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_MATERIAL_ID_MASK\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_SHADOW_MASK)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DIRECT_SHADOW_MASK \\\n"
"		, __global float *filmDirectShadowMask\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DIRECT_SHADOW_MASK\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SHADOW_MASK)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_SHADOW_MASK \\\n"
"		, __global float *filmIndirectShadowMask\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_SHADOW_MASK\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_UV)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_UV \\\n"
"		, __global float *filmUV\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_UV\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_RAYCOUNT)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_RAYCOUNT \\\n"
"		, __global float *filmRayCount\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_RAYCOUNT\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_BY_MATERIAL_ID)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_BY_MATERIAL_ID \\\n"
"		, __global float *filmByMaterialID\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_BY_MATERIAL_ID\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_IRRADIANCE)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_IRRADIANCE \\\n"
"		, __global float *filmIrradiance\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_IRRADIANCE\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_OBJECT_ID)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_OBJECT_ID \\\n"
"		, __global uint *filmObjectID\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_OBJECT_ID\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_OBJECT_ID_MASK)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_OBJECT_ID_MASK \\\n"
"		, __global float *filmObjectIDMask\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_OBJECT_ID_MASK\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_BY_OBJECT_ID)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_BY_OBJECT_ID \\\n"
"		, __global float *filmByObjectID\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_BY_OBJECT_ID\n"
"#endif\n"
"\n"
"#define KERNEL_ARGS_FILM \\\n"
"		, const uint filmWidth, const uint filmHeight \\\n"
"		, const uint filmSubRegion0, const uint filmSubRegion1 \\\n"
"		, const uint filmSubRegion2, const uint filmSubRegion3 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_0 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_1 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_2 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_3 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_4 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_5 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_6 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_7 \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_ALPHA \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_DEPTH \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_POSITION \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_GEOMETRY_NORMAL \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_SHADING_NORMAL \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_MATERIAL_ID \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_DIRECT_DIFFUSE \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_DIRECT_GLOSSY \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_EMISSION \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_INDIRECT_DIFFUSE \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_INDIRECT_GLOSSY \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_INDIRECT_SPECULAR \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_MATERIAL_ID_MASK \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_DIRECT_SHADOW_MASK \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_INDIRECT_SHADOW_MASK \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_UV \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_RAYCOUNT \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_BY_MATERIAL_ID \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_IRRADIANCE \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_OBJECT_ID \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_OBJECT_ID_MASK \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_BY_OBJECT_ID\n"
"\n"
"#define KERNEL_ARGS_INFINITELIGHTS \\\n"
"		, const float worldCenterX \\\n"
"		, const float worldCenterY \\\n"
"		, const float worldCenterZ \\\n"
"		, const float worldRadius\n"
"\n"
"#define KERNEL_ARGS_NORMALS_BUFFER \\\n"
"		, __global const Vector* restrict vertNormals\n"
"#define KERNEL_ARGS_UVS_BUFFER \\\n"
"		, __global const UV* restrict vertUVs\n"
"#define KERNEL_ARGS_COLS_BUFFER \\\n"
"		, __global const Spectrum* restrict vertCols\n"
"#define KERNEL_ARGS_ALPHAS_BUFFER \\\n"
"		, __global const float* restrict vertAlphas\n"
"\n"
"#define KERNEL_ARGS_ENVLIGHTS \\\n"
"		, __global const uint* restrict envLightIndices \\\n"
"		, const uint envLightCount\n"
"\n"
"#define KERNEL_ARGS_INFINITELIGHT \\\n"
"		, __global const float* restrict infiniteLightDistribution\n"
"\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_0)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_0 \\\n"
"		, __global const ImageMap* restrict imageMapDescs, __global const float* restrict imageMapBuff0\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_0\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_1)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_1 \\\n"
"		, __global const float* restrict imageMapBuff1\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_1\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_2)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_2 \\\n"
"		, __global const float* restrict imageMapBuff2\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_2\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_3)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_3 \\\n"
"		, __global const float* restrict imageMapBuff3\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_3\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_4)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_4 \\\n"
"		, __global const float* restrict imageMapBuff4\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_4\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_5)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_5 \\\n"
"		, __global const float* restrict imageMapBuff5\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_5\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_6)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_6 \\\n"
"		, __global const float* restrict imageMapBuff6\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_6\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_7)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_7 \\\n"
"		, __global const float* restrict imageMapBuff7\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_7\n"
"#endif\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGES \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_0 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_1 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_2 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_3 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_4 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_5 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_6 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_7\n"
"\n"
"#define KERNEL_ARGS_FAST_PIXEL_FILTER \\\n"
"		, __global float *pixelFilterDistribution\n"
"\n"
"#define KERNEL_ARGS \\\n"
"		__global GPUTask *tasks \\\n"
"		, __global GPUTaskDirectLight *tasksDirectLight \\\n"
"		, __global GPUTaskState *tasksState \\\n"
"		, __global GPUTaskStats *taskStats \\\n"
"		KERNEL_ARGS_FAST_PIXEL_FILTER \\\n"
"		, __global Sample *samples \\\n"
"		, __global float *samplesData \\\n"
"		KERNEL_ARGS_VOLUMES \\\n"
"		, __global Ray *rays \\\n"
"		, __global RayHit *rayHits \\\n"
"		/* Film parameters */ \\\n"
"		KERNEL_ARGS_FILM \\\n"
"		/* Scene parameters */ \\\n"
"		KERNEL_ARGS_INFINITELIGHTS \\\n"
"		, __global const Material* restrict mats \\\n"
"		, __global const Texture* restrict texs \\\n"
"		, __global const SceneObject* restrict sceneObjs \\\n"
"		, __global const Mesh* restrict meshDescs \\\n"
"		, __global const Point* restrict vertices \\\n"
"		KERNEL_ARGS_NORMALS_BUFFER \\\n"
"		KERNEL_ARGS_UVS_BUFFER \\\n"
"		KERNEL_ARGS_COLS_BUFFER \\\n"
"		KERNEL_ARGS_ALPHAS_BUFFER \\\n"
"		, __global const Triangle* restrict triangles \\\n"
"		, __global const Camera* restrict camera \\\n"
"		/* Lights */ \\\n"
"		, __global const LightSource* restrict lights \\\n"
"		KERNEL_ARGS_ENVLIGHTS \\\n"
"		, __global const uint* restrict meshTriLightDefsOffset \\\n"
"		KERNEL_ARGS_INFINITELIGHT \\\n"
"		, __global const float* restrict lightsDistribution \\\n"
"		, __global const float* restrict infiniteLightSourcesDistribution \\\n"
"		/* Images */ \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGES\n"
"\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// To initialize image maps page pointer table\n"
"//------------------------------------------------------------------------------\n"
"\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_0)\n"
"#define INIT_IMAGEMAPS_PAGE_0 imageMapBuff[0] = imageMapBuff0;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_0\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_1)\n"
"#define INIT_IMAGEMAPS_PAGE_1 imageMapBuff[1] = imageMapBuff1;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_1\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_2)\n"
"#define INIT_IMAGEMAPS_PAGE_2 imageMapBuff[2] = imageMapBuff2;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_2\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_3)\n"
"#define INIT_IMAGEMAPS_PAGE_3 imageMapBuff[3] = imageMapBuff3;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_3\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_4)\n"
"#define INIT_IMAGEMAPS_PAGE_4 imageMapBuff[4] = imageMapBuff4;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_4\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_5)\n"
"#define INIT_IMAGEMAPS_PAGE_5 imageMapBuff[5] = imageMapBuff5;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_5\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_6)\n"
"#define INIT_IMAGEMAPS_PAGE_6 imageMapBuff[6] = imageMapBuff6;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_6\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_7)\n"
"#define INIT_IMAGEMAPS_PAGE_7 imageMapBuff[7] = imageMapBuff7;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_7\n"
"#endif\n"
"\n"
"#if defined(PARAM_HAS_IMAGEMAPS)\n"
"#define INIT_IMAGEMAPS_PAGES \\\n"
"	__global const float* restrict imageMapBuff[PARAM_IMAGEMAPS_COUNT]; \\\n"
"	INIT_IMAGEMAPS_PAGE_0 \\\n"
"	INIT_IMAGEMAPS_PAGE_1 \\\n"
"	INIT_IMAGEMAPS_PAGE_2 \\\n"
"	INIT_IMAGEMAPS_PAGE_3 \\\n"
"	INIT_IMAGEMAPS_PAGE_4 \\\n"
"	INIT_IMAGEMAPS_PAGE_5 \\\n"
"	INIT_IMAGEMAPS_PAGE_6 \\\n"
"	INIT_IMAGEMAPS_PAGE_7\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGES\n"
"#endif\n"
; } }