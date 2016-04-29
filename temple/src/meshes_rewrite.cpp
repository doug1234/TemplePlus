
#include <chrono>
#include <gsl/gsl.h>

#include <infrastructure/vfs.h>
#include <infrastructure/logging.h>
#include <infrastructure/exception.h>
#include <graphics/math.h>

#include "temple/meshes.h"

#include "meshes_rewrite.h"

using namespace DirectX;

SkaFile::SkaFile(const std::string& filename) : mData(vfs->ReadAsBinary(filename)) {

}

SkmFile::SkmFile(const std::string& filename) : mData(vfs->ReadAsBinary(filename)) {
}

AasClass2::AasClass2(const SkaFile& skeleton, SkmFile& mesh, AasEventListener& eventListener)
	: mEventListener(eventListener), mSkeleton(skeleton), mMesh(mesh) {
	XMStoreFloat4x3A(&mWorldMatrix, XMMatrixIdentity());
}

void AasClass2::LoadMesh(MaterialResolver matResolver) {

	ResetSubmeshes();

	/*
		For some reason, any attachments to #ClothBone are removed, without recording first
		what the attachment was -> why???
	*/
	if (mHasClothBones) {

		for (size_t i = 0; i < mMesh.GetVertexCount(); i++) {
			auto& vertex = mMesh.GetVertex(i);

			if (vertex.attachmentCount <= 1) {
				continue;
			}

			// For some reason this only seems to count if the cloth bone is not the only
			// attachment???
			auto weightSum = 0.0f;

			for (size_t j = 0; j < vertex.attachmentCount; j++) {
				// This is the bone id of the #clothBone
				if (vertex.attachmentBone[j] == mNormalBoneCount) {
					vertex.attachmentWeight[j] = 0;
				} else {
					weightSum += vertex.attachmentWeight[j];
				}
			}

			// Renormalize the weights
			if (weightSum > 0) {
				for (size_t j = 0; j < vertex.attachmentCount; j++) {
					// This is the bone id of the #clothBone
					if (vertex.attachmentBone[j] != mNormalBoneCount) {
						vertex.attachmentWeight[j] = vertex.attachmentWeight[j] / weightSum;
					}
				}
			}
		}

	}

	for (size_t i = 0; i < mMesh.GetMaterialCount(); i++) {
		auto& material = mMesh.GetMaterial(i);

		auto resolvedMat = matResolver(material.id);
		auto& submesh = GetOrCreateSubmeshForMaterial(resolvedMat);
		// TODO: Second submesh function
	}

	// TODO: Lots of cloth-bone stuff

}

void AasClass2::ResetSubmeshes() {

	signed int v2; // ebp@2
	int v3; // edi@3

	if (field_C) {

		mSubmeshes.clear();

		/*for (auto i = 0; i < submeshCount; ++i) {
			if (mHasClothBones) {
				j__free_0(v1->submeshes[v3].field_30);
				j__free_0(v1->submeshes[v3].field_34);
			}
			j__free_0(v1->submeshes[v3].field_10);
			v1->submeshes[v3].field_10 = 0;
			j__free_0(v1->submeshes[v3].field_14);
			v1->submeshes[v3].field_14 = 0;
			j__free_0(v1->submeshes[v3].field_18);
			v1->submeshes[v3].field_18 = 0;
			j__free_0(v1->submeshes[v3].positions);
			v1->submeshes[v3].positions = 0;
			j__free_0(v1->submeshes[v3].normals);
			v1->submeshes[v3].normals = 0;
			j__free_0(v1->submeshes[v3].uv);
			v1->submeshes[v3].uv = 0;
			j__free_0(v1->submeshes[v3].indices);
			v1->submeshes[v3].indices = 0;
		}*/

		field_C = 0;

	}

}

bool AasClass2::SetAnim(const std::string& name) {

	// Find the animation
	for (size_t i = 0; i < mSkeleton.GetAnimCount(); i++) {
		auto& anim = mSkeleton.GetAnim(i);
		if (!_stricmp(anim.name, name.c_str())) {
			return AddRunningAnim(i);
		}
	}

	return false;
}

void AasClass2::InitializeBones() {

	mBoneMatrices.resize(mSkeleton.GetBoneCount() + 1);

	// Count the number of normal bones in the skeleton (ones that are unaffected by cloth sim)
	mNormalBoneCount = 0;
	for (size_t i = 0; i < mSkeleton.GetBoneCount(); ++i) {
		auto bone = mSkeleton.GetBone(i);
		if (!_stricmp("#ClothBone", bone.GetName())) {
			mHasClothBones = true;
			break;
		}
		mNormalBoneCount++;
	}

	field_68 = 0;

	if (mHasClothBones) {

		for (size_t i = 0; i < mSkeleton.GetBoneCount(); ++i) {

			auto bone = mSkeleton.GetBone(i);
			auto name = bone.GetName();

			if (!_strnicmp("#Sphere", name, 7)) {

				// Find the opening brace to parse the parameters
				auto params = name;
				while (*params) {
					if (*params++ == '{')
						break;
				}

				if (*params) {
					AasClothSphere sphere;
					sphere.boneId = i;
					sphere.param1 = (float) atof(params); // Most likely radius					
					mClothSpheres.emplace_back(sphere);
				} else {
					logger->warn("Found cloth bone with invalid name '{}'", name);
				}
			} else if (!_strnicmp("#Cylinder", name, 9)) {
				AasClothCylinder cylinder;
				cylinder.boneId = i;

				auto params = name;
				while (*params) {
					if (*params++ == '{') {
						break;
					}
				}
				cylinder.param1 = (float)atof(params);

				while (*params) {
					if (*params++ == ',') {
						break;
					}
				}
				cylinder.param2 = (float)atof(params);

				mClothCylinders.emplace_back(cylinder);
			}


		}
	}

}

class AasRunningStream {
public:
	void* operator new(size_t s) {
		return _aligned_malloc(s, 16);
	}

	void operator delete(void* ptr) {
		_aligned_free(ptr);
	}
};


bool AasClass2::AddRunningAnim(size_t animId) {

	Expects(animId < mSkeleton.GetAnimCount());

	auto& anim = mSkeleton.GetAnim(animId);

	/// PREVIOUSLY: 
	/// ToEE had support for propability based variations i think, but all animations 
	/// Only ever used 1, so we're stripping this feature out.

	// TODO: Finish
	return true;

}

AasSubmesh& AasClass2::GetOrCreateSubmeshForMaterial(MaterialRef material) {

	// Check for an existing submesh
	for (auto& submesh : mSubmeshes) {
		if (submesh.material == material) {
			return submesh;
		}
	}

	// Create a new one
	AasSubmesh submesh;
	submesh.material = material;
	mSubmeshes.emplace_back(submesh);

	return mSubmeshes.back();

}

AasModelAndSkeleton::AasModelAndSkeleton(const std::shared_ptr<SkaFile>& skaFile,
                                         const std::shared_ptr<SkmFile>& skmFile,
                                         AasEventListener& eventListener,
                                         gfx::EncodedAnimId animId)
	: skaFile(skaFile), skmFile(skmFile), aas2(*skaFile, *skmFile, eventListener), mEventListener(eventListener), mAnimId(animId) {

	timeLoaded = std::chrono::steady_clock::now();

	// TODO animations[v8].aas_class2_obj->vt->SetEventListener(animations[v8].aas_class2_obj, aasEventListener);

	SetAnimId(animId);
	// AasAnimatedModelSetAnimId(animations[v8].id, animId);
	// AasAnimatedModelAdvance(animations[v8].id, 0.0, 0.0, 0.0, state, &savedregs);

}


// See AasAnimatedModelSetAnimId
void AasModelAndSkeleton::SetAnimId(gfx::EncodedAnimId animId) {

	mAnimId = animId;
	if (aas2.SetAnim(animId.GetName())) {
		return;
	}

	auto fallbackState = 0;

	while (true) {
		if (animId.ToFallback()) {
			if (aas2.SetAnim(animId.GetName())) {
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

			if (aas2.SetAnim(animId.GetName())) {
				return;
			}
			continue;
		}

		if (fallbackState != 1) {
			throw TempleException("Could not find fallback animation for {} in {}", animId.GetName(), skaFilename);
		}

		fallbackState = 2;
		animId = gfx::EncodedAnimId(gfx::NormalAnimType::ItemIdle);

		if (aas2.SetAnim(animId.GetName())) {
			return;
		}

	}

}

AasRunningAnim::AasRunningAnim(AasClass2& owner, SkaAnimHeader& animHeader)
	: owner(owner), mAnimHeader(animHeader) {

	if (animHeader.streamCount > 0) {

		auto& streamHeader = animHeader.streams[0];

		if (streamHeader.frames > 0) {
			mDuration = (streamHeader.frames - 1) / streamHeader.frameRate;
		}

		mStream = std::make_unique<AasRunningStream>();

	}

}

