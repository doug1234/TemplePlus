
#pragma once

#include <cstdint>

namespace temple {

	enum AasStatus : uint32_t {
		AAS_OK = 0,
		AAS_ERROR = 1
	};

	/*
	Bitfield that indicates once-per-animation events that happened
	during advancing the animation's time.
	*/
	enum AasEventFlag : uint32_t {
		AEF_NONE = 0,
		AEF_ACTION = 1,
		AEF_END = 2
	};

#pragma pack(push, 1)
	struct AasSubmesh {
		int field_0;
		int vertexCount;
		int primCount;
		float* positions;
		float* normals;
		float* uv;
		uint16_t* indices;
	};
	
	/*
	Configuration for the overall system
	*/
	typedef AasStatus(*FnAasGetFilename)(int meshId, char* filenameOut);
	typedef AasStatus(*FnAasGetAnimName)(int animid, char* nameOut);
	typedef void(*FnAasRunScript)(const char* script);

	struct LegacyAasConfig {
		float scaleX = 28.284271f;
		float scaleY = 28.284271f;
		FnAasGetFilename getSkaFilename = nullptr;
		FnAasGetFilename getSkmFilename = nullptr;
		FnAasGetAnimName getAnimName = nullptr;
		void* mapSpecialAnimId = nullptr; // TODO
		void* field18 = nullptr; // TODO
		FnAasRunScript runScript = nullptr;
	};

#pragma pack(pop)

}

