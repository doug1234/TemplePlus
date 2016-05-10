
#include "stdafx.h"

#include <infrastructure/stringutil.h>
#include <infrastructure/vfs.h>

#include "location.h"

#include "aas_facade.h"

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

const AasMatrix AasMatrix::sIdentity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

using namespace temple;

std::ostream &operator<<(std::ostream &out, const gfx::EncodedAnimId id) {
  out << id.GetName();
  return out;
}

std::ostream &operator<<(std::ostream &out, const AnimSlot &anim) {
  out << "Id: " << anim.id << " AnimId: " << anim.animId
      << " Flags: " << anim.flags << " Floatconst: " << anim.floatConst
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

temple::AasStatus
AASSystem::CreateAnimFromIds(int skaId, int skmId, gfx::EncodedAnimId idleAnim,
                             const temple::AasAnimParams &state,
                             temple::AasHandle *handleOut) {
  auto skeletonPath = GetSkeletonFilename(skaId);
  if (skeletonPath.empty()) {
    logger->error(
        "meshes.mes entry for id {} is missing. Could not load skeleton.",
        skaId);
    return AAS_ERROR;
  }

  auto meshPath = GetMeshFilename(skmId);
  if (meshPath.empty()) {
    logger->error("meshes.mes entry for id {} is missing. Could not load mesh.",
                  skmId);
    return AAS_ERROR;
  }

  return CreateAnimFromFiles(meshPath, skeletonPath, idleAnim, state,
                             handleOut);
}

temple::AasStatus AASSystem::CreateAnimFromFiles(
    const std::string &meshPath, const std::string &skeletonPath,
    gfx::EncodedAnimId idleAnim, const temple::AasAnimParams &state,
    temple::AasHandle *handleOut) {
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

  anim->flags = 0;
  anim->animId = gfx::EncodedAnimId(0);
  anim->floatConst = 6.394000053405762;
  anim->addMeshCount = 0;

  // Previously the ID was set here, but i think it must already be set in
  // aas_init
  assert(anim == GetRunningAnim(anim->id));

  anim->skaFilename = _strdup(skeletonPath.c_str());
  anim->skmFilename = _strdup(meshPath.c_str());

  // This is currently manual, but should be replaced more or less
  auto model = (AnimatedModel *)operator new(0x100u);

  // Set the original vtable pointer
  *((uint32_t *)model) = (uint32_t)temple::GetPointer<uint32_t>(0x102A8DC8);
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

void AASSystem::SetAnimId(temple::AasHandle handle, gfx::EncodedAnimId animId) {

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

temple::AasEventFlag AASSystem::Advance(AasHandle handle, float deltaTimeInSecs,
                                        float deltaDistance,
                                        float deltaRotation,
                                        const AasAnimParams &state) {
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

temple::AasStatus
AASSystem::GetBoneWorldMatrix(temple::AasHandle handle,
                              const temple::AasAnimParams &state,
                              XMFLOAT4X4 *matrixOut, const char *boneName) {
  // Previously, ToEE actually computed the effective world matrix here,
  // but in reality, the
  // get bone matrix call below will never ever use the input value
  // auto aasWorldMat = aasSystem->GetEffectiveWorldMatrix(*state);

  auto anim = GetRunningAnim(handle);
  if (anim) {
    anim->model->GetBoneWorldMatrix((AasMatrix *)matrixOut, boneName);

    matrixOut->_41 = 0;
    matrixOut->_42 = 0;
    matrixOut->_43 = 0;
    matrixOut->_44 = 1;

    using namespace DirectX;
    XMStoreFloat4x4(matrixOut, XMMatrixTranspose(XMLoadFloat4x4(matrixOut)));

    return AAS_OK;
  } else {
    return AAS_ERROR;
  }
}

temple::AasStatus
AASSystem::GetBoneWorldMatrix(temple::AasHandle parentHandle,
                              temple::AasHandle childHandle,
                              const temple::AasAnimParams &state,
                              XMFLOAT4X4 *matrixOut, const char *boneName) {
  // there is no actual discernible difference since the input world matrix to
  // model->GetBoneMatrix is discarded anyway.
  return GetBoneWorldMatrix(childHandle, state, matrixOut, boneName);
}

temple::AasStatus AASSystem::Free(temple::AasHandle handle) {
  auto anim = GetRunningAnim(handle);

  if (!anim) {
    return AAS_ERROR;
  }

  // TODO: Verify that this actually calls the vtable method 1
  // delete anim->model;
  anim->model = nullptr;

  // UnloadSkaFile(anim->skaFile);
  anim->skaFile = nullptr;

  for (size_t i = 0; i < anim->addMeshCount; i++) {
    // free(anim->addMeshData[i]);
    // free(anim->addMeshNames[i]);
  }
  anim->addMeshCount = 0;

  free(anim->skmFile);
  anim->skmFile = nullptr;
  anim->flags = ASF_FREE;
  if (anim->skaFilename) {
    // free(anim->skaFilename);
	anim->skaFilename = nullptr;
  }
  if (anim->skmFilename) {
    // free(anim->skmFilename);
	anim->skmFilename = nullptr;
  }

  return AAS_OK;
}

temple::AasStatus AASSystem::SetTime(temple::AasHandle handle,
                                     const AasAnimParams &state, float time) {

  auto anim = GetRunningAnim(handle);
  if (anim) {
    // This method is only used by the particle system, so calculate a particle
    // system world matrix here. Oddly enough this is not equivalent to the
    // actual code used by GetSubmesh... I don't know why though (Parent anim is
    // ignored)
    auto aasWorldMat = GetEffectiveWorldMatrixForParticles(state, 0);

    anim->model->SetTime(time, &aasWorldMat);
  }

  return AAS_OK;
}

temple::AasStatus AASSystem::GetSubmeshes(temple::AasHandle handle, int ** submeshMaterials, int * pSubmeshCountOut)
{
	auto anim = GetRunningAnim(handle);
	if (!anim) {
		return AAS_ERROR;
	}

	*pSubmeshCountOut = 25;
	static int sSubmeshMaterialIds[25];

	anim->model->GetSubmeshes(pSubmeshCountOut, &sSubmeshMaterialIds[0]);

	*submeshMaterials = &sSubmeshMaterialIds[0];

	return AAS_OK;
}

temple::AasStatus AASSystem::GetSubmesh(temple::AasHandle handle, AasSubmesh ** pSubmeshOut, const AasAnimParams & state, int submeshIdx)
{
	auto anim = GetRunningAnim(handle);
	if (!anim) {
		return AAS_ERROR;
	}

	auto result = new AasSubmesh;
	*pSubmeshOut = result;
	result->field_0 = 0;

	auto aasWorldMat = GetEffectiveWorldMatrix(state);
	anim->model->SetWorldMatrix(&aasWorldMat);
	anim->model->SetScale(state.scale);
	anim->model->GetSubmesh(submeshIdx, &result->vertexCount,
		&result->positions, &result->normals,
		&result->uv, &result->primCount,
		&result->indices);

	return AAS_OK;
}

temple::AasStatus AASSystem::AddAddmesh(temple::AasHandle handle, const char * filename)
{
	auto anim = GetRunningAnim(handle);
	if (!anim) {
		return AAS_ERROR;
	}

	if (anim->addMeshCount != 31) {
		ReadModel(filename, &anim->addMeshData[anim->addMeshCount]);
		anim->addMeshNames[anim->addMeshCount] = _strdup(filename);
		anim->model->SetSkmFile(anim->addMeshData[anim->addMeshCount], mMaterialResolver, 0);
		anim->addMeshCount += 1;
	}

	return AAS_OK;
}

temple::AasStatus AASSystem::ClearAddmeshes(temple::AasHandle handle)
{
	auto anim = GetRunningAnim(handle);
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
}

temple::AasStatus AASSystem::GetAnimId(temple::AasHandle handle, gfx::EncodedAnimId * animIdOut)
{
	auto anim = GetRunningAnim(handle);
	if (!anim) {
		// ToEE does not check if the anim handle is valid and still returns
		// something here...
		*animIdOut = gfx::EncodedAnimId(0);
	}
	else {
		*animIdOut = anim->animId;
	}
	return AAS_OK;
}

AnimSlot *AASSystem::GetFreeAnim() {
  for (size_t i = 1; i < MaxSlots; i++) {
    if (mAnimations[i].IsFree()) {
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

  if (anim->IsFree()) {
    logger->warn("Trying to access AAS anim handle {}, which is already freed.",
                 handle);
    return nullptr;
  }

  return anim;
}

AasMatrix AASSystem::GetEffectiveWorldMatrix(const AasAnimParams &state) const {
  AasHandle parentId = 0;
  if (state.flags & 2) {
    parentId = state.parentAnim;
  }
  return GetEffectiveWorldMatrix(state, parentId);
}

AasMatrix AASSystem::GetEffectiveWorldMatrix(const temple::AasAnimParams &state,
                                             AasHandle parentHandle) const {
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

  if (parentHandle) {
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

AasMatrix AASSystem::GetEffectiveWorldMatrixForParticles(
    const AasAnimParams &state) const {
  AasHandle parentId = 0;
  if (state.flags & 2) {
    parentId = state.parentAnim;
  }
  return GetEffectiveWorldMatrixForParticles(state, parentId);
}

AasMatrix AASSystem::GetEffectiveWorldMatrixForParticles(
    const temple::AasAnimParams &state, temple::AasHandle parentHandle) const {
  using namespace DirectX;

  auto scalingMatrix = XMMatrixScaling(-1, 1, 1);
  auto translationMatrix = XMMatrixTranslation(
      state.locX * INCH_PER_TILE + INCH_PER_TILE * 0.5f + state.offsetX,
      state.offsetZ,
      state.locY * INCH_PER_TILE + INCH_PER_TILE * 0.5f + state.offsetY);
  auto rotationMatrix = XMMatrixRotationRollPitchYaw(
      state.rotationPitch, state.rotationYaw, state.rotationRoll);
  auto worldMat = XMMatrixMultiply(
      scalingMatrix, XMMatrixMultiply(rotationMatrix, translationMatrix));
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

std::string AASSystem::GetSkeletonFilename(int id) const {
  char filename[260] = {
      '\0',
  };

  if (!mGetSkeletonFilenameFn(id, &filename[0])) {
    return filename;
  }

  return std::string();
}

std::string AASSystem::GetMeshFilename(int id) const {
  char filename[260] = {
      '\0',
  };

  if (!mGetMeshFilenameFn(id, &filename[0])) {
    return filename;
  }

  return std::string();
}

bool AASSystem::LoadSkaFile(const std::string &filename,
                            const SKAFile **dataOut) {
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
  entry.skaFilePtr = (SKAFile *)&entry.skaData[0];
  entry.refCount = 1;

  *dataOut = entry.skaFilePtr;

  return true;
}

void AASSystem::UnloadSkaFile(const SKAFile *skaFile) {
  auto it = mSkaCache.begin();
  while (it != mSkaCache.end()) {
    if (it->second.skaFilePtr == skaFile) {
      if (--(it->second.refCount) == 0) {
        it = mSkaCache.erase(it);
        continue;
      }
    }
    it++;
  }
}

bool AASSystem::LoadSkmFile(const std::string &filename, SKMFile **dataOut) {
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
    auto length = vfs->Length(fh);
    buffer.reset((SKMFile *)malloc(length));
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
