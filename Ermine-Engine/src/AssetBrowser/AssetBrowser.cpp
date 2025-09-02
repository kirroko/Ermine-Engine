#include "PreCompile.h"
#include "AssetBrowser.h"

namespace Ermine {
    namespace ImguiUI {

        const ImGuiTableSortSpecs* ExampleAssetB::current_sortSpecs = NULL;

        // Compare function to be used by qsort()
        int IMGUI_CDECL ExampleAssetB::CompareWithSortSpecs(const void* lhs, const void* rhs)
        {
            const ExampleAssetB* a = (const ExampleAssetB*)lhs;
            const ExampleAssetB* b = (const ExampleAssetB*)rhs;
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

        void ExampleAssetB::SortWithSortSpecs(ImGuiTableSortSpecs* sort_specs, ExampleAssetB* items, int items_count)
        {
            current_sortSpecs = sort_specs; // Store in variable accessible by the sort function.
            if (items_count > 1)
                qsort(items, (size_t)items_count, sizeof(items[0]), ExampleAssetB::CompareWithSortSpecs);
            current_sortSpecs = NULL;
        }

        void AssetBrowser::Update() {

        }

        void AssetBrowser::Render() {
            static ExampleAssetsBrowser assets_browser;
            assets_browser.Draw("Assets Browser");
        }
    }
}