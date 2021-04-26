#include <Game/Game.h>

/*
* Sandbox
*/

class Sandbox : public Game
{
public:
    Sandbox()   = default;
    ~Sandbox()  = default;

    virtual Bool Init() override;

    virtual void Tick(Timestamp DeltaTime) override;

private:
    void FreeCamMode(Timestamp DeltaTime);
    void TrackMode();

    bool TryLoadTrackFile(const char* Filename);

    XMFLOAT3 CameraSpeed = XMFLOAT3(0.0f, 0.0f, 0.0f);

    TArray<XMFLOAT3> ControlPoints;
    TArray<XMFLOAT4> ControlRotations;

    UInt32 CurrentPoint = 0;

    Float SlerpT = 0.0f;

    Float TotalLength  = 0.0f;

    bool CaptureModeEnabled = false;
    bool Captured = false;
    bool EnablePressed = false;
    bool Saved = false;
    bool Loaded = false;

    bool TestFinished = false;

    char Filename[256];

    TArray<XMFLOAT3> SavedPoints;
    TArray<XMFLOAT4> SavedRotations;

    std::string TestName;

    UInt32 FrameCount = 0;
};