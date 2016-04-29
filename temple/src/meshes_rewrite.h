
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <graphics/math.h>

class AasClass2;
class AasRunningStream;
struct SkaAnimHeader;

using MaterialRef = int;
using MaterialResolver = std::function<MaterialRef(const char*)>;

class AasRunningAnim {
public:
	AasRunningAnim(AasClass2& owner, SkaAnimHeader& animHeader);

private:

	float mDuration = 0;
	std::unique_ptr<AasRunningStream> mStream;

	// These were part of the baseclass
	AasClass2& owner;
	int boneIdx = -2;
	AasRunningAnim* nextRunningAnim = nullptr;
	AasRunningAnim* prevRunningAnim = nullptr;
	uint8_t field_C = 1;
	uint8_t field_D = 0;
	int elapsedTimeRelated = 0;
	int field_14 = 0x40000000;
	int eventHandlingDepth = 0;

	// Part of the subclass
	int streamCount = 0;
	int currentTime = 0;
	int field_58 = 0;

	SkaAnimHeader& mAnimHeader;

};

#pragma pack(push, 1)
struct SkaAnimStream {
	uint16_t frames;
	int16_t variationId;
	float frameRate;
	float dps;
	// Offset to the start of the key-frame stream in relation to animationStart
	uint32_t dataOffset;
};

struct SkaAnimHeader {
	char name[64];
	uint8_t driveType;
	uint8_t loopable;
	uint16_t eventCount;
	uint32_t eventOffset;
	uint32_t streamCount;
	SkaAnimStream streams[10];
};
#pragma pack(pop)

class SkaBone {
public:
	explicit SkaBone(const uint8_t* data) : mData(data) {
	}

	uint16_t GetFlags() const {
		return *(uint16_t*)(mData);
	}

	int16_t GetParentId() const {
		return *(uint16_t*)(mData + 2);
	}

	const char* GetName() const {
		return reinterpret_cast<const char*>(mData + 4);
	}

private:
	const uint8_t* mData;
};

class SkaFile {
public:
	explicit SkaFile(const std::string& filename);

	size_t GetBoneCount() const {
		return Read<uint32_t>(0);
	}

	size_t GetAnimCount() const {
		return Read<uint32_t>(16);
	}

	SkaBone GetBone(size_t id) const {
		auto boneData = mData.data() + GetBoneDataStart() + id * 0x64;
		return SkaBone(boneData);
	}

	SkaAnimHeader& GetAnim(size_t id) const {
		auto animData = mData.data() + GetAnimDataStart();
		return *(SkaAnimHeader*)(animData + id * sizeof(SkaAnimHeader));
	}

private:

	template <typename T>
	T Read(uint32_t offset) const {
		return *reinterpret_cast<const T*>(mData.data() + offset);
	}

	size_t GetBoneDataStart() const {
		return Read<uint32_t>(4);
	}

	size_t GetAnimDataStart() const {
		return Read<uint32_t>(20);
	}

	std::vector<uint8_t> mData;

};

#pragma pack(push, 1)
struct SkmVertex {
	static constexpr size_t sMaxBoneAttachments = 6;
	XMFLOAT4 pos;
	XMFLOAT4 normal;
	XMFLOAT2 uv;
	uint16_t padding;
	uint16_t attachmentCount;
	uint16_t attachmentBone[sMaxBoneAttachments];
	float attachmentWeight[sMaxBoneAttachments];
};

struct SkmMaterial {
	char id[128];
};
#pragma pack(pop)

class SkmFile {
public:
	explicit SkmFile(const std::string& filename);

	uint32_t GetBoneCount() const {
		return Read<uint32_t>(0);
	}

	uint32_t GetMaterialCount() const {
		return Read<uint32_t>(8);
	}

	uint32_t GetVertexCount() const {
		return Read<uint32_t>(16);
	}

	uint32_t GetFaceCount() const {
		return Read<uint32_t>(24);
	}

	SkmVertex& GetVertex(uint32_t vertexId) const {
		return ((SkmVertex*)GetVertexData())[vertexId];
	}

	SkmMaterial& GetMaterial(uint32_t materialId) const {
		return ((SkmMaterial*)GetMaterialData())[materialId];
	}

private:
	std::vector<uint8_t> mData;

	const uint8_t* GetBoneData() const {
		return mData.data() + Read<uint32_t>(4);
	}

	const uint8_t* GetMaterialData() const {
		return mData.data() + Read<uint32_t>(12);
	}

	const uint8_t* GetVertexData() const {
		return mData.data() + Read<uint32_t>(20);
	}

	const uint8_t* GetFaceData() const {
		return mData.data() + Read<uint32_t>(28);
	}

	template <typename T>
	T Read(uint32_t offset) const {
		return *reinterpret_cast<const T*>(mData.data() + offset);
	}
};

struct AasClothSphere {
	int boneId;
	float param1;
	XMFLOAT4X3A someMatrix;
	XMFLOAT4X3A someMatrix2;
};

struct AasClothCylinder {
	int boneId;
	float param1;
	float param2;
	XMFLOAT4X3A someMatrix;
	XMFLOAT4X3A someMatrix2;
};

class AasEventListener {
public:
	virtual ~AasEventListener();
};

struct AasSubmesh {

	MaterialRef material;

	void* field_30 = nullptr; // cloth related
	void* field_34 = nullptr; // cloth related

	void* field_10 = nullptr;
	void* field_14 = nullptr;
	void* field_18 = nullptr;

	XMFLOAT3* positions = nullptr;
	XMFLOAT3* normals = nullptr;
	XMFLOAT2* uv = nullptr;
	uint16_t* indices = nullptr;
};

class AasClass2 {
public:
	explicit AasClass2(const SkaFile& skaFile, SkmFile& skmFile, AasEventListener& eventListener);

	void LoadMesh(MaterialResolver matResolver);

	void ResetSubmeshes();

	bool SetAnim(const std::string& name);

private:
	void InitializeBones();

	bool AddRunningAnim(size_t animId);

	AasSubmesh& GetOrCreateSubmeshForMaterial(MaterialRef material);

	AasEventListener& mEventListener;

	const SkaFile& mSkeleton;
	SkmFile& mMesh;

	float scale = 1.0f;
	float scaleInv = 1.0f;

	int field_C = 0; // TODO: Check LOBYTE access pattern

	std::vector<AasSubmesh> mSubmeshes;

	int submeshCount = 0; // TODO: Check hi/lo access pattern
	int submshes = 0;

	int newestRunningAnim = 0;
	int runningAnimsHead = 0;

	XMFLOAT4X3A mWorldMatrix;

	std::vector<XMFLOAT4X3A> mBoneMatrices;

	void* eventListener = nullptr;

	bool mHasClothBones = false;

	size_t mNormalBoneCount = 0;

	std::vector<AasClothSphere> mClothSpheres;
	std::vector<AasClothCylinder> mClothCylinders;

	int field_68 = 0;
	int timeRel1 = 0;
	int timeRel2 = 0;
	int drivenDistance = 0;
	int drivenRotation = 0;
};

class AasModelAndSkeleton {
public:

	AasModelAndSkeleton(const std::shared_ptr<SkaFile>& skaFile,
		const std::shared_ptr<SkmFile>& skmFile,
		AasEventListener& eventListener,
		gfx::EncodedAnimId animId);

	float floatConst = 6.394000053405762f;

	std::shared_ptr<SkaFile> skaFile;
	std::shared_ptr<SkmFile> skmFile;

	AasEventListener& mEventListener;

	AasClass2 aas2;

	void SetAnimId(gfx::EncodedAnimId animId);

	std::chrono::time_point<std::chrono::steady_clock> timeLoaded;

	std::string skaFilename;
	std::string skmFilename;

	gfx::EncodedAnimId mAnimId;

};
