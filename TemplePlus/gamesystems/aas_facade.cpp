
#include "stdafx.h"

#include <infrastructure/vfs.h>
#include <infrastructure/stringutil.h>
#include <infrastructure/macros.h>
#include <temple/aas.h>
#include <temple/dll.h>
#include <temple/meshes.h>

#include "location.h"
#include <graphics/math.h>

using DirectX::XMFLOAT4X3;

#include "util/fixes.h"

namespace aas {
// TODO: REMOVE
static void CompareMatrices(XMFLOAT4X4 &a, XMFLOAT4X4 &b) {
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 4; col++) {
      auto ul = std::abs(std::max(a(row, col), b(row, col))) * 0.0001;
      if (std::abs(a(row, col) - b(row, col)) > ul) {
        logger->error("{},{} -> {} != {}", row, col, a(row, col), b(row, col));
      }
    }
  }
}

/*
        Legacy structures found in the DLL.
*/

struct AasMatrix {
  float m[3][4];
  
  static const AasMatrix sIdentity;
};

const AasMatrix AasMatrix::sIdentity { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0 };

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
  virtual void SetTime() = 0;
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

struct AnimSlot {
  temple::AasHandle id;
  gfx::EncodedAnimId animId;
  bool freed;
  float floatConst;
  uint32_t timeLoaded;
  AnimatedModel *model;
  SKMFile*skmFile;
  const SKAFile *skaFile;
  uint32_t addMeshCount;
  SKMFile *addMeshData[32];
  char *addMeshNames[32];
  char *skaFilename;
  char *skmFilename;
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

  temple::AasStatus Free(temple::AasHandle handle);

private:
  AnimSlot *GetFreeAnim();

  AnimSlot *GetRunningAnim(temple::AasHandle handle) const;

  AasMatrix GetEffectiveWorldMatrix(const temple::AasAnimParams &state) const;

  AasMatrix GetEffectiveWorldMatrixForParticles(const temple::AasAnimParams &state) const;

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

using namespace temple;

std::ostream &operator<<(std::ostream &out, const gfx::EncodedAnimId id) {
  out << id.GetName();
  return out;
}

std::ostream &operator<<(std::ostream &out, const AnimSlot &anim) {
  out << "Id: " << anim.id << " AnimId: " << anim.animId
      << " Free: " << anim.freed << " Floatconst: " << anim.floatConst
      << " TimeLoaded: " << anim.timeLoaded << " AddMeshes: [";
  for (uint32_t i = 0; i < anim.addMeshCount; i++) {
    if (i != 0) {
      out << ", ";
    }
    out << anim.addMeshNames[i];
  }
  out << "] SKA: " << anim.skaFilename << " SKM: " << anim.skmFilename;
  return out;
}

inline void XM_CALLCONV StoreAasMatrix(AasMatrix *target,
                                       DirectX::FXMMATRIX src) {

  using namespace DirectX;

  static XMFLOAT4X4A sTmpMatrix;
  XMStoreFloat4x4A(&sTmpMatrix, XMMatrixTranspose(src));

  memcpy(target, &sTmpMatrix, sizeof(AasMatrix));
}

static class AASSystemHooks : public TempleFix {
public:
  void apply() override {
    static auto aasSystem = std::make_unique<AASSystem>();

    // AasAnimatedModelGetSubmeshes
    replaceFunction<AasStatus(AasHandle, int **, int *)>(
        0x10263a50,
        [](AasHandle handle, int **submeshMaterials, int *pSubmeshCountOut) {

          auto anim = aasSystem->GetRunningAnim(handle);
          if (!anim) {
            return AAS_ERROR;
          }

          *pSubmeshCountOut = 25;
          static int sSubmeshMaterialIds[25];

          anim->model->GetSubmeshes(pSubmeshCountOut, &sSubmeshMaterialIds[0]);

          *submeshMaterials = &sSubmeshMaterialIds[0];

          return AAS_OK;
        });

    // AasAnimatedModelAddAddmesh
    replaceFunction<AasStatus(AasHandle, const char *)>(
        0x10262e30, [](AasHandle handle, const char *filename) {

          auto anim = aasSystem->GetRunningAnim(handle);
          if (!anim) {
            return AAS_ERROR;
          }

          if (anim->addMeshCount != 31) {
            aasSystem->ReadModel(filename,
                                 &anim->addMeshData[anim->addMeshCount]);
            anim->addMeshNames[anim->addMeshCount] = _strdup(filename);
            anim->model->SetSkmFile(anim->addMeshData[anim->addMeshCount],
                                    aasSystem->mMaterialResolver, 0);
            anim->addMeshCount += 1;
          }

          return AAS_OK;

        });

    // AasAnimatedModelClearAddmeshes
    replaceFunction<AasStatus(AasHandle)>(0x10262ec0, [](AasHandle handle) {

      auto anim = aasSystem->GetRunningAnim(handle);
      if (!anim) {
        return AAS_ERROR;
      }

      for (uint32_t i = 0; i < anim->addMeshCount; i++) {
        auto addMesh = anim->addMeshData[i];
        auto addMeshName = anim->addMeshNames[i];
        anim->model->FreeAddMeshStuffMaybe(addMesh);
        free(addMesh);
        free(addMeshName);
      }

      anim->addMeshCount = 0;

      return AAS_OK;

    });

    // AasAnimatedModelGetAnimId
    replaceFunction<AasStatus(AasHandle, gfx::EncodedAnimId *)>(
        0x102627e0, [](AasHandle handle, gfx::EncodedAnimId *animIdOut) {
          auto anim = aasSystem->GetRunningAnim(handle);
          if (!anim) {
            // ToEE does not check if the anim handle is valid and still returns
            // something here...
            *animIdOut = gfx::EncodedAnimId(0);
          } else {
            *animIdOut = anim->animId;
          }
          return AAS_OK;
        });

    // AasAnimatedModelGetSubmesh
    replaceFunction<AasStatus(AasHandle, AasSubmesh **, const AasAnimParams *,
                              int)>(
        0x10263400, [](AasHandle handle, AasSubmesh **pSubmeshOut,
                       const AasAnimParams *state, int submeshIdx) {

          auto anim = aasSystem->GetRunningAnim(handle);
          if (!anim) {
            return AAS_ERROR;
          }

          auto result = new AasSubmesh;
          *pSubmeshOut = result;
          result->field_0 = 0;

          auto aasWorldMat = aasSystem->GetEffectiveWorldMatrix(*state);
          anim->model->SetWorldMatrix(&aasWorldMat);
          anim->model->SetScale(state->scale);
          anim->model->GetSubmesh(submeshIdx, &result->vertexCount,
                                  &result->positions, &result->normals,
                                  &result->uv, &result->primCount,
                                  &result->indices);

          return AAS_OK;
        });

    // AasAnimatedModelFreeSubmesh
    replaceFunction<AasStatus(AasSubmesh *)>(0x10262500,
                                             [](AasSubmesh *submesh) {
                                               delete submesh;
                                               return AAS_OK;
                                             });

    // AasAnimatedModelAdvance
    replaceFunction<AasStatus(AasHandle, float, float, float,
                              const AasAnimParams *, AasEventFlag *)>(
        0x10262c10, [](AasHandle handle, float deltaTimeInSecs,
                       float deltaDistance, float deltaRotation,
                       const AasAnimParams *state, AasEventFlag *eventOut) {
          
		  *eventOut = aasSystem->Advance(handle, deltaTimeInSecs, deltaDistance, deltaRotation, *state);

          return AAS_OK;
        });

    // AasAnimatedModelGetBoneCount
    replaceFunction<int(AasHandle)>(0x10262f40, [](AasHandle handle) {
      auto anim = aasSystem->GetRunningAnim(handle);
      if (anim) {
        return anim->model->GetBoneCount();
      }
      return 0;
    });

    // AasAnimatedModelGetBoneNameById
    replaceFunction<const char *(AasHandle, int)>(
        0x10262f80, [](AasHandle handle, int boneIdx) -> const char * {
          auto anim = aasSystem->GetRunningAnim(handle);
          if (anim) {
            return anim->model->GetBoneName(boneIdx);
          }
          return nullptr;
        });

    // AasAnimatedModelGetBoneParentId
    replaceFunction<int(AasHandle, int)>(
        0x10262fc0, [](AasHandle handle, int boneIdx) {
          auto anim = aasSystem->GetRunningAnim(handle);
          if (anim) {
            return anim->model->GetBoneParentId(boneIdx);
          }
          return -1;
        });

    // AasAnimatedModelGetBoneWorldMatrixByName
    replaceFunction<AasStatus(AasHandle, AasAnimParams *, XMFLOAT4X4 *,
                              const char *)>(
        0x10263000, [](AasHandle handle, AasAnimParams *state, XMFLOAT4X4 *pOut,
                       const char *boneName) {

          // Previously, ToEE actually computed the effective world matrix here,
          // but in reality, the
          // get bone matrix call below will never ever use the input value
          // auto aasWorldMat = aasSystem->GetEffectiveWorldMatrix(*state);

          auto anim = aasSystem->GetRunningAnim(handle);
          if (anim) {
            anim->model->GetBoneWorldMatrix((AasMatrix *)pOut, boneName);

            pOut->_41 = 0;
            pOut->_42 = 0;
            pOut->_43 = 0;
            pOut->_44 = 1;

            using namespace DirectX;
            XMStoreFloat4x4(pOut, XMMatrixTranspose(XMLoadFloat4x4(pOut)));

            return AAS_OK;
          } else {
            return AAS_ERROR;
          }

        });

    // AasAnimatedModelSetClothFlagSth
    replaceFunction<void(AasHandle)>(0x102633c0, [](AasHandle handle) {
      auto anim = aasSystem->GetRunningAnim(handle);
      if (anim) {
        anim->model->SetClothFlagSth();
      }
    });

    // AasAnimatedModelHasBone
    replaceFunction<BOOL(AasHandle, const char *)>(
        0x10263a10, [](AasHandle handle, const char *boneName) {

          auto anim = aasSystem->GetRunningAnim(handle);

          if (anim) {
            return anim->model->HasBone(boneName) ? TRUE : FALSE;
          }

          return FALSE;

        });

    // AasAnimatedModelFromIds
    replaceFunction<AasStatus(int, int, gfx::EncodedAnimId,
                              const AasAnimParams *, AasHandle *)>(
        0x102641b0, [](int skmId, int skaId, gfx::EncodedAnimId idleAnim,
                       const AasAnimParams *state, AasHandle *handleOut) {
		return aasSystem->CreateAnimFromIds(skmId, skaId, idleAnim, *state, handleOut);
        });


	// AasAnimatedModelFromNames
	replaceFunction<AasStatus(const char *, const char *, gfx::EncodedAnimId,
		const AasAnimParams *, AasHandle *)>(
			0x102643A0, [](const char *skmPath, const char *skaPath, gfx::EncodedAnimId idleAnim,
				const AasAnimParams *state, AasHandle *handleOut) {
		return aasSystem->CreateAnimFromFiles(skmPath, skaPath, idleAnim, *state, handleOut);
	});

	// AasAnimatedModelFree
	replaceFunction<AasStatus(AasHandle)>(0x10264510, [](AasHandle handle) {
		return aasSystem->Free(handle);
	});
  }
} hooks;

temple::AasStatus AASSystem::CreateAnimFromIds(int skaId, int skmId, gfx::EncodedAnimId idleAnim, const temple::AasAnimParams & state, temple::AasHandle * handleOut)
{
	auto skeletonPath = GetSkeletonFilename(skaId);
	if (skeletonPath.empty()) {
		logger->error("meshes.mes entry for id {} is missing. Could not load skeleton.", skaId);
		return AAS_ERROR;
	}

	auto meshPath = GetMeshFilename(skmId);
	if (meshPath.empty()) {
		logger->error("meshes.mes entry for id {} is missing. Could not load mesh.", skmId);
		return AAS_ERROR;
	}

	return CreateAnimFromFiles(meshPath, skeletonPath, idleAnim, state, handleOut);
}

temple::AasStatus AASSystem::CreateAnimFromFiles(const std::string & meshPath, 
	const std::string &skeletonPath, 
	gfx::EncodedAnimId idleAnim, 
	const temple::AasAnimParams & state, 
	temple::AasHandle * handleOut)
{
	auto anim = GetFreeAnim();

	if (!anim) {
		logger->error("The maximum number of animations has been exceeded.");
		return AAS_ERROR;
	}

	if (!LoadSkaFile(skeletonPath, &anim->skaFile)) {
		return AAS_ERROR;
	}

	if (!LoadSkmFile(meshPath, &anim->skmFile)) {
		UnloadSkaFile(anim->skaFile);
		return AAS_ERROR;
	}

	anim->freed = false;
	anim->animId = gfx::EncodedAnimId(0);
	anim->floatConst = 6.394000053405762;
	anim->addMeshCount = 0;

	// Previously the ID was set here, but i think it must already be set in aas_init
	assert(anim == GetRunningAnim(anim->id));

	anim->skaFilename = _strdup(skeletonPath.c_str());
	anim->skmFilename = _strdup(meshPath.c_str());

	// This is currently manual, but should be replaced more or less
	auto model = (AnimatedModel*) operator new(0x100u);

	// Set the original vtable pointer
	*((uint32_t*)model) = (uint32_t)temple::GetPointer<uint32_t>(0x102A8DC8);
	model->skaData = nullptr;
	model->submeshesValid = false;
	model->submeshCount = 0;
	model->field_7E = 0;
	model->submeshes = nullptr;
	model->newestRunningAnim = nullptr;
	model->runningAnimsHead = nullptr;
	model->worldMatrix = AasMatrix::sIdentity;
	model->boneMatrices = nullptr;
	model->eventListener = nullptr;
	model->hasClothBones = false;
	model->clothBoneId = 0; // "normalBoneCount" (?)
	model->clothStuff1Count = 0;
	model->clothSpheresHead = nullptr;
	model->clothCylindersHead = nullptr;
	model->timeRel1 = 0;
	model->timeRel2 = 0;
	model->drivenDistance = 0;
	model->drivenRotation = 0;

	anim->model = model;

	model->SetSkaFile(anim->skaFile, 0, 0);

	model->SetSkmFile(anim->skmFile, mMaterialResolver, nullptr);

	anim->timeLoaded = timeGetTime();

	anim->model->SetEventListener(mEventListener);
	SetAnimId(anim->id, idleAnim);
	Advance(anim->id, 0, 0, 0, state);
	*handleOut = anim->id;

	return AAS_OK;
}

void AASSystem::SetAnimId(temple::AasHandle handle, gfx::EncodedAnimId animId)
{

	auto anim = GetRunningAnim(handle);

	if (!anim) {
		return;
	}

	anim->animId = animId;
	if (anim->model->SetAnim(animId.GetName())) {
		return;
	}

	auto fallbackState = 0;

	while (true) {
		if (animId.ToFallback()) {
			if (anim->model->SetAnim(animId.GetName())) {
				return;
			}
			continue;
		}

		if (fallbackState)
			break;
		fallbackState = 1;

		if (animId.IsWeaponAnim()) {
			// Try the unarmed_unarmed version of it
			animId = gfx::EncodedAnimId(animId.GetWeaponAnim());

			if (anim->model->SetAnim(animId.GetName())) {
				return;
			}
			continue;
		}

		if (fallbackState != 1) {
			throw TempleException("Could not find fallback animation for {} in {}", 
				animId.GetName(), anim->skaFilename);
		}

		fallbackState = 2;
		animId = gfx::EncodedAnimId(gfx::NormalAnimType::ItemIdle);

		if (anim->model->SetAnim(animId.GetName())) {
			return;
		}

	}
}

temple::AasEventFlag AASSystem::Advance(AasHandle handle, float deltaTimeInSecs, float deltaDistance, float deltaRotation, const AasAnimParams &state)
{
	auto aasWorldMat = GetEffectiveWorldMatrix(state);

	AasEventFlag eventOut = AEF_NONE;
	auto anim = GetRunningAnim(handle);
	if (anim) {
		mEventListener->SetOutput(&eventOut);
		anim->model->SetScale(state.scale);
		anim->model->Advance(&aasWorldMat, deltaTimeInSecs, deltaDistance,
			deltaRotation);
		mEventListener->ClearOutput();
	}
	return eventOut;
}

temple::AasStatus AASSystem::Free(temple::AasHandle handle)
{
	auto anim = GetRunningAnim(handle);

	if (!anim) {
		return AAS_ERROR;
	}

	// TODO: Verify that this actually calls the vtable method 1
	delete anim->model;

	UnloadSkaFile(anim->skaFile);
	
	for (size_t i = 0; i < anim->addMeshCount; i++) {
		free(anim->addMeshData[i]);
		free(anim->addMeshNames[i]);
	}
	free(anim->skmFile);
	anim->freed = true;
	if (anim->skaFilename) {
		free(anim->skaFilename);
	}
	if (anim->skmFilename) {
		free(anim->skmFilename);
	}

	return AAS_OK;
}

AnimSlot * AASSystem::GetFreeAnim()
{
	for (size_t i = 1; i < MaxSlots; i++) {
		if (mAnimations[i].freed) {
			// Ensure the ID is set correctly
			mAnimations[i].id = i;
			return &mAnimations[i];
		}
	}

	return nullptr;
}

AnimSlot *AASSystem::GetRunningAnim(AasHandle handle) const {
  if (!handle || handle >= MaxSlots) {
    logger->warn("Trying to access invalid AAS anim handle: {}.", handle);
    return nullptr;
  }

  auto anim = &mAnimations[handle];

  if (anim->freed) {
    logger->warn("Trying to access AAS anim handle {}, which is already freed.",
                 handle);
    return nullptr;
  }

  return anim;
}

AasMatrix AASSystem::GetEffectiveWorldMatrix(const AasAnimParams &state) const {
  using namespace DirectX;

  auto rotation = state.rotation - XM_PI * 0.75f;

  auto scalingMatrix = XMMatrixScaling(-1, 1, 1);
  auto rotationMatrix = XMMatrixRotationY(rotation);
  auto rotationPitchMatrix = XMMatrixRotationX(state.rotationPitch);
  auto translationMatrix = XMMatrixTranslation(
      state.locX * INCH_PER_TILE + INCH_PER_TILE * 0.5f + state.offsetX,
      state.offsetZ,
      state.locY * INCH_PER_TILE + INCH_PER_TILE * 0.5f + state.offsetY);

  auto worldMat = XMMatrixMultiply(
      scalingMatrix,
      XMMatrixMultiply(rotationMatrix, XMMatrixMultiply(rotationPitchMatrix,
                                                        translationMatrix)));
  AasMatrix aasWorldMat;
  StoreAasMatrix(&aasWorldMat, worldMat);

  if (state.flags & 2 && state.parentAnim) {
    auto parentAnim = GetRunningAnim(state.parentAnim);
    if (parentAnim) {
      AasMatrix parentWorldMatrix;
      parentAnim->model->GetBoneWorldMatrix(&parentWorldMatrix,
                                            state.attachedBoneName);
      // TODO: Rewrite ourselves
      static auto AasMatrixMakeOrthoNorm =
          temple::GetPointer<void(AasMatrix * a1, signed int size,
                                  int colStride, int rowStride)>(0x102652d0);
      AasMatrixMakeOrthoNorm(&parentWorldMatrix, 3, 4, 1);
      aasWorldMat = parentWorldMatrix;
    }
  }

  return aasWorldMat;
}

AasMatrix AASSystem::GetEffectiveWorldMatrixForParticles(const AasAnimParams &state) const {
	using namespace DirectX;

	auto scalingMatrix = XMMatrixScaling(-1, 1, 1);
	auto translationMatrix = XMMatrixTranslation(
		state.locX * INCH_PER_TILE + INCH_PER_TILE * 0.5f + state.offsetX,
		state.offsetZ,
		state.locY * INCH_PER_TILE + INCH_PER_TILE * 0.5f + state.offsetY);
	auto rotationMatrix = XMMatrixRotationRollPitchYaw(state.rotationPitch, state.rotationYaw, state.rotationRoll);
	auto worldMat = XMMatrixMultiply(scalingMatrix, XMMatrixMultiply(rotationMatrix, translationMatrix));
	AasMatrix aasWorldMat;
	StoreAasMatrix(&aasWorldMat, worldMat);

	if (state.flags & 2 && state.parentAnim) {
		auto parentAnim = GetRunningAnim(state.parentAnim);
		if (parentAnim) {
			AasMatrix parentWorldMatrix;
			parentAnim->model->GetBoneWorldMatrix(&parentWorldMatrix,
				state.attachedBoneName);
			// TODO: Rewrite ourselves
			static auto AasMatrixMakeOrthoNorm =
				temple::GetPointer<void(AasMatrix * a1, signed int size,
					int colStride, int rowStride)>(0x102652d0);
			AasMatrixMakeOrthoNorm(&parentWorldMatrix, 3, 4, 1);
			aasWorldMat = parentWorldMatrix;
		}
	}

	return aasWorldMat;
}

bool AASSystem::ReadModel(const char *filename, SKMFile **modelOut) const {
  assert(modelOut);

  *modelOut = nullptr;

  auto fh = vfs->Open(filename, "rb");
  if (!fh) {
    logger->error("Unable to open model {}", filename);
    return false;
  }

  auto size = vfs->Length(fh);
  std::unique_ptr<SKMFile> data((SKMFile *)malloc(size));
  if (vfs->Read(data.get(), size, fh) != size) {
    vfs->Close(fh);
    logger->error("Unable to read model {}", filename);
    return false;
  }

  vfs->Close(fh);

  *modelOut = data.release();
  return true;
}

std::string AASSystem::GetSkeletonFilename(int id) const
{
	char filename[260] = { '\0', };

	if (!mGetSkeletonFilenameFn(id, &filename[0])) {
		return filename;
	}

	return std::string();
}

std::string AASSystem::GetMeshFilename(int id) const
{
	char filename[260] = { '\0', };

	if (!mGetMeshFilenameFn(id, &filename[0])) {
		return filename;
	}

	return std::string();
}

bool AASSystem::LoadSkaFile(const std::string & filename, const SKAFile ** dataOut)
{
	auto filenameLower = tolower(filename);

	auto it = mSkaCache.find(filenameLower);

	if (it != mSkaCache.end()) {
		auto &entry = it->second;
		entry.refCount++;
		*dataOut = entry.skaFilePtr;
		return true;
	}

	std::vector<uint8_t> skaData;
	try {
		skaData = vfs->ReadAsBinary(filename);
	} catch (TempleException &e) {
		logger->error("Unable to open model file {}: {}", filename, e.what());
		return false;
	}

	auto &entry = mSkaCache[filenameLower];
	entry.filename = filename;
	entry.skaData = std::move(skaData);
	entry.skaFilePtr = (SKAFile*)&entry.skaData[0];
	entry.refCount = 1;

	*dataOut = entry.skaFilePtr;

	return true;
}

void AASSystem::UnloadSkaFile(const SKAFile * skaFile)
{
	auto it = mSkaCache.begin();
	while (it != mSkaCache.end()) {
		if (it->second.skaFilePtr == skaFile) {
			if (--it->second.refCount == 0) {
				it = mSkaCache.erase(it);
				continue;
			}
		}
		it++;
	}
}

bool AASSystem::LoadSkmFile(const std::string & filename, SKMFile ** dataOut)
{
	std::unique_ptr<SKMFile> buffer;

	*dataOut = nullptr;

	Vfs::FileHandle fh;
	try {
		fh = vfs->Open(filename.c_str(), "rb");
	} catch (TempleException &e) {
		logger->error("Unable to open model file {}: {}", filename, e.what());
		return false;
	}

	try {
		auto		length = vfs->Length(fh);
		buffer.reset((SKMFile*)malloc(length));
		vfs->Read(buffer.get(), length, fh);
		vfs->Close(fh);
	} catch (TempleException &e) {
		vfs->Close(fh);
		logger->error("Unable to read model file {}: {}", filename, e.what());
		return false;
	}
	
	*dataOut = buffer.release();
	return true;
}

}
