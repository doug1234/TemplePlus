
#include "stdafx.h"

#include <infrastructure/vfs.h>
#include <temple/aas.h>
#include <temple/dll.h>
#include <temple/meshes.h>

#include <graphics/math.h>
#include "location.h"

using DirectX::XMFLOAT4X3;

#include "util/fixes.h"

namespace aas {

/*
        Legacy structures found in the DLL.
*/

struct IEventListener {
  virtual void Handle() = 0;
  virtual ~IEventListener() = 0;
};

struct IMaterialResolver {
	virtual uint32_t ResolveMaterial(const char *name, int unk1, int unk2) = 0;
	virtual void UnknownFunc() = 0; // Probably unused since it's a nullsub in ToEE
};

struct SKAFile {};

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
  virtual int SetSkaFile(SKAFile *skaData, int a3, int a4) = 0;
  virtual void SetScale(float scale) = 0;
  virtual int GetBoneCount() = 0;
  virtual char *GetBoneName(int idx) = 0;
  virtual int GetBoneParentId(int idx) = 0;
  virtual int SetSkmFile(SKMFile *skmData, void *matResolver,
                         IMaterialResolver *matResolverArg) = 0;
  virtual int FreeAddMeshStuffMaybe(SKMFile*) = 0;
  virtual int SetEventListener(IEventListener *eventListener) = 0;
  virtual void ResetSubmeshes() = 0;
  virtual void Method11() = 0;
  virtual void SetClothFlagSth() = 0;
  virtual void GetSubmeshes(int *submeshCountCount,
                            int *submeshMaterialsOut) = 0;
  virtual void Method14() = 0;
  virtual int HasAnimation(char *anim) = 0;
  virtual int SetAnimIdx(int animIdx) = 0;
  virtual void Advance(const XMFLOAT4X3 *worldMatrix, float deltaTime,
                       float deltaDistance, float deltaRotation) = 0;
  virtual void SetWorldMatrix(const XMFLOAT4X3 *worldMatrix) = 0;
  virtual void Method19() = 0;
  virtual void SetTime() = 0;
  virtual void Method21() = 0;
  virtual void GetSubmesh(int submeshIdx, int *vertexCountOut, float **posOut,
                          float **normalsOut, float **uvOut, int *primCountOut,
                          uint16_t **indicesOut) = 0;
  virtual void AddPlayer() = 0;
  virtual void RemovePlayer(AnimPlayer *player) = 0;
  virtual void Method25() = 0;
  virtual void Method26() = 0;
  virtual void HasBone() = 0;
  virtual void AddReplacementMaterial() = 0;
  virtual void GetDistPerSec() = 0;
  virtual void GetRotationPerSec() = 0;

  // Call original GetSubmeshes function regardless of vtable
  void GetSubmeshesOrg(int *submeshCountOut, int *materialsOut) {
    using GetSubmeshesOrgFn = void(__thiscall *)(
        AnimatedModel * pThis, int *countOut, int *materialIdsOut);
    static auto getSubmeshesOrg =
        (GetSubmeshesOrgFn)temple::GetPointer<void>(0x102660E0);
    getSubmeshesOrg(this, submeshCountOut, materialsOut);
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
  XMFLOAT4X3 someMatrix;
  XMFLOAT4X3 worldMatrix;
  XMFLOAT4X3 *boneMatrices;
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
  void *skmFile;
  void *skaFile;
  uint32_t addMeshCount;
  SKMFile* addMeshData[32];
  char *addMeshNames[32];
  char *skaFilename;
  char *skmFilename;
};

static_assert(temple::validate_size<AnimSlot, 0x12C>::value,
              "AnimSlot has incorrect size");

class AASSystem {
  friend class AASSystemHooks;

public:
  static constexpr size_t MaxSlots = 5000;

  IMaterialResolver *&mMaterialResolver = temple::GetRef<IMaterialResolver*>(0x10EFB8F4);

private:
  AnimSlot *GetRunningAnim(temple::AasHandle handle) const;

  using AnimSlotArray = AnimSlot[MaxSlots];

  AnimSlotArray &mAnimations = temple::GetRef<AnimSlotArray>(0x10EFB900);

  bool ReadModel(const char *filename, SKMFile **modelOut) const;
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

          logger->info("Getting submeshes for {}", *anim);

          *pSubmeshCountOut = 25;
          static int sSubmeshMaterialIds[25];

          anim->model->GetSubmeshes(pSubmeshCountOut, &sSubmeshMaterialIds[0]);

          *submeshMaterials = &sSubmeshMaterialIds[0];

          return AAS_OK;
        });

	// AasAnimatedModelAddAddmesh
	replaceFunction<AasStatus(AasHandle, const char *)>(0x10262e30, [](AasHandle handle, const char *filename) {
		
		auto anim = aasSystem->GetRunningAnim(handle);
		if (!anim) {
			return AAS_ERROR;
		}

		logger->info("Adding addmesh {} to {}", filename, *anim);

		if (anim->addMeshCount != 31) {
			aasSystem->ReadModel(filename, &anim->addMeshData[anim->addMeshCount]);
			anim->addMeshNames[anim->addMeshCount] = _strdup(filename);
			anim->model->SetSkmFile(
				anim->addMeshData[anim->addMeshCount],
				aasSystem->mMaterialResolver,
				0);
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

		logger->info("Clearing addmeshes for {}", *anim);

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
	replaceFunction<AasStatus(AasHandle, gfx::EncodedAnimId*)>(0x102627e0, [](AasHandle handle, gfx::EncodedAnimId *animIdOut) {
		auto anim = aasSystem->GetRunningAnim(handle);
		if (!anim) {
			// ToEE does not check if the anim handle is valid and still returns something here...
			*animIdOut = gfx::EncodedAnimId(0);
		} else {
			*animIdOut = anim->animId;
		}
		return AAS_OK;
	});

	// AasAnimatedModelGetSubmesh
	using AasAnimatedModelGetSubmeshFn = AasStatus(AasHandle, AasSubmesh**, const AasAnimParams*, int);
	static AasAnimatedModelGetSubmeshFn *sOldGetSubmesh;
	sOldGetSubmesh = replaceFunction<AasStatus(AasHandle, AasSubmesh**, const AasAnimParams*, int)>(0x10263400, [](AasHandle handle, AasSubmesh **pSubmeshOut, const AasAnimParams *state, int submeshIdx) {

		using namespace DirectX;

		auto rotation = state->rotation - XM_PI * 0.75f;

		auto scalingMatrix = XMMatrixScaling(-1, 1, 1);
		auto rotationMatrix = XMMatrixRotationY(rotation);
		auto rotationPitchMatrix = XMMatrixRotationX(state->rotationPitch);
		auto translationMatrix = XMMatrixTranslation(
			state->locX * INCH_PER_TILE + INCH_PER_TILE * 0.5f + state->offsetX,
			state->offsetZ,
			state->locY * INCH_PER_TILE + INCH_PER_TILE * 0.5f + state->offsetY
		);
		
		auto mat = XMMatrixMultiply(translationMatrix, XMMatrixMultiply(rotationPitchMatrix, XMMatrixMultiply(rotationMatrix, scalingMatrix)));
		
		XMFLOAT4X4 worldMat;
		XMStoreFloat4x4(&worldMat, XMMatrixTranspose(mat));

		XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), rotation);

		sOldGetSubmesh(handle, pSubmeshOut, state, submeshIdx);

		auto anim = aasSystem->GetRunningAnim(handle);
		XMFLOAT4X4 orgWorldMat;
		memcpy(&orgWorldMat, &anim->model->worldMatrix, 3 * 4 * sizeof(float));
		
		return AAS_OK;
	});

	// AasAnimatedModelFreeSubmesh
	replaceFunction<AasStatus(AasSubmesh*)>(0x10262500, [](AasSubmesh *submesh) {
		free(submesh);
		return AAS_OK;
	});

  }
} hooks;

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

bool AASSystem::ReadModel(const char * filename, SKMFile ** modelOut) const
{
	assert(modelOut);

	*modelOut = nullptr;

	auto fh = vfs->Open(filename, "rb");
	if (!fh) {
		logger->error("Unable to open model {}", filename);
		return false;
	}

	auto size = vfs->Length(fh);
	std::unique_ptr<SKMFile> data((SKMFile*)malloc(size));
	if (vfs->Read(data.get(), size, fh) != size) {
		vfs->Close(fh);
		logger->error("Unable to read model {}", filename);
		return false;
	}

	vfs->Close(fh);

	*modelOut = data.release();
	return true;
}

}
