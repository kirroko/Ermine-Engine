/* Start Header ************************************************************************/
/*!
\file       ConsoleGUI.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       20/10/2025
\brief      This file contains the Unity-like Console window and log collector for the editor.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "ConsoleGUI.h"

using namespace Ermine;

// =======================
// EditorConsole
// =======================
EditorConsole& EditorConsole::GetInstance()
{
    static EditorConsole s_instance;
    return s_instance;
}

void EditorConsole::SetCapacity(size_t cap)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_capacity = cap < 100 ? 100 : cap;
    while (m_logs.size() > m_capacity)
        m_logs.pop_front();
}

void EditorConsole::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.clear();
    m_counts = { 0,0,0,0 };
}

static uint64_t now_ms()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void EditorConsole::AddLog(ConsoleLogType type, std::string_view message,
    std::string_view channel, std::string_view details,
    uint64_t frame)
{
    ConsoleLogEntry e;
    e.type = type;
    e.message.assign(message.begin(), message.end());
    e.channel.assign(channel.begin(), channel.end());
    e.details.assign(details.begin(), details.end());
    e.frame = frame;
    e.timestampMs = now_ms();

    std::lock_guard<std::mutex> lock(m_mutex);
    addLogUnlocked(std::move(e));
}

void EditorConsole::AddLogFmt(ConsoleLogType type, const char* fmt, ...)
{
    // Format with dynamic buffer using vsnprintf
    va_list args;
    va_start(args, fmt);

    // First try with a small buffer
    char stackBuf[512];
    va_list argsCopy;
#if defined(_MSC_VER)
    argsCopy = args;
#else
    va_copy(argsCopy, args);
#endif
    int needed = vsnprintf(stackBuf, sizeof(stackBuf), fmt, argsCopy);
#if !(defined(_MSC_VER))
    va_end(argsCopy);
#endif

    std::string msg;
    if (needed < 0)
    {
        // Fallback: store raw format on failure
        msg = fmt ? fmt : "";
    }
    else if (static_cast<size_t>(needed) < sizeof(stackBuf))
    {
        msg.assign(stackBuf, stackBuf + needed);
    }
    else
    {
        size_t size = static_cast<size_t>(needed) + 1;
        std::string dyn;
        dyn.resize(size);
        vsnprintf(dyn.data(), size, fmt, args);
        // Drop trailing '\0'
        if (!dyn.empty() && dyn.back() == '\0') dyn.pop_back();
        msg = std::move(dyn);
    }

    va_end(args);

    ConsoleLogEntry e;
    e.type = type;
    e.message = std::move(msg);
    e.timestampMs = now_ms();

    std::lock_guard<std::mutex> lock(m_mutex);
    addLogUnlocked(std::move(e));
}

void EditorConsole::addLogUnlocked(ConsoleLogEntry&& e)
{
    // Update counts
    m_counts[static_cast<size_t>(e.type)]++;

    // Collapse consecutive duplicates (same type + message + channel)
    if (!m_logs.empty())
    {
        auto& last = m_logs.back();
        if (last.type == e.type && last.message == e.message && last.channel == e.channel)
        {
            last.count++;
        }
        else
        {
            m_logs.emplace_back(std::move(e));
        }
    }
    else
    {
        m_logs.emplace_back(std::move(e));
    }

    // Enforce capacity
    while (m_logs.size() > m_capacity)
    {
        auto& front = m_logs.front();
        m_counts[static_cast<size_t>(front.type)] =
            (m_counts[static_cast<size_t>(front.type)] > front.count)
            ? (m_counts[static_cast<size_t>(front.type)] - front.count)
            : 0;
        m_logs.pop_front();
    }
}

void EditorConsole::Snapshot(std::vector<ConsoleLogEntry>& out) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    out.assign(m_logs.begin(), m_logs.end());
}

std::array<uint32_t, 4> EditorConsole::Counts() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_counts;
}

// =======================
// ConsoleGUI
// =======================
ConsoleGUI::ConsoleGUI()
    : ImGUIWindow("Console")
{
}

ImU32 ConsoleGUI::colorFor(ConsoleLogType t)
{
    switch (t)
    {
    case ConsoleLogType::Info:    return IM_COL32(200, 200, 200, 255);
    case ConsoleLogType::Warning: return IM_COL32(255, 200, 0, 255);
    case ConsoleLogType::Error:   return IM_COL32(255, 80, 80, 255);
    case ConsoleLogType::Debug:   return IM_COL32(150, 200, 255, 255);
    default:                      return IM_COL32(200, 200, 200, 255);
    }
}

const char* ConsoleGUI::iconFor(ConsoleLogType t)
{
    switch (t)
    {
    case ConsoleLogType::Info:    return "i";
    case ConsoleLogType::Warning: return "!";
    case ConsoleLogType::Error:   return "X";
    case ConsoleLogType::Debug:   return "D";
    default:                      return "?";
    }
}

void ConsoleGUI::copyToClipboard(const std::vector<ConsoleLogEntry>& src,
    const std::vector<int>& indices,
    const std::vector<uint32_t>* collapsedCounts,
    bool onlySelected, int selectedIndex)
{
    ImGui::LogToClipboard();
    auto emit = [&](int i, uint32_t countOverride)
        {
            const auto& e = src[static_cast<size_t>(i)];
            const char* lvl =
                (e.type == ConsoleLogType::Info) ? "Info" :
                (e.type == ConsoleLogType::Warning) ? "Warning" :
                (e.type == ConsoleLogType::Error) ? "Error" : "Debug";

            // Time formatting (relative ms since start for simplicity)
            ImGui::LogText("[%s]%s %s%s%s (x%u)\n",
                lvl,
                e.channel.empty() ? "" : ("[" + e.channel + "]").c_str(),
                e.message.c_str(),
                e.details.empty() ? "" : " -- ",
                e.details.empty() ? "" : e.details.c_str(),
                countOverride > 0 ? countOverride : e.count);
        };

    if (onlySelected && selectedIndex >= 0 && selectedIndex < static_cast<int>(indices.size()))
    {
        int i = indices[static_cast<size_t>(selectedIndex)];
        uint32_t cnt = collapsedCounts ? (*collapsedCounts)[static_cast<size_t>(selectedIndex)] : 0;
        emit(i, cnt);
    }
    else
    {
        for (size_t idx = 0; idx < indices.size(); ++idx)
        {
            int i = indices[idx];
            uint32_t cnt = collapsedCounts ? (*collapsedCounts)[idx] : 0;
            emit(i, cnt);
        }
    }
    ImGui::LogFinish();
}

void ConsoleGUI::buildDisplayList(std::vector<int>& outDisplayIndices,
    std::vector<uint32_t>& outCollapsedCounts) const
{
    outDisplayIndices.clear();
    outCollapsedCounts.clear();

    const bool typeAllow[4] = {
        m_showInfo, m_showWarning, m_showError, m_showDebug
    };

    // Filter then optionally collapse by (type+message+channel) globally (Unity's Collapse)
    // We collapse across the entire snapshot, not just consecutive.
    if (m_collapse)
    {
        struct Key { ConsoleLogType t; std::string msg; std::string ch; };
        struct KeyHash {
            size_t operator()(Key const& k) const noexcept {
                std::hash<std::string> hs;
                return (static_cast<size_t>(k.t) * 1315423911u) ^ hs(k.msg) ^ (hs(k.ch) << 1);
            }
        };
        struct KeyEq {
            bool operator()(Key const& a, Key const& b) const noexcept {
                return a.t == b.t && a.msg == b.msg && a.ch == b.ch;
            }
        };

        std::unordered_map<Key, uint32_t, KeyHash, KeyEq> agg;
        std::unordered_map<Key, int, KeyHash, KeyEq> firstIndex;
        for (int i = 0; i < static_cast<int>(m_snapshot.size()); ++i)
        {
            const auto& e = m_snapshot[static_cast<size_t>(i)];
            if (!typeAllow[static_cast<size_t>(e.type)])
                continue;
            if (!m_filter.PassFilter(e.message.c_str()) && (e.channel.empty() || !m_filter.PassFilter(e.channel.c_str())))
                continue;

            Key k{ e.type, e.message, e.channel };
            auto it = agg.find(k);
            if (it == agg.end())
            {
                agg.emplace(k, e.count);
                firstIndex.emplace(k, i);
            }
            else
            {
                it->second += e.count;
            }
        }
        outDisplayIndices.reserve(agg.size());
        outCollapsedCounts.reserve(agg.size());
        for (auto& kv : firstIndex)
        {
            outDisplayIndices.push_back(kv.second);
            outCollapsedCounts.push_back(agg[kv.first]);
        }
        // Keep original order by sorting on first occurrence index
        std::vector<size_t> order(outDisplayIndices.size());
        for (size_t i = 0; i < order.size(); ++i) order[i] = i;
        std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return outDisplayIndices[a] < outDisplayIndices[b]; });

        std::vector<int> di; di.reserve(order.size());
        std::vector<uint32_t> dc; dc.reserve(order.size());
        for (size_t oi : order) { di.push_back(outDisplayIndices[oi]); dc.push_back(outCollapsedCounts[oi]); }
        outDisplayIndices.swap(di);
        outCollapsedCounts.swap(dc);
    }
    else
    {
        for (int i = 0; i < static_cast<int>(m_snapshot.size()); ++i)
        {
            const auto& e = m_snapshot[static_cast<size_t>(i)];
            if (!typeAllow[static_cast<size_t>(e.type)])
                continue;
            if (!m_filter.PassFilter(e.message.c_str()) && (e.channel.empty() || !m_filter.PassFilter(e.channel.c_str())))
                continue;
            outDisplayIndices.push_back(i);
        }
    }
}

void ConsoleGUI::Update()
{
    if (!show)
        return;

    if (ImGui::Begin("Console", &show))
    {
        // Refresh snapshot (always collect, Pause only affects scrolling)
        EditorConsole::GetInstance().Snapshot(m_snapshot);

        // Toolbar
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, 6.f));
            ImGui::BeginGroup();

            // Level toggles
            ImGui::BeginDisabled(false);
            ImGui::Checkbox("Info", &m_showInfo); ImGui::SameLine();
            ImGui::Checkbox("Warning", &m_showWarning); ImGui::SameLine();
            ImGui::Checkbox("Error", &m_showError); ImGui::SameLine();
            ImGui::Checkbox("Debug", &m_showDebug);
            ImGui::EndDisabled();

            ImGui::SameLine();

            // Collapse/Pause/Auto-scroll
            ImGui::Checkbox("Collapse", &m_collapse); ImGui::SameLine();
            ImGui::Checkbox("Pause", &m_pause); ImGui::SameLine();
            ImGui::Checkbox("Auto-scroll", &m_autoScroll);

            ImGui::SameLine();

            // Clear / Copy
            if (ImGui::Button("Clear"))
            {
                EditorConsole::GetInstance().Clear();
                m_selectedIndex = -1;
            }
            ImGui::SameLine();
            bool copyOnlySelected = (m_selectedIndex >= 0);
            if (ImGui::Button(copyOnlySelected ? "Copy Selected" : "Copy All"))
            {
                std::vector<int> indices;
                std::vector<uint32_t> counts;
                buildDisplayList(indices, counts);
                copyToClipboard(m_snapshot, indices, m_collapse ? &counts : nullptr, copyOnlySelected, m_selectedIndex);
            }

            ImGui::SameLine();

            // Search
            constexpr float searchWidth = 240.0f;
            ImGui::SetNextItemWidth(searchWidth);
            if (m_focusSearch) { ImGui::SetKeyboardFocusHere(); m_focusSearch = false; }
            m_filter.Draw("Search");

            ImGui::EndGroup();
            ImGui::PopStyleVar();
        }

        ImGui::Separator();

        // Counts
        {
            auto counts = EditorConsole::GetInstance().Counts();
            ImGui::Text("Info: %u  |  Warning: %u  |  Error: %u  |  Debug: %u",
                counts[0], counts[1], counts[2], counts[3]);
        }

        // Build display indices (+ optional collapse counts)
        std::vector<int> displayIndices;
        std::vector<uint32_t> collapsedCounts;
        buildDisplayList(displayIndices, collapsedCounts);

        // Main list region
        constexpr float detailsHeight = 120.0f;
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::BeginChild("ConsoleListRegion", ImVec2(0, avail.y - detailsHeight), false,
            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_HorizontalScrollbar /*| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse*/);

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(displayIndices.size()));
        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
            {
                const int srcIdx = displayIndices[static_cast<size_t>(row)];
                const auto& e = m_snapshot[static_cast<size_t>(srcIdx)];

                ImGui::PushStyleColor(ImGuiCol_Text, colorFor(e.type));
                bool selected = (m_selectedIndex == row);
                // Row text
                std::string line;
                line.reserve(e.message.size() + 64);
                if (!e.channel.empty()) { line += "["; line += e.channel; line += "] "; }
                line += e.message;

                // Icon + Selectable
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(iconFor(e.type));
                ImGui::SameLine();
                if (ImGui::Selectable(line.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick))
                {
                    m_selectedIndex = row;
                }
                ImGui::PopStyleColor();

                // Draw collapsed count on the right if >1
                uint32_t shownCount = m_collapse ? collapsedCounts[static_cast<size_t>(row)] : e.count;
                if (shownCount > 1)
                {
                    float right = ImGui::GetWindowContentRegionMax().x + ImGui::GetWindowPos().x;
                    ImVec2 cursor = ImGui::GetCursorScreenPos();
                    ImGui::SameLine();
                    ImGui::SetCursorScreenPos(ImVec2(right - 48.0f, cursor.y));
                    ImGui::TextDisabled("x%u", shownCount);
                }
            }
        }
        clipper.End();

        // Auto-scroll behavior
        if (!m_pause && m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 5.0f)
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();

        // Details panel
        ImGui::Separator();
        ImGui::BeginChild("ConsoleDetailsRegion", ImVec2(0, 0), false);
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(displayIndices.size()))
        {
            const int srcIdx = displayIndices[static_cast<size_t>(m_selectedIndex)];
            const auto& e = m_snapshot[static_cast<size_t>(srcIdx)];

            ImGui::Text("Level: ");
            ImGui::SameLine();
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colorFor(e.type)), "%s",
                (e.type == ConsoleLogType::Info) ? "Info" :
                (e.type == ConsoleLogType::Warning) ? "Warning" :
                (e.type == ConsoleLogType::Error) ? "Error" : "Debug");

            if (!e.channel.empty())
            {
                ImGui::SameLine(); ImGui::Text("| Channel: %s", e.channel.c_str());
            }

            ImGui::TextWrapped("%s", e.message.c_str());
            if (!e.details.empty())
            {
                ImGui::Separator();
                ImGui::TextDisabled("%s", e.details.c_str());
            }
        }
        else
        {
            ImGui::TextDisabled("Select a log entry to see details...");
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void ConsoleGUI::Render()
{
}

#if defined(ERMINE_USE_SPDLOG)
void ImGuiConsoleSink_mt::sink_it_(const spdlog::details::log_msg& msg)
{
    spdlog::memory_buf_t formatted;
    base_sink<std::mutex>::formatter_->format(msg, formatted);
    std::string text = fmt::to_string(formatted);

    ConsoleLogType t = ConsoleLogType::Info;
    switch (msg.level)
    {
    case spdlog::level::trace:
    case spdlog::level::debug:   t = ConsoleLogType::Debug; break;
    case spdlog::level::info:    t = ConsoleLogType::Info; break;
    case spdlog::level::warn:    t = ConsoleLogType::Warning; break;
    case spdlog::level::err:
    case spdlog::level::critical:t = ConsoleLogType::Error; break;
    default: break;
    }

    EditorConsole::GetInstance().AddLog(
        t,
        std::string_view{ text.data(), text.size() },
        std::string_view{ msg.logger_name.data(), msg.logger_name.size() }
    );
}
#endif