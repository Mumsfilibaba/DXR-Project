#include "Profiler.h"
#include "Console.h"

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
* Console Vars
*/

DECL_CONSOLE_VARIABLE(DrawProfiler);
DECL_CONSOLE_VARIABLE(DrawFps);

/*
* Profiler
*/

Profiler::Profiler()
	: Clock()
	, FrameTime(0)
	, Samples()
{
}

void Profiler::Init()
{
	INIT_CONSOLE_VARIABLE(DrawFps, ConsoleVariableType_Bool);
	DrawFps->SetBool(false);

	INIT_CONSOLE_VARIABLE(DrawProfiler, ConsoleVariableType_Bool);
	DrawProfiler->SetBool(false);
}

void Profiler::Tick()
{
	Clock.Tick();

	CurrentFps++;
	if (Clock.GetTotalTime().AsSeconds() > 1.0f)
	{
		Fps = CurrentFps;
		CurrentFps = 0;

		Clock.Reset();
	}

	if (DrawFps->GetBool())
	{
		DebugUI::DrawUI([]()
		{
			const UInt32 WindowWidth = GlobalMainWindow->GetWidth();

			ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(5.0f, 5.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));
			
			ImGui::SetNextWindowPos(
				ImVec2(Float(WindowWidth), 0.0f),
				ImGuiCond_Always,
				ImVec2(1.0f, 0.0f));

			ImGui::Begin(
				"FPS Window", 
				nullptr,
				ImGuiWindowFlags_NoDecoration		|
				ImGuiWindowFlags_NoInputs			|
				ImGuiWindowFlags_AlwaysAutoResize	| 
				ImGuiWindowFlags_NoSavedSettings);

			const std::string FpsStr = std::to_string(GlobalProfiler.Fps);
			ImGui::Text("%s", FpsStr.c_str());
			
			ImGui::End();

			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();
		});
	}

	if (DrawProfiler->GetBool())
	{
		const Double Delta = Clock.GetDeltaTime().AsMilliSeconds();
		FrameTime.AddSample(Float(Delta));

		DebugUI::DrawUI([]()
		{
			// Draw DebugWindow with DebugStrings
			const UInt32 WindowWidth	= GlobalMainWindow->GetWidth();
			const UInt32 WindowHeight	= GlobalMainWindow->GetHeight();
			const Float Width			= Math::Max(WindowWidth * 0.6f, 400.0f);
			const Float Height			= WindowHeight * 0.75f;

			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
			ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
			ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);

			ImGui::SetNextWindowPos(
				ImVec2(Float(WindowWidth) * 0.5f, Float(WindowHeight) * 0.175f),
				ImGuiCond_Appearing,
				ImVec2(0.5f, 0.0f));

			ImGui::SetNextWindowSize(
				ImVec2(Width, Height),
				ImGuiCond_Appearing);

			const ImGuiWindowFlags Flags =
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoSavedSettings;

			Bool TempDrawProfiler = DrawProfiler->GetBool();
			if (ImGui::Begin(
				"Profiler",
				&TempDrawProfiler,
				Flags))
			{
				ImGui::Text("CPU Timings:");
				ImGui::Separator();

				ImGui::Columns(2);

				ImGui::Text("FPS:");
				ImGui::NextColumn();

				ImGui::Text("%d", GlobalProfiler.Fps);
				ImGui::NextColumn();

				ImGui::Text("FrameTime:");
				ImGui::NextColumn();

				const Float FtAvg = GlobalProfiler.FrameTime.GetAverage();
				ImGui::Text("%.4f ms", FtAvg);
				ImGui::PlotLines(
					"",
					GlobalProfiler.FrameTime.Samples.Data(),
					GlobalProfiler.FrameTime.SampleCount,
					GlobalProfiler.FrameTime.CurrentSample,
					nullptr,
					0.0f,
					ImGui_GetMaxLimit(FtAvg),
					ImVec2(0, 30.0f));

				ImGui::Columns(1);

				ImGui::Separator();

				ImGui::Columns(2);

				TStaticArray<Float, NUM_PROFILER_SAMPLES> Floats;
				for (auto& Sample : GlobalProfiler.Samples)
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
			}

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();

			ImGui::End();

			DrawProfiler->SetBool(TempDrawProfiler);
		});
	}
}

void Profiler::AddSample(const Char* Name, Float NewSample)
{
	if (DrawProfiler->GetBool())
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
