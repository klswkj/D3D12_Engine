#include "stdafx.h"
#include "Profiling.h"
#include "Color.h"
#include "CommandContext.h"
#include "TextManager.h"
#include "GpuTime.h"
#include "CpuTime.h"
#include "Graphics.h"
#include "Window.h"
#include "WindowInput.h"

#define PERF_GRAPH_ERROR uint32_t(0xFFFFFFFF)
namespace Profiling
{
	bool Paused = false;
}

// m_CPUTime
// m_GPUTime
class StatHistory
{
public:
	StatHistory()
	{
		for (size_t i = 0; i < kHistorySize; ++i)
		{
			m_RecentHistory[i] = 0.0f;
		}
		for (size_t i = 0; i < kExtendedHistorySize; ++i)
		{
			m_ExtendedHistory[i] = 0.0f;
		}

		ZeroMemory(&m_RecentHistory[0],   kHistorySize);
		ZeroMemory(&m_ExtendedHistory[0], kExtendedHistorySize);

		m_Recent = 0.0f;
		m_Average = 0.0f;
		m_Minimum = 0.0f;
		m_Maximum = 0.0f;
	}

	void RecordStat(uint32_t FrameIndex, float Value)
	{
		m_RecentHistory[FrameIndex % kHistorySize] = Value;
		m_ExtendedHistory[FrameIndex % kExtendedHistorySize] = Value;
		m_Recent = Value;

		uint32_t ValidCount = 0;
		m_Minimum = FLT_MAX;
		m_Maximum = 0.0f;
		m_Average = 0.0f;

		for (float val : m_RecentHistory)
		{
			if (0.0f < val)
			{
				++ValidCount;
				m_Average += val;
				m_Minimum = min(val, m_Minimum);
				m_Maximum = max(val, m_Maximum);
			}
		}

		if (0 < ValidCount)
		{
			m_Average /= (float)ValidCount;
		}
		else
		{
			m_Minimum = 0.0f;
		}
	}

	float GetLast(void) const { return m_Recent; }
	float GetMax(void) const { return m_Maximum; }
	float GetMin(void) const { return m_Minimum; }
	float GetAvg(void) const { return m_Average; }

	const float* GetHistory(void) const { return m_ExtendedHistory; }
	uint32_t GetHistoryLength(void) const { return kExtendedHistorySize; }

private:
	static const uint32_t kHistorySize = 64u;
	static const uint32_t kExtendedHistorySize = 256u;
	float m_RecentHistory[kHistorySize];
	float m_ExtendedHistory[kExtendedHistorySize];
	float m_Recent;
	float m_Average;
	float m_Minimum;
	float m_Maximum;
};

class StatPlot
{
public:
	StatPlot(StatHistory& Data, custom::Color Col = custom::Color(1.0f, 1.0f, 1.0f))
		: m_StatData(Data), m_PlotColor(Col)
	{
	}

	void SetColor(custom::Color Col)
	{
		m_PlotColor = Col;
	}

private:
	StatHistory& m_StatData;
	custom::Color m_PlotColor;
};

class StatGraph
{
public:
	StatGraph(const std::wstring& Label, D3D12_RECT Window)
		: m_Label(Label), m_Window(Window), m_BGColor(0.0f, 0.0f, 0.0f, 0.2f)
	{
		m_PeakValue = 0.0f;
	}

	void SetLabel(const std::wstring& Label)
	{
		m_Label = Label;
	}

	void SetWindow(D3D12_RECT Window)
	{
		m_Window = Window;
	}

	uint32_t AddPlot(const StatPlot& P)
	{
		uint32_t Idx = (uint32_t)m_Stats.size();
		m_Stats.push_back(P);
		return Idx;
	}

	StatPlot& GetPlot(uint32_t Handle)
	{
		if (Handle < m_Stats.size())
		{
			return m_Stats[Handle];
		}
	}

	void Draw(custom::GraphicsContext& Context);

private:
	std::wstring m_Label;
	D3D12_RECT m_Window;
	std::vector<StatPlot> m_Stats;
	custom::Color m_BGColor;
	float m_PeakValue;
};

////////////////////
///////////////////

class GpuTimer
{
public:
	GpuTimer()
	{
		m_TimerIndex = GPUTime::NewTimer();
	}

	void Start(custom::CommandContext& Context)
	{
		GPUTime::StartTimer(Context, m_TimerIndex);
	}

	void Stop(custom::CommandContext& Context)
	{
		GPUTime::StopTimer(Context, m_TimerIndex);
	}

	float GetTime()
	{
		return GPUTime::GetTime(m_TimerIndex);
	}

	uint32_t GetTimerIndex()
	{
		return m_TimerIndex;
	}

private:
	uint32_t m_TimerIndex;
};

class NestedTimingTree
{
public:
	NestedTimingTree(const std::wstring& name, NestedTimingTree* parent = nullptr)
		: m_Name(name), m_Parent(parent), m_IsExpanded(false), m_IsGraphed(false)/*, m_GraphHandle(PERF_GRAPH_ERROR)*/ 
	{
		m_StartTick = 0ull;
		m_EndTick = 0ull;
	}

	NestedTimingTree* GetChild(const std::wstring& name)
	{
		auto iter = m_LookUpHashMap.find(name);
		if (iter != m_LookUpHashMap.end())
		{
			return iter->second;
		}

		NestedTimingTree* node = new NestedTimingTree(name, this);
		m_Children.push_back(node);
		m_LookUpHashMap[name] = node;
		return node;
	}

	NestedTimingTree* NextScope(void)
	{
		if (m_IsExpanded && m_Children.size())
		{
			return m_Children[0];
		}

		return m_Parent->NextChild(this);
	}

	NestedTimingTree* PrevScope(void)
	{
		NestedTimingTree* prev = m_Parent->PrevChild(this);
		return prev == m_Parent ? prev : prev->LastChild();
	}

	NestedTimingTree* FirstChild(void)
	{
		return m_Children.size() == 0 ? nullptr : m_Children[0];
	}

	NestedTimingTree* LastChild(void)
	{
		if (!m_IsExpanded || m_Children.size() == 0)
		{
			return this;
		}

		return m_Children.back()->LastChild();
	}

	NestedTimingTree* NextChild(NestedTimingTree* curChild)
	{
		ASSERT(curChild->m_Parent == this);

		for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
		{
			if (*iter == curChild)
			{
				auto nextChild = iter; ++nextChild;
				if (nextChild != m_Children.end())
					return *nextChild;
			}
		}

		if (m_Parent != nullptr)
		{
			return m_Parent->NextChild(this);
		}
		else
		{
			return &sm_RootScope;
		}
	}

	NestedTimingTree* PrevChild(NestedTimingTree* curChild)
	{
		ASSERT(curChild->m_Parent == this);

		if (*m_Children.begin() == curChild)
		{
			if (this == &sm_RootScope)
			{
				return sm_RootScope.LastChild();
			}
			else
			{
				return this;
			}
		}

		for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
		{
			if (*iter == curChild)
			{
				auto prevChild = iter; --prevChild;
				return *prevChild;
			}
		}

		ASSERT(false, "All attempts to find a previous timing sample failed");
		return nullptr;
	}

	void StartTiming(custom::CommandContext* Context)
	{
		m_StartTick = CPUTime::GetCurrentTick();

		if (Context == nullptr)
		{
			return;
		}

		m_GpuTimer.Start(*Context);

		Context->PIXBeginEvent(m_Name.c_str());
	}

	void StopTiming(custom::CommandContext* Context)
	{
		m_EndTick = CPUTime::GetCurrentTick();
		if (Context == nullptr)
			return;

		m_GpuTimer.Stop(*Context);

		Context->PIXEndEvent();
	}

	void GatherTimes(uint32_t FrameIndex)
	{
		if (sm_SelectedScope == this)
		{
			// GraphRenderer::SetSelectedIndex(m_GpuTimer.GetTimerIndex());
		}
		if (Profiling::Paused)
		{
			for (auto node : m_Children)
			{
				node->GatherTimes(FrameIndex);
			}
			return;
		}
		m_CpuTime.RecordStat(FrameIndex, 1000.0f * (float)CPUTime::TimeBetweenTicks(m_StartTick, m_EndTick));
		m_GpuTime.RecordStat(FrameIndex, 1000.0f * m_GpuTimer.GetTime());

		for (auto node : m_Children)
		{
			node->GatherTimes(FrameIndex);
		}

		m_StartTick = 0;
		m_EndTick = 0;
	}

	void SumInclusiveTimes(float& cpuTime, float& gpuTime)
	{
		cpuTime = 0.0f;
		gpuTime = 0.0f;
		for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
		{
			cpuTime += (*iter)->m_CpuTime.GetLast();
			gpuTime += (*iter)->m_GpuTime.GetLast();
		}
	}

	static void PushProfilingMarker(const std::wstring& name, custom::CommandContext* Context);
	static void PopProfilingMarker(custom::CommandContext* Context);

	static void Update();
	static void UpdateTimes()
	{
		uint32_t FrameIndex = (uint32_t)graphics::GetFrameCount();

		GPUTime::BeginReadBack();
		sm_RootScope.GatherTimes(FrameIndex);
		s_FrameDelta.RecordStat(FrameIndex, GPUTime::GetTime(0));
		GPUTime::EndReadBack();

		float TotalCpuTime, TotalGpuTime;
		sm_RootScope.SumInclusiveTimes(TotalCpuTime, TotalGpuTime);
		s_TotalCpuTime.RecordStat(FrameIndex, TotalCpuTime);
		s_TotalGpuTime.RecordStat(FrameIndex, TotalGpuTime);

		// GraphRenderer::Update(DirectX::XMFLOAT2(TotalCpuTime, TotalGpuTime), 0, GraphType::Global);
	}

	static float GetTotalCpuTime(void) { return s_TotalCpuTime.GetAvg(); }
	static float GetTotalGpuTime(void) { return s_TotalGpuTime.GetAvg(); }
	static float GetFrameDelta(void) { return s_FrameDelta.GetAvg(); }

	static void Display(TextContext& Text, float x)
	{
		float curX = Text.GetCursorX();
		Text.DrawString("  ");
		float indent = Text.GetCursorX() - curX;
		Text.SetCursorX(curX);
		sm_RootScope.DisplayNode(Text, x - indent, indent);
		sm_RootScope.StoreToGraph();
	}

	void Toggle()
	{
		//if (m_GraphHandle == PERF_GRAPH_ERROR)
		//    m_GraphHandle = GraphRenderer::InitGraph(GraphType::Profile);
		//m_IsGraphed = GraphRenderer::ManageGraphs(m_GraphHandle, GraphType::Profile);
	}
	bool IsGraphed() { return m_IsGraphed; }

private:
	void DisplayNode(TextContext& Text, float x, float indent);
	void StoreToGraph();
	void DeleteChildren()
	{
		for (auto node : m_Children)
		{
			delete node;
		}

		m_Children.clear();
	}

	std::wstring m_Name;
	NestedTimingTree* m_Parent;
	std::vector<NestedTimingTree*> m_Children;
	std::unordered_map<std::wstring, NestedTimingTree*> m_LookUpHashMap;
	int64_t m_StartTick;
	int64_t m_EndTick;
	StatHistory m_CpuTime;
	StatHistory m_GpuTime;
	bool m_IsExpanded;
	GpuTimer m_GpuTimer;
	bool m_IsGraphed;
	// GraphHandle m_GraphHandle;
	static StatHistory s_TotalCpuTime;
	static StatHistory s_TotalGpuTime;
	static StatHistory s_FrameDelta;
	static NestedTimingTree sm_RootScope;
	static NestedTimingTree* sm_CurrentNode;
	static NestedTimingTree* sm_SelectedScope;

	static bool sm_CursorOnGraph;

};

StatHistory NestedTimingTree::s_TotalCpuTime;
StatHistory NestedTimingTree::s_TotalGpuTime;
StatHistory NestedTimingTree::s_FrameDelta;
NestedTimingTree NestedTimingTree::sm_RootScope(L"");
NestedTimingTree* NestedTimingTree::sm_CurrentNode = &NestedTimingTree::sm_RootScope;
NestedTimingTree* NestedTimingTree::sm_SelectedScope = &NestedTimingTree::sm_RootScope;
bool NestedTimingTree::sm_CursorOnGraph = true;

namespace Profiling
{
	// BoolVar DrawFrameRate("Display Frame Rate", true);
	// BoolVar DrawProfiler("Display Profiler", true);
	// BoolVar DrawPerfGraph("Display Performance Graph", true);
	// const bool DrawPerfGraph = false;

	bool DrawFrameRate = true;
	bool DrawProfiler = true;
	bool DrawPerfGraph = true;

	void Update(void)
	{
		if (windowInput::IsFirstPressed(windowInput::DigitalInput::kStartButton)
			|| windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_space))
		{
			Paused = !Paused;
		}
		NestedTimingTree::UpdateTimes();
	}

	void BeginBlock(const std::wstring& name, custom::CommandContext* Context)
	{
		NestedTimingTree::PushProfilingMarker(name, Context);
	}

	void EndBlock(custom::CommandContext* Context)
	{
		NestedTimingTree::PopProfilingMarker(Context);
	}

	bool IsPaused()
	{
		return Paused;
	}

	void DisplayFrameRate(TextContext& Text)
	{
		if (!DrawFrameRate)
		{
			return;
		}

		float cpuTime = NestedTimingTree::GetTotalCpuTime();
		float gpuTime = NestedTimingTree::GetTotalGpuTime();
		float frameRate = 1.0f / NestedTimingTree::GetFrameDelta();

		Text.DrawFormattedString
		(
			"CPU %7.3f ms, GPU %7.3f ms, %3u Hz\n",
			cpuTime, gpuTime, 
			(uint32_t)(frameRate + 0.5f)
		);
	}

	void DisplayPerfGraph(custom::GraphicsContext& Context)
	{
		if (DrawPerfGraph)
		{
			// GraphRenderer::RenderGraphs(Context, GraphType::Global);
		}
	}

	void Display(TextContext& Text, float x, float y, float /*w*/, float /*h*/)
	{
		Text.ResetCursor(x, y);

		if (DrawProfiler)
		{
			// Text.GetCommandContext().SetScissor((uint32_t)Floor(x), (uint32_t)Floor(y), (uint32_t)Ceiling(w), (uint32_t)Ceiling(h));

			NestedTimingTree::Update();

			Text.SetColor(custom::Color(0.5f, 1.0f, 1.0f));
			Text.DrawString("Engine Profiling");
			Text.SetColor(custom::Color(0.8f, 0.8f, 0.8f));
			Text.SetTextSize(20.0f);
			Text.DrawString("             CPU    GPU");
			Text.SetTextSize(24.0f);
			Text.NewLine();
			Text.SetTextSize(20.0f);
			Text.SetColor(custom::Color(1.0f, 1.0f, 1.0f));

			NestedTimingTree::Display(Text, x);
		}

		Text.GetCommandContext().SetScissor(0, 0, window::g_TargetWindowWidth, window::g_TargetWindowHeight);
	}

} // Profiling

void NestedTimingTree::PushProfilingMarker(const std::wstring& name, custom::CommandContext* Context)
{
	sm_CurrentNode = sm_CurrentNode->GetChild(name);
	sm_CurrentNode->StartTiming(Context);
}

void NestedTimingTree::PopProfilingMarker(custom::CommandContext* Context)
{
	sm_CurrentNode->StopTiming(Context);
	sm_CurrentNode = sm_CurrentNode->m_Parent;
}

void NestedTimingTree::Update()
{
	ASSERT(sm_SelectedScope != nullptr, "Corrupted profiling data structure");

	if (sm_SelectedScope == &sm_RootScope)
	{
		sm_SelectedScope = sm_RootScope.FirstChild();
		if (sm_SelectedScope == &sm_RootScope)
		{
			return;
		}
	}

	if (windowInput::IsFirstPressed(windowInput::DigitalInput::kDPadLeft) ||
		windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_left))
	{
		//if still on graphs go back to text
		if (sm_CursorOnGraph)
		{
			sm_CursorOnGraph = !sm_CursorOnGraph;
		}
		else
		{
			sm_SelectedScope->m_IsExpanded = false;
		}
	}
	else if (windowInput::IsFirstPressed(windowInput::DigitalInput::kDPadRight) ||
		windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_right))
	{
		if (sm_SelectedScope->m_IsExpanded == true && !sm_CursorOnGraph)
		{
			sm_CursorOnGraph = true;
		}
		else
		{
			sm_SelectedScope->m_IsExpanded = true;
		}
	}
	else if (windowInput::IsFirstPressed(windowInput::DigitalInput::kDPadDown) ||
		windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_down))
	{
		sm_SelectedScope = sm_SelectedScope ? sm_SelectedScope->NextScope() : nullptr;
	}
	else if (windowInput::IsFirstPressed(windowInput::DigitalInput::kDPadUp) ||
		windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_up))
	{
		sm_SelectedScope = sm_SelectedScope ? sm_SelectedScope->PrevScope() : nullptr;
	}
	else if (windowInput::IsFirstPressed(windowInput::DigitalInput::kAButton) ||
		windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_return))
	{
		sm_SelectedScope->Toggle();
	}

}

void NestedTimingTree::DisplayNode(TextContext& Text, float leftMargin, float indent)
{
	if (this == &sm_RootScope)
	{
		m_IsExpanded = true;
		sm_RootScope.FirstChild()->m_IsExpanded = true;
	}
	else
	{
		if (sm_SelectedScope == this && !sm_CursorOnGraph)
		{
			Text.SetColor(custom::Color(1.0f, 1.0f, 0.5f));
		}
		else
		{
			Text.SetColor(custom::Color(1.0f, 1.0f, 1.0f));
		}

		Text.SetLeftMargin(leftMargin);
		Text.SetCursorX(leftMargin);

		if (m_Children.size() == 0)
		{
			Text.DrawString("  ");
		}
		else if (m_IsExpanded)
		{
			Text.DrawString("- ");
		}
		else
		{
			Text.DrawString("+ ");
		}

		Text.DrawString(m_Name.c_str());
		Text.SetCursorX(leftMargin + 300.0f);
		Text.DrawFormattedString("%6.3f %6.3f   ", m_CpuTime.GetAvg(), m_GpuTime.GetAvg());

		if (IsGraphed())
		{
			Text.SetColor(custom::Color(1.0f, 1.0f, 1.0f));
			// Text.SetColor(GraphRenderer::GetGraphColor(m_GraphHandle, GraphType::Profile));
			Text.DrawString("  []\n");
		}
		else
		{
			Text.DrawString("\n");
		}
	}

	if (!m_IsExpanded)
		return;

	for (auto node : m_Children)
		node->DisplayNode(Text, leftMargin + indent, indent);
}

void NestedTimingTree::StoreToGraph()
{
	//if (m_GraphHandle != PERF_GRAPH_ERROR)
	//{
	//	GraphRenderer::Update(DirectX::XMFLOAT2(m_CpuTime.GetLast(), m_GpuTime.GetLast()), m_GraphHandle, GraphType::Profile);
	//}

	for (NestedTimingTree* node : m_Children)
	{
		node->StoreToGraph();
	}
}
