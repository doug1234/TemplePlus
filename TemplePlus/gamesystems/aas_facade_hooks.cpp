
#include "stdafx.h"

#include <temple/dll.h>
#include <util/fixes.h>

#include "aas_facade.h"

using namespace temple;

namespace aas {

	static class AASSystemHooks : public TempleFix {
	public:
		void apply() override {

			static auto aasSystem = std::make_unique<AASSystem>();

			// AasAnimatedModelGetSubmeshes
			replaceFunction<AasStatus(AasHandle, int **, int *)>(0x10263a50, [](AasHandle handle, int **submeshMaterials, int *pSubmeshCountOut) {
				return aasSystem->GetSubmeshes(handle, submeshMaterials, pSubmeshCountOut);
			});

			// AasAnimatedModelAddAddmesh
			replaceFunction<AasStatus(AasHandle, const char *)>(0x10262e30, [](AasHandle handle, const char *filename) {
				return aasSystem->AddAddmesh(handle, filename);
			});

			// AasAnimatedModelClearAddmeshes
			replaceFunction<AasStatus(AasHandle)>(0x10262ec0, [](AasHandle handle) {
				return aasSystem->ClearAddmeshes(handle);
			});

			// AasAnimatedModelGetAnimId
			replaceFunction<AasStatus(AasHandle, gfx::EncodedAnimId *)>(0x102627e0, [](AasHandle handle, gfx::EncodedAnimId *animIdOut) {
				return aasSystem->GetAnimId(handle, animIdOut);
			});

			// AasAnimatedModelGetSubmesh
			replaceFunction<AasStatus(AasHandle, AasSubmesh **, const AasAnimParams *, int)>(0x10263400, [](AasHandle handle, AasSubmesh **pSubmeshOut, const AasAnimParams *state, int submeshIdx) {
				return aasSystem->GetSubmesh(handle, pSubmeshOut, *state, submeshIdx);
			});

			// AasAnimatedModelFreeSubmesh
			replaceFunction<AasStatus(AasSubmesh *)>(0x10262500, [](AasSubmesh *submesh) {
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

				return aasSystem->GetBoneWorldMatrix(handle, *state, pOut, boneName);
			});

			// AasAnimatedModelGetBoneWorldMatrixByNameForChild
			replaceFunction<AasStatus(AasHandle, AasHandle, const AasAnimParams*, XMFLOAT4X4*, const char*)>(0x102631E0,
				[](AasHandle parentHandle, AasHandle handle, const AasAnimParams* state, XMFLOAT4X4* matrixOut, const char *boneName) {
				return aasSystem->GetBoneWorldMatrix(parentHandle, handle, *state, matrixOut, boneName);
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
			/*replaceFunction<AasStatus(int, int, gfx::EncodedAnimId,
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
			});*/

			// AasAnimatedModelFree
			replaceFunction<AasStatus(AasHandle)>(0x10264510, [](AasHandle handle) {
				return aasSystem->Free(handle);
			});
		}
	} hooks;

}
