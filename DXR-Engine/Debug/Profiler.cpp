#include "Profiler.h"

#include "Rendering/DebugUI.h"

#include "RenderingCore/RenderLayer.h"

constexpr Float MICROSECONDS		= 1000.0f;
constexpr Float MILLISECONDS		= 1000.0f * 1000.0f;
constexpr Float SECONDS				= 1000.0f * 1000.0f * 1000.0f;
constexpr Float INV_MICROSECONDS	= 1.0f / MICROSECONDS;
constexpr Float INV_MILLISECONDS	= 1.0f / MILLISECONDS;
constexpr Float INV_SECONDS			= 1.0f / SECONDS;

constexpr Float MAX_FRAMETIME_MS = 1000.0f / 30.0f;

static void ImGui_PrintTiming(const Char* Text, Float Num)
{
	ImGui::Text("%s: ", Text);

	ImGui::NextColumn();

	if (Num < MICROSECONDS)
	{
		ImGui::Text("%.4f ns", Num);
	}
	else if (Num < MICROSECONDS)
	{
		const Float Time = Num * INV_MICROSECONDS;
		ImGui::Text("%.4f qs", Time);
	}
	else if (Num < SECONDS)
	{
		const Float Time = Num * INV_MILLISECONDS;
		ImGui::Text("%.4f ms", Time);
	}
	else
	{
		const Float Time = Num * INV_SECONDS;
		ImGui::Text("%.4f s", Time);
	}
}

static Float ImGui_ConvertNumber(Float Num)
{
	if (Num < MICROSECONDS)
	{
		return Num;
	}
	else if (Num < MICROSECONDS)
	{
		return Num * INV_MICROSECONDS;
	}
	else if (Num < SECONDS)
	{
		return Num * INV_MILLISECONDS;
	}
	else
	{
		return Num * INV_SECONDS;
	}
}

static Float ImGui_GetMaxLimit(Float Num)
{
	if (Num < 0.01f)
	{
		return 0.01f;
	}
	else if (Num < 0.1f)
	{
		return 0.1f;
	}
	else if (Num < 1.0f)
	{
		return 1.0f;
	}
	else if (Num < 3.0f)
	{
		return 3.0f;
	}
	else if (Num < 17.0f)
	{
		return 17.0f;
	}
	else if (Num < 34.0f)
	{
		return 34.0f;
	}
	else if (Num < 100.0f)
	{
		return 100.0f;
	}
	else
	{
		return 1000.0f;
	}
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

void Profiler::Tick()
{
	if (GlobalProfilerEnabled)
	{
		Clock.Tick();

		CurrentFps++;
		if (Clock.GetTotalTime().AsSeconds() > 1.0f)
		{
			Fps = CurrentFps;
			CurrentFps = 0;

			Clock.Reset();
		}

		const Double Delta = Clock.GetDeltaTime().AsMilliSeconds();
		FrameTime.AddSample(Float(Delta));

		DebugUI::DrawUI([]()
		{
			GlobalProfiler.DrawUI();
		});
	}
}

void Profiler::AddSample(const Char* Name, Float NewSample)
{
	if (GlobalProfilerEnabled)
	{
		const std::string ScopeName = Name;
	
		auto Entry = Samples.find(ScopeName);
		if (Entry != Samples.end())
		{
			Entry->second.AddSample(NewSample);
		}
		else
		{
			Samples.insert(std::make_pair(ScopeName, Sample(NewSample)));
		}
	}
}

void Profiler::DrawUI()
{
	if (GlobalDrawProfiler)
	{
		// Draw DebugWindow with DebugStrings
		UInt32 WindowWidth	= GlobalMainWindow->GetWidth();
		UInt32 WindowHeight	= GlobalMainWindow->GetHeight();
		const Float Width	= 300.0f;
		const Float Height	= WindowHeight * 0.8f;

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
		ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
		ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);

		ImGui::SetNextWindowPos(
			ImVec2(Float(WindowWidth), Float(WindowHeight) * 0.15f),
			ImGuiCond_Always,
			ImVec2(1.0f, 0.0f));

		ImGui::SetNextWindowSizeConstraints(
			ImVec2(Width, Height),
			ImVec2(Float(WindowWidth) * 0.3f, Height));

		ImGui::Begin(
			"Profile Window", 
			nullptr,
			ImGuiWindowFlags_NoTitleBar		|
			ImGuiWindowFlags_NoScrollbar	|
			ImGuiWindowFlags_NoSavedSettings);

		ImGui::Text("CPU Timings:");
		ImGui::Separator();

		ImGui::Columns(2);

		ImGui::Text("FPS:");
		ImGui::NextColumn();

		ImGui::Text("%d", Fps);
		ImGui::NextColumn();

		ImGui::Text("FrameTime:");
		ImGui::NextColumn();
		
		const Float FtAvg = FrameTime.GetAverage();
		ImGui::Text("%.4f ms", FtAvg);
		ImGui::PlotLines(
			"",
			FrameTime.Samples.Data(),
			FrameTime.SampleCount,
			FrameTime.CurrentSample,
			nullptr,
			0.0f,
			ImGui_GetMaxLimit(FtAvg),
			ImVec2(0, 30.0f));

		ImGui::Columns(1);

		ImGui::Separator();

		ImGui::Columns(2);

		TStaticArray<Float, 50> Floats;
		for (auto& Sample : Samples)
		{
			Memory::Memzero(Floats.Data(), Floats.SizeInBytes());
			
			Float Average = Sample.second.GetAverage();
			if (Average < MICROSECONDS)
			{
				for (Int32 n = 0; n < Sample.second.SampleCount; n++)
				{
					Floats[n] = Sample.second.Samples[n];
				}
			}
			else if (Average < MICROSECONDS)
			{
				for (Int32 n = 0; n < Sample.second.SampleCount; n++)
				{
					Floats[n] = Sample.second.Samples[n] * INV_MICROSECONDS;
				}
			}
			else if (Average < SECONDS)
			{
				for (Int32 n = 0; n < Sample.second.SampleCount; n++)
				{
					Floats[n] = Sample.second.Samples[n] * INV_MILLISECONDS;
				}
			}
			else
			{
				for (Int32 n = 0; n < Sample.second.SampleCount; n++)
				{
					Floats[n] = Sample.second.Samples[n] * INV_SECONDS;
				}
			}

			const Char* Name = Sample.first.c_str();
			ImGui_PrintTiming(Name, Average);

			if (Sample.second.SampleCount > 1)
			{
				const Float Avg = ImGui_ConvertNumber(Average);
				const Float Max = ImGui_GetMaxLimit(Avg);
				ImGui::PlotLines(
					"",
					Floats.Data(),
					Sample.second.SampleCount,
					Sample.second.CurrentSample,
					nullptr,
					0.0f,
					Max,
					ImVec2(0, 30.0f));
			}
			else
			{
				ImGui::NewLine();
			}

			ImGui::NextColumn();
		}

		ImGui::Columns(1);

		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::End();
	}
}
