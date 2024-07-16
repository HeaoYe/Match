#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    /*
    从这里开始是vulkan/vulkan_enums.hpp 的代码
    */
    template <typename FlagBitsType>
    struct FlagTraits
    {
        static VULKAN_HPP_CONST_OR_CONSTEXPR bool isBitmask = false;
    };

    template <typename BitType>
    class Flags
    {
    public:
        using MaskType = typename std::underlying_type<BitType>::type;

        // constructors
        VULKAN_HPP_CONSTEXPR Flags() VULKAN_HPP_NOEXCEPT : m_mask( 0 ) {}

        VULKAN_HPP_CONSTEXPR Flags( BitType bit ) VULKAN_HPP_NOEXCEPT : m_mask( static_cast<MaskType>( bit ) ) {}

        VULKAN_HPP_CONSTEXPR Flags( Flags<BitType> const & rhs ) VULKAN_HPP_NOEXCEPT = default;

        VULKAN_HPP_CONSTEXPR explicit Flags( MaskType flags ) VULKAN_HPP_NOEXCEPT : m_mask( flags ) {}

        // relational operators
    #if defined( VULKAN_HPP_HAS_SPACESHIP_OPERATOR )
        auto operator<=>( Flags<BitType> const & ) const = default;
    #else
        VULKAN_HPP_CONSTEXPR bool operator<( Flags<BitType> const & rhs ) const VULKAN_HPP_NOEXCEPT
        {
        return m_mask < rhs.m_mask;
        }

        VULKAN_HPP_CONSTEXPR bool operator<=( Flags<BitType> const & rhs ) const VULKAN_HPP_NOEXCEPT
        {
        return m_mask <= rhs.m_mask;
        }

        VULKAN_HPP_CONSTEXPR bool operator>( Flags<BitType> const & rhs ) const VULKAN_HPP_NOEXCEPT
        {
        return m_mask > rhs.m_mask;
        }

        VULKAN_HPP_CONSTEXPR bool operator>=( Flags<BitType> const & rhs ) const VULKAN_HPP_NOEXCEPT
        {
        return m_mask >= rhs.m_mask;
        }

        VULKAN_HPP_CONSTEXPR bool operator==( Flags<BitType> const & rhs ) const VULKAN_HPP_NOEXCEPT
        {
        return m_mask == rhs.m_mask;
        }

        VULKAN_HPP_CONSTEXPR bool operator!=( Flags<BitType> const & rhs ) const VULKAN_HPP_NOEXCEPT
        {
        return m_mask != rhs.m_mask;
        }
    #endif

        // logical operator
        VULKAN_HPP_CONSTEXPR bool operator!() const VULKAN_HPP_NOEXCEPT
        {
        return !m_mask;
        }

        // bitwise operators
        VULKAN_HPP_CONSTEXPR Flags<BitType> operator&( Flags<BitType> const & rhs ) const VULKAN_HPP_NOEXCEPT
        {
        return Flags<BitType>( m_mask & rhs.m_mask );
        }

        VULKAN_HPP_CONSTEXPR Flags<BitType> operator|( Flags<BitType> const & rhs ) const VULKAN_HPP_NOEXCEPT
        {
        return Flags<BitType>( m_mask | rhs.m_mask );
        }

        VULKAN_HPP_CONSTEXPR Flags<BitType> operator^( Flags<BitType> const & rhs ) const VULKAN_HPP_NOEXCEPT
        {
        return Flags<BitType>( m_mask ^ rhs.m_mask );
        }

        VULKAN_HPP_CONSTEXPR Flags<BitType> operator~() const VULKAN_HPP_NOEXCEPT
        {
        return Flags<BitType>( m_mask ^ FlagTraits<BitType>::allFlags.m_mask );
        }

        // assignment operators
        VULKAN_HPP_CONSTEXPR_14 Flags<BitType> & operator=( Flags<BitType> const & rhs ) VULKAN_HPP_NOEXCEPT = default;

        VULKAN_HPP_CONSTEXPR_14 Flags<BitType> & operator|=( Flags<BitType> const & rhs ) VULKAN_HPP_NOEXCEPT
        {
        m_mask |= rhs.m_mask;
        return *this;
        }

        VULKAN_HPP_CONSTEXPR_14 Flags<BitType> & operator&=( Flags<BitType> const & rhs ) VULKAN_HPP_NOEXCEPT
        {
        m_mask &= rhs.m_mask;
        return *this;
        }

        VULKAN_HPP_CONSTEXPR_14 Flags<BitType> & operator^=( Flags<BitType> const & rhs ) VULKAN_HPP_NOEXCEPT
        {
        m_mask ^= rhs.m_mask;
        return *this;
        }

        // cast operators
        explicit VULKAN_HPP_CONSTEXPR operator bool() const VULKAN_HPP_NOEXCEPT
        {
        return !!m_mask;
        }

        explicit VULKAN_HPP_CONSTEXPR operator MaskType() const VULKAN_HPP_NOEXCEPT
        {
        return m_mask;
        }

    #if defined( VULKAN_HPP_FLAGS_MASK_TYPE_AS_PUBLIC )
    public:
    #else
    private:
    #endif
        MaskType m_mask;
    };

    #if !defined( VULKAN_HPP_HAS_SPACESHIP_OPERATOR )
    // relational operators only needed for pre C++20
    template <typename BitType>
    VULKAN_HPP_CONSTEXPR bool operator<( BitType bit, Flags<BitType> const & flags ) VULKAN_HPP_NOEXCEPT
    {
        return flags.operator>( bit );
    }

    template <typename BitType>
    VULKAN_HPP_CONSTEXPR bool operator<=( BitType bit, Flags<BitType> const & flags ) VULKAN_HPP_NOEXCEPT
    {
        return flags.operator>=( bit );
    }

    template <typename BitType>
    VULKAN_HPP_CONSTEXPR bool operator>( BitType bit, Flags<BitType> const & flags ) VULKAN_HPP_NOEXCEPT
    {
        return flags.operator<( bit );
    }

    template <typename BitType>
    VULKAN_HPP_CONSTEXPR bool operator>=( BitType bit, Flags<BitType> const & flags ) VULKAN_HPP_NOEXCEPT
    {
        return flags.operator<=( bit );
    }

    template <typename BitType>
    VULKAN_HPP_CONSTEXPR bool operator==( BitType bit, Flags<BitType> const & flags ) VULKAN_HPP_NOEXCEPT
    {
        return flags.operator==( bit );
    }

    template <typename BitType>
    VULKAN_HPP_CONSTEXPR bool operator!=( BitType bit, Flags<BitType> const & flags ) VULKAN_HPP_NOEXCEPT
    {
        return flags.operator!=( bit );
    }
    #endif

    // bitwise operators
    template <typename BitType>
    VULKAN_HPP_CONSTEXPR Flags<BitType> operator&( BitType bit, Flags<BitType> const & flags ) VULKAN_HPP_NOEXCEPT
    {
        return flags.operator&( bit );
    }

    template <typename BitType>
    VULKAN_HPP_CONSTEXPR Flags<BitType> operator|( BitType bit, Flags<BitType> const & flags ) VULKAN_HPP_NOEXCEPT
    {
        return flags.operator|( bit );
    }

    template <typename BitType>
    VULKAN_HPP_CONSTEXPR Flags<BitType> operator^( BitType bit, Flags<BitType> const & flags ) VULKAN_HPP_NOEXCEPT
    {
        return flags.operator^( bit );
    }

    // bitwise operators on BitType
    template <typename BitType, typename std::enable_if<FlagTraits<BitType>::isBitmask, bool>::type = true>
    VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR Flags<BitType> operator&( BitType lhs, BitType rhs ) VULKAN_HPP_NOEXCEPT
    {
        return Flags<BitType>( lhs ) & rhs;
    }

    template <typename BitType, typename std::enable_if<FlagTraits<BitType>::isBitmask, bool>::type = true>
    VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR Flags<BitType> operator|( BitType lhs, BitType rhs ) VULKAN_HPP_NOEXCEPT
    {
        return Flags<BitType>( lhs ) | rhs;
    }

    template <typename BitType, typename std::enable_if<FlagTraits<BitType>::isBitmask, bool>::type = true>
    VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR Flags<BitType> operator^( BitType lhs, BitType rhs ) VULKAN_HPP_NOEXCEPT
    {
        return Flags<BitType>( lhs ) ^ rhs;
    }

    template <typename BitType, typename std::enable_if<FlagTraits<BitType>::isBitmask, bool>::type = true>
    VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR Flags<BitType> operator~( BitType bit ) VULKAN_HPP_NOEXCEPT
    {
        return ~( Flags<BitType>( bit ) );
    }
    /*
    vulkan/vulkan_enums.hpp 的代码到这里结束
    */

    enum class AttachmentType {
        eColor,
        eDepth,
        eStencil,
        eDepthBuffer,
        eStencilBuffer,
        eFloat4Buffer,
        eUint64Buffer,
        eColorBuffer,
    };

    struct AccessInfo {
        vk::PipelineStageFlags stage;
        vk::AccessFlags access;
    };

    enum class VertexType {
        eInt8, eInt8x2, eInt8x3, eInt8x4,
        eInt16, eInt16x2, eInt16x3, eInt16x4,
        eInt32, eInt32x2, eInt32x3, eInt32x4,
        eInt64, eInt64x2, eInt64x3, eInt64x4,
        eUint8, eUint8x2, eUint8x3, eUint8x4,
        eUint16, eUint16x2, eUint16x3, eUint16x4,
        eUint32, eUint32x2, eUint32x3, eUint32x4,
        eUint64, eUint64x2, eUint64x3, eUint64x4,
        eFloat, eFloat2, eFloat3, eFloat4,
        eDouble, eDouble2, eDouble3, eDouble4,
    };

    enum class IndexType {
        eUint16,
        eUint32,
    };

    typedef VertexType ConstantType;

    enum class SamplerFilter {
        eNearest,
        eLinear,
    };

    enum class SamplerAddressMode {
        eRepeat,
        eMirroredRepeat,
        eClampToEdge,
        eClampToBorder,
    };

    enum class SamplerBorderColor {
        eFloatTransparentBlack,
        eIntTransparentBlack,
        eFloatOpaqueBlack,
        eIntOpaqueBlack,
        eFloatOpaqueWhite,
        eIntOpaqueWhite,
    };
    enum class Topology {
        ePointList,
        eLineList,
        eLineStrip,
        eTriangleList,
        eTriangleFan,
    };

    enum class PolygonMode {
        eFill,
        eLine,
        ePoint,
    };

    enum class CullMode {
        eNone,
        eFront,
        eBack,
        eFrontAndBack,
    };

    enum class FrontFace {
        eClockwise,
        eCounterClockwise,
    };

    enum class ShaderStage : uint32_t {
        eVertex = static_cast<uint32_t>(vk::ShaderStageFlagBits::eVertex),
        eFragment = static_cast<uint32_t>(vk::ShaderStageFlagBits::eFragment),
        eRaygen = static_cast<uint32_t>(vk::ShaderStageFlagBits::eRaygenKHR),
        eMiss = static_cast<uint32_t>(vk::ShaderStageFlagBits::eMissKHR),
        eClosestHit = static_cast<uint32_t>(vk::ShaderStageFlagBits::eClosestHitKHR),
        eIntersection = static_cast<uint32_t>(vk::ShaderStageFlagBits::eIntersectionKHR),
        eCompute = static_cast<uint32_t>(vk::ShaderStageFlagBits::eCompute),
    };

    using ShaderStages = Flags<ShaderStage>;
    template <>
    struct FlagTraits<ShaderStage> {
        static VULKAN_HPP_CONST_OR_CONSTEXPR bool             isBitmask = true;
        static VULKAN_HPP_CONST_OR_CONSTEXPR ShaderStages allFlags = ShaderStage::eVertex | ShaderStage::eFragment | ShaderStage::eRaygen | ShaderStage::eMiss | ShaderStage::eClosestHit;
    };

    enum class InputRate {
        ePerVertex,
        ePerInstance,
    };

    enum class DescriptorType {
        eUniform,
        eTexture,
        eTextureAttachment,
        eInputAttachment,
        eStorageBuffer,
        eStorageImage,
        eRayTracingInstance,
    };

    enum class SampleCount {
        e1 = 1,
        e2 = 2,
        e4 = 4,
        e8 = 8,
        e16 = 16,
        e32 = 32,
        e64 = 64,
    };

    union BasicConstantValue {
        bool b;
        BasicConstantValue(bool b) : b(b) {}
        int32_t i;
        BasicConstantValue(int32_t i) : i(i) {}
        uint32_t ui;
        BasicConstantValue(uint32_t ui) : ui(ui) {}
        float f;
        BasicConstantValue(float f) : f(f) {}
    };
}
