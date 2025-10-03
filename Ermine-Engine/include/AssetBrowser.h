/* Start Header ************************************************************************/
/*!
\file       AssetBrowser.h
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       02/09/2025
\brief      This file contains declarations for for ImGUI UI Asset Browser.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#define NOMINMAX
#include "PreCompile.h"
#include "imgui.h"
#include "ImGuiUIWindow.h"

#ifndef IMGUI_CDECL
#ifdef _MSC_VER
#define IMGUI_CDECL __cdecl
#else
#define IMGUI_CDECL
#endif
#endif

#define IM_MIN(A, B)            (((A) < (B)) ? (A) : (B))
#define IM_MAX(A, B)            (((A) >= (B)) ? (A) : (B))
#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

namespace Ermine::ImguiUI {
	//inline constexpr const char* categories[] = { "Audio", "Images", "Fonts", "Prefabs", "Scenes" };
	inline constexpr const char* categories[] = { "Textures", "Shaders", "Models" };
	constexpr size_t num_of_categories = sizeof(categories) / sizeof(categories[0]);

	struct SelectionWithDeletion : ImGuiSelectionBasicStorage
	{
		/*!***********************************************************************
		\brief
			Apply deletion before looping over items.
		\param[in,out] ms_io
			Pointer to ImGui multi-select IO.
		\param[in] items_count
			Total number of items.
		\return
			Number of items deleted.
		*************************************************************************/
		int ApplyDeletionPreLoop(ImGuiMultiSelectIO* ms_io, int items_count);

		/*!***********************************************************************
		\brief
			Apply deletion after looping over items and update vector contents.
		\param[in,out] ms_io
			Pointer to ImGui multi-select IO.
		\param[in,out] items
			Vector of items to modify.
		\param[in] item_curr_idx_to_select
			Index of item currently selected.
		*************************************************************************/
		template<typename ITEM_TYPE>
		void ApplyDeletionPostLoop(ImGuiMultiSelectIO* ms_io, ImVector<ITEM_TYPE>& items, int item_curr_idx_to_select)
		{
			ImVector<ITEM_TYPE> new_items;
			new_items.reserve(items.Size - Size);
			int item_next_idx_to_select = -1;
			for (int idx = 0; idx < items.Size; idx++)
			{
				if (!Contains(GetStorageIdFromIndex(idx)))
					new_items.push_back(items[idx]);
				if (item_curr_idx_to_select == idx)
					item_next_idx_to_select = new_items.Size - 1;
			}
			items.swap(new_items);

			Clear();
			if (item_next_idx_to_select != -1 && ms_io->NavIdSelected)
				SetItemSelected(GetStorageIdFromIndex(item_next_idx_to_select), true);
		}
	};

	struct Asset
	{
		ImGuiID ID;
		int     Type;
		std::string Name;
		bool IsSelected;
		ImTextureID Icon;
		std::string realName;

		/*!***********************************************************************
		\brief
			Construct an asset with all fields.
		*************************************************************************/
		Asset(ImGuiID id, int type, std::string name, bool select, ImTextureID icon, std::string _realName);
		/*!***********************************************************************
		\brief
		   Construct an asset without a real name.
		*************************************************************************/
		Asset(ImGuiID id, int type, std::string name, bool select, ImTextureID icon);

		static const ImGuiTableSortSpecs* current_sortSpecs;

		/*!***********************************************************************
		\brief
		   Compare two assets using current sort specs.
		*************************************************************************/
		static int IMGUI_CDECL CompareWithSortSpecs(const void* lhs, const void* rhs);
		/*!***********************************************************************
		\brief
		   Sort assets based on ImGui table sort specs.
		*************************************************************************/
		static void SortWithSortSpecs(ImGuiTableSortSpecs* sort_specs, Asset* items, int items_count);
	};

	struct Browser
	{
		// Options
		bool            ShowTypeOverlay = true;
		bool            AllowSorting = true;
		bool            AllowDragUnselected = false;
		bool            AllowBoxSelect = true;
		float           IconSize = 120.0f;
		int             IconSpacing = 30;
		int             IconHitSpacing = 4;
		bool            StretchSpacing = true;

		// State
		std::vector<std::string> ItemNames;
		ImVector<Asset> Items;
		ImVector<Asset*> filteredAssets;
		SelectionWithDeletion Selection;
		ImGuiID         NextItemId = 0;
		bool            RequestDelete = false;
		bool            RequestSort = false;
		float           ZoomWheelAccum = 0.0f;

		ImVec2          LayoutItemSize;
		ImVec2          LayoutItemStep;
		float           LayoutItemSpacing = 0.0f;
		float           LayoutSelectableSpacing = 0.0f;
		float           LayoutOuterPadding = 0.0f;
		int             LayoutColumnCount = 0;
		int             LayoutLineCount = 0;

		ImVector<std::string> myStrings;
		ImTextureID placeholderIcon = 0;

		// Functions
		/*!***********************************************************************
		\brief
			Default constructor for Browser.
		*************************************************************************/
		Browser();
		/*!***********************************************************************
		\brief
			Add items to the browser.
		*************************************************************************/
		void AddItems(int count, int type, std::string name);
		/*!***********************************************************************
		\brief
		   Remove all items.
		*************************************************************************/
		void ClearItems();
		/*!***********************************************************************
		\brief
			Get all items.
		*************************************************************************/
		const ImVector<Asset>& GetItems() const;
		/*!***********************************************************************
		\brief
		   Get all item names.
		*************************************************************************/
		const std::vector<std::string>& GetItemNames() const;
		/*!***********************************************************************
		\brief
		   Get the type of an asset by ID.
		*************************************************************************/
		const int GetType(ImGuiID id) const;
		/*!***********************************************************************
		\brief
			Clear all item names.
		*************************************************************************/
		void ClearItemNames();
		/*!***********************************************************************
		\brief
		   Update layout sizes based on available width.
		*************************************************************************/
		void UpdateLayoutSizes(float avail_width);
		/*!***********************************************************************
		\brief
		   Extract the file name from a file path.
		*************************************************************************/
		std::string ExtractFileName(const std::string& filePath);
		/*!***********************************************************************
		\brief
		   Get the file extension from a path.
		*************************************************************************/
		std::string GetFileExtension(const std::string& path);
		/*!***********************************************************************
		\brief
		   Copy a file into the assets directory.
		*************************************************************************/
		bool CopyFileToAssets(const std::string& sourceFilePath);
		/*!***********************************************************************
		\brief
			Handle external files dropped into the asset browser.
		*************************************************************************/
		void HandleDroppedFiles(const std::vector<std::string>& filePaths);
		/*!***********************************************************************
		\brief
			Get directories containing assets.
		*************************************************************************/
		std::string GetDirectories();
		/*!***********************************************************************
		\brief
		   Get the full path of a selected asset.
		*************************************************************************/
		std::string GetSelectedFilePath(std::string name);
		/*!***********************************************************************
		\brief
		   Draw the asset browser window.
		*************************************************************************/
		void Draw(const char* title);

		// Reference to functions for creating Game Objects with components using Asset Browser //

		/*bool CreateObjectWithAsset(ImGuiID id) {
			GameObject* newObj;
			std::string name = "GameObject_" + std::to_string(GAMEOBJECTFACTORY.GetGameObjects().size());
			newObj = GAMEOBJECTFACTORY.CreateGameObject(name);

			if (!newObj)
				return false;

			if (Items[id].Type == 0) {
				newObj->AddComponent(Component::ComponentType::Audio);
				BS::Component::AudioComponent* audio = newObj->GetComponent<BS::Component::AudioComponent>();

				if (!audio)
					return false;

				audio->SetAudio(GetItemNames()[id]);
				return true;
			}
			if (Items[id].Type == 1) {
				newObj->AddComponent(Component::ComponentType::Renderer);
				BS::Component::RendererComponent* renderer = newObj->GetComponent<BS::Component::RendererComponent>();

				if (!renderer)
					return false;

				renderer->SetTextureName(GetItemNames()[id]);
				return true;
			}
			if (Items[id].Type == 2) {
				newObj->AddComponent(Component::ComponentType::Text);
				BS::Component::TextComponent* text = newObj->GetComponent<BS::Component::TextComponent>();

				if (!text)
					return false;

				text->SetFontType(GetItemNames()[id]);
				return true;
			}

			return false;
		}*/

		/*bool CreateComponentWithAsset(ImGuiID id, IObject* obj) {
			if (!obj)
				return false;

			if (Items[id].Type == 0) {
				obj->AddComponent(Component::ComponentType::Audio);
				BS::Component::AudioComponent* audio = obj->GetComponent<BS::Component::AudioComponent>();

				if (!audio)
					return false;

				audio->SetAudio(GetItemNames()[id]);
				return true;
			}
			if (Items[id].Type == 1) {
				obj->AddComponent(Component::ComponentType::Renderer);
				BS::Component::RendererComponent* renderer = obj->GetComponent<BS::Component::RendererComponent>();

				if (!renderer)
					return false;

				renderer->SetTextureName(GetItemNames()[id]);
				return true;
			}
			if (Items[id].Type == 2) {
				obj->AddComponent(Component::ComponentType::Text);
				BS::Component::TextComponent* text = obj->GetComponent<BS::Component::TextComponent>();

				if (!text)
					return false;

				text->SetFontType(GetItemNames()[id]);
				return true;
			}

			return false;
		}*/
	};

	class AssetBrowser : public ImGUIWindow
	{
	public:
		/*!***********************************************************************
		\brief
		   Constructor for AssetBrowser window.
		*************************************************************************/
		AssetBrowser() : ImGUIWindow("Asset Browser IMGUI") {}
		/*!***********************************************************************
		\brief
		   Update the AssetBrowser window.
		*************************************************************************/
		void Update() override;
		/*!***********************************************************************
		\brief
			Render the AssetBrowser window.
		*************************************************************************/
		void Render() override;

		Browser assets_browser;
	private:
		int m_objToSpawn{ 0 };
	};
}
