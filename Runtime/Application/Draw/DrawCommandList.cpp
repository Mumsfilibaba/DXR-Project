#include "Application/Draw/DrawCommandList.h"

FDrawCommandList::FDrawCommandList()
    : Commands()
    , ClipStack()
    , EmptyClipRectangle()
    , UnmatchedPopCount(0)
{
}

FDrawCommandList::~FDrawCommandList() = default;

void FDrawCommandList::AddBox(int32 LayerId, const FRectangle& Bounds, const FFloatColor& Tint, float CornerRadius)
{
    FDrawCommand& Command = Commands.Emplace();
    Command.Type         = EDrawCommandType::Box;
    Command.Bounds       = Bounds;
    Command.Tint         = Tint;
    Command.LayerId      = LayerId;
    Command.CornerRadius = CornerRadius;
}

void FDrawCommandList::AddText(int32 LayerId, const FRectangle& Bounds, const String& InText, const IFontFace* Font, const FFloatColor& Tint)
{
    FDrawCommand& Command = Commands.Emplace();
    Command.Type    = EDrawCommandType::Text;
    Command.Bounds  = Bounds;
    Command.Tint    = Tint;
    Command.Text    = InText;
    Command.Font    = Font;
    Command.LayerId = LayerId;
}

void FDrawCommandList::AddLine(int32 LayerId, const FRectangle& Bounds, const FFloatColor& Tint)
{
    FDrawCommand& Command = Commands.Emplace();
    Command.Type    = EDrawCommandType::Line;
    Command.Bounds  = Bounds;
    Command.Tint    = Tint;
    Command.LayerId = LayerId;
}

void FDrawCommandList::PushClip(int32 LayerId, const FRectangle& ClipRectangle)
{
    const FRectangle Resolved = ClipStack.IsEmpty() ? ClipRectangle : ClipStack.Last().Intersect(ClipRectangle);
    ClipStack.Add(Resolved);

    FDrawCommand& Command = Commands.Emplace();
    Command.Type    = EDrawCommandType::ClipPush;
    Command.Bounds  = Resolved;
    Command.LayerId = LayerId;
}

void FDrawCommandList::PopClip(int32 LayerId)
{
    if (ClipStack.IsEmpty())
    {
        UnmatchedPopCount++;
    }
    else
    {
        ClipStack.RemoveAt(ClipStack.LastIndex());
    }

    FDrawCommand& Command = Commands.Emplace();
    Command.Type    = EDrawCommandType::ClipPop;
    Command.LayerId = LayerId;
}

void FDrawCommandList::Reset()
{
    Commands.Clear();
    ClipStack.Clear();
    UnmatchedPopCount = 0;
}

int32 FDrawCommandList::CountCommandsOfType(EDrawCommandType Type) const
{
    int32 Count = 0;
    for (const FDrawCommand& Command : Commands)
    {
        if (Command.Type == Type)
        {
            Count++;
        }
    }

    return Count;
}

int32 FDrawCommandList::FindTextCommand(const StringView& InText) const
{
    for (int32 Index = 0; Index < Commands.Size(); Index++)
    {
        const FDrawCommand& Command = Commands[Index];
        if (Command.Type == EDrawCommandType::Text && Command.Text.Equals(InText.Data(), InText.Length()))
        {
            return Index;
        }
    }

    return InvalidIndex;
}
