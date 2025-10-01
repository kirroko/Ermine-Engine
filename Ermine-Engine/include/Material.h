/* Start Header ************************************************************************/
/*!
\file       Material.h
\author     Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu
\date       Sep 9, 2025
\brief      Material system for graphics rendering with UBO support

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

#include "PreCompile.h"
#include "Shader.h"
#include "Texture.h"
#include "Cubemap.h"
#include "MathVector.h"

namespace Ermine::graphics
{
    /*!***********************************************************************
    \brief
        Material parameter types for type safety
    *************************************************************************/
    enum class MaterialParamType
    {
        FLOAT,
        VEC2,
        VEC3,
        VEC4,
        INT,
        BOOL,
        TEXTURE_2D
    };

    /*!***********************************************************************
    \brief
        Material parameter wrapper
    *************************************************************************/
    struct MaterialParam
    {
        MaterialParamType type;
        std::vector<float> floatValues;
        int intValue = 0;
        bool boolValue = false;
        std::shared_ptr<Texture> texture = nullptr;
        std::shared_ptr<Cubemap> cubemap = nullptr;

        // Default constructor
        MaterialParam() : type(MaterialParamType::FLOAT), intValue(0), boolValue(false) {}

        // Constructors for different types
        MaterialParam(float value) : type(MaterialParamType::FLOAT), floatValues{ value } {}
        MaterialParam(const Vec2& value) : type(MaterialParamType::VEC2), floatValues{ value.x, value.y } {}
        MaterialParam(const Vec3& value) : type(MaterialParamType::VEC3), floatValues{ value.x, value.y, value.z } {}
        MaterialParam(const Vec4& value) : type(MaterialParamType::VEC4), floatValues{ value.x, value.y, value.z, value.w } {}
        MaterialParam(int value) : type(MaterialParamType::INT), intValue(value) {}
        MaterialParam(bool value) : type(MaterialParamType::BOOL), boolValue(value) {}
        MaterialParam(std::shared_ptr<Texture> tex) : type(MaterialParamType::TEXTURE_2D), texture(std::move(tex)) {}
    };

    /*!***********************************************************************
    \brief
        GPU-compatible material structure
    *************************************************************************/
    struct MaterialUBO
    {
        Vec4 albedo{ 0.8f, 0.8f, 0.8f, 1.0f };  // 16 bytes (0-15)

        float metallic{ 0.0f };                  // 4 bytes (16-19)
        float roughness{ 0.5f };                 // 4 bytes (20-23)
        float ao{ 1.0f };                        // 4 bytes (24-27)
        float normalStrength{ 1.0f };            // 4 bytes (28-31)

        Vec3 emissive{ 0.0f, 0.0f, 0.0f };      // 12 bytes (32-43)
        float emissiveIntensity{ 0.0f };         // 4 bytes (44-47)

        int shadingModel{ 0 };                   // 4 bytes (48-51)
        float _pad0{};                           // 4 bytes (52-55)
        float _pad1{};                           // 4 bytes (56-59)
        float _pad2{};                           // 4 bytes (60-63)

        int hasAlbedoMap{ 0 };                   // 4 bytes (64-67)
        int hasNormalMap{ 0 };                   // 4 bytes (68-71)
        int hasRoughnessMap{ 0 };                // 4 bytes (72-75)
        int hasMetallicMap{ 0 };                 // 4 bytes (76-79)

        int hasAoMap{ 0 };                       // 4 bytes (80-83)
        int hasEmissiveMap{ 0 };                 // 4 bytes (84-87)
        float _pad3{};                           // 4 bytes (88-91)
        float _pad4{};                           // 4 bytes (92-95)
    };

    // Forward declaration
    class Material;

    // Predefined material templates
    class MaterialTemplates
    {
    public:
        static std::map<std::string, MaterialParam> PBR_RED()
        {
            return {
                {"materialAlbedo", Vec4(1.0f, 0.0f, 0.0f, 1.0f)},
                {"materialMetallic", 0.0f},
                {"materialRoughness", 0.3f},
                {"materialAo", 1.0f},
                {"materialEmissive", Vec3(0.0f, 0.0f, 0.0f)},
                {"materialEmissiveIntensity", 0.0f},
                {"materialNormalStrength", 1.0f},
                {"materialShadingModel", 0},
                {"materialHasAlbedoMap", false},
                {"materialHasNormalMap", false},
                {"materialHasRoughnessMap", false},
                {"materialHasMetallicMap", false},
                {"materialHasAoMap", false},
                {"materialHasEmissiveMap", false}
            };
        }

        static std::map<std::string, MaterialParam> PBR_METAL()
        {
            return {
                {"materialAlbedo", Vec4(0.7f, 0.7f, 0.8f, 1.0f)},
                {"materialMetallic", 1.0f},  // Full metallic
                {"materialRoughness", 0.15f}, // Slightly rough for visible reflections
                {"materialAo", 1.0f},
                {"materialEmissive", Vec3(0.0f, 0.0f, 0.0f)},
                {"materialEmissiveIntensity", 0.0f},
                {"materialNormalStrength", 1.0f},
                {"materialShadingModel", 0},
                {"materialHasAlbedoMap", false},
                {"materialHasNormalMap", false},
                {"materialHasRoughnessMap", false},
                {"materialHasMetallicMap", false},
                {"materialHasAoMap", false},
                {"materialHasEmissiveMap", false}
            };
        }

        static std::map<std::string, MaterialParam> PBR_WHITE()
        {
            return {
                {"materialAlbedo", Vec4(0.8f, 0.8f, 0.8f, 1.0f)},
                {"materialMetallic", 0.0f},
                {"materialRoughness", 0.3f},
                {"materialAo", 1.0f},
                {"materialEmissive", Vec3(0.0f, 0.0f, 0.0f)},
                {"materialEmissiveIntensity", 0.0f},
                {"materialNormalStrength", 1.0f},
                {"materialShadingModel", 0},
                {"materialHasAlbedoMap", false},
                {"materialHasNormalMap", false},
                {"materialHasRoughnessMap", false},
                {"materialHasMetallicMap", false},
                {"materialHasAoMap", false},
                {"materialHasEmissiveMap", false}
            };
        }

        static std::map<std::string, MaterialParam> EMISSIVE(const Vec3& color, float intensity)
        {
            return {
                {"materialAlbedo", Vec4(0.0f, 0.0f, 0.0f, 1.0f)},
                {"materialMetallic", 0.0f},
                {"materialRoughness", 1.0f},
                {"materialAo", 1.0f},
                {"materialEmissive", color},
                {"materialEmissiveIntensity", intensity},
                {"materialNormalStrength", 1.0f},
                {"materialShadingModel", 0},
                {"materialHasAlbedoMap", false},
                {"materialHasNormalMap", false},
                {"materialHasRoughnessMap", false},
                {"materialHasMetallicMap", false},
                {"materialHasAoMap", false},
                {"materialHasEmissiveMap", false}
            };
        }

        // Glass material (transparent)
        static std::map<std::string, MaterialParam> PBR_GLASS(float transparency = 0.9f)
        {
            return {
                {"materialAlbedo", Vec4(0.95f, 0.95f, 0.95f, 1.0f - transparency)},
                {"materialMetallic", 0.0f},
                {"materialRoughness", 0.05f},
                {"materialAo", 1.0f},
                {"materialEmissive", Vec3(0.0f, 0.0f, 0.0f)},
                {"materialEmissiveIntensity", 0.0f},
                {"materialNormalStrength", 1.0f},
                {"materialShadingModel", 0},
                {"materialHasAlbedoMap", false},
                {"materialHasNormalMap", false},
                {"materialHasRoughnessMap", false},
                {"materialHasMetallicMap", false},
                {"materialHasAoMap", false},
                {"materialHasEmissiveMap", false}
            };
        }

        // Water material (transparent)
        static std::map<std::string, MaterialParam> PBR_WATER(float transparency = 0.7f)
        {
            return {
                {"materialAlbedo", Vec4(0.1f, 0.3f, 0.6f, 1.0f - transparency)},
                {"materialMetallic", 0.0f},
                {"materialRoughness", 0.1f},
                {"materialAo", 1.0f},
                {"materialEmissive", Vec3(0.0f, 0.0f, 0.0f)},
                {"materialEmissiveIntensity", 0.0f},
                {"materialNormalStrength", 1.0f},
                {"materialShadingModel", 0},
                {"materialHasAlbedoMap", false},
                {"materialHasNormalMap", false},
                {"materialHasRoughnessMap", false},
                {"materialHasMetallicMap", false},
                {"materialHasAoMap", false},
                {"materialHasEmissiveMap", false}
            };
        }

    };

    /*!***********************************************************************
    \brief
        Main Material class with UBO support
    *************************************************************************/
    class Material
    {
    private:
        std::map<std::string, MaterialParam> m_parameters;
        std::shared_ptr<Shader> m_shader;

        // Texture slots management
        std::map<std::string, int> m_textureSlots;
        int m_nextTextureSlot = 0;
        std::unordered_map<std::string, std::shared_ptr<Cubemap>> cubemaps;

        // UBO management
        mutable MaterialUBO m_materialData;
        mutable bool m_uboDirty = true;

        void UpdateUBOData() const
        {
            if (!m_uboDirty) return;

            // Update material data from parameters
            if (auto param = GetParameter("materialAlbedo"))
            {
                if (param->type == MaterialParamType::VEC4 && param->floatValues.size() >= 4)
                {
                    m_materialData.albedo = Vec4(param->floatValues[0],
                        param->floatValues[1],
                        param->floatValues[2],
                        param->floatValues[3]);
                }
                else if (param->type == MaterialParamType::VEC3 && param->floatValues.size() >= 3)
                {
                    m_materialData.albedo = Vec4(param->floatValues[0],
                        param->floatValues[1],
                        param->floatValues[2],
                        1.0f);
                }
            }

            if (auto param = GetParameter("materialMetallic"))
                m_materialData.metallic = param->floatValues[0];

            if (auto param = GetParameter("materialRoughness"))
                m_materialData.roughness = param->floatValues[0];

            if (auto param = GetParameter("materialAo"))
                m_materialData.ao = param->floatValues[0];

            if (auto param = GetParameter("materialNormalStrength"))
                m_materialData.normalStrength = param->floatValues[0];

            if (auto param = GetParameter("materialEmissive"))
            {
                if (param->floatValues.size() >= 3)
                {
                    m_materialData.emissive = Vec3(param->floatValues[0],
                        param->floatValues[1],
                        param->floatValues[2]);
                }
            }

            if (auto param = GetParameter("materialEmissiveIntensity"))
                m_materialData.emissiveIntensity = param->floatValues[0];

            if (auto param = GetParameter("materialShadingModel"))
                m_materialData.shadingModel = param->intValue;

            // Update texture flags
            m_materialData.hasAlbedoMap = GetParameter("materialHasAlbedoMap") &&
                GetParameter("materialHasAlbedoMap")->boolValue ? 1 : 0;
            m_materialData.hasNormalMap = GetParameter("materialHasNormalMap") &&
                GetParameter("materialHasNormalMap")->boolValue ? 1 : 0;
            m_materialData.hasRoughnessMap = GetParameter("materialHasRoughnessMap") &&
                GetParameter("materialHasRoughnessMap")->boolValue ? 1 : 0;
            m_materialData.hasMetallicMap = GetParameter("materialHasMetallicMap") &&
                GetParameter("materialHasMetallicMap")->boolValue ? 1 : 0;
            m_materialData.hasAoMap = GetParameter("materialHasAoMap") &&
                GetParameter("materialHasAoMap")->boolValue ? 1 : 0;
            m_materialData.hasEmissiveMap = GetParameter("materialHasEmissiveMap") &&
                GetParameter("materialHasEmissiveMap")->boolValue ? 1 : 0;

            m_uboDirty = false;
        }

    public:
        Material() = default;

        Material(std::shared_ptr<Shader> shader, const std::map<std::string, MaterialParam>& params = {})
            : m_shader(std::move(shader)), m_parameters(params) {
        }

        void SetShader(std::shared_ptr<Shader> shader) { m_shader = std::move(shader); }
        std::shared_ptr<Shader> GetShader() const { return m_shader; }

        void SetParameter(const std::string& name, const MaterialParam& param)
        {
            m_parameters[name] = param;
            m_uboDirty = true;

            if (param.type == MaterialParamType::TEXTURE_2D)
            {
                if (m_textureSlots.find(name) == m_textureSlots.end())
                {
                    m_textureSlots[name] = m_nextTextureSlot++;
                }
            }
        }

        void SetFloat(const std::string& name, float value) {
            SetParameter(name, MaterialParam(value));
        }
        void SetVec3(const std::string& name, const Vec3& value) {
            SetParameter(name, MaterialParam(value));
        }
        void SetVec4(const std::string& name, const Vec4& value) {
            SetParameter(name, MaterialParam(value));
        }
        void SetInt(const std::string& name, int value) {
            SetParameter(name, MaterialParam(value));
        }
        void SetBool(const std::string& name, bool value) {
            SetParameter(name, MaterialParam(value));
        }
        void SetTexture(const std::string& name, std::shared_ptr<Texture> texture)
        {
            SetParameter(name, MaterialParam(std::move(texture)));
        }
        std::shared_ptr<Texture> GetTexture(const std::string& name)
        {
            if (auto param = GetParameter(name))
            {
                if (param->type == MaterialParamType::TEXTURE_2D)
                {
                    return param->texture;
                }
            }
            return nullptr;
        }

        bool HasParameter(const std::string& name) const
        {
            return m_parameters.find(name) != m_parameters.end();
        }

        const MaterialParam* GetParameter(const std::string& name) const
        {
            auto it = m_parameters.find(name);
            return it != m_parameters.end() ? &it->second : nullptr;
        }

        const MaterialUBO& GetUBOData() const
        {
            UpdateUBOData();
            return m_materialData;
        }

        void BindTextures() const
        {
            if (!m_shader || !m_shader->IsValid()) return;

            for (const auto& [name, param] : m_parameters)
            {
                if (param.type == MaterialParamType::TEXTURE_2D &&
                    param.texture && param.texture->IsValid())
                {
                    auto slotIt = m_textureSlots.find(name);
                    if (slotIt != m_textureSlots.end())
                    {
                        param.texture->Bind(slotIt->second);
                        m_shader->SetUniform1i(name, slotIt->second);
                    }
                }
            }
        }

        void Bind() const
        {
            if (!m_shader || !m_shader->IsValid()) return;
            m_shader->Bind();
            BindTextures();
        }

        void Unbind() const
        {
            for (const auto& [name, param] : m_parameters)
            {
                if (param.type == MaterialParamType::TEXTURE_2D &&
                    param.texture && param.texture->IsValid())
                {
                    param.texture->Unbind();
                }
            }

            if (m_shader) m_shader->Unbind();
        }

        void LoadTemplate(const std::map<std::string, MaterialParam>& templateParams)
        {
            for (const auto& [name, param] : templateParams)
            {
                SetParameter(name, param);
            }
        }

        Material(const Material& other)
            : m_parameters(other.m_parameters)
            , m_shader(other.m_shader)
            , m_textureSlots(other.m_textureSlots)
            , m_nextTextureSlot(other.m_nextTextureSlot)
            , m_materialData(other.m_materialData)
            , m_uboDirty(true)
        {
        }

        Material& operator=(const Material& other)
        {
            if (this != &other)
            {
                m_parameters = other.m_parameters;
                m_shader = other.m_shader;
                m_textureSlots = other.m_textureSlots;
                m_nextTextureSlot = other.m_nextTextureSlot;
                m_materialData = other.m_materialData;
                m_uboDirty = true;
            }
            return *this;
        }

        const std::unordered_map<std::string, std::shared_ptr<Cubemap>>& GetCubemaps() const {
            return cubemaps;
        }
    };

    /*!***********************************************************************
    \brief
        Material factory for common materials
    *************************************************************************/
    class MaterialFactory
    {
    public:
        static std::unique_ptr<Material> CreatePBRMaterial(std::shared_ptr<Shader> shader, const Vec3& albedo,
            float metallic, float roughness)
        {
            auto material = std::make_unique<Material>(shader);
            material->SetVec3("materialAlbedo", albedo);
            material->SetFloat("materialMetallic", metallic);
            material->SetFloat("materialRoughness", roughness);
            material->SetFloat("materialAo", 1.0f);
            material->SetVec3("materialEmissive", Vec3(0.0f, 0.0f, 0.0f));
            material->SetFloat("materialEmissiveIntensity", 0.0f);
            material->SetInt("materialShadingModel", 0);
            return material;
        }

        static std::unique_ptr<Material> CreateEmissiveMaterial(std::shared_ptr<Shader> shader,
            const Vec3& color, float intensity)
        {
            auto material = std::make_unique<Material>(shader);
            material->LoadTemplate(MaterialTemplates::EMISSIVE(color, intensity));
            return material;
        }
    };
}