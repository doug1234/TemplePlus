
#include "stdafx.h"
#include "lightningrenderer.h"
#include <util/fixes.h>
#include <temple/dll.h>
#include <tig/tig_timer.h>
#include <sound.h>
#include <gamesystems/gamesystems.h>
#include <gamesystems/particlesystems.h>
#include "graphics/math.h"

using namespace gfx;

constexpr int MAX_SEGMENTS = 600;
// Minimum length of a fork in line-segments
constexpr int MinForkSegments = 16;
constexpr int ForkCount = 2;
class LightningRendererImpl;
std::unique_ptr<LightningRendererImpl> mLRImpl(nullptr);


using JitterArray = float[600];
class LightningRendererImpl {
public:

	LightningRendererImpl() {
		CreateIndexBuffer();
	}

	void Render();
	void RenderChainLightning();

	void RenderMainArc(XMFLOAT3 from, XMFLOAT3 to, int segments, XMFLOAT3 normal, float colorRamp);
	void RenderForks(XMFLOAT3 from, XMFLOAT3 to, int segments, XMFLOAT3 normal, float colorRamp, int minLength, int maxLength);

	float GetOffsetAt(int i, int segments);
	void CalculateLineJitter(int elapsedMs, int segments, float lengthPerSegment, float noiseY);

	
	const float ForkArcWidth = 8;
	const float MaxJitterOffset = 80.f;

protected:
	JitterArray& mJitterArray = temple::GetRef<float[600]>(0x10B397C0);


	XMFLOAT4 mPositions[MAX_SEGMENTS * 2];
	XMCOLOR color[MAX_SEGMENTS * 2];
	uint16_t mIndices[MAX_SEGMENTS * 6];

	void CreateIndexBuffer() {
		auto indexIdx = 0;
		for (auto i = 0; i < 2 * MAX_SEGMENTS; i += 2) {
			mIndices[indexIdx + 0] = i;
			mIndices[indexIdx + 1] = i + 1;
			mIndices[indexIdx + 2] = i + 3;
			mIndices[indexIdx + 3] = i;
			mIndices[indexIdx + 4] = i + 3;
			mIndices[indexIdx + 5] = i + 2;
			indexIdx += 6;
		}
	};

};

static class LightningRenderHooks : public TempleFix {
public:
	void RedirectChainLightningTargets();
	void apply() override {
		redirectCall(0x10088147, Render);
		redirectCall(0x100882FC, Render);
		redirectCall(0x100885F5, Render);
		redirectCall(0x10088A43, Render);
		redirectCall(0x10088D75, Render);
		redirectCall(0x100891BB, Render);
		redirectCall(0x100894FB, Render);
		redirectCall(0x10089933, Render);
		redirectCall(0x10089C96, Render);
		redirectCall(0x10089EBC, Render);

		RedirectChainLightningTargets();

		// Fix for chain lightning renderer with long segments - copy-pasted constant 0x120 (288) from lightning bolt was used instead of 0x258 (600)
		writeHex(0x100887B6 + 2, "58 02"); // cmp     esi, 120h
		writeHex(0x10088A80 + 2, "58 02"); // lea     ecx, [ebp+120h]
		writeHex(0x10088F3A + 2, "58 02"); // cmp     esi, 120h
		writeHex(0x100891FA + 2, "58 02"); // add     esi, 120h

	}

	static LightningRenderer* renderer;

	static int Render(int vertexCount,
		XMFLOAT4 *vertices,
		XMFLOAT4 *normals,
		XMCOLOR *diffuse,
		XMFLOAT2 *uv,
		int primCount,
		uint16_t *indices,
		int shaderId);

	ChainLightningTarget chainTargets[30]; // 0x10AB7E68
	

} lightningHooks;

LightningRenderer* LightningRenderHooks::renderer = nullptr;

#pragma pack(push, 1)
struct LightningVertex {
	XMFLOAT4 pos;
	XMFLOAT4 normal;
	XMCOLOR diffuse;
	XMFLOAT2 texCoord;
};
#pragma pack(pop)

LightningRenderer::LightningRenderer(MdfMaterialFactory& mdfFactory,
	RenderingDevice& device) : mMdfFactory(mdfFactory), mDevice(device), mBufferBinding(device.CreateMdfBufferBinding(true)) {

	mMaterial = mdfFactory.LoadMaterial("art/meshes/lightning.mdf", [](gfx::MdfMaterial& mat) {
		mat.perVertexColor = true; });

	mIndexBuffer = device.CreateEmptyIndexBuffer((MAX_SEGMENTS-1)*6 );
	mVertexBuffer = device.CreateEmptyVertexBuffer(sizeof(LightningVertex) * MAX_SEGMENTS*2);

	mBufferBinding.AddBuffer(mVertexBuffer, 0, sizeof(LightningVertex))
		.AddElement(VertexElementType::Float4, VertexElementSemantic::Position)
		.AddElement(VertexElementType::Float4, VertexElementSemantic::Normal)
		.AddElement(VertexElementType::Color, VertexElementSemantic::Color)
		.AddElement(VertexElementType::Float2, VertexElementSemantic::TexCoord);

	LightningRenderHooks::renderer = this;
	mLRImpl = std::make_unique<LightningRendererImpl>();
}

LightningRenderer::~LightningRenderer() {
	LightningRenderHooks::renderer = nullptr;

}

void LightningRenderer::Render() {
	mLRImpl.get()->Render();
}

double PerlinNoise2D(float x, float y, float persistence = 4, float lacunarity = 4, int octaves = 3) {
	static auto perlinNoise2D = temple::GetRef<double(float, float, float, float, int)>(0x10087AC0);

	return perlinNoise2D(x, y, persistence, lacunarity, octaves);
}

float GetJitterTimePeriod(int elapsedMs) {
	return (elapsedMs % 127) / 200.f;
}

// calculates mJitterArray 
void LightningRendererImpl::CalculateLineJitter(int elapsedMs, int segments, float lengthPerSegment, float noiseY) {
	
	auto timePeriod = GetJitterTimePeriod(elapsedMs);
	mJitterArray[0] = static_cast<float>(PerlinNoise2D(timePeriod, noiseY) * MaxJitterOffset);

	for (auto i = 0; i < segments; ++i) {
		auto y = i / (720.f / lengthPerSegment - 1.0f) * 720.f / 400.f + noiseY;
		auto absOffset = PerlinNoise2D(timePeriod, y) * MaxJitterOffset;
		mJitterArray[i] = static_cast<float>(absOffset) - mJitterArray[0];
	}

	// The start of the chain-lightning should not be offset, it should be anchored at the caster
	mJitterArray[0] = 0.f;
}

void LightningRendererImpl::RenderChainLightning()
{
	/// Delay in milliseconds before an arc that is the result of chaining to another target is shown.
	/// First arc is shown immediately.
	const int ChainDelay = 1 * 256;
	/// How long an arc should be visible in milliseconds.
	const int Duration = 7 * ChainDelay;

	

	// The length of a single line segment
	const float LineSegmentLength = 2.5f;
	// The maximum number of segments that can be rendered (length will be clamped so this is not exceeded)
	const int MaxLineSegments = 600;

	auto& mChainLightningPfxStartTime = temple::GetRef<uint32_t>(0x10AB7E50);
	auto& mChainLightningPfxState = temple::GetRef<int>(0x10AB7E54);
	auto& mChainLightningRefPt = temple::GetRef<XMFLOAT3>(0x10AB7E58);
	auto& mChainLightningTgtCount = temple::GetRef<int>(0x10AB7E64);

	if (mChainLightningPfxState == 1) {
		mChainLightningPfxStartTime = TigGetSystemTime();
		mChainLightningPfxState = 2;
	}
	else if (mChainLightningPfxState != 2) {
		return;
	}

	
	auto etime = TigElapsedSystemTime(mChainLightningPfxStartTime);
	auto& targets = lightningHooks.chainTargets;

	for (auto i = 0; i < mChainLightningTgtCount; ++i) {

		auto tgtElapsed = etime - i * ChainDelay;
		if (tgtElapsed < 0 || tgtElapsed > Duration) {
			continue;
		}

		auto& tgt = targets[i];
		if (tgt.obj) {
			sound.PlaySoundAtObj(7027, tgt.obj, 1);
			gameSystems->GetParticleSys().CreateAtObj("sp-Chain Lightning-hit", tgt.obj);
			tgt.obj = objHndl::null;
		}

		auto from = i > 0 ? targets[i - 1].vec : mChainLightningRefPt;
		auto to = targets[i].vec;
		{

			auto xmline = DirectX::XMVectorSubtract(XMLoadFloat3(&to), XMLoadFloat3(&from) );
			auto xmlineLength = DirectX::XMVector3Length(xmline);
			float lineLength;
			DirectX::XMStoreFloat(&lineLength, xmlineLength);
			xmline = DirectX::XMVector3Normalize(xmline);
			
			XMFLOAT3 line;
			DirectX::XMStoreFloat3(&line, xmline);

			auto normal = XMFLOAT3(-line.z, 0, line.x); // Normal to the line on the XZ plane (floor plane)

			auto colorRamp = cosf( static_cast<float>((tgtElapsed % Duration) / (float)(Duration-1) * M_PI_2 ));

			auto segments = min(MaxLineSegments, (int) roundf(lineLength / LineSegmentLength)); // fixes issue where out of bounds was reached; will now cap at 600 and make longer segments if arc is longer

			// This causes the noise to be sampled in a different grid-cell per target
			auto targetNoiseOffset = 0.5f * i;

			CalculateLineJitter(tgtElapsed, segments, LineSegmentLength, targetNoiseOffset);


			RenderMainArc(from, to, segments, normal, colorRamp);

			RenderForks(from, to, segments, normal, colorRamp, MinForkSegments, segments);

		}
	}


	auto totalDuration = (mChainLightningTgtCount - 1) * ChainDelay + Duration;
	if (etime > totalDuration) {
		mChainLightningPfxState = 0;
	}
}

void LightningRenderer::Render(size_t vertexCount, XMFLOAT4* positions, XMFLOAT4* normals, XMCOLOR* diffuse, XMFLOAT2* uv, size_t primCount, uint16_t* indices) {

	if (!primCount)
		return;

	auto vbLock(mDevice.Map<LightningVertex>(*mVertexBuffer));
	
	for (size_t i = 0; i < vertexCount; ++i) {
		auto &vertex = vbLock[i];
		vertex.pos.x = positions[i].x;
		vertex.pos.y = positions[i].y;
		vertex.pos.z = positions[i].z;
		vertex.pos.w = 1;

		vertex.normal.x = 0; //normals[i].x;
		vertex.normal.y = 1; //normals[i].y;
		vertex.normal.z = 0; //normals[i].z;
		vertex.normal.w = 0;

		vertex.diffuse = diffuse[i];
		
		auto leftVertex = (i % 2) == 0;
		vertex.texCoord = //uv[i];
			leftVertex ? XMFLOAT2(1.0f, 0.5f) : XMFLOAT2(0.0, 0.5);
			
	}

	vbLock.Unmap();

	mIndexBuffer->Update(gsl::span(indices, primCount * 3));

	mBufferBinding.Bind();
	mDevice.SetIndexBuffer(*mIndexBuffer);

	mMaterial->Bind(mDevice, {});

	mDevice.DrawIndexed(
		gfx::PrimitiveType::TriangleList,
		vertexCount,
		primCount * 3);

}

float GetMainArcDistanceFade(int i, int segments) {
	auto result = 1.0f; //cosf((float)i / (segments - 1) * M_PI_2);
	return result;
}

void LightningRendererImpl::RenderMainArc(XMFLOAT3 from, XMFLOAT3 to, int segments, XMFLOAT3 normal, float timeFade)
{
	constexpr float MainArcWidth = 24.f;

	XMFLOAT3 extrudedVec(normal.x * MainArcWidth / 2,
		normal.y * MainArcWidth / 2,
		normal.z * MainArcWidth / 2);

	

	for (auto i = 0; i < segments; ++i) {
		auto& leftVtx = mPositions[2*i];
		auto& rightVtx = mPositions[2*i+1];
		auto alpha = (uint8_t)(timeFade * GetMainArcDistanceFade(i, segments) * 255);
		color[2 * i + 1] = color[2*i]   = XMCOLOR_ARGB(alpha, alpha, alpha, alpha);
		

		auto jitterOffset = GetOffsetAt(i, segments);

		auto xmpos = DirectX::XMVectorLerp(
			DirectX::XMLoadFloat3(&from),
			DirectX::XMLoadFloat3(&to), 
			i / (float)(segments - 1) 
		);
		XMFLOAT3 pos;
		DirectX::XMStoreFloat3(&pos, xmpos);
		leftVtx.x = pos.x;
		leftVtx.y = pos.y;
		leftVtx.z = pos.z;
		rightVtx.x = pos.x;
		rightVtx.y = pos.y;
		rightVtx.z = pos.z;

		leftVtx.x += jitterOffset * normal.x + extrudedVec.x;
		leftVtx.y += jitterOffset * normal.y + extrudedVec.y;
		leftVtx.z += jitterOffset * normal.z + extrudedVec.z;
		rightVtx.x += jitterOffset * normal.x - extrudedVec.x;
		rightVtx.y += jitterOffset * normal.y - extrudedVec.y;
		rightVtx.z += jitterOffset * normal.z - extrudedVec.z;
	}

	LightningRenderHooks::renderer->Render(segments * 2, mPositions, nullptr, color, nullptr, (segments-1 )* 2, mIndices);
}

void LightningRendererImpl::RenderForks(XMFLOAT3 from, XMFLOAT3 to, int segments, XMFLOAT3 perpenNormal, float timeFade, int minLength, int maxLength)
{
	Expects(minLength < maxLength);

	if (segments < minLength)
		return;

	for (auto i = 0; i < ForkCount; ++i) {
		auto forkSegments = minLength + rand() % (maxLength - minLength);

		// "Reuse" a different segment of the arc's jitter to make the offsets for the fork look different
		auto segmentSection = segments - forkSegments;
		if (segmentSection < 1) {
			segmentSection = 1;
		}
			
		auto jitterOverlayStart = rand() % segmentSection;
		auto forkStartSegment = rand() % segmentSection;

		//// The offset at the start of the fork should equal the main arc's offset so that the fork seems
		//// to "split" from it at that point.
		auto forkBaseOffset = GetOffsetAt(forkStartSegment + 1, segments);

		//// Forks build backwards, interestingly
		auto curForkSegment = forkStartSegment + forkSegments;

		//// Forks have a width of 8
		auto forkExtrudeVec = XMFLOAT3(
			perpenNormal.x * ForkArcWidth / 2,
			perpenNormal.y * ForkArcWidth / 2, 
			perpenNormal.z * ForkArcWidth / 2);

		for (auto j = 0; j < forkSegments; j++)
		{
			auto& leftVtx = mPositions[2 * j];
			auto& rightVtx = mPositions[2 * j + 1];

			auto xmpos = DirectX::XMVectorLerp(
				DirectX::XMLoadFloat3(&from),
				DirectX::XMLoadFloat3(&to),
				curForkSegment / (float)(segments - 1)
			);
		
			auto jitterOffset = forkBaseOffset;
			// Amplify the jitter for the arc (but keep it 0 at the start)
			jitterOffset += 2 * (mJitterArray[jitterOverlayStart + j] - mJitterArray[jitterOverlayStart + forkSegments - 1]);

			XMFLOAT3 pos;
			DirectX::XMStoreFloat3(&pos, xmpos);
			leftVtx.x = pos.x;
			leftVtx.y = pos.y;
			leftVtx.z = pos.z;
			rightVtx.x = pos.x;
			rightVtx.y = pos.y;
			rightVtx.z = pos.z;

			leftVtx.x += jitterOffset * perpenNormal.x + forkExtrudeVec.x;
			leftVtx.y += jitterOffset * perpenNormal.y + forkExtrudeVec.y;
			leftVtx.z += jitterOffset * perpenNormal.z + forkExtrudeVec.z;
			rightVtx.x += jitterOffset * perpenNormal.x - forkExtrudeVec.x;
			rightVtx.y += jitterOffset * perpenNormal.y - forkExtrudeVec.y;
			rightVtx.z += jitterOffset * perpenNormal.z - forkExtrudeVec.z;

			auto distanceFade = j / (float)(forkSegments - 1);
			distanceFade *= GetMainArcDistanceFade(curForkSegment, segments);

			
			auto forkAlpha = (uint8_t)(timeFade * distanceFade * 255);
			auto forkColor = XMCOLOR_ARGB(forkAlpha, forkAlpha, forkAlpha, forkAlpha);
			color[2 * j + 1] = color[2 * j] = forkColor;
			--curForkSegment;
		}
		LightningRenderHooks::renderer->Render(forkSegments * 2, mPositions, nullptr, color, nullptr, (forkSegments - 1) * 2, mIndices);
	}
}

float LightningRendererImpl::GetOffsetAt(int i, int segments)
{
	auto result = mJitterArray[i] - (mJitterArray[segments - 1] - mJitterArray[0]) * i / (segments - 1);
	return result;
}

void LightningRenderHooks::RedirectChainLightningTargets()
{
	int writeval;
	
	writeval = (int)&chainTargets[0].obj;
	write(0x1008866E + 1, &writeval, sizeof(void*));
	writeval = sizeof(int) + (int)&chainTargets[0].obj;
	write(0x10088673 + 2, &writeval, sizeof(void*));


	writeval = sizeof(int) + (int)&chainTargets[0].obj;
	write(0x1008868D + 1, &writeval, sizeof(void*));
	writeval = (int)&chainTargets[0].obj;
	write(0x10088692 + 2, &writeval, sizeof(void*));
	

	writeval = (int)&chainTargets[0].obj;
	write(0x100886B3 + 2, &writeval, sizeof(void*));
	writeval = sizeof(int) + (int)&chainTargets[0].obj;
	write(0x100886B9 + 2, &writeval, sizeof(void*));
	

	writeval = (int)&chainTargets[0].vec.x;
	write(0x100886BF + 2, &writeval, sizeof(void*));
	writeval = (int)&chainTargets[0].vec.y;
	write(0x100886DC + 2, &writeval, sizeof(void*));
	writeval = (int)&chainTargets[0].vec.z;
	write(0x100886EC + 2, &writeval, sizeof(void*));
	

	writeval = (int)&chainTargets[1];
	write(0x10088DAF + 1, &writeval, sizeof(void*));


	writeval = (int)&chainTargets[0].vec.z;
	write(0x100874FE + 1, &writeval, sizeof(void*));
	
}

int LightningRenderHooks::Render(int vertexCount, XMFLOAT4* vertices, XMFLOAT4* normals, XMCOLOR* diffuse, XMFLOAT2* uv, int primCount, uint16_t* indices, int shaderId) {
	if (renderer) {
		renderer->Render(vertexCount, vertices, normals, diffuse, uv, primCount, indices);
	}
	return 0;
}




void LightningRendererImpl::Render()
{
	gfx::PerfGroup perfGroup(LightningRenderHooks::renderer->mDevice, "Lightning PFX");

	static auto render_pfx_lightning_related = temple::GetPointer<int()>(0x10087e60);

	static auto& mCallLightningPfxState = temple::GetRef<int>(0x10B397B4);

	auto& mChainLightningPfxState = temple::GetRef<int>(0x10AB7E54);
	if (mChainLightningPfxState != 0) {
		return RenderChainLightning();
	}

	render_pfx_lightning_related();
}
