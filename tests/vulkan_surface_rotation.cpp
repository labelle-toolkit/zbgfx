#include "../libs/bgfx/src/surface_rotation_vk.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define CHECK(_condition) do { if (!(_condition)) { fprintf(stderr, "line %d: %s\n", __LINE__, #_condition); abort(); } } while (0)
using bgfx::vk::SurfaceRotationVK;

static void checkRect(SurfaceRotationVK::Rect r, int x, int y, unsigned w, unsigned h)
{
	CHECK(r.x == x && r.y == y && r.width == w && r.height == h);
}

int main()
{
	VkSurfaceTransformFlagBitsKHR selected;
	SurfaceRotationVK::Rotation rotation;
	const VkSurfaceTransformFlagBitsKHR flags[] = { VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR, VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR, VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR };
	for (unsigned angle = 0; angle < 4; ++angle)
	{
		const auto flag = flags[angle];
		CHECK(SurfaceRotationVK::select(flag, flag, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, selected, rotation));
		CHECK(selected == flag && unsigned(rotation) == angle);
	}
	CHECK(SurfaceRotationVK::select(VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR | VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR
		, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, selected, rotation));
	CHECK(selected == VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR && rotation == SurfaceRotationVK::Identity);
	CHECK(!SurfaceRotationVK::select(VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR, VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR
		, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, selected, rotation));
	CHECK(!SurfaceRotationVK::select(VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR, VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR
		, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, selected, rotation));
	CHECK(SurfaceRotationVK::select(VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR, VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR
		, VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR, selected, rotation));
	CHECK(selected == VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR && rotation == SurfaceRotationVK::Identity);
	CHECK(SurfaceRotationVK::select(VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
		, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, selected, rotation));
	CHECK(selected == VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR && rotation == SurfaceRotationVK::Identity);
	CHECK(!SurfaceRotationVK::select(VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, 0
		, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, selected, rotation));
	VkSurfaceCapabilitiesKHR caps = {};
	caps.currentTransform = VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR;
	caps.supportedTransforms = VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR;
	caps.minImageExtent = { 1, 1 }; caps.maxImageExtent = { 20, 20 };
	caps.currentExtent = { 9, 3 };
	SurfaceRotationVK layout(0, 0, SurfaceRotationVK::Identity);
	CHECK(SurfaceRotationVK::layout(caps, 7, 5, false, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, layout, selected));
	CHECK(layout.width == 7 && layout.height == 5 && layout.physicalWidth() == 5 && layout.physicalHeight() == 7);
	CHECK(SurfaceRotationVK::layout(caps, 7, 5, true, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, layout, selected));
	CHECK(layout.width == 9 && layout.height == 3 && layout.physicalWidth() == 3 && layout.physicalHeight() == 9);
	// Caller depth policy is shared by first creation, reset and recovery.
	CHECK(!SurfaceRotationVK::layoutWithDepth(caps, 7, 5, false, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, 7, 5, layout, selected));
	caps.supportedTransforms |= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	CHECK(SurfaceRotationVK::layoutWithDepth(caps, 7, 5, false, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, 7, 5, layout, selected));
	CHECK(selected == VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR && layout.width == 7 && layout.height == 5);
	CHECK(SurfaceRotationVK::layoutWithDepth(caps, 7, 5, false, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, 7, 7, layout, selected));
	CHECK(selected == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR && layout.physicalWidth() == 5 && layout.physicalHeight() == 7);
	CHECK(SurfaceRotationVK::layoutWithDepth(caps, 7, 5, true, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, 9, 3, layout, selected));
	CHECK(selected == VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR && layout.width == 9 && layout.height == 3);
	caps.currentExtent = { UINT32_MAX, UINT32_MAX };
	CHECK(SurfaceRotationVK::layout(caps, 7, 5, true, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, layout, selected));
	CHECK(layout.width == 7 && layout.height == 5);
	caps.minImageExtent = { 6, 8 }; caps.maxImageExtent = { 6, 8 };
	CHECK(SurfaceRotationVK::layout(caps, 7, 5, false, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, layout, selected));
	CHECK(layout.width == 8 && layout.height == 6 && layout.physicalWidth() == 6 && layout.physicalHeight() == 8);
	caps.minImageExtent = { 7, 8 };
	CHECK(!SurfaceRotationVK::layout(caps, 7, 5, false, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, layout, selected));
	caps.minImageExtent = { 0, 0 }; caps.maxImageExtent = { 0, 0 };
	CHECK(!SurfaceRotationVK::layout(caps, 7, 5, false, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, layout, selected));
	// Hand-derived asymmetric rectangle expectations: square/full-target
	// cases alone would miss swapped axes, wrong direction and edge offsets.
	const SurfaceRotationVK::Rect expected[] = { { 1, 2, 3, 2 }, { 1, 1, 2, 3 }, { 3, 1, 3, 2 }, { 2, 3, 2, 3 } };
	for (unsigned angle = 0; angle < 4; ++angle)
	{
		const SurfaceRotationVK transform(7, 5, SurfaceRotationVK::Rotation(angle));
		const auto r = transform.mapRect({ 1, 2, 3, 2 });
		checkRect(r, expected[angle].x, expected[angle].y, expected[angle].width, expected[angle].height);
		checkRect(transform.clipRect({ -2, -1, 5, 4 }), 0, 0, 3, 3);
		checkRect(transform.clipRect({ 6, 4, 10, 10 }), 6, 4, 1, 1);
		checkRect(transform.clipRect({ 9, 0, 2, 2 }), 0, 0, 0, 0);
		checkRect(transform.clipRect({ -10, -10, 1, 1 }), 0, 0, 0, 0);
		checkRect(transform.clipRect({ 1, 1, 0, 2 }), 0, 0, 0, 0);
		checkRect(transform.clipRect({ -2, -1, UINT32_MAX, UINT32_MAX }), 0, 0, 7, 5);
		checkRect(transform.clipRect({ INT32_MAX, INT32_MAX, UINT32_MAX, UINT32_MAX }), 0, 0, 0, 0);
		const auto full = transform.mapRect({ 0, 0, 7, 5 });
		checkRect(full, 0, 0, transform.physicalWidth(), transform.physicalHeight());

		// Every logical pixel must map bijectively inside physical extent.
		bool used[35] = {};
		for (unsigned y = 0; y < 5; ++y) for (unsigned x = 0; x < 7; ++x)
		{
			const auto p = transform.mapPixel(x, y);
			CHECK(p.x < transform.physicalWidth() && p.y < transform.physicalHeight());
			const unsigned index = p.y*transform.physicalWidth()+p.x;
			CHECK(!used[index]); used[index] = true;
		}

		// Pixel-center rasterization with a negative-height viewport must agree
		// with the rectangle/pixel mapping (including edge pixel minus-one).
		float identity[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }, projection[16];
		transform.rotateProjection(projection, identity);
		for (unsigned y = 0; y < 5; ++y) for (unsigned x = 0; x < 7; ++x)
		{
			const float cx = 2.0f*(x+0.5f)/7-1, cy = 1-2.0f*(y+0.5f)/5;
			const float rx = cx*projection[0]+cy*projection[4];
			const float ry = cx*projection[1]+cy*projection[5];
			const auto p = transform.mapPixel(x, y);
			CHECK(fabsf((rx+1)*0.5f*transform.physicalWidth()-(p.x+0.5f)) < 0.00001f);
			CHECK(fabsf((1-ry)*0.5f*transform.physicalHeight()-(p.y+0.5f)) < 0.00001f);
		}
		// Non-identity perspective/translation values, including z/w, and alias.
		float source[16], copy[16], rotated[16];
		for (unsigned i = 0; i < 16; ++i) source[i] = copy[i] = float(i+1);
		transform.rotateProjection(rotated, source);
		transform.rotateProjection(copy, copy);
		CHECK(0 == memcmp(copy, rotated, sizeof(copy)));
		for (unsigned i = 0; i < 16; i += 4) CHECK(rotated[i+2] == source[i+2] && rotated[i+3] == source[i+3]);

		// Upright asymmetric byte pattern across padded rows and formats.
		for (unsigned bpp = 1; bpp <= 16; ++bpp)
		{
			unsigned char physical[1024], logical[1024];
			memset(physical, 0xcd, sizeof(physical)); memset(logical, 0xef, sizeof(logical));
			const unsigned srcPitch = transform.physicalWidth()*bpp+3, dstPitch = 7*bpp+5;
			for (unsigned y = 0; y < 5; ++y) for (unsigned x = 0; x < 7; ++x)
			{
				const auto p = transform.mapPixel(x, y);
				for (unsigned byte = 0; byte < bpp; ++byte) physical[p.y*srcPitch+p.x*bpp+byte] = (y*7+x)*3+byte;
			}
			CHECK(transform.readLogicalPixels(logical, dstPitch, sizeof(logical), physical, srcPitch, sizeof(physical), bpp));
			for (unsigned y = 0; y < 5; ++y)
			{
				for (unsigned x = 0; x < 7; ++x) for (unsigned byte = 0; byte < bpp; ++byte) CHECK(logical[y*dstPitch+x*bpp+byte] == (y*7+x)*3+byte);
				for (unsigned byte = 7*bpp; byte < dstPitch; ++byte) CHECK(logical[y*dstPitch+byte] == 0xef);
			}
			CHECK(logical[5*dstPitch] == 0xef);
			CHECK(!transform.readLogicalPixels(logical, 7*bpp-1, sizeof(logical), physical, srcPitch, sizeof(physical), bpp));
			CHECK(!transform.readLogicalPixels(logical, dstPitch, sizeof(logical), physical, transform.physicalWidth()*bpp-1, sizeof(physical), bpp));
			CHECK(!transform.readLogicalPixels(logical, dstPitch, dstPitch*4, physical, srcPitch, sizeof(physical), bpp));
			CHECK(!transform.readLogicalPixels(logical, dstPitch, sizeof(logical), physical, srcPitch, srcPitch*(transform.physicalHeight()-1), bpp));
			CHECK(!transform.readLogicalPixels(physical+1, dstPitch, sizeof(physical)-1, physical, srcPitch, sizeof(physical), bpp));
		}
	}
	puts("PASS supported/unsupported/mirror/inherit selection; 0/90/180/270 asymmetric rect, clipping, projection/pixel-center agreement, bijection and padded readback (1..16 byte pixels)");
}
