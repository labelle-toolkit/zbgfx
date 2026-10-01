/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#ifndef BGFX_SURFACE_ROTATION_VK_H_HEADER_GUARD
#define BGFX_SURFACE_ROTATION_VK_H_HEADER_GUARD

#include <stdint.h>
#include <string.h>
#include "../3rdparty/khronos/vulkan-local/vulkan_core.h"

namespace bgfx { namespace vk
{
	// Coordinates presented to the application remain logical/top-left. Only
	// swapchain attachments use physical/pre-rotated coordinates. Offscreen
	// targets must never acquire this transform.
	struct SurfaceRotationVK
	{
		enum Rotation { Identity, Clockwise90, Clockwise180, Clockwise270 };
		struct Rect { int32_t x, y; uint32_t width, height; };
		struct Point { uint32_t x, y; };

		static bool select(VkSurfaceTransformFlagBitsKHR _current, VkSurfaceTransformFlagsKHR _supported
			, VkSurfaceTransformFlagBitsKHR _preferred, VkSurfaceTransformFlagBitsKHR& _selected, Rotation& _rotation)
		{
			_rotation = Identity;
			if (0 != (_supported & _current))
			{
				switch (_current)
				{
				case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR: _selected = _current; return true;
				case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR: _selected = _current; _rotation = Clockwise90; return true;
				case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR: _selected = _current; _rotation = Clockwise180; return true;
				case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR: _selected = _current; _rotation = Clockwise270; return true;
				default: break;
				}
			}
			// Keep existing compositor/platform-managed paths for mirrors and
			// INHERIT. Never choose an unsupported bit or interpret a mirror as a
			// rotation (which would also invert winding).
			if (VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR == _preferred && 0 != (_supported & _preferred))
			{
				_selected = _preferred; return true;
			}
			if (0 != (_supported & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR))
			{
				_selected = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; return true;
			}
			return false;
		}

		SurfaceRotationVK(uint32_t _width, uint32_t _height, Rotation _rotation)
			: width(_width), height(_height), rotation(_rotation) {}

		bool swapsAxes() const { return Clockwise90 == rotation || Clockwise270 == rotation; }
		uint32_t physicalWidth() const { return swapsAxes() ? height : width; }
		uint32_t physicalHeight() const { return swapsAxes() ? width : height; }

		static bool layout(const VkSurfaceCapabilitiesKHR& _caps, uint32_t _width, uint32_t _height, bool _recoverExtent
			, VkSurfaceTransformFlagBitsKHR _preferred, SurfaceRotationVK& _out, VkSurfaceTransformFlagBitsKHR& _selected)
		{
			Rotation rotation;
			if (!select(_caps.currentTransform, _caps.supportedTransforms, _preferred, _selected, rotation)
			|| _caps.minImageExtent.width > _caps.maxImageExtent.width
			|| _caps.minImageExtent.height > _caps.maxImageExtent.height)
			{
				return false;
			}
			const bool concrete = _recoverExtent && UINT32_MAX != _caps.currentExtent.width && UINT32_MAX != _caps.currentExtent.height;
			const SurfaceRotationVK requested(concrete ? _caps.currentExtent.width : _width, concrete ? _caps.currentExtent.height : _height, rotation);
			uint32_t physicalWidth = requested.physicalWidth(), physicalHeight = requested.physicalHeight();
			physicalWidth = physicalWidth < _caps.minImageExtent.width ? _caps.minImageExtent.width : physicalWidth;
			physicalWidth = physicalWidth > _caps.maxImageExtent.width ? _caps.maxImageExtent.width : physicalWidth;
			physicalHeight = physicalHeight < _caps.minImageExtent.height ? _caps.minImageExtent.height : physicalHeight;
			physicalHeight = physicalHeight > _caps.maxImageExtent.height ? _caps.maxImageExtent.height : physicalHeight;
			if (0 == physicalWidth || 0 == physicalHeight || physicalWidth > INT32_MAX || physicalHeight > INT32_MAX) { return false; }
			_out = SurfaceRotationVK(requested.swapsAxes() ? physicalHeight : physicalWidth, requested.swapsAxes() ? physicalWidth : physicalHeight, rotation);
			return true;
		}

		static bool layoutWithDepth(const VkSurfaceCapabilitiesKHR& _caps, uint32_t _width, uint32_t _height, bool _recoverExtent
			, VkSurfaceTransformFlagBitsKHR _preferred, uint32_t _depthWidth, uint32_t _depthHeight
			, SurfaceRotationVK& _out, VkSurfaceTransformFlagBitsKHR& _selected)
		{
			if (!layout(_caps, _width, _height, _recoverExtent, _preferred, _out, _selected)) { return false; }
			// Preserve caller-owned logical-sized depth attachments. If they cannot
			// cover the rotated image, retain compositor-managed presentation.
			if (Identity != _out.rotation && (_depthWidth < _out.physicalWidth() || _depthHeight < _out.physicalHeight()))
			{
				VkSurfaceCapabilitiesKHR fallback = _caps;
				fallback.currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
				return layout(fallback, _width, _height, _recoverExtent, _preferred, _out, _selected);
			}
			return true;
		}

		// Viewports can extend outside the target: clipping them would change the
		// projection's scale. Clip scissors/render areas first, then map them.
		Rect mapRect(Rect _rect) const
		{
			switch (rotation)
			{
			case Clockwise90: return { int32_t(int64_t(height)-_rect.y-_rect.height), _rect.x, _rect.height, _rect.width };
			case Clockwise180: return { int32_t(int64_t(width)-_rect.x-_rect.width), int32_t(int64_t(height)-_rect.y-_rect.height), _rect.width, _rect.height };
			case Clockwise270: return { _rect.y, int32_t(int64_t(width)-_rect.x-_rect.width), _rect.height, _rect.width };
			default: return _rect;
			}
		}

		Rect clipRect(Rect _rect) const
		{
			const int64_t x0 = _rect.x < 0 ? 0 : _rect.x;
			const int64_t y0 = _rect.y < 0 ? 0 : _rect.y;
			const int64_t right = int64_t(_rect.x)+_rect.width;
			const int64_t bottom = int64_t(_rect.y)+_rect.height;
			const int64_t x1 = right > width ? width : right;
			const int64_t y1 = bottom > height ? height : bottom;
			if (x0 >= x1 || y0 >= y1) { return { 0, 0, 0, 0 }; }
			return { int32_t(x0), int32_t(y0), uint32_t(x1-x0), uint32_t(y1-y0) };
		}

		Point mapPixel(uint32_t _x, uint32_t _y) const
		{
			switch (rotation)
			{
			case Clockwise90: return { height-1-_y, _x };
			case Clockwise180: return { width-1-_x, height-1-_y };
			case Clockwise270: return { _y, width-1-_x };
			default: return { _x, _y };
			}
		}

		// bgfx uses a negative-height Vulkan viewport. Clockwise screen rotation
		// therefore maps clip (x,y) to (y,-x), unlike a positive-height viewport.
		// Rotate output columns; preserve z/w and allow in-place application.
		void rotateProjection(float* _out, const float* _in) const
		{
			for (uint32_t row = 0; row < 4; ++row)
			{
				const uint32_t ii = row*4;
				const float x = _in[ii], y = _in[ii+1];
				switch (rotation)
				{
				case Clockwise90: _out[ii] = y; _out[ii+1] = -x; break;
				case Clockwise180: _out[ii] = -x; _out[ii+1] = -y; break;
				case Clockwise270: _out[ii] = -y; _out[ii+1] = x; break;
				default: _out[ii] = x; _out[ii+1] = y; break;
				}
				_out[ii+2] = _in[ii+2]; _out[ii+3] = _in[ii+3];
			}
		}

		// Restore upright logical pixels without changing byte format/channel
		// order. Source/destination row padding is independent and untouched.
		bool readLogicalPixels(void* _dst, uint32_t _dstPitch, uint64_t _dstSize, const void* _src, uint32_t _srcPitch, uint64_t _srcSize, uint32_t _bytesPerPixel) const
		{
			if (0 == width || 0 == height || 0 == _bytesPerPixel
			|| uint64_t(width)*_bytesPerPixel > _dstPitch
			|| uint64_t(physicalWidth())*_bytesPerPixel > _srcPitch
			|| NULL == _dst || NULL == _src || _dst == _src)
			{
				return false;
			}
			const uint64_t dstNeeded = uint64_t(height-1)*_dstPitch+uint64_t(width)*_bytesPerPixel;
			const uint64_t srcNeeded = uint64_t(physicalHeight()-1)*_srcPitch+uint64_t(physicalWidth())*_bytesPerPixel;
			const uintptr_t dstAddress = reinterpret_cast<uintptr_t>(_dst), srcAddress = reinterpret_cast<uintptr_t>(_src);
			if (dstNeeded > _dstSize || srcNeeded > _srcSize
			|| dstNeeded > UINTPTR_MAX-dstAddress || srcNeeded > UINTPTR_MAX-srcAddress
			|| (dstAddress < srcAddress+srcNeeded && srcAddress < dstAddress+dstNeeded))
			{
				return false;
			}
			for (uint32_t yy = 0; yy < height; ++yy)
			{
				for (uint32_t xx = 0; xx < width; ++xx)
				{
					const Point point = mapPixel(xx, yy);
					memcpy(static_cast<uint8_t*>(_dst)+uint64_t(yy)*_dstPitch+uint64_t(xx)*_bytesPerPixel
						, static_cast<const uint8_t*>(_src)+uint64_t(point.y)*_srcPitch+uint64_t(point.x)*_bytesPerPixel
						, _bytesPerPixel);
				}
			}
			return true;
		}

		uint32_t width, height;
		Rotation rotation;
	};
} }
#endif // BGFX_SURFACE_ROTATION_VK_H_HEADER_GUARD
