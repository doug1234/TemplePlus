
#pragma once

#include <infrastructure/macros.h>
#include <temple/aas.h>
#include <temple/dll.h>
#include <temple/meshes.h>
#include <graphics/math.h>

namespace aas {
	
	/*
	Legacy structures found in the DLL.
	*/
	struct AasMatrix {
		float m[3][4];

		static const AasMatrix sIdentity;
	};

	inline void XM_CALLCONV StoreAasMatrix(AasMatrix *target,
		DirectX::FXMMATRIX src) {

		using namespace DirectX;

		static XMFLOAT4X4A sTmpMatrix;
		XMStoreFloat4x4A(&sTmpMatrix, XMMatrixTranspose(src));

		memcpy(target, &sTmpMatrix, sizeof(AasMatrix));
	}

	struct IEventListener {
		virtual void Handle() = 0;
		virtual ~IEventListener() = 0;

		temple::AasEventFlag *mOutput = nullptr;
		void SetOutput(temple::AasEventFlag *output) {
			mOutput = output;
			*output = (temple::AasEventFlag)0;
		}

		void ClearOutput() { mOutput = nullptr; }
	};

	struct IMaterialResolver {
		virtual uint32_t ResolveMaterial(const char *name, int unk1, int unk2) = 0;
		virtual void
			UnknownFunc() = 0; // Probably unused since it's a nullsub in ToEE
	};

	struct SKABone {
		uint8_t flags;
		uint16_t parentId;
		char name[64];
		XMFLOAT3 scale;
		XMFLOAT4 orient;
		XMFLOAT3 offset;
	};

	struct SKAVariation {

	};

	enum class DriveType : uint8_t {
		TimeDriven = 0,
		DistanceDriven,
		RotationDriven
	};

	struct SKAAnimStream {
		uint16_t frame_count;
		int16_t variationId;
		float framerate;
		float distancePerSecond;
		uint32_t dataStart; // From start of animation
	};

	struct SKAAnim {
		char name[64];
		DriveType driveType;
		uint8_t loopable;
		unsigned char event_count;
		int event_offset;
		unsigned short stream_count;
		SKAAnimStream streams[10];
	};

	static_assert(temple::validate_size<SKAAnim, 0xEC>::value,
		"SKAAnim has incorrect size");

	struct SKAFile {

		size_t bonesCount;
		size_t bonesStart;

		size_t variationCount;
		size_t variationStart;

		size_t animsCount;
		size_t animsStart;

		const SKABone &GetBone(size_t index) const {
			Expects(index < bonesCount);
			auto& bone = ((SKABone*)(GetData() + bonesStart))[index];
			Expects(bone.parentId >= -1 && bone.parentId < bonesCount);
			return bone;
		}

		const SKAAnim &GetAnim(size_t index) const {
			Expects(index < animsCount);
			return ((SKAAnim*)(GetData() + animsStart))[index];
		}

	private:

		const char* GetData() const {
			return reinterpret_cast<const char*>(this);
		}

	};

	struct SKMFile {};

	struct SKAAnimation {};

	struct SKAEvent {};

	struct AnimatedModel;

	struct AnimPlayerBase {
		virtual ~AnimPlayerBase() = 0;

		AnimatedModel *ownerAnim;
		int boneIdx;
		char field_C[4];
		float elapsedTimeRelated;
		uint32_t field_14;
		int eventHandlingDepth;
		AnimPlayerBase *nextPlayer;
		AnimPlayerBase *prevPlayer;
	};

	struct AnimStream {};

	struct AnimPlayer : AnimPlayerBase {

		static constexpr auto MaxStreams = 4;

		virtual ~AnimPlayer() = 0;
		virtual float GetDistPerSec() = 0;
		virtual float GetRotationPerSec() = 0;
		virtual void AdvanceEvents() = 0;
		virtual void AddTime() = 0;
		virtual void Method6() = 0;
		virtual void SetTime() = 0;
		virtual void GetCurrentFrame() = 0;
		virtual void Method9() = 0;
		virtual void Method10() = 0;
		virtual void EnterEventHandling() = 0;
		virtual void LeaveEventHandling() = 0;
		virtual void GetEventHandlingDepth() = 0;

		SKAAnimation *skaAnimation;
		int streamCount;
		AnimStream *streams[MaxStreams];
		float streamFps[MaxStreams];
		uint8_t field_4c[MaxStreams];
		int field_50;
		int8_t variationId[MaxStreams];
		int field_58;
		float currentTime;
		float duration;
		float frameRate;
		float distancePerSecond;
		IEventListener *eventListener;
		int eventCount;
		SKAEvent *events;
	};

	static_assert(temple::validate_size<AnimPlayer, 0x78>::value,
		"AASRunningAnim has incorrect size");

	static constexpr auto MaxVariations = 5;

	struct VariationSelector {
		int variationId;
		float propability;
	};

	struct ClothSphere {
		int boneId;
		int param1;
		XMFLOAT4X3 someMatrix;
		XMFLOAT4X3 someMatrix2;
		ClothSphere *next;
	};

	struct ClothCylinder {
		int boneId;
		int param1;
		int param2;
		XMFLOAT4X3 someMatrix1;
		XMFLOAT4X3 someMatrix2;
		ClothCylinder *next;
	};

	struct ClothStuff {
		uint32_t field_0;
		XMFLOAT4 *clothVertexPos1;
		XMFLOAT4 *clothVertexPos2;
		XMFLOAT4 *clothVertexPos3;
		uint8_t *bytePerVertex;
		uint32_t field_14;
		uint32_t field_18;
		uint32_t field_1C;
		uint32_t field_20;
		uint32_t field_24;
		uint32_t field_28;
		ClothSphere *spheres;
		ClothCylinder *cylinders;
		uint32_t field_34;
	};

	struct ClothStuff1 {
		SKMFile *skmFile;
		uint32_t clothVertexCount;
		uint16_t *vertexIdxForClothVertexIdx;
		uint8_t *bytePerClothVertex;
		uint8_t *bytePerClothVertex2;
		ClothStuff *clothStuff;
		uint32_t *field_18;
	};

	struct Submesh {
		int materialId;
		void *matResolver;
		uint16_t field_8;
		uint16_t vertexCount;
		uint16_t primCount;
		uint16_t field_E;
		uint32_t field_10;
		uint32_t field_14;
		uint32_t field_18;
		XMFLOAT2 *uv;
		XMFLOAT4 *positions;
		XMFLOAT4 *normals;
		uint16_t field_28;
		uint16_t field_2A;
		uint32_t field_2C;
		uint32_t field_30;
		uint32_t field_34;
		uint16_t *indices;
		uint16_t countForOtherSkmFile_;
		uint16_t countForOtherSkmFile;
		SKMFile *otherSkmFile;
		int *otherMaterialId;
	};

	// Pure virtual interface to generate a compatible vtable API for AasClass2
	struct AnimatedModel {
		virtual ~AnimatedModel() = 0;
		virtual int SetSkaFile(const SKAFile *skaData, int unk1, int unk2) = 0;
		virtual void SetScale(float scale) = 0;
		virtual int GetBoneCount() = 0;
		virtual char *GetBoneName(int idx) = 0;
		virtual int GetBoneParentId(int idx) = 0;
		virtual int SetSkmFile(SKMFile *skmData, IMaterialResolver *matResolver,
			void *matResolverArg) = 0;
		virtual int FreeAddMeshStuffMaybe(SKMFile *) = 0;
		virtual int SetEventListener(IEventListener *eventListener) = 0;
		virtual void ResetSubmeshes() = 0;
		virtual void Method11() = 0;
		virtual void SetClothFlagSth() = 0;
		virtual void GetSubmeshes(int *submeshCountCount,
			int *submeshMaterialsOut) = 0;
		// Always returns -1
		virtual void Method14() = 0;
		virtual int HasAnimation(char *anim) = 0;
		virtual int SetAnimIdx(size_t animIdx) = 0;
		virtual void Advance(const AasMatrix *worldMatrix, float deltaTime,
			float deltaDistance, float deltaRotation) = 0;
		virtual void SetWorldMatrix(const AasMatrix *worldMatrix) = 0;
		// This may be "update bone matrices"
		virtual void Method19() = 0;
		virtual void SetTime(float time, const AasMatrix *worldMatrix) = 0;
		virtual void Method21() = 0;
		virtual void GetSubmesh(int submeshIdx, int *vertexCountOut, float **posOut,
			float **normalsOut, float **uvOut, int *primCountOut,
			uint16_t **indicesOut) = 0;
		virtual void AddPlayer() = 0;
		virtual void RemovePlayer(AnimPlayer *player) = 0;
		// Returns some part of the SKA Data (@0x4)
		virtual void GetAnimCount() = 0;
		virtual void GetAnimName() = 0;
		virtual bool HasBone(const char *boneName) = 0;
		virtual void AddReplacementMaterial() = 0;
		virtual void GetDistPerSec() = 0;
		virtual void GetRotationPerSec() = 0;

		AasMatrix *GetBoneWorldMatrix(AasMatrix *matrixOut, const char *boneName) {
			static auto aas_class2_GetBoneMatrix =
				(AasMatrix * (__thiscall *)(AnimatedModel *, AasMatrix *, const char *))
				temple::GetPointer<void>(0x10268910);
			return aas_class2_GetBoneMatrix(this, matrixOut, boneName);
		}

		// Call original GetSubmeshes function regardless of vtable
		void GetSubmeshesOrg(int *submeshCountOut, int *materialsOut) {
			using GetSubmeshesOrgFn = void(__thiscall *)(
				AnimatedModel * pThis, int *countOut, int *materialIdsOut);
			static auto getSubmeshesOrg =
				(GetSubmeshesOrgFn)temple::GetPointer<void>(0x102660E0);
			getSubmeshesOrg(this, submeshCountOut, materialsOut);
		}

		/**
		* Searches for the animation by the given name (case-insensitive)
		* and add it to the model.
		*/
		bool SetAnim(const std::string &name) {
			if (!skaData) {
				return false;
			}

			for (size_t i = 0; i < skaData->animsCount; i++) {
				auto &anim = skaData->GetAnim(i);

				if (!_stricmp(anim.name, name.c_str())) {
					return SetAnimIdx(i) != -1;
				}
			}

			return false;
		}

		// Data fields
		AnimPlayer *runningAnimsHead;
		AnimPlayer *newestRunningAnim;
		bool submeshesValid;
		float scale;
		float scaleInv;
		SKAFile *skaData;
		uint32_t variationCount;
		VariationSelector variations[8];
		bool hasClothBones;
		uint32_t clothBoneId;
		uint32_t clothStuff1Count;
		ClothStuff1 *clothStuff1;
		ClothSphere *clothSpheresHead;
		ClothCylinder *clothCylindersHead;
		IEventListener *eventListener;
		uint16_t submeshCount;
		uint16_t field_7E;
		Submesh *submeshes;
		float timeRel1;
		float timeRel2;
		float drivenDistance;
		float drivenRotation;
		AasMatrix someMatrix;
		AasMatrix worldMatrix;
		AasMatrix *boneMatrices;
		uint32_t field_F8;
		uint32_t field_FC;
	};

	static_assert(temple::validate_size<AnimatedModel, 0x100>::value,
		"AnimatedModel has incorrect size");

	enum AnimSlotFlags {
		ASF_FREE = 0x1
	};

	struct AnimSlot {
		temple::AasHandle id;
		gfx::EncodedAnimId animId;
		uint32_t flags = ASF_FREE;
		float floatConst;
		uint32_t timeLoaded;
		AnimatedModel *model;
		SKMFile *skmFile;
		const SKAFile *skaFile;
		uint32_t addMeshCount;
		SKMFile *addMeshData[32];
		char *addMeshNames[32];
		char *skaFilename;
		char *skmFilename;

		bool IsFree() const {
			return (flags & ASF_FREE) == ASF_FREE;
		}
	};

	static_assert(temple::validate_size<AnimSlot, 0x12C>::value,
		"AnimSlot has incorrect size");

	struct SKACacheEntry {
		SKACacheEntry() = default;

		std::string filename;
		uint32_t refCount = 0; // Should be replaced by a shared_ptr later on
		std::vector<uint8_t> skaData;
		const SKAFile *skaFilePtr; // Points to skaData[0]
		NO_COPY_OR_MOVE(SKACacheEntry);
	};

	std::ostream &operator<<(std::ostream &out, const gfx::EncodedAnimId id);
	std::ostream &operator<<(std::ostream &out, const AnimSlot &anim);

	class AASSystem {
		friend class AASSystemHooks;

	public:
		static constexpr size_t MaxSlots = 5000;

		IMaterialResolver *&mMaterialResolver =
			temple::GetRef<IMaterialResolver *>(0x10EFB8F4);

		IEventListener *&mEventListener =
			temple::GetRef<IEventListener *>(0x11069C70);

		temple::AasStatus CreateAnimFromIds(int skaId,
			int skmId,
			gfx::EncodedAnimId idleAnim,
			const temple::AasAnimParams &state,
			temple::AasHandle *handleOut);

		temple::AasStatus CreateAnimFromFiles(const std::string &meshPath,
			const std::string &skeletonPath,
			gfx::EncodedAnimId idleAnim,
			const temple::AasAnimParams &state,
			temple::AasHandle *handleOut);

		void SetAnimId(temple::AasHandle handle, gfx::EncodedAnimId anim);

		temple::AasEventFlag Advance(temple::AasHandle handle,
			float deltaTimeInSecs,
			float deltaDistance,
			float deltaRotation,
			const temple::AasAnimParams &state);

		temple::AasStatus GetBoneWorldMatrix(temple::AasHandle handle,
			const temple::AasAnimParams &state,
			XMFLOAT4X4 *matrixOut,
			const char *boneName);

		temple::AasStatus GetBoneWorldMatrix(temple::AasHandle parentHandle,
			temple::AasHandle childHandle,
			const temple::AasAnimParams &state,
			XMFLOAT4X4 *matrixOut,
			const char *boneName);

		temple::AasStatus Free(temple::AasHandle handle);

		temple::AasStatus SetTime(temple::AasHandle handle, const temple::AasAnimParams &state, float time);

		temple::AasStatus GetSubmeshes(temple::AasHandle handle, int **submeshMaterials, int *pSubmeshCountOut);

		temple::AasStatus GetSubmesh(temple::AasHandle handle, temple::AasSubmesh **pSubmeshOut, const temple::AasAnimParams &state, int submeshIdx);

		temple::AasStatus AddAddmesh(temple::AasHandle handle, const char *filename);

		temple::AasStatus ClearAddmeshes(temple::AasHandle handle);

		temple::AasStatus GetAnimId(temple::AasHandle handle, gfx::EncodedAnimId *animIdOut);

	private:
		AnimSlot *GetFreeAnim();

		AnimSlot *GetRunningAnim(temple::AasHandle handle) const;
		
		AasMatrix GetEffectiveWorldMatrix(const temple::AasAnimParams &state) const;

		// Same as above, but overrides the parent anim
		AasMatrix GetEffectiveWorldMatrix(const temple::AasAnimParams &state, 
			temple::AasHandle parentHandle) const;

		AasMatrix GetEffectiveWorldMatrixForParticles(const temple::AasAnimParams &state) const;
		
		// Same as above, but overrides the parent anim
		AasMatrix GetEffectiveWorldMatrixForParticles(const temple::AasAnimParams &state, 
			temple::AasHandle parentHandle) const;

		using AnimSlotArray = AnimSlot[MaxSlots];

		AnimSlotArray &mAnimations = temple::GetRef<AnimSlotArray>(0x10EFB900);

		bool ReadModel(const char *filename, SKMFile **modelOut) const;

		using GetFilenameFn = int(int id, char* filenameOut);
		GetFilenameFn*& mGetMeshFilenameFn = temple::GetRef<GetFilenameFn*>(0x11069C74);
		GetFilenameFn*& mGetSkeletonFilenameFn = temple::GetRef<GetFilenameFn*>(0x11069C60);

		std::string GetSkeletonFilename(int id) const;
		std::string GetMeshFilename(int id) const;

		bool LoadSkaFile(const std::string &filename, const SKAFile **dataOut);
		void UnloadSkaFile(const SKAFile *skaFile);

		bool LoadSkmFile(const std::string &filename, SKMFile **dataOut);

		std::unordered_map<std::string, SKACacheEntry> mSkaCache;

	};
		
}
