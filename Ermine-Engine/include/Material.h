/* Start Header ************************************************************************/
/*!
\file       Material.h
\author     Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu
\co-author  Ridhwan Afandi, moahamedridhwan.b, 2301367, moahamedridhwan.b\@digipen.edu
\date       Sep 9, 2025
\brief      Material system for graphics rendering with SSBO support.

Copyright (C) 2025 DigiPen Institute of Technology.
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
        Material texture flag bits for packed bitfield
    *************************************************************************/
    enum MaterialTextureFlags : uint32_t
    {
        MAT_FLAG_ALBEDO_MAP = 1 << 0,  // bit 0: hasAlbedoMap
        MAT_FLAG_NORMAL_MAP = 1 << 1,  // bit 1: hasNormalMap
        MAT_FLAG_ROUGHNESS_MAP = 1 << 2,  // bit 2: hasRoughnessMap
        MAT_FLAG_METALLIC_MAP = 1 << 3,  // bit 3: hasMetallicMap
        MAT_FLAG_AO_MAP = 1 << 4,  // bit 4: hasAoMap
        MAT_FLAG_EMISSIVE_MAP = 1 << 5   // bit 5: hasEmissiveMap
    };

    /*!***********************************************************************
    \brief
        Material parameter types for type safety (Used for Templates/Legacy)
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
        Material parameter wrapper (Used for Templates/Legacy/Serialization)
    *************************************************************************/
    struct MaterialParam
    {
        MaterialParamType type;
        std::vector<float> floatValues;
        int intValue = 0;
        bool boolValue = false;
        std::shared_ptr<Texture> texture = nullptr;
        std::shared_ptr<Cubemap> cubemap = nullptr;

        MaterialParam() : type(MaterialParamType::FLOAT), intValue(0), boolValue(false) {}
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
        GPU-compatible material structure for SSBO
        Uses std430 layout rules
    *************************************************************************/
    struct MaterialSSBO
    {
        Vec4 albedo{ 0.8f, 0.8f, 0.8f, 1.0f };  // 16 bytes (0-15)

        float metallic{ 0.0f };                  // 4 bytes (16-19)
        float roughness{ 0.5f };                 // 4 bytes (20-23)
        float ao{ 1.0f };                        // 4 bytes (24-27)
        float normalStrength{ 1.0f };            // 4 bytes (28-31)

        Vec3 emissive{ 0.0f, 0.0f, 0.0f };      // 12 bytes (32-43)
        float emissiveIntensity{ 0.0f };         // 4 bytes (44-47)

        int shadingModel{ 0 };                   // 4 bytes (48-51)
        uint32_t textureFlags{ 0 };              // 4 bytes (52-55)
        int castsShadows{ 1 };                   // 4 bytes (56-59)
        float _pad0{};                           // 4 bytes (60-63)

        // UV Scale and Offset
        Vec2 uvScale{ 1.0f, 1.0f };             // 8 bytes (64-71)
        Vec2 uvOffset{ 0.0f, 0.0f };            // 8 bytes (72-79)

        // Texture Array Indices (indices into bindless texture array)
        int albedoMapIndex{ -1 };               // 4 bytes (80-83)
        int normalMapIndex{ -1 };               // 4 bytes (84-87)
        int roughnessMapIndex{ -1 };            // 4 bytes (88-91)
        int metallicMapIndex{ -1 };             // 4 bytes (92-95)

        int aoMapIndex{ -1 };                   // 4 bytes (96-99)
        int emissiveMapIndex{ -1 };             // 4 bytes (100-103)
        int _pad1{};                            // 4 bytes (104-107)
        int _pad2{};                            // 4 bytes (108-111)
    };

    // Predefined material templates
    class MaterialTemplates
    {
    public:
        static std::map<std::string, MaterialParam> PBR_RED() {
            return { {"materialAlbedo", Vec4(1.0f, 0.0f, 0.0f, 1.0f)}, {"materialRoughness", 0.3f}, {"materialMetallic", 0.0f} };
        }
        static std::map<std::string, MaterialParam> PBR_METAL() {
            return { {"materialAlbedo", Vec4(0.7f, 0.7f, 0.8f, 1.0f)}, {"materialMetallic", 1.0f}, {"materialRoughness", 0.15f} };
        }
        static std::map<std::string, MaterialParam> PBR_WHITE() {
            return { {"materialAlbedo", Vec4(0.8f, 0.8f, 0.8f, 1.0f)}, {"materialMetallic", 0.0f}, {"materialRoughness", 0.3f} };
        }
        static std::map<std::string, MaterialParam> EMISSIVE(const Vec3& color, float intensity) {
            return { {"materialAlbedo", Vec4(0,0,0,1)}, {"materialEmissive", color}, {"materialEmissiveIntensity", intensity} };
        }
        static std::map<std::string, MaterialParam> PBR_GLASS(float transparency = 0.9f) {
            return { {"materialAlbedo", Vec4(0.95f, 0.95f, 0.95f, 1.0f - transparency)}, {"materialRoughness", 0.05f} };
        }
        static std::map<std::string, MaterialParam> PBR_WATER(float transparency = 0.7f) {
            return { {"materialAlbedo", Vec4(0.1f, 0.3f, 0.6f, 1.0f - transparency)}, {"materialRoughness", 0.1f} };
        }
    };

    /*!***********************************************************************
    \brief
        Main Material class.
        OPTIMIZED: Stores MaterialSSBO directly to avoid map lookups during rendering.
    *************************************************************************/
    class Material
    {
    private:
        // Direct data storage (Shadow Copy)
        MaterialSSBO m_data;

        // Shader reference
        std::shared_ptr<Shader> m_shader;

        // Texture references (kept for RefCounting and Binding)
        std::shared_ptr<Texture> m_albedoMap;
        std::shared_ptr<Texture> m_normalMap;
        std::shared_ptr<Texture> m_roughnessMap;
        std::shared_ptr<Texture> m_metallicMap;
        std::shared_ptr<Texture> m_aoMap;
        std::shared_ptr<Texture> m_emissiveMap;

        std::unordered_map<std::string, std::shared_ptr<Cubemap>> cubemaps;

        // State
        bool m_dirty = true;
        int m_materialIndex = -1;

    public:
        Material() = default;

        Material(std::shared_ptr<Shader> shader, const std::map<std::string, MaterialParam>& params = {})
            : m_shader(std::move(shader))
        {
            LoadTemplate(params);
        }

        // --- Direct Setters (Fast Path) ---

        void SetAlbedo(const Vec4& color) {
            if (m_data.albedo != color) { m_data.albedo = color; m_dirty = true; }
        }
        void SetAlbedo(const Vec3& color) {
            Vec4 c(color.x, color.y, color.z, 1.0f);
            if (m_data.albedo != c) { m_data.albedo = c; m_dirty = true; }
        }
        void SetMetallic(float val) {
            if (m_data.metallic != val) { m_data.metallic = val; m_dirty = true; }
        }
        void SetRoughness(float val) {
            if (m_data.roughness != val) { m_data.roughness = val; m_dirty = true; }
        }
        void SetAO(float val) {
            if (m_data.ao != val) { m_data.ao = val; m_dirty = true; }
        }
        void SetEmissive(const Vec3& color) {
            if (m_data.emissive != color) { m_data.emissive = color; m_dirty = true; }
        }
        void SetEmissiveIntensity(float val) {
            if (m_data.emissiveIntensity != val) { m_data.emissiveIntensity = val; m_dirty = true; }
        }
        void SetNormalStrength(float val) {
            if (m_data.normalStrength != val) { m_data.normalStrength = val; m_dirty = true; }
        }
        void SetCastsShadows(bool val) {
            int v = val ? 1 : 0;
            if (m_data.castsShadows != v) { m_data.castsShadows = v; m_dirty = true; }
        }
        void SetShadingModel(int val) {
            if (m_data.shadingModel != val) { m_data.shadingModel = val; m_dirty = true; }
        }
        void SetUVScale(const Vec2& scale) {
            if (m_data.uvScale != scale) { m_data.uvScale = scale; m_dirty = true; }
        }
        void SetUVOffset(const Vec2& offset) {
            if (m_data.uvOffset != offset) { m_data.uvOffset = offset; m_dirty = true; }
        }

        // --- Texture Setters (Fast Path) ---

        void SetTexture(const std::string& name, std::shared_ptr<Texture> texture)
        {
            if (name == "materialAlbedoMap" || name == "material.albedoMap") {
                m_albedoMap = texture;
                if (texture && texture->IsValid()) m_data.textureFlags |= MAT_FLAG_ALBEDO_MAP;
                else m_data.textureFlags &= ~MAT_FLAG_ALBEDO_MAP;
            }
            else if (name == "materialNormalMap" || name == "material.normalMap") {
                m_normalMap = texture;
                if (texture && texture->IsValid()) m_data.textureFlags |= MAT_FLAG_NORMAL_MAP;
                else m_data.textureFlags &= ~MAT_FLAG_NORMAL_MAP;
            }
            else if (name == "materialRoughnessMap" || name == "material.roughnessMap") {
                m_roughnessMap = texture;
                if (texture && texture->IsValid()) m_data.textureFlags |= MAT_FLAG_ROUGHNESS_MAP;
                else m_data.textureFlags &= ~MAT_FLAG_ROUGHNESS_MAP;
            }
            else if (name == "materialMetallicMap" || name == "material.metallicMap") {
                m_metallicMap = texture;
                if (texture && texture->IsValid()) m_data.textureFlags |= MAT_FLAG_METALLIC_MAP;
                else m_data.textureFlags &= ~MAT_FLAG_METALLIC_MAP;
            }
            else if (name == "materialAoMap" || name == "material.aoMap") {
                m_aoMap = texture;
                if (texture && texture->IsValid()) m_data.textureFlags |= MAT_FLAG_AO_MAP;
                else m_data.textureFlags &= ~MAT_FLAG_AO_MAP;
            }
            else if (name == "materialEmissiveMap" || name == "material.emissiveMap") {
                m_emissiveMap = texture;
                if (texture && texture->IsValid()) m_data.textureFlags |= MAT_FLAG_EMISSIVE_MAP;
                else m_data.textureFlags &= ~MAT_FLAG_EMISSIVE_MAP;
            }
            m_dirty = true;
        }

        // --- Generic Parameter Setters (String-based, slightly slower, for serialization) ---

        void SetParameter(const std::string& name, const MaterialParam& param)
        {
            if (param.type == MaterialParamType::TEXTURE_2D) {
                SetTexture(name, param.texture);
                return;
            }

            if (name == "materialAlbedo" || name == "material.albedo") {
                if (param.type == MaterialParamType::VEC4) SetAlbedo(Vec4(param.floatValues[0], param.floatValues[1], param.floatValues[2], param.floatValues[3]));
                else if (param.type == MaterialParamType::VEC3) SetAlbedo(Vec3(param.floatValues[0], param.floatValues[1], param.floatValues[2]));
            }
            else if (name == "materialMetallic" || name == "material.metallic") SetMetallic(param.floatValues[0]);
            else if (name == "materialRoughness" || name == "material.roughness") SetRoughness(param.floatValues[0]);
            else if (name == "materialAo" || name == "material.ao") SetAO(param.floatValues[0]);
            else if (name == "materialNormalStrength" || name == "material.normalStrength") SetNormalStrength(param.floatValues[0]);
            else if (name == "materialEmissive" || name == "material.emissive") {
                if (param.floatValues.size() >= 3) SetEmissive(Vec3(param.floatValues[0], param.floatValues[1], param.floatValues[2]));
            }
            else if (name == "materialEmissiveIntensity" || name == "material.emissiveIntensity") SetEmissiveIntensity(param.floatValues[0]);
            else if (name == "materialShadingModel") SetShadingModel(param.intValue);
            else if (name == "materialCastsShadows") SetCastsShadows(param.boolValue);

            // Manual Flag Overrides (if loaded from file)
            else if (name.find("HasAlbedoMap") != std::string::npos) {
                if (param.boolValue) m_data.textureFlags |= MAT_FLAG_ALBEDO_MAP; else m_data.textureFlags &= ~MAT_FLAG_ALBEDO_MAP; m_dirty = true;
            }
            else if (name.find("HasNormalMap") != std::string::npos) {
                if (param.boolValue) m_data.textureFlags |= MAT_FLAG_NORMAL_MAP; else m_data.textureFlags &= ~MAT_FLAG_NORMAL_MAP; m_dirty = true;
            }
            // ... other flags mapped similarly if strictly needed
        }

        // Convenience Wrappers
        void SetFloat(const std::string& name, float value) { SetParameter(name, MaterialParam(value)); }
        void SetVec2(const std::string& name, const Vec2& value) {
            if (name == "uvScale") SetUVScale(value);
            else if (name == "uvOffset") SetUVOffset(value);
            else SetParameter(name, MaterialParam(value));
        }
        void SetVec3(const std::string& name, const Vec3& value) { SetParameter(name, MaterialParam(value)); }
        void SetVec4(const std::string& name, const Vec4& value) { SetParameter(name, MaterialParam(value)); }
        void SetInt(const std::string& name, int value) { SetParameter(name, MaterialParam(value)); }
        void SetBool(const std::string& name, bool value) { SetParameter(name, MaterialParam(value)); }

        // --- Getters (Constructs temp param for serialization support) ---
        // Note: This creates overhead but is only used during serialization/debugging.
        std::unique_ptr<MaterialParam> GetParameter(const std::string& name) const
        {
            if (name == "materialAlbedo") return std::make_unique<MaterialParam>(m_data.albedo);
            if (name == "materialMetallic") return std::make_unique<MaterialParam>(m_data.metallic);
            if (name == "materialRoughness") return std::make_unique<MaterialParam>(m_data.roughness);
            if (name == "materialAo") return std::make_unique<MaterialParam>(m_data.ao);
            if (name == "materialEmissive") return std::make_unique<MaterialParam>(m_data.emissive);
            if (name == "materialEmissiveIntensity") return std::make_unique<MaterialParam>(m_data.emissiveIntensity);
            if (name == "materialNormalStrength") return std::make_unique<MaterialParam>(m_data.normalStrength);
            if (name == "materialShadingModel") return std::make_unique<MaterialParam>(m_data.shadingModel);
            if (name == "materialCastsShadows") return std::make_unique<MaterialParam>(m_data.castsShadows != 0);

            // Boolean flags
            if (name == "materialHasAlbedoMap") return std::make_unique<MaterialParam>((m_data.textureFlags & MAT_FLAG_ALBEDO_MAP) != 0);
            if (name == "materialHasNormalMap") return std::make_unique<MaterialParam>((m_data.textureFlags & MAT_FLAG_NORMAL_MAP) != 0);
            if (name == "materialHasRoughnessMap") return std::make_unique<MaterialParam>((m_data.textureFlags & MAT_FLAG_ROUGHNESS_MAP) != 0);
            if (name == "materialHasMetallicMap") return std::make_unique<MaterialParam>((m_data.textureFlags & MAT_FLAG_METALLIC_MAP) != 0);
            if (name == "materialHasAoMap") return std::make_unique<MaterialParam>((m_data.textureFlags & MAT_FLAG_AO_MAP) != 0);
            if (name == "materialHasEmissiveMap") return std::make_unique<MaterialParam>((m_data.textureFlags & MAT_FLAG_EMISSIVE_MAP) != 0);

            // Texture getters
            if (name.find("AlbedoMap") != std::string::npos) return std::make_unique<MaterialParam>(m_albedoMap);
            if (name.find("NormalMap") != std::string::npos) return std::make_unique<MaterialParam>(m_normalMap);
            if (name.find("RoughnessMap") != std::string::npos) return std::make_unique<MaterialParam>(m_roughnessMap);
            if (name.find("MetallicMap") != std::string::npos) return std::make_unique<MaterialParam>(m_metallicMap);
            if (name.find("AoMap") != std::string::npos) return std::make_unique<MaterialParam>(m_aoMap);
            if (name.find("EmissiveMap") != std::string::npos) return std::make_unique<MaterialParam>(m_emissiveMap);

            return nullptr;
        }

        // Check if parameter exists (Helper wrapper)
        bool HasParameter(const std::string& name) const
        {
            return GetParameter(name) != nullptr;
        }

        // --- Core Systems ---

        void SetShader(std::shared_ptr<Shader> shader) { m_shader = std::move(shader); }
        std::shared_ptr<Shader> GetShader() const { return m_shader; }

        // Optimized: Returns const ref to member, zero cost
        const MaterialSSBO& GetSSBOData() const { return m_data; }

        bool IsDirty() const { return m_dirty; }
        void ClearDirty() { m_dirty = false; }

        void SetMaterialIndex(int index) { m_materialIndex = index; }
        int GetMaterialIndex() const { return m_materialIndex; }

        std::shared_ptr<Texture> GetTexture(const std::string& name)
        {
            if (name.find("Albedo") != std::string::npos) return m_albedoMap;
            if (name.find("Normal") != std::string::npos) return m_normalMap;
            if (name.find("Roughness") != std::string::npos) return m_roughnessMap;
            if (name.find("Metallic") != std::string::npos) return m_metallicMap;
            if (name.find("Ao") != std::string::npos) return m_aoMap;
            if (name.find("Emissive") != std::string::npos) return m_emissiveMap;
            return nullptr;
        }

        void SetTextureArrayIndex(const std::string& textureName, int index)
        {
            if (textureName == "materialAlbedoMap") m_data.albedoMapIndex = index;
            else if (textureName == "materialNormalMap") m_data.normalMapIndex = index;
            else if (textureName == "materialRoughnessMap") m_data.roughnessMapIndex = index;
            else if (textureName == "materialMetallicMap") m_data.metallicMapIndex = index;
            else if (textureName == "materialAoMap") m_data.aoMapIndex = index;
            else if (textureName == "materialEmissiveMap") m_data.emissiveMapIndex = index;
            m_dirty = true;
        }

        int GetTextureArrayIndex(const std::string& textureName) const
        {
            if (textureName == "materialAlbedoMap") return m_data.albedoMapIndex;
            if (textureName == "materialNormalMap") return m_data.normalMapIndex;
            if (textureName == "materialRoughnessMap") return m_data.roughnessMapIndex;
            if (textureName == "materialMetallicMap") return m_data.metallicMapIndex;
            if (textureName == "materialAoMap") return m_data.aoMapIndex;
            if (textureName == "materialEmissiveMap") return m_data.emissiveMapIndex;
            return -1;
        }

        void Bind() const
        {
            if (!m_shader || !m_shader->IsValid()) return;
            m_shader->Bind();
            BindTextures();
        }

        void BindTextures() const
        {
            // Bind textures to fixed slots expected by standard shaders
            if (m_albedoMap && m_albedoMap->IsValid()) { m_albedoMap->Bind(0); m_shader->SetUniform1i("materialAlbedoMap", 0); }
            if (m_normalMap && m_normalMap->IsValid()) { m_normalMap->Bind(1); m_shader->SetUniform1i("materialNormalMap", 1); }
            if (m_roughnessMap && m_roughnessMap->IsValid()) { m_roughnessMap->Bind(2); m_shader->SetUniform1i("materialRoughnessMap", 2); }
            if (m_metallicMap && m_metallicMap->IsValid()) { m_metallicMap->Bind(3); m_shader->SetUniform1i("materialMetallicMap", 3); }
            if (m_aoMap && m_aoMap->IsValid()) { m_aoMap->Bind(4); m_shader->SetUniform1i("materialAoMap", 4); }
            if (m_emissiveMap && m_emissiveMap->IsValid()) { m_emissiveMap->Bind(5); m_shader->SetUniform1i("materialEmissiveMap", 5); }
        }

        void Unbind() const
        {
            if (m_albedoMap) m_albedoMap->Unbind();
            if (m_normalMap) m_normalMap->Unbind();
            if (m_roughnessMap) m_roughnessMap->Unbind();
            if (m_metallicMap) m_metallicMap->Unbind();
            if (m_aoMap) m_aoMap->Unbind();
            if (m_emissiveMap) m_emissiveMap->Unbind();
            if (m_shader) m_shader->Unbind();
        }

        void LoadTemplate(const std::map<std::string, MaterialParam>& templateParams)
        {
            for (const auto& [name, param] : templateParams)
            {
                SetParameter(name, param);
            }
        }

        // --- Cubemap support ---
        const std::unordered_map<std::string, std::shared_ptr<Cubemap>>& GetCubemaps() const {
            return cubemaps;
        }

        // Copy/Move Semantics
        Material(const Material& other)
            : m_data(other.m_data), m_shader(other.m_shader),
            m_albedoMap(other.m_albedoMap), m_normalMap(other.m_normalMap),
            m_roughnessMap(other.m_roughnessMap), m_metallicMap(other.m_metallicMap),
            m_aoMap(other.m_aoMap), m_emissiveMap(other.m_emissiveMap),
            m_dirty(true) {
        }

        Material& operator=(const Material& other)
        {
            if (this != &other) {
                m_data = other.m_data;
                m_shader = other.m_shader;
                m_albedoMap = other.m_albedoMap; m_normalMap = other.m_normalMap;
                m_roughnessMap = other.m_roughnessMap; m_metallicMap = other.m_metallicMap;
                m_aoMap = other.m_aoMap; m_emissiveMap = other.m_emissiveMap;
                m_dirty = true;
            }
            return *this;
        }

        // Getters for individual properties (used by GUI/Editor)
        Vec2 GetUVScale() const { return m_data.uvScale; }
        Vec2 GetUVOffset() const { return m_data.uvOffset; }
    };

    /*!***********************************************************************
    \brief
        Material factory for common materials
    *************************************************************************/
    class MaterialFactory
    {
    public:
        static std::unique_ptr<Material> CreatePBRMaterial(std::shared_ptr<Shader> shader, const Vec3& albedo, float metallic, float roughness)
        {
            auto material = std::make_unique<Material>(shader);
            material->SetAlbedo(albedo);
            material->SetMetallic(metallic);
            material->SetRoughness(roughness);
            material->SetAO(1.0f);
            material->SetEmissive(Vec3(0.0f, 0.0f, 0.0f));
            material->SetEmissiveIntensity(0.0f);
            material->SetShadingModel(0);
            return material;
        }

        static std::unique_ptr<Material> CreateEmissiveMaterial(std::shared_ptr<Shader> shader, const Vec3& color, float intensity)
        {
            auto material = std::make_unique<Material>(shader);
            material->LoadTemplate(MaterialTemplates::EMISSIVE(color, intensity));
            return material;
        }
    };
}