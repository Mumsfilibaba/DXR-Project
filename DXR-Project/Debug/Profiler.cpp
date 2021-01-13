#include "Profiler.h"

static void ImGui_PrintTiming(const Char* Text, Int64 Nanoseconds)
{
	constexpr Double MICROSECONDS		= 1000.0;
	constexpr Double MILLISECONDS		= 1000.0 * 1000.0;
	constexpr Double SECONDS			= 1000.0 * 1000.0 * 1000.0;
	constexpr Double INV_MICROSECONDS	= 1.0 / MICROSECONDS;
	constexpr Double INV_MILLISECONDS	= 1.0 / MILLISECONDS;
	constexpr Double INV_SECONDS		= 1.0 / SECONDS;

	ImGui::Text("%s: ", Text);

	ImGui::NextColumn();

	const Double NanosecondsFloat = Double(Nanoseconds);
	if (NanosecondsFloat < MICROSECONDS)
	{
		ImGui::Text("%.4f ns", NanosecondsFloat);
	}
	else if (NanosecondsFloat < MICROSECONDS)
	{
		const Double Time = NanosecondsFloat * INV_MICROSECONDS;
		ImGui::Text("%.4f qs", Time);
	}
	else if (NanosecondsFloat < SECONDS)
	{
		const Double Time = NanosecondsFloat * INV_MILLISECONDS;
		ImGui::Text("%.4f ms", Time);
	}
	else
	{
		const Double Time = NanosecondsFloat * INV_SECONDS;
		ImGui::Text("%.4f s", Time);
	}

	ImGui::NextColumn();
}

/*
* Profiler
*/

Profiler::Profiler()
	: Clock()
	, FrameTime(0)
	, Samples()
{
}

void Profiler::BeginFrame()
{
	Clock.Tick();
}

void Profiler::EndFrame()
{
	const Int64 Delta = Clock.GetDeltaTime().AsNanoSeconds();
	FrameTime.AddSample(Delta);
}

void Profiler::AddSample(const Char* Name, Int64 Nanoseconds)
{
	if (GlobalProfilerEnabled)
	{
		const std::string ScopeName = Name;
	
		auto Entry = Samples.find(ScopeName);
		if (Entry != Samples.end())
		{
			Entry->second.AddSample(Nanoseconds);
		}
		else
		{
			Samples.insert(std::make_pair(ScopeName, Sample(Nanoseconds)));
		}
	}
}

void Profiler::DrawUI()
{
	if (GlobalProfilerEnabled && GlobalDrawProfiler)
	{
		ImGui::Text("CPU Timings:");
		ImGui::Separator();

		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, 260.0f);

		ImGui_PrintTiming("FrameTime", FrameTime.CurrentSample);
		ImGui::Columns(1);

		ImGui::Separator();

		ImGui::Columns(2, nullptr, false);
		for (auto& Sample : Samples)
		{
			const Char* Name = Sample.first.c_str();
			ImGui_PrintTiming(Name, Sample.second.CurrentSample);
		}
		ImGui::Columns(1);
	}
}
