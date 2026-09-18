#include "Application/Docking/DockLayoutFile.h"
#include "Core/Misc/IniFile.h"
#include "Core/Templates/CString.h"

// The section every saved layout starts with, and the prefixes the node and window sections are named by
constexpr const CHAR* LAYOUT_SECTION       = "Layout";
constexpr const CHAR* LAYOUT_NODE_PREFIX   = "Node";
constexpr const CHAR* LAYOUT_WINDOW_PREFIX = "Window";

static void SplitList(const String& Value, TArray<String>& OutTokens)
{
    String Token;
    for (int32 Index = 0; Index < Value.Length(); ++Index)
    {
        const CHAR Character = Value[Index];
        if (Character == ',')
        {
            if (!Token.IsEmpty())
            {
                OutTokens.Add(Token);
            }

            Token.Clear();
            continue;
        }

        Token.Append(Character);
    }

    if (!Token.IsEmpty())
    {
        OutTokens.Add(Token);
    }
}

static String JoinList(const TArray<String>& Tokens)
{
    String Result;
    for (int32 Index = 0; Index < Tokens.Size(); ++Index)
    {
        if (Index > 0)
        {
            Result.Append(',');
        }

        Result.Append(Tokens[Index]);
    }

    return Result;
}

static String WritePoint(const IntVector2& Point)
{
    return String::Printf("%d,%d", Point.X, Point.Y);
}

static IntVector2 ReadPoint(const String& Value)
{
    TArray<String> Tokens;
    SplitList(Value, Tokens);

    if (Tokens.Size() != 2)
    {
        return IntVector2(0, 0);
    }

    return IntVector2(CString::Atoi(Tokens[0].Data()), CString::Atoi(Tokens[1].Data()));
}

static int32 WriteNode(FIniFile& File, const FDockNode& Node, int32& InOutNextIndex)
{
    const int32 NodeIndex = InOutNextIndex++;

    TArray<String> ChildIndices;
    if (Node.Kind == EDockNodeKind::Split)
    {
        for (const FDockNode& Child : Node.Children)
        {
            ChildIndices.Add(String::Printf("%d", WriteNode(File, Child, InOutNextIndex)));
        }
    }

    const String SectionName = String::Printf("%s%d", LAYOUT_NODE_PREFIX, NodeIndex);
    if (Node.Kind == EDockNodeKind::Split)
    {
        TArray<String> Fractions;
        for (float Fraction : Node.ChildFractions)
        {
            Fractions.Add(String::Printf("%.6f", Fraction));
        }

        File.SetOrAddString(SectionName.Data(), "Kind", String("Split"));
        File.SetOrAddString(SectionName.Data(), "Orientation", String(Node.Orientation == EDockSplitOrientation::Horizontal ? "Horizontal" : "Vertical"));
        File.SetOrAddString(SectionName.Data(), "Children", JoinList(ChildIndices));
        File.SetOrAddString(SectionName.Data(), "Fractions", JoinList(Fractions));
    }
    else
    {
        File.SetOrAddString(SectionName.Data(), "Kind", String("Tabs"));
        File.SetOrAddString(SectionName.Data(), "Tabs", JoinList(Node.TabIds));
        File.SetOrAddInt(SectionName.Data(), "ActiveTab", Node.ActiveTabIndex);
    }

    return NodeIndex;
}

static bool ReadNode(FIniFile& File, int32 NodeIndex, int32 NumNodes, int32 Depth, FDockNode& OutNode)
{
    if (NodeIndex < 0 || NodeIndex >= NumNodes || Depth > NumNodes)
    {
        return false;
    }

    const String SectionName = String::Printf("%s%d", LAYOUT_NODE_PREFIX, NodeIndex);

    String Kind;
    if (!File.GetString(SectionName.Data(), "Kind", Kind))
    {
        return false;
    }

    if (Kind == "Tabs")
    {
        OutNode.Kind = EDockNodeKind::Tabs;

        String Tabs;
        File.GetString(SectionName.Data(), "Tabs", Tabs);
        SplitList(Tabs, OutNode.TabIds);

        int32 ActiveTab = 0;
        File.GetInt(SectionName.Data(), "ActiveTab", ActiveTab);

        OutNode.ActiveTabIndex = ActiveTab;
        return true;
    }

    OutNode.Kind = EDockNodeKind::Split;

    String Orientation;
    File.GetString(SectionName.Data(), "Orientation", Orientation);

    OutNode.Orientation = Orientation == "Vertical" ? EDockSplitOrientation::Vertical : EDockSplitOrientation::Horizontal;

    String Children;
    File.GetString(SectionName.Data(), "Children", Children);

    TArray<String> ChildIndices;
    SplitList(Children, ChildIndices);

    for (const String& ChildIndex : ChildIndices)
    {
        FDockNode ChildNode;
        if (!ReadNode(File, CString::Atoi(ChildIndex.Data()), NumNodes, Depth + 1, ChildNode))
        {
            return false;
        }

        OutNode.Children.Add(ChildNode);
    }

    String Fractions;
    File.GetString(SectionName.Data(), "Fractions", Fractions);

    TArray<String> FractionTokens;
    SplitList(Fractions, FractionTokens);

    if (FractionTokens.Size() == OutNode.Children.Size())
    {
        for (const String& Token : FractionTokens)
        {
            OutNode.ChildFractions.Add(CString::Atof(Token.Data()));
        }
    }
    else
    {
        OutNode.NormalizeFractions();
    }

    return true;
}

FDockWindowLayout::FDockWindowLayout()
    : Title()
    , Position()
    , Size()
    , bIsMaximized(false)
    , Root()
{
}

bool FDockLayoutFile::Save(const String& Filename, const TArray<FDockWindowLayout>& Windows)
{
    FIniFile File;
    File.Filename = Filename;

    int32 NextIndex = 0;
    for (int32 Index = 0; Index < Windows.Size(); ++Index)
    {
        const FDockWindowLayout& Layout    = Windows[Index];
        const int32              RootIndex = WriteNode(File, Layout.Root, NextIndex);

        const String SectionName = String::Printf("%s%d", LAYOUT_WINDOW_PREFIX, Index);

        File.SetOrAddString(SectionName.Data(), "Title", Layout.Title);
        File.SetOrAddString(SectionName.Data(), "Position", WritePoint(Layout.Position));
        File.SetOrAddString(SectionName.Data(), "Size", WritePoint(Layout.Size));
        File.SetOrAddInt(SectionName.Data(), "Maximized", Layout.bIsMaximized ? 1 : 0);
        File.SetOrAddInt(SectionName.Data(), "Root", RootIndex);
    }

    File.SetOrAddInt(LAYOUT_SECTION, "Version", CurrentVersion);
    File.SetOrAddInt(LAYOUT_SECTION, "NumNodes", NextIndex);
    File.SetOrAddInt(LAYOUT_SECTION, "NumWindows", Windows.Size());
    File.SetOrAddInt(LAYOUT_SECTION, "Root", 0);
    return File.WriteToFile();
}

bool FDockLayoutFile::Load(const String& Filename, TArray<FDockWindowLayout>& OutWindows)
{
    OutWindows.Clear();

    FIniFile File;
    if (!File.LoadFromFile(Filename))
    {
        return false;
    }

    int32 Version = 0;
    if (!File.GetInt(LAYOUT_SECTION, "Version", Version))
    {
        return false;
    }

    if (Version != CurrentVersion && Version != SingleWindowVersion)
    {
        return false;
    }

    int32 NumNodes = 0;
    File.GetInt(LAYOUT_SECTION, "NumNodes", NumNodes);

    int32 NumWindows = 1;
    if (Version == CurrentVersion)
    {
        File.GetInt(LAYOUT_SECTION, "NumWindows", NumWindows);
    }

    for (int32 Index = 0; Index < NumWindows; ++Index)
    {
        FDockWindowLayout Layout;

        int32 RootIndex = 0;
        if (Version == CurrentVersion)
        {
            const String SectionName = String::Printf("%s%d", LAYOUT_WINDOW_PREFIX, Index);

            String Value;
            if (File.GetString(SectionName.Data(), "Title", Value))
            {
                Layout.Title = Value;
            }

            if (File.GetString(SectionName.Data(), "Position", Value))
            {
                Layout.Position = ReadPoint(Value);
            }

            if (File.GetString(SectionName.Data(), "Size", Value))
            {
                Layout.Size = ReadPoint(Value);
            }

            int32 MaximizedValue = 0;
            if (File.GetInt(SectionName.Data(), "Maximized", MaximizedValue))
            {
                Layout.bIsMaximized = MaximizedValue != 0;
            }

            File.GetInt(SectionName.Data(), "Root", RootIndex);
        }
        else
        {
            File.GetInt(LAYOUT_SECTION, "Root", RootIndex);
        }

        if (!ReadNode(File, RootIndex, NumNodes, 0, Layout.Root))
        {
            if (Index == 0)
            {
                return false;
            }

            continue;
        }

        OutWindows.Add(Layout);
    }

    return !OutWindows.IsEmpty();
}
