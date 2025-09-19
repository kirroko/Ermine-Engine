/* Start Header ************************************************************************/
/*!
\file       AssetBrowser.cpp
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       02/09/2025
\brief      This file contains definitions for ImGUI UI Asset Browser.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "AssetBrowser.h"
#include "AssetManager.h"


namespace fs = std::filesystem;

// Helper Functions
std::string getFileName(const std::string& path) {
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash == std::string::npos) {
        return path; // If there's no separator, the entire path is the file name
    }
    return path.substr(lastSlash + 1); // Extracts everything after the last slash
}

std::wstring StringToWString(const std::string& str) { // Helper function to convert std::string to std::wstring
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

namespace Ermine {

	namespace ImguiUI {
        
        
        int SelectionWithDeletion::ApplyDeletionPreLoop(ImGuiMultiSelectIO* ms_io, int items_count)
        {
            if (Size == 0)
                return -1;

            const int focused_idx = (int)ms_io->NavIdItem;
            if (ms_io->NavIdSelected == false)
            {
                ms_io->RangeSrcReset = true;
                return focused_idx;
            }

            for (int idx = focused_idx + 1; idx < items_count; idx++)
                if (!Contains(GetStorageIdFromIndex(idx)))
                    return idx;

            for (int idx = IM_MIN(focused_idx, items_count) - 1; idx >= 0; idx--)
                if (!Contains(GetStorageIdFromIndex(idx)))
                    return idx;

            return -1;
        }

        Asset::Asset(ImGuiID id, int type, std::string name, bool select, ImTextureID icon, std::string _realName) {
            ID = id; Type = type; Name = name; IsSelected = select; Icon = icon; realName = _realName;
        }
        Asset::Asset(ImGuiID id, int type, std::string name, bool select, ImTextureID icon) {
            ID = id; Type = type; Name = name; IsSelected = select; Icon = icon;
        }

        const ImGuiTableSortSpecs* Asset::current_sortSpecs = NULL;

        int IMGUI_CDECL Asset::CompareWithSortSpecs(const void* lhs, const void* rhs)
        {
            const Asset* a = (const Asset*)lhs;
            const Asset* b = (const Asset*)rhs;
            for (int n = 0; n < current_sortSpecs->SpecsCount; n++)
            {
                const ImGuiTableColumnSortSpecs* sort_spec = &current_sortSpecs->Specs[n];
                int delta = 0;
                if (sort_spec->ColumnIndex == 0)
                    delta = ((int)a->ID - (int)b->ID);
                else if (sort_spec->ColumnIndex == 1)
                    delta = (a->Type - b->Type);
                if (delta > 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? +1 : -1;
                if (delta < 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? -1 : +1;
            }
            return ((int)a->ID - (int)b->ID);
        }

        void Asset::SortWithSortSpecs(ImGuiTableSortSpecs* sort_specs, Asset* items, int items_count)
        {
            current_sortSpecs = sort_specs;
            if (items_count > 1)
                qsort(items, (size_t)items_count, sizeof(items[0]), Asset::CompareWithSortSpecs);
            current_sortSpecs = NULL;
        }

        Browser::Browser()
        {
            // Load placeholder.png to be used as default icon for certain assets in the asset browser
            auto icon = AssetManager::GetInstance().LoadTexture("../Resources/Textures/placeholder.png");
            if (icon && icon->IsValid())
                placeholderIcon = (ImTextureID)(intptr_t)icon->GetRendererID();
        }

        void Browser::AddItems(int count, int type, std::string name)
        {
            if (Items.Size == 0)
                NextItemId = 0;
            Items.reserve(Items.Size + count);
            for (int n = 0; n < count; n++, NextItemId++)
            {
                ImTextureID icon_id = 0;
                // Load icon for texture assets
                if (type == 0) {
                    auto tex = AssetManager::GetInstance().LoadTexture("../Resources/Textures/" + name);
                    if (tex && tex->IsValid()) {
                        icon_id = (ImTextureID)(intptr_t)tex->GetRendererID();
                    }
                }
                else { // Default icon for other asset types
                    icon_id = placeholderIcon;
                }

                std::string temp = name;
                if (name.size() >= 16)
                {
                    temp = name.substr(0, 12) + "...";
                }
                ItemNames.push_back(name);
                Items.push_back(Asset(NextItemId, type, temp, false, icon_id, name));
            }
            RequestSort = true;
        }

        void Browser::ClearItems() {
            Items.clear();
            Selection.Clear();
        }

        const ImVector<Asset>& Browser::GetItems() const { return Items; }
        const std::vector<std::string>& Browser::GetItemNames() const { return ItemNames; }
        const int Browser::GetType(ImGuiID id) const { return Items[id].Type; }
        void Browser::ClearItemNames() { ItemNames.clear(); }

        void Browser::UpdateLayoutSizes(float avail_width)
        {
            LayoutItemSpacing = (float)IconSpacing;
            if (StretchSpacing == false)
                avail_width += floorf(LayoutItemSpacing * 0.5f);

            LayoutItemSize = ImVec2(floorf(IconSize), floorf(IconSize));
            LayoutColumnCount = IM_MAX((int)(avail_width / (LayoutItemSize.x + LayoutItemSpacing)), 1);
            LayoutLineCount = (Items.Size + LayoutColumnCount - 1) / LayoutColumnCount;

            if (StretchSpacing && LayoutColumnCount > 1)
                LayoutItemSpacing = floorf(avail_width - LayoutItemSize.x * LayoutColumnCount) / LayoutColumnCount;

            LayoutItemStep = ImVec2(LayoutItemSize.x + LayoutItemSpacing, LayoutItemSize.y + LayoutItemSpacing);
            LayoutSelectableSpacing = IM_MAX(floorf(LayoutItemSpacing) - IconHitSpacing, 0.0f);
            LayoutOuterPadding = floorf(LayoutItemSpacing * 0.5f);
        }


        std::string Browser::ExtractFileName(const std::string& filePath) {
            size_t lastSlash = filePath.find_last_of("/\\");
            if (lastSlash == std::string::npos) {
                return filePath; // No path separators, return the full string
            }
            return filePath.substr(lastSlash + 1);
        }

        std::string Browser::GetFileExtension(const std::string& path) {
            size_t lastSlash = path.find_last_of(".");
            if (lastSlash == std::string::npos) {
                return ""; // If there's no ., no extension
            }
            return path.substr(lastSlash + 1); // Extracts everything after the last slash
        }

        bool Browser::CopyFileToAssets(const std::string& sourceFilePath) {
            EE_CORE_INFO("COPYING");

            // Determine asset type and target folder based on file extension
            int type = 0;
            std::string extension = GetFileExtension(sourceFilePath);

            std::string destinationFolder;
            if (extension == "mp3" || extension == "wav" || extension == "ogg") {
                type = 0; // Audio
                destinationFolder = "Audio";
            }
            else if (extension == "png" || extension == "jpg" || extension == "jpeg") {
                type = 1; // Image
                destinationFolder = "Image";
            }
            else if (extension == "ttf" || extension == "otf") {
                type = 2; // Font
                destinationFolder = "Font";
            }
            else if (extension == "json") {
                type = 3; // Prefab
                // implement if statement and function to check for prefab or scene data
                // if (IsPrefab(file))
                destinationFolder = "Prefabs";
                // else
                // destinationFolder = "Scene";
            }
            else if (extension == "scene") {
                type = 4;
                destinationFolder = "Scenes";
            }
            else {
                EE_CORE_ERROR("Unsupported file type: {}", extension);
                return false; // Unsupported file type
            }

            std::string assetsFolder = "Assets/" + destinationFolder + "/";
            std::string destFilePath = assetsFolder + ExtractFileName(sourceFilePath);

            std::ifstream src(sourceFilePath, std::ios::binary);
            std::ofstream dst(destFilePath, std::ios::binary);

            if (!src) {
                EE_CORE_ERROR("Failed to open source file: {}", sourceFilePath);
                return false;
            }
            if (!dst) {
                EE_CORE_ERROR("Failed to create destination file: {}", destFilePath);
                return false;
            }

            dst << src.rdbuf();
            EE_CORE_INFO("File successfully copied to: {}", destFilePath);
            return true; // Successfully copied
        }

        void Browser::HandleDroppedFiles(const std::vector<std::string>& filePaths) {
            for (const auto& filePath : filePaths) {
                if (CopyFileToAssets(filePath)) {
                    EE_CORE_INFO("File successfully added to Asset Browser: {}", filePath);
                }
                else {
                    EE_CORE_ERROR("Failed to add file to Asset Browser: {}", filePath);
                    //AssetManager::errorMessage.push_back("Unsupported file type: " + ExtractFileName(filePath));
                    //AssetManager::showErrorPopup = true;
                }
            }

            // Refresh the browser contents
            //AssetManager::SetRefreshStatus(true);
            //AssetManager::Refresh();
        }

        std::string Browser::GetDirectories() {
            char dir[FILENAME_MAX]; // store path to current working directory

            // if _getcwd is successful
            if (_getcwd(dir, FILENAME_MAX)) {
                // return current working directory
                return std::string(dir);
            }
            return "";
        }

        std::string Browser::GetSelectedFilePath(std::string name)
        {
            std::string destinationFolder = "";
            if (Selection.Size > 0)
            {
                std::string extension = GetFileExtension(name);

                if (extension == "mp3" || extension == "wav" || extension == "ogg") {

                    std::string projectPath = GetDirectories();
                    destinationFolder = projectPath + "/Assets/Audio/";
                }
                else if (extension == "png" || extension == "jpg" || extension == "jpeg") {
                    std::string projectPath = GetDirectories();
                    destinationFolder = projectPath + "/Assets/Image/";
                }
                else if (extension == "ttf" || extension == "otf") {
                    std::string projectPath = GetDirectories();
                    destinationFolder = projectPath + "/Assets/Font/";
                }
                else if (extension == "prefab") {
                    // implement if statement and function to check for prefab or scene data
                    // if (IsPrefab(file))
                    std::string projectPath = GetDirectories();
                    destinationFolder = projectPath + "/Assets/Prefabs/";
                    // else
                    // destinationFolder = "Scene";
                }
                else if (extension == "scene") {
                    std::string projectPath = GetDirectories();
                    destinationFolder = projectPath + "/Assets/Scenes/";
                }
                else {
                    EE_CORE_ERROR("Unsupported file type: {}", extension);
                }

            }
            return destinationFolder; // Return an empty string if no valid selection
        }

        void Browser::Draw(const char* title)
        {
            //EE_CORE_INFO("string {}", myStrings[0]);
            ImGui::SetNextWindowSize(ImVec2(IconSize * 25, IconSize * 15), ImGuiCond_FirstUseEver); // 3000 x 1800
            if (!ImGui::Begin(title))
            {
                ImGui::End();
                return;
            }

            ImGui::SameLine();
            //if (ImGui::Button("Refresh Asset"))
            //    AssetManager::Refresh();

            // Asset Categories
            //const char* categories[] = { "Audio", "Images", "Fonts", "Prefabs", "Scenes" };
            const char* categories[] = { "Textures", "Shaders", "Models" };
            for (int type = 0; type < 3; ++type)
            {
                if (ImGui::TreeNode(categories[type]))
                {
                    // Filter assets by the current category
                    filteredAssets.clear();
                    for (int i = 0; i < Items.Size; ++i)
                    {
                        if (Items[i].Type == type)
                            filteredAssets.push_back(&Items[i]);
                    }

                    const float avail_width = ImGui::GetContentRegionAvail().x;
                    UpdateLayoutSizes(avail_width);

                    const int column_count = LayoutColumnCount;
                    const int total_items = (int)filteredAssets.size();
                    const int total_rows = (total_items + column_count - 1) / column_count;
                    const float total_height = total_rows * LayoutItemStep.y + LayoutOuterPadding * 2;
                    ImVec2 region_size(ImGui::GetContentRegionAvail().x, total_height);
                    ImGui::BeginChild("AssetSelectionRegion", region_size, true);

                    if (AllowSorting)
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                        ImGuiTableFlags table_flags_for_sort_specs = ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders;
                        if (ImGui::BeginTable("for_sort_specs_only", 1, table_flags_for_sort_specs, ImVec2(0.0f, ImGui::GetFrameHeight())))
                        {
                            ImGui::TableSetupColumn("Index");
                            ImGui::TableHeadersRow();
                            if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs())
                                if (sort_specs->SpecsDirty || RequestSort)
                                {
                                    if (filteredAssets.Data != nullptr)
                                        Asset::SortWithSortSpecs(sort_specs, *filteredAssets.Data, filteredAssets.Size);

                                    sort_specs->SpecsDirty = RequestSort = false;
                                }
                            ImGui::EndTable();
                        }
                        ImGui::PopStyleVar();
                    }

                    // Start displaying assets under this category
                    ImGuiIO& io = ImGui::GetIO();
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();

                    ImVec2 start_pos = ImGui::GetCursorScreenPos();
                    start_pos = ImVec2(start_pos.x + LayoutOuterPadding, start_pos.y + LayoutOuterPadding);
                    ImGui::SetCursorScreenPos(start_pos);

                    ImGuiMultiSelectFlags ms_flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ClearOnClickVoid;

                    // This is causing double click to open node and select
                    if (AllowBoxSelect)
                        ms_flags |= ImGuiMultiSelectFlags_BoxSelect2d;

                    if (AllowDragUnselected)
                        ms_flags |= ImGuiMultiSelectFlags_SelectOnClickRelease;

                    ms_flags |= ImGuiMultiSelectFlags_NavWrapX;


                    ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ms_flags, Selection.Size, filteredAssets.Size);

                    Selection.UserData = this;
                    Selection.AdapterIndexToStorageId = [](ImGuiSelectionBasicStorage* self_, int idx) { Browser* self = (Browser*)self_->UserData; return self->filteredAssets[idx]->ID; };
                    Selection.ApplyRequests(ms_io);

                    const bool want_delete = (ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_Repeat) && (Selection.Size > 0)) || RequestDelete;
                    const int item_curr_idx_to_focus = want_delete ? Selection.ApplyDeletionPreLoop(ms_io, filteredAssets.Size) : -1;
                    RequestDelete = false;

                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(LayoutSelectableSpacing, LayoutSelectableSpacing));

                    const ImU32 asset_type_overlay_colors[5] = { IM_COL32(255, 0, 0, 255), IM_COL32(0, 0, 255, 255), IM_COL32(255, 255, 0, 255), IM_COL32(0, 255, 0, 255), IM_COL32(255, 0, 255, 255) };
                    //const ImU32 asset_bg_color = ImGui::GetColorU32(ImGuiCol_MenuBarBg);
                    const ImVec4 color_with_alpha = ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg);
                    const ImU32 asset_bg_color = ImGui::ColorConvertFloat4ToU32(ImVec4(color_with_alpha.x, color_with_alpha.y, color_with_alpha.z, 0.0f));
                    const ImVec2 asset_type_overlay_size = ImVec2(16.0f, 16.0f);
                    const bool display_name_label = (LayoutItemSize.x >= ImGui::CalcTextSize("999").x);

                    // Dynamically adjust the size of the ImGui::ListClipper
                    ImGuiListClipper clipper;
                    clipper.Begin(total_rows);

                    if (item_curr_idx_to_focus >= 0 && item_curr_idx_to_focus < total_items)
                        clipper.IncludeItemByIndex(item_curr_idx_to_focus / column_count);
                    if (ms_io->RangeSrcItem >= 0 && ms_io->RangeSrcItem < filteredAssets.Size)
                        clipper.IncludeItemByIndex((int)ms_io->RangeSrcItem / column_count);

                    while (clipper.Step())
                    {
                        for (int line_idx = clipper.DisplayStart; line_idx < clipper.DisplayEnd; ++line_idx)
                        {
                            const int item_min_idx_for_current_line = line_idx * column_count;
                            const int item_max_idx_for_current_line = std::min((line_idx + 1) * column_count, total_items);

                            for (int item_idx = item_min_idx_for_current_line; item_idx < item_max_idx_for_current_line; ++item_idx)
                            {
                                Asset* item_data = filteredAssets[item_idx];

                                ImGui::PushID((int)item_data->ID);
                                ImVec2 pos = ImVec2(start_pos.x + (item_idx % column_count) * LayoutItemStep.x, start_pos.y + line_idx * LayoutItemStep.y);
                                ImGui::SetCursorScreenPos(pos);

                                ImGui::SetNextItemSelectionUserData((ImGuiID)item_idx);

                                bool item_is_selected = Selection.Contains((ImGuiID)item_data->ID);
                                bool item_is_visible = ImGui::IsRectVisible(LayoutItemSize);

                                // Render selectable
                                ImGui::Selectable("", item_is_selected, ImGuiSelectableFlags_None, LayoutItemSize);

                                if (ImGui::IsItemToggledSelection())
                                    item_is_selected = !item_is_selected;

                                if (item_curr_idx_to_focus == item_idx)
                                    ImGui::SetKeyboardFocusHere(-1);

                                if (ImGui::BeginDragDropSource())
                                {
                                    ImVector<ImGuiID> payload_items;
                                    void* it = NULL;
                                    ImGuiID id = 0;

                                    // Populate payload_items based on selection status
                                    if (!item_is_selected)
                                        payload_items.push_back(item_data->ID);
                                    else
                                        while (Selection.GetNextSelectedItem(&it, &id))
                                            payload_items.push_back(id);

                                    // Set the drag-drop payload with the selected items
                                    ImGui::SetDragDropPayload("ASSETS_BROWSER_ITEMS", payload_items.Data, payload_items.size_in_bytes());

                                    // Display the drag status to the user
                                    ImGui::Text("Dragging %d assets", payload_items.Size);

                                    ImGui::EndDragDropSource();
                                }

                                // Check if item is clicked
                                if (ImGui::IsItemClicked()) {
                                    // Check for scene type
                                    //EE_INFO(item_data->Type);
                                    //EE_INFO(item_data->Name);
                                    //EE_INFO(item_data->realName);

                                    if (item_data->Type == 4) {
                                        //SCENE_MANAGER.LoadScene(item_data->realName);
                                    }
                                }

                                // Render asset icon
                                if (item_is_visible)
                                {
                                    ImVec2 box_min(pos.x - 1, pos.y - 1);
                                    ImVec2 box_max(box_min.x + LayoutItemSize.x + 2, box_min.y + LayoutItemSize.y + 2);
                                    draw_list->AddRectFilled(box_min, box_max, asset_bg_color);

                                    // ASSET ICON
                                    if (item_data->Icon) {
                                        ImGui::SetCursorScreenPos(pos); // Reset cursor position
                                        ImGui::Image(item_data->Icon, LayoutItemSize, ImVec2(0, 1), ImVec2(1, 0));
                                    }
                                    if (ShowTypeOverlay)
                                    {
                                        ImU32 type_col = asset_type_overlay_colors[item_data->Type % IM_ARRAYSIZE(asset_type_overlay_colors)];
                                        draw_list->AddRectFilled(ImVec2(box_max.x - 2 - asset_type_overlay_size.x, box_min.y + 2), ImVec2(box_max.x - 2, box_min.y + 2 + asset_type_overlay_size.y), type_col);
                                    }
                                    if (display_name_label)
                                    {
                                        ImU32 label_col = ImGui::GetColorU32(item_is_selected ? ImGuiCol_Text : ImGuiCol_TextDisabled);
                                        //EE_CORE_INFO("TEST {}", item_data->Name);
                                        ImVec2 text_pos(box_min.x, box_max.y - ImGui::GetFontSize());
                                        ImVec2 offset_pos(text_pos.x, text_pos.y + 12.5f);
                                        draw_list->AddText(offset_pos, label_col, item_data->Name.c_str());
                                    }
                                }

                                if (item_is_selected)
                                {
                                    item_data->IsSelected = true;
                                }
                                else
                                {
                                    item_data->IsSelected = false;
                                }
                                ImGui::PopID();
                            }
                        }
                    }
                    clipper.End();
                    ImGui::PopStyleVar();

                    // Drag and drop
                    if (ImGui::BeginPopupContextWindow())
                    {
                        ImGui::Text("Selection: %d items", Selection.Size);
                        ImGui::Separator();
                        if (ImGui::MenuItem("Delete", "Del", false, Selection.Size > 0)) {
                            //EE_CORE_INFO("deleted");
                            // Retrieve the file path of the selected asset.
                            void* iterator = nullptr;
                            ImGuiID selectedID;
                            std::string filePath = "";

                            // Iterate over all selected items
                            while (Selection.GetNextSelectedItem(&iterator, &selectedID))
                            {
                                int selectedIndex = static_cast<int>(selectedID); // Assuming `selectedID` corresponds to an index

                                if (selectedIndex >= 0 && selectedIndex < ItemNames.size())
                                {
                                    const std::string& selectedItemName = ItemNames[selectedIndex];

                                    //EE_CORE_INFO("Selected: {}", selectedItemName);

                                    filePath = GetSelectedFilePath(selectedItemName) + ItemNames[selectedIndex];

                                    // Perform actions with selected items, like deletion
                                }

                                // Check if the file path is valid and not empty
                                if (!filePath.empty())
                                {
                                    // Use std::remove to delete the file
                                    if (std::remove(filePath.c_str()) == 0)
                                    {
                                        //EE_CORE_INFO("File deleted successfully: {}", filePath);
                                        //AssetManager::Refresh();
                                    }
                                    else
                                    {
                                        //EE_CORE_ERROR("File delete fail: {}", filePath);
                                    }
                                }
                                else
                                    //EE_CORE_ERROR("File path empty");

                                    RequestDelete = true;
                            }
                        }
                        ImGui::EndPopup();
                    }

                    ms_io = ImGui::EndMultiSelect();
                    ImGui::EndChild();
                    Selection.ApplyRequests(ms_io);
                    if (want_delete)
                        Selection.ApplyDeletionPostLoop(ms_io, filteredAssets, item_curr_idx_to_focus);

                    // Zooming with CTRL+Wheel
                    if (ImGui::IsWindowAppearing())
                        ZoomWheelAccum = 0.0f;
                    if (ImGui::IsWindowHovered() && io.MouseWheel != 0.0f && ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsAnyItemActive() == false)
                    {
                        ZoomWheelAccum += io.MouseWheel;
                        if (fabsf(ZoomWheelAccum) >= 1.0f)
                        {
                            const float hovered_item_nx = (io.MousePos.x - start_pos.x + LayoutItemSpacing * 0.5f) / LayoutItemStep.x;
                            const float hovered_item_ny = (io.MousePos.y - start_pos.y + LayoutItemSpacing * 0.5f) / LayoutItemStep.y;
                            const int hovered_item_idx = ((int)hovered_item_ny * LayoutColumnCount) + (int)hovered_item_nx;

                            IconSize *= powf(1.1f, (float)(int)ZoomWheelAccum);
                            IconSize = IM_CLAMP(IconSize, 16.0f, 128.0f);
                            ZoomWheelAccum -= (int)ZoomWheelAccum;
                            UpdateLayoutSizes(avail_width);

                            float hovered_item_rel_pos_y = ((float)(hovered_item_idx / LayoutColumnCount) + fmodf(hovered_item_ny, 1.0f)) * LayoutItemStep.y;
                            hovered_item_rel_pos_y += ImGui::GetStyle().WindowPadding.y;
                            float mouse_local_y = io.MousePos.y - ImGui::GetWindowPos().y;
                            ImGui::SetScrollY(hovered_item_rel_pos_y - mouse_local_y);
                        }
                    }
                    ImGui::TreePop();
                }
            }

            // Menu bar
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Add 100 items"))
                        AddItems(100, 1, "100");
                    if (ImGui::MenuItem("Clear items"))
                        ClearItems();
                    ImGui::Separator();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit"))
                {
                    if (ImGui::MenuItem("Delete", "Del", false, Selection.Size > 0))
                        RequestDelete = true;
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Options"))
                {
                    ImGui::PushItemWidth(ImGui::GetFontSize() * 10);

                    ImGui::SeparatorText("Contents");
                    ImGui::Checkbox("Show Type Overlay", &ShowTypeOverlay);
                    ImGui::Checkbox("Allow Sorting", &AllowSorting);

                    ImGui::SeparatorText("Selection Behavior");
                    ImGui::Checkbox("Allow dragging unselected item", &AllowDragUnselected);
                    ImGui::Checkbox("Allow box-selection", &AllowBoxSelect);

                    ImGui::SeparatorText("Layout");
                    ImGui::SliderFloat("Icon Size", &IconSize, 16.0f, 128.0f, "%.0f");
                    ImGui::SameLine(); HelpMarker("Use CTRL+Wheel to zoom");
                    ImGui::SliderInt("Icon Spacing", &IconSpacing, 0, 32);
                    ImGui::SliderInt("Icon Hit Spacing", &IconHitSpacing, 0, 32);
                    ImGui::Checkbox("Stretch Spacing", &StretchSpacing);
                    ImGui::PopItemWidth();
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::Text("Selected: %d/%d items", Selection.Size, Items.Size);
            ImGui::End();
        }

        void AssetBrowser::Update() {
            static size_t count = 0;

            //if (AssetManager::GetRefreshStatus())
            //{
            //}

            std::vector<std::string> textureFiles;
            std::vector<std::string> shaderFiles;
            std::vector<std::string> modelFiles;

            // Scan Textures folder
            for (const auto& entry : fs::directory_iterator("../Resources/Textures")) {
                if (entry.is_regular_file()) {
                    const auto& path = entry.path();
                    if (path.extension() == ".png" || path.extension() == ".jpg" || path.extension() == ".jpeg")
                        textureFiles.push_back(path.string());
                }
            }

            // Scan Shaders folder
            for (const auto& entry : fs::directory_iterator("../Resources/Shaders")) {
                if (entry.is_regular_file()) {
                    const auto& path = entry.path();
                    if (path.extension() == ".vert" || path.extension() == ".frag" || path.extension() == ".glsl")
                        shaderFiles.push_back(path.string());
                }
            }

            // Scan Models folder
            for (const auto& entry : fs::directory_iterator("../Resources/Models")) {
                if (entry.is_regular_file()) {
                    const auto& path = entry.path();
                    if (path.extension() == ".fbx" || path.extension() == ".obj" || path.extension() == ".gltf")
                        modelFiles.push_back(path.string());
                }
            }

            size_t currentCount = textureFiles.size() + shaderFiles.size();
            if (currentCount != count)
            {
                assets_browser.ClearItems();
                assets_browser.ClearItemNames();

                // Populate textures assets
                for (const auto& texPath : textureFiles)
                    assets_browser.AddItems(1, 0, getFileName(texPath));

                // Populate shaders assets
                for (const auto& shaderKey : shaderFiles)
                    assets_browser.AddItems(1, 1, getFileName(shaderKey));

                // Populate models assets
                for (const auto& modelPath : modelFiles)
                    assets_browser.AddItems(1, 2, getFileName(modelPath));

                count = currentCount;
                //AssetManager::SetRefreshStatus(false);
            }
        }

        void AssetBrowser::Render() {
            assets_browser.Draw("Asset Browser");
     
            /*if (AssetManager::showErrorPopup) {
                ImGui::OpenPopup("Error pop-up");
            }

            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Error pop-up", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
                // Display all error messages
                for (const auto& message : AssetManager::errorMessage) {
                    ImGui::Text("%s", message.c_str());
                }

                ImGui::Separator();

                if (ImGui::Button("OK", ImVec2(120, 0))) { // OK button to close
                    AssetManager::showErrorPopup = false;
                    AssetManager::errorMessage.clear(); // Clear all error messages
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::EndPopup();
            }*/
        }
    }
}