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
        TEXTURE_2D,
        TEXTURE_CUBE
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
        MaterialParam(std::shared_ptr<Texture> tex, MaterialParamType texType = MaterialParamType::TEXTURE_2D)
            : type(texType), texture(std::move(tex)) {
        }
        MaterialParam(std::shared_ptr<Cubemap> cube) 
            : type(MaterialParamType::TEXTURE_CUBE), cubemap(std::move(cube)) {
        }
    };

    /*!***********************************************************************
    \brief
        GPU-compatible material structure
    *************************************************************************/
    struct MaterialUBO
    {
        alignas(16) Vec3 albedo { 0.8f, 0.8f, 0.8f };
        alignas(4) float metallic{ 0.0f };
        alignas(4) float roughness{ 0.5f };
        alignas(4) float ao{ 1.0f };
        alignas(16) Vec3 emissive { 0.0f, 0.0f, 0.0f };
        alignas(4) float emissiveIntensity{ 0.0f };
        alignas(4) float normalStrength{ 1.0f };
        alignas(4) int shadingModel{ 0 }; // 0 = PBR, 1 = Blinn-Phong

        // Texture presence flags (packed as ints for std140 compatibility)
        alignas(4) int hasAlbedoMap{ 0 };
        alignas(4) int hasNormalMap{ 0 };
        alignas(4) int hasRoughnessMap{ 0 };
        alignas(4) int hasMetallicMap{ 0 };
        alignas(4) int hasAoMap{ 0 };
        alignas(4) int hasEmissiveMap{ 0 };
        alignas(4) int hasEnvironmentMap{ 0 };
        alignas(4) int hasIrradianceMap{ 0 };

        // Environment mapping parameters
        alignas(4) float reflectance{ 0.04f };
        alignas(4) float environmentIntensity{ 1.0f };
        
        // Padding to ensure proper alignment
        alignas(4) int padding1{ 0 };
        alignas(4) int padding2{ 0 };
    };

    // Forward declaration
    class Material;

    // Predefined material templates
    class MaterialTemplates
    {
    public:
        // PBR Material templates
        static std::map<std::string, MaterialParam> PBR_RED()
        {
            return {
                {"materialAlbedo", Vec3(1.0f, 0.0f, 0.0f)},
                {"materialMetallic", 0.0f},
                {"materialRoughness", 0.3f},
                {"materialAo", 1.0f},
                {"materialEmissive", Vec3(0.0f, 0.0f, 0.0f)},
                {"materialEmissiveIntensity", 0.0f},
                {"materialNormalStrength", 1.0f},
                {"materialShadingModel", 0}, // 0 = PBR
                {"materialHasAlbedoMap", false},
                {"materialHasNormalMap", false},
                {"materialHasRoughnessMap", false},
                {"materialHasMetallicMap", false},
                {"materialHasAoMap", false},
                {"materialHasEmissiveMap", false},
                {"materialHasEnvironmentMap", false},
                {"materialHasIrradianceMap", false}
            };
        }

        static std::map<std::string, MaterialParam> PBR_METAL()
        {
            return {
                {"materialAlbedo", Vec3(0.7f, 0.7f, 0.8f)},
                {"materialMetallic", 1.0f},
                {"materialRoughness", 0.1f},
                {"materialAo", 1.0f},
                {"materialEmissive", Vec3(0.0f, 0.0f, 0.0f)},
                {"materialEmissiveIntensity", 0.0f},
                {"materialNormalStrength", 1.0f},
                {"materialShadingModel", 0},
                {"materialHasAlbedoMap", true},
                {"materialHasNormalMap", false},
                {"materialHasRoughnessMap", false},
                {"materialHasMetallicMap", false},
                {"materialHasAoMap", false},
                {"materialHasEmissiveMap", false},
                {"materialHasEnvironmentMap", false},
                {"materialHasIrradianceMap", false}
            };
        }

        static std::map<std::string, MaterialParam> PBR_WHITE()
        {
            return {
                {"materialAlbedo", Vec3(0.8f, 0.8f, 0.8f)},
                {"materialMetallic", 0.0f},
                {"materialRoughness", 0.5f},
                {"materialAo", 1.0f},
                {"materialEmissive", Vec3(0.0f, 0.0f, 0.0f)},
                {"materialEmissiveIntensity", 0.0f},
                {"materialNormalStrength", 1.0f},
                {"materialShadingModel", 0},
                {"materialHasAlbedoMap", true}, // Will use texture
                {"materialHasNormalMap", false},
                {"materialHasRoughnessMap", false},
                {"materialHasMetallicMap", false},
                {"materialHasAoMap", false},
                {"materialHasEmissiveMap", false},
                {"materialHasEnvironmentMap", false},
                {"materialHasIrradianceMap", false}
            };
        }

        // Emissive material for lights
        static std::map<std::string, MaterialParam> EMISSIVE(const Vec3& color, float intensity)
        {
            return {
                {"materialAlbedo", Vec3(0.0f, 0.0f, 0.0f)},
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
                {"materialHasEmissiveMap", false},
                {"materialHasEnvironmentMap", false},
                {"materialHasIrradianceMap", false}
            };
        }

        // Reflective material template with environment mapping
        static std::map<std::string, MaterialParam> PBR_REFLECTIVE(float metallic = 1.0f, float roughness = 0.1f)
        {
            return {
                {"materialAlbedo", Vec3(0.8f, 0.8f, 0.8f)},
                {"materialMetallic", metallic},
                {"materialRoughness", roughness},
                {"materialAo", 1.0f},
                {"materialEmissive", Vec3(0.0f, 0.0f, 0.0f)},
                {"materialEmissiveIntensity", 0.0f},
                {"materialNormalStrength", 1.0f},
                {"materialShadingModel", 0},
                {"materialHasAlbedoMap", true},
                {"materialHasNormalMap", false},
                {"materialHasRoughnessMap", false},
                {"materialHasMetallicMap", false},
                {"materialHasAoMap", false},
                {"materialHasEmissiveMap", false},
                {"materialHasEnvironmentMap", true},
                {"materialHasIrradianceMap", true},
                {"materialReflectance", 0.04f},
                {"materialEnvironmentIntensity", 1.0f}
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
                if (param->floatValues.size() >= 3)
                {
                    m_materialData.albedo = Vec3(param->floatValues[0],
                        param->floatValues[1],
                        param->floatValues[2]);
                }
            }

            if (auto param = GetParameter("materialMetallic"))
                m_materialData.metallic = param->floatValues[0];

            if (auto param = GetParameter("materialRoughness"))
                m_materialData.roughness = param->floatValues[0];

            if (auto param = GetParameter("materialAo"))
                m_materialData.ao = param->floatValues[0];

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

            if (auto param = GetParameter("materialNormalStrength"))
                m_materialData.normalStrength = param->floatValues[0];

            if (auto param = GetParameter("materialShadingModel"))
                m_materialData.shadingModel = param->intValue;

            if (auto param = GetParameter("materialReflectance"))
                m_materialData.reflectance = param->floatValues[0];

            if (auto param = GetParameter("materialEnvironmentIntensity"))
                m_materialData.environmentIntensity = param->floatValues[0];

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
            m_materialData.hasEnvironmentMap = GetParameter("materialHasEnvironmentMap") &&
                GetParameter("materialHasEnvironmentMap")->boolValue ? 1 : 0;
            m_materialData.hasIrradianceMap = GetParameter("materialHasIrradianceMap") &&
                GetParameter("materialHasIrradianceMap")->boolValue ? 1 : 0;

            m_uboDirty = false;
        }

    public:
        Material() = default;

        Material(std::shared_ptr<Shader> shader, const std::map<std::string, MaterialParam>& params = {})
            : m_shader(std::move(shader)), m_parameters(params) {
        }

        /**
         * @brief Sets the shader for the material.
         * @param shader A shared pointer to the shader to be set.
         */
        void SetShader(std::shared_ptr<Shader> shader) { m_shader = std::move(shader); }
        
        //Getter for shader
        std::shared_ptr<Shader> GetShader() const { return m_shader; }

        /**
         * @brief Sets a material parameter.
         * @param name The name of the parameter.
         * @param param The parameter to set.
         */
        void SetParameter(const std::string& name, const MaterialParam& param)
        {
            m_parameters[name] = param;
            m_uboDirty = true;

            // Assign texture slots for textures
            if (param.type == MaterialParamType::TEXTURE_2D || param.type == MaterialParamType::TEXTURE_CUBE)
            {
                if (m_textureSlots.find(name) == m_textureSlots.end())
                {
                    m_textureSlots[name] = m_nextTextureSlot++;
                }
            }
        }

        /**
         * @brief Sets a float material parameter.
         * @param name The name of the parameter.
         * @param value The float value to set.
         */
        void SetFloat(const std::string& name, float value) {
            SetParameter(name, MaterialParam(value));
        }
        /**
         * @brief Sets a `Vec3` material parameter.
         * @param name The name of the parameter.
         * @param value The `Vec3` value to set.
         */
        void SetVec3(const std::string& name, const Vec3& value) {
            SetParameter(name, MaterialParam(value));
        }
        /**
         * @brief Sets a `Vec4` material parameter.
         * @param name The name of the parameter.
         * @param value The `Vec4` value to set.
         */
        void SetVec4(const std::string& name, const Vec4& value) {
            SetParameter(name, MaterialParam(value));
        }

        /**
         * @brief Sets an integer material parameter.
         * @param name The name of the parameter.
         * @param value The integer value to set.
         */
        void SetInt(const std::string& name, int value) {
            SetParameter(name, MaterialParam(value));
        }

        /**
         * @brief Sets a boolean material parameter.
         * @param name The name of the parameter.
         * @param value The boolean value to set.
         */
        void SetBool(const std::string& name, bool value) {
            SetParameter(name, MaterialParam(value));
        }
        /**
         * @brief Sets a texture material parameter.
         * @param name The name of the parameter.
         * @param texture A shared pointer to the texture to set.
         */
        void SetTexture(const std::string& name, std::shared_ptr<Texture> texture)
        {
            SetParameter(name, MaterialParam(std::move(texture)));
        }
        /**
         * @brief Gets a texture
         * @param name The name of the parameter.
         * @return texture A shared pointer to the texture to set.
         */
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

        /**
         * @brief Sets a cubemap material parameter.
         * @param name The name of the parameter.
         * @param cubemap A shared pointer to the cubemap to set.
         */
        void SetCubemap(const std::string& name, std::shared_ptr<Cubemap> cubemap)
        {
            SetParameter(name, MaterialParam(std::move(cubemap)));
        }
        /**
         * @brief Checks if a material parameter exists.
         * @param name The name of the parameter.
         * @return True if the parameter exists, false otherwise.
         */
        bool HasParameter(const std::string& name) const
        {
            return m_parameters.find(name) != m_parameters.end();
        }

        /**
         * @brief Gets a material parameter by name.
         * @param name The name of the parameter.
         * @return A pointer to the material parameter, or nullptr if not found.
         */
        const MaterialParam* GetParameter(const std::string& name) const
        {
            auto it = m_parameters.find(name);
            return it != m_parameters.end() ? &it->second : nullptr;
        }

        /**
         * @brief Retrieves the material's UBO data for binding.
         * @return A reference to the material's UBO data.
         */
        const MaterialUBO& GetUBOData() const
        {
            UpdateUBOData();
            return m_materialData;
        }

        /**
         * @brief Binds all textures associated with the material.
         */
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
                else if (param.type == MaterialParamType::TEXTURE_CUBE &&
                         param.cubemap && param.cubemap->IsValid())
                {
                    auto slotIt = m_textureSlots.find(name);
                    if (slotIt != m_textureSlots.end())
                    {
                        param.cubemap->Bind(slotIt->second);
                        m_shader->SetUniform1i(name, slotIt->second);
                    }
                }
            }
        }

        /**
         * @brief Binds the material's shader and textures for rendering.
         */
        void Bind() const
        {
            if (!m_shader || !m_shader->IsValid()) return;
            m_shader->Bind();
            BindTextures();
        }

        /**
         * @brief Unbinds the material's shader and textures after rendering.
         */
        void Unbind() const
        {
            // Unbind textures
            for (const auto& [name, param] : m_parameters)
            {
                if (param.type == MaterialParamType::TEXTURE_2D &&
                    param.texture && param.texture->IsValid())
                {
                    param.texture->Unbind();
                }
                else if (param.type == MaterialParamType::TEXTURE_CUBE &&
                         param.cubemap && param.cubemap->IsValid())
                {
                    param.cubemap->Unbind();
                }
            }

            if (m_shader) m_shader->Unbind();
        }

        /**
         * @brief Loads a material template into the material.
         * @param templateParams The template parameters to load.
         */
        void LoadTemplate(const std::map<std::string, MaterialParam>& templateParams)
        {
            for (const auto& [name, param] : templateParams)
            {
                SetParameter(name, param);
            }
        }

         /**
         * @brief Copy constructor and assignment
         * @param other The material to copy from.
         */
        Material(const Material& other)
            : m_parameters(other.m_parameters)
            , m_shader(other.m_shader)
            , m_textureSlots(other.m_textureSlots)
            , m_nextTextureSlot(other.m_nextTextureSlot)
            , m_materialData(other.m_materialData)
            , m_uboDirty(true)
        {
        }
        /**
         * @brief Assignment operator for the Material class.
         * @param other The material to copy from.
         * @return A reference to the current material after the assignment.
         */
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
        /**
         * @brief Creates a PBR material with the given properties.
         * @param shader A shared pointer to the shader.
         * @param albedo The albedo color of the material.
         * @param metallic The metallic factor of the material.
         * @param roughness The roughness factor of the material.
         * @return A unique pointer to the created material.
         */
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

        /**
         * @brief Creates an emissive material with the given color and intensity.
         * @param shader A shared pointer to the shader.
         * @param color The color of the emissive material.
         * @param intensity The intensity of the emissive material.
         * @return A unique pointer to the created material.
         */
        static std::unique_ptr<Material> CreateEmissiveMaterial(std::shared_ptr<Shader> shader,
            const Vec3& color, float intensity)
        {
            auto material = std::make_unique<Material>(shader);
            material->LoadTemplate(MaterialTemplates::EMISSIVE(color, intensity));
            return material;
        }
    };
}