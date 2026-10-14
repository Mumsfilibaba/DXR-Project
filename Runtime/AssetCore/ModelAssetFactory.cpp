#include "Core/Containers/StringConversion.h"
#include "Core/Filesystem/File.h"
#include "Core/Json/Json.h"
#include "Core/Platform/PlatformFile.h"
#include "AssetCore/ModelImporters/FBXImporter.h"
#include "AssetCore/ModelImporters/OBJImporter.h"
#include "AssetCore/MaterialAsset.h"
#include "AssetCore/MeshAsset.h"
#include "AssetCore/ModelAssetFactory.h"

static constexpr const CHAR_T* MeshFolder     = TEXT("Meshes/");
static constexpr const CHAR_T* MaterialFolder = TEXT("Materials/");
static constexpr const CHAR_T* TextureFolder  = TEXT("Textures/");

static constexpr EMaterialFlags ImportedMaterialFlags = EMaterialFlags::EnableHeight | EMaterialFlags::EnableAlpha | EMaterialFlags::EnableNormalMapping | EMaterialFlags::DoubleSided | EMaterialFlags::ForceForwardPass;

class FAssetNames
{
public:
    FString Add(const CHAR_T* Folder, const FString& Name)
    {
        const FString Base      = FString(Folder) + FModelAssetFactory::MakeFileName(Name);
        FString       Candidate = Base;

        for (int32 Suffix = 2; Used.Contains(Candidate.ToLower()); ++Suffix)
        {
            Candidate = FString::Printf(TEXT("%s_%d"), *Base, Suffix);
        }

        Used.Add(Candidate.ToLower());
        return Candidate;
    }

private:
    TArray<FString> Used;
};

static bool ReadBool(const FJsonValue& Object, const CHAR_T* Name, bool& OutValue, FString& OutError)
{
    const FJsonValue* Value = Object.IsObject() ? Object.Find(Name) : nullptr;
    if (Value && !Value->TryGetBool(OutValue))
    {
        OutError = FString::Printf(TEXT("%s isn't true or false"), Name);
        return false;
    }

    return true;
}

static bool GetTextureUsage(const FImportMaterial& Material, EMaterialTexture::Type SourceTexture, ETextureUsage& OutUsage)
{
    switch (SourceTexture)
    {
        case EMaterialTexture::Diffuse:  OutUsage = ETextureUsage::Color;  return true;
        case EMaterialTexture::Normal:   OutUsage = ETextureUsage::Normal; return true;
        case EMaterialTexture::Emissive: return false;
        default: break;
    }

    bool bRouted  = false;
    bool bOnlyRed = true;

    for (const FMaterialSourceRoute& Route : Material.Routes)
    {
        if (Route.IsRouted() && Route.Texture == SourceTexture)
        {
            bRouted  = true;
            bOnlyRed = bOnlyRed && Route.Channel == ETextureChannel::R;
        }
    }

    OutUsage = bOnlyRed ? ETextureUsage::Single : ETextureUsage::Mask;
    return bRouted;
}

static TArray<uint8> ToUtf8(const FString& Text)
{
    const TStringConversion<ANSICHAR, CHAR_T> Utf8 = StringCast<ANSICHAR>(Text);

    TArray<uint8> Bytes;
    Bytes.Reset(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
    return Bytes;
}

bool FModelAssetFactory::ReadSettings(const FJsonValue& Value, FModelImportSettings& OutSettings, FString& OutError)
{
    if (!Value.IsNull() && !Value.IsObject())
    {
        OutError = FString(TEXT("they aren't a JSON object"));
        return false;
    }

    FModelImportSettings Settings;

    if (const FJsonValue* Mode = Value.IsObject() ? Value.Find(TEXT("Mode")) : nullptr)
    {
        FString ModeName;
        if (!Mode->TryGetString(ModeName) || !(ModeName.Equals(TEXT("Separate"), EStringCaseType::NoCase) || ModeName.Equals(TEXT("Combine"), EStringCaseType::NoCase)))
        {
            OutError = FString(TEXT("Mode isn't Separate or Combine"));
            return false;
        }

        Settings.Mode = ModeName.Equals(TEXT("Combine"), EStringCaseType::NoCase) ? EModelImportMode::Combine : EModelImportMode::Separate;
    }

    if (const FJsonValue* Nodes = Value.IsObject() ? Value.Find(TEXT("Nodes")) : nullptr)
    {
        if (!Nodes->IsArray())
        {
            OutError = FString(TEXT("Nodes isn't an array of node paths"));
            return false;
        }

        for (int32 Index = 0; Index < Nodes->Num(); ++Index)
        {
            FString Path;
            if (!(*Nodes)[Index].TryGetString(Path))
            {
                OutError = FString::Printf(TEXT("Nodes[%d] isn't a node path"), Index);
                return false;
            }

            Settings.Nodes.Add(Path);
        }
    }

    if (!ReadBool(Value, TEXT("ForceLeftHanded"), Settings.bForceLeftHanded, OutError) ||
        !ReadBool(Value, TEXT("InvertAxisX"), Settings.bInvertAxisX, OutError) ||
        !ReadBool(Value, TEXT("ApplyScaleFactor"), Settings.bApplyScaleFactor, OutError) ||
        !ReadBool(Value, TEXT("RecalculateTangents"), Settings.bRecalculateTangents, OutError) ||
        !ReadBool(Value, TEXT("CompressTextures"), Settings.bCompressTextures, OutError))
    {
        return false;
    }

    if (const FJsonValue* MaxResolution = Value.IsObject() ? Value.Find(TEXT("MaxTextureResolution")) : nullptr)
    {
        int64 Resolution = 0;
        if (!MaxResolution->TryGetInt64(Resolution) || Resolution < 0 || Resolution > 32768)
        {
            OutError = FString(TEXT("MaxTextureResolution isn't a size from 1 to 32768, or 0 for each source's size"));
            return false;
        }

        Settings.MaxTextureResolution = static_cast<int32>(Resolution);
    }

    OutSettings = ::Move(Settings);
    return true;
}

FJsonValue FModelAssetFactory::WriteSettings(const FModelImportSettings& Settings)
{
    FJsonValue Nodes = FJsonValue::MakeArray();
    for (const FString& Path : Settings.Nodes)
    {
        Nodes.Add(FJsonValue(Path));
    }

    FJsonValue Value = FJsonValue::MakeObject();
    Value.AddMember(TEXT("Mode"), FJsonValue(Settings.Mode == EModelImportMode::Combine ? TEXT("Combine") : TEXT("Separate")));
    Value.AddMember(TEXT("Nodes"), ::Move(Nodes));
    Value.AddMember(TEXT("ForceLeftHanded"), FJsonValue(Settings.bForceLeftHanded));
    Value.AddMember(TEXT("InvertAxisX"), FJsonValue(Settings.bInvertAxisX));
    Value.AddMember(TEXT("ApplyScaleFactor"), FJsonValue(Settings.bApplyScaleFactor));
    Value.AddMember(TEXT("RecalculateTangents"), FJsonValue(Settings.bRecalculateTangents));
    Value.AddMember(TEXT("CompressTextures"), FJsonValue(Settings.bCompressTextures));
    Value.AddMember(TEXT("MaxTextureResolution"), FJsonValue(Settings.MaxTextureResolution));
    return Value;
}

FString FModelAssetFactory::GetNodePath(const FImportScene& Scene, int32 NodeIndex)
{
    FString Path = Scene.Nodes[NodeIndex].Name;
    for (int32 Parent = Scene.Nodes[NodeIndex].Parent; Parent >= 0; Parent = Scene.Nodes[Parent].Parent)
    {
        Path = Scene.Nodes[Parent].Name + TEXT("/") + Path;
    }

    return Path;
}

FString FModelAssetFactory::MakeFileName(const FString& Name)
{
    FString Result = Name;
    for (int32 Index = 0; Index < Result.Length(); ++Index)
    {
        const CHAR_T Char = Result[Index];
        if (Char < 32 || Char == '\\' || Char == '/' || Char == ':' || Char == '*' || Char == '?' || Char == '"' || Char == '<' || Char == '>' || Char == '|')
        {
            Result[Index] = '_';
        }
    }

    while (!Result.IsEmpty() && (Result[Result.Length() - 1] == ' ' || Result[Result.Length() - 1] == '.'))
    {
        Result = Result.SubString(0, Result.Length() - 1);
    }

    return Result.IsEmpty() ? FString(TEXT("Unnamed")) : Result;
}

FModelAssetFactory::FModelAssetFactory(const TSharedPtr<FTextureAssetFactory>& InTextureFactory)
    : TextureFactory(InTextureFactory)
    , Importers()
{
    Importers.Add(MakeSharedPtr<FOBJImporter>());
    Importers.Add(MakeSharedPtr<FFBXImporter>());
}

FModelAssetFactory::~FModelAssetFactory() = default;

const CHAR_T* FModelAssetFactory::GetName() const
{
    return TEXT("Model");
}

TArray<FString> FModelAssetFactory::GetExtensions() const
{
    return { FString(TEXT(".obj")), FString(TEXT(".fbx")) };
}

TArray<EAssetType> FModelAssetFactory::GetAssetTypes() const
{
    return { EAssetType::Mesh, EAssetType::Material, EAssetType::Texture };
}

void FModelAssetFactory::GetCompanionFiles(const FString& SourcePath, TArray<FString>& OutFiles) const
{
    IModelImporter* Importer = FindImporter(SourcePath);
    if (!Importer)
    {
        return;
    }

    if (File::ExtractExtension(SourcePath).Equals(TEXT(".obj"), EStringCaseType::NoCase))
    {
        TArray<CHAR_T>          Text;
        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(SourcePath);

        if (FileHandle && File::ReadTextFile(FileHandle.Get(), Text))
        {
            const FString Contents(Text.Data());

            int32 LineStart = 0;
            while (LineStart < Contents.Length())
            {
                int32 LineEnd = Contents.FindChar('\n', LineStart);
                if (LineEnd == FString::InvalidIndex)
                {
                    LineEnd = Contents.Length();
                }

                const FString Line = Contents.SubString(LineStart, LineEnd - LineStart).Trim();
                if (Line.StartsWith(TEXT("mtllib ")))
                {
                    const FString MaterialFile = File::CombinePath(File::GetDirectoryOf(SourcePath), Line.SubString(7, Line.Length() - 7).Trim());
                    if (FPlatformFile::IsFile(*MaterialFile))
                    {
                        OutFiles.AddUnique(MaterialFile);
                    }
                }

                LineStart = LineEnd + 1;
            }
        }
    }

    FImportScene Scene;
    if (Importer->ImportFromFile(FStringView(SourcePath), EMeshImportFlags::Default, Scene))
    {
        for (const FImportTexture& Texture : Scene.Textures)
        {
            if (FPlatformFile::IsFile(*Texture.SourcePath))
            {
                OutFiles.AddUnique(Texture.SourcePath);
            }
        }
    }
}

bool FModelAssetFactory::Import(FAssetImportContext& Context, TArray<FImportedAsset>& OutAssets, FJsonValue& OutSettings, FString& OutError)
{
    FModelImportSettings Settings;
    if (!ReadSettings(Context.GetSettings(), Settings, OutError))
    {
        OutError = FString::Printf(TEXT("the settings aren't valid: %s"), *OutError);
        return false;
    }

    IModelImporter* Importer = FindImporter(Context.GetSourcePath());
    if (!Importer)
    {
        OutError = FString(TEXT("no model importer reads it"));
        return false;
    }

    EMeshImportFlags Flags = EMeshImportFlags::None;
    if (Settings.bForceLeftHanded)
    {
        Flags |= EMeshImportFlags::ForceLeftHanded;
    }

    if (Settings.bInvertAxisX)
    {
        Flags |= EMeshImportFlags::InvertAxisX;
    }

    if (Settings.bApplyScaleFactor)
    {
        Flags |= EMeshImportFlags::ApplyScaleFactor;
    }

    if (Settings.bRecalculateTangents)
    {
        Flags |= EMeshImportFlags::RecalculateTangents;
    }

    FImportScene Scene;
    if (!Importer->ImportFromFile(FStringView(Context.GetSourcePath()), Flags, Scene))
    {
        OutError = FString(TEXT("it couldn't be read"));
        return false;
    }

    TArray<bool> SelectedNodes;
    SelectedNodes.Reset(Scene.Nodes.Size(), Settings.Nodes.IsEmpty());

    if (!Settings.Nodes.IsEmpty())
    {
        TArray<FString> NodePaths;
        for (int32 NodeIndex = 0; NodeIndex < Scene.Nodes.Size(); ++NodeIndex)
        {
            NodePaths.Add(GetNodePath(Scene, NodeIndex));
        }

        for (const FString& Path : Settings.Nodes)
        {
            if (!NodePaths.Contains(Path))
            {
                OutError = FString::Printf(TEXT("Nodes lists '%s', which isn't a node in it"), *Path);
                return false;
            }
        }

        for (int32 NodeIndex = 0; NodeIndex < Scene.Nodes.Size(); ++NodeIndex)
        {
            const int32 Parent = Scene.Nodes[NodeIndex].Parent;
            SelectedNodes[NodeIndex] = Settings.Nodes.Contains(NodePaths[NodeIndex]) || (Parent >= 0 && SelectedNodes[Parent]);
        }
    }

    TArray<int32> MeshOrder;
    TArray<int32> FirstNodeOfMesh;
    TArray<bool>  UsedMaterials;

    FirstNodeOfMesh.Reset(Scene.Meshes.Size(), -1);
    UsedMaterials.Reset(Scene.Materials.Size(), false);

    for (int32 NodeIndex = 0; NodeIndex < Scene.Nodes.Size(); ++NodeIndex)
    {
        const FImportNode& Node = Scene.Nodes[NodeIndex];
        if (!SelectedNodes[NodeIndex] || Node.Mesh < 0)
        {
            continue;
        }

        if (FirstNodeOfMesh[Node.Mesh] < 0)
        {
            FirstNodeOfMesh[Node.Mesh] = NodeIndex;
            MeshOrder.Add(Node.Mesh);
        }

        for (int32 MaterialIndex : Node.Materials)
        {
            if (MaterialIndex >= 0)
            {
                UsedMaterials[MaterialIndex] = true;
            }
        }
    }

    if (MeshOrder.IsEmpty())
    {
        OutError = FString(TEXT("the nodes to import place no meshes"));
        return false;
    }

    FAssetNames Names;

    TArray<int32> TextureUsages;
    TextureUsages.Reset(Scene.Textures.Size(), -1);

    for (int32 MaterialIndex = 0; MaterialIndex < Scene.Materials.Size(); ++MaterialIndex)
    {
        if (!UsedMaterials[MaterialIndex])
        {
            continue;
        }

        const FImportMaterial& Material = Scene.Materials[MaterialIndex];
        for (int32 SourceTexture = 0; SourceTexture < EMaterialTexture::Count; ++SourceTexture)
        {
            const int32   TextureIndex = Material.Textures[SourceTexture];
            ETextureUsage Usage        = ETextureUsage::Color;

            if (TextureIndex < 0 || !GetTextureUsage(Material, EMaterialTexture::Type(SourceTexture), Usage))
            {
                continue;
            }

            // A texture one material reads one channel of and another reads more of keeps them all
            if (TextureUsages[TextureIndex] < 0 || 
                (TextureUsages[TextureIndex] == int32(ETextureUsage::Single) && Usage == ETextureUsage::Mask))
            {
                TextureUsages[TextureIndex] = int32(Usage);
            }
        }
    }

    TArray<int32>            JobTextures;
    TArray<FTextureBuildJob> Jobs;

    for (int32 TextureIndex = 0; TextureIndex < Scene.Textures.Size(); ++TextureIndex)
    {
        if (TextureUsages[TextureIndex] < 0)
        {
            continue;
        }

        FTextureImportSettings TextureSettings;
        TextureSettings.Usage         = ETextureUsage(TextureUsages[TextureIndex]);
        TextureSettings.bSRGB         = TextureSettings.Usage == ETextureUsage::Color;
        TextureSettings.bCompress     = Settings.bCompressTextures;
        TextureSettings.MaxResolution = Settings.MaxTextureResolution;

        FTextureBuildJob& Job = Jobs.Emplace();
        Job.SourcePath = Scene.Textures[TextureIndex].SourcePath;
        Job.Settings   = FTextureAssetFactory::WriteSettings(TextureSettings);
        JobTextures.Add(TextureIndex);
    }

    TextureFactory->BuildTextures(Jobs);

    TArray<FAssetRef> TextureRefs;
    TextureRefs.Resize(Scene.Textures.Size());

    for (int32 JobIndex = 0; JobIndex < Jobs.Size(); ++JobIndex)
    {
        FTextureBuildJob& Job = Jobs[JobIndex];
        if (!Job.Error.IsEmpty())
        {
            Context.AddWarning(FString::Printf(TEXT("A texture is left out: %s"), *Job.Error));
            continue;
        }

        FImportedAsset& Asset = OutAssets.Emplace();
        Asset.Name      = Names.Add(TextureFolder, File::ExtractFilenameWithoutExtension(Job.SourcePath));
        Asset.Type      = EAssetType::Texture;
        Asset.Guid      = Context.GetGuid(Asset.Name, Asset.Type);
        Asset.Data      = ::Move(Job.Data);
        Asset.Thumbnail = ::Move(Job.Thumbnail);

        TextureRefs[JobTextures[JobIndex]] = FAssetRef(Asset.Guid, Context.GetAssetPath(Asset.Name, Asset.Type));
    }

    TArray<FAssetRef> MaterialRefs;
    MaterialRefs.Resize(Scene.Materials.Size());

    for (int32 MaterialIndex = 0; MaterialIndex < Scene.Materials.Size(); ++MaterialIndex)
    {
        if (!UsedMaterials[MaterialIndex])
        {
            continue;
        }

        const FImportMaterial& Source = Scene.Materials[MaterialIndex];

        FMaterialAssetData Material;
        Material.Info.Albedo           = FFloatColor(Source.Diffuse.X, Source.Diffuse.Y, Source.Diffuse.Z, 1.0f);
        Material.Info.AmbientOcclusion = Source.AmbientFactor;
        Material.Info.Metallic         = Source.Metallic;
        Material.Info.Roughness        = Source.Roughness;
        Material.Info.MaterialFlags    = Source.MaterialFlags & ImportedMaterialFlags;

        EMaterialTextureSlot::Type SourceSlots[EMaterialTexture::Count];
        EMaterialTextureSlot::Type NextMaskSlot = EMaterialTextureSlot::MaskA;

        for (int32 SourceTexture = 0; SourceTexture < EMaterialTexture::Count; ++SourceTexture)
        {
            SourceSlots[SourceTexture] = EMaterialTextureSlot::Count;

            const int32 TextureIndex = Source.Textures[SourceTexture];
            if (TextureIndex < 0 || !TextureRefs[TextureIndex].IsSet() || SourceTexture == EMaterialTexture::Emissive)
            {
                continue;
            }

            const FAssetRef& Texture = TextureRefs[TextureIndex];
            if (SourceTexture == EMaterialTexture::Diffuse || SourceTexture == EMaterialTexture::Normal)
            {
                SourceSlots[SourceTexture] = (SourceTexture == EMaterialTexture::Diffuse) ? EMaterialTextureSlot::BaseColor : EMaterialTextureSlot::Normal;
                Material.Textures[SourceSlots[SourceTexture]] = Texture;
                continue;
            }

            for (int32 Slot = EMaterialTextureSlot::MaskA; Slot < NextMaskSlot; ++Slot)
            {
                if (Material.Textures[Slot] == Texture)
                {
                    SourceSlots[SourceTexture] = EMaterialTextureSlot::Type(Slot);
                }
            }

            if (SourceSlots[SourceTexture] == EMaterialTextureSlot::Count)
            {
                if (NextMaskSlot == EMaterialTextureSlot::Count)
                {
                    Context.AddWarning(FString::Printf(TEXT("'%s' uses more mask textures than a material has slots for, so '%s' is left out"), *Source.Name, *Texture.GetPath()));
                    continue;
                }

                SourceSlots[SourceTexture]  = NextMaskSlot;
                Material.Textures[NextMaskSlot] = Texture;
                NextMaskSlot = EMaterialTextureSlot::Type(NextMaskSlot + 1);
            }
        }

        if (Material.Textures[EMaterialTextureSlot::Normal].IsSet())
        {
            Material.Info.MaterialFlags |= EMaterialFlags::EnableNormalMapping;
        }

        for (int32 Scalar = 0; Scalar < EMaterialScalar::Count; ++Scalar)
        {
            const FMaterialSourceRoute&      Route = Source.Routes[Scalar];
            const EMaterialTextureSlot::Type Slot  = Route.IsRouted() ? SourceSlots[Route.Texture] : EMaterialTextureSlot::Count;

            Material.Info.Routes[Scalar] = (Slot < EMaterialTextureSlot::Count) ? FMaterialTextureRoute(Slot, Route.Channel, Route.bInvert) : FMaterialTextureRoute();
        }

        const FMaterialTextureRoute& OpacityRoute = Material.Info.Routes[EMaterialScalar::Opacity];
        if (OpacityRoute.IsRouted() && OpacityRoute.Slot != EMaterialTextureSlot::BaseColor)
        {
            Material.Info.MaterialFlags |= EMaterialFlags::EnableAlpha;
        }

        FImportedAsset& Asset = OutAssets.Emplace();
        Asset.Name    = Names.Add(MaterialFolder, Source.Name);
        Asset.Type    = EAssetType::Material;
        Asset.Guid    = Context.GetGuid(Asset.Name, Asset.Type);
        Material.Guid = Asset.Guid;
        Asset.Data    = ToUtf8(Json::ToString(MaterialAssets::ToJson(Material)));

        MaterialRefs[MaterialIndex] = FAssetRef(Asset.Guid, Context.GetAssetPath(Asset.Name, Asset.Type));
    }

    TArray<FMeshAssetData> Meshes;
    if (Settings.Mode == EModelImportMode::Separate)
    {
        for (int32 MeshIndex : MeshOrder)
        {
            const FImportNode& Node = Scene.Nodes[FirstNodeOfMesh[MeshIndex]];

            FMeshAssetData& Mesh = Meshes.Emplace();
            Mesh.Mesh = Scene.Meshes[MeshIndex];

            for (int32 MaterialIndex : Node.Materials)
            {
                FMeshAssetSlot& Slot = Mesh.MaterialSlots.Emplace();
                Slot.Name     = MaterialIndex >= 0 ? Scene.Materials[MaterialIndex].Name : FString(TEXT("None"));
                Slot.Material = MaterialIndex >= 0 ? MaterialRefs[MaterialIndex] : FAssetRef();
            }

            for (FSubMesh& SubMesh : Mesh.Mesh.SubMeshes)
            {
                if (SubMesh.MaterialIndex >= Mesh.MaterialSlots.Size())
                {
                    SubMesh.MaterialIndex = -1;
                }
            }
        }
    }
    else
    {
        FMeshAssetData& Combined = Meshes.Emplace();
        Combined.Mesh.Name = Context.GetImportName();

        TArray<int32> MaterialSlots;
        MaterialSlots.Reset(Scene.Materials.Size(), -1);

        for (int32 NodeIndex = 0; NodeIndex < Scene.Nodes.Size(); ++NodeIndex)
        {
            if (!SelectedNodes[NodeIndex] || Scene.Nodes[NodeIndex].Mesh < 0)
            {
                continue;
            }

            FMeshData    Placed     = ImportScenes::BakeNodeMesh(Scene, NodeIndex);
            const uint32 BaseVertex = static_cast<uint32>(Combined.Mesh.Vertices.Size());
            const uint32 BaseIndex  = static_cast<uint32>(Combined.Mesh.Indices.Size());

            Combined.Mesh.Vertices.Append(Placed.Vertices);
            for (uint32 Index : Placed.Indices)
            {
                Combined.Mesh.Indices.Add(Index + BaseVertex);
            }

            if (Placed.SubMeshes.IsEmpty())
            {
                FSubMesh& Whole = Placed.SubMeshes.Emplace();
                Whole.VertexCount = static_cast<uint32>(Placed.Vertices.Size());
                Whole.IndexCount  = static_cast<uint32>(Placed.Indices.Size());
            }

            for (FSubMesh SubMesh : Placed.SubMeshes)
            {
                SubMesh.BaseVertex += BaseVertex;
                SubMesh.StartIndex += BaseIndex;

                const int32 MaterialIndex = SubMesh.MaterialIndex;
                if (MaterialIndex >= 0 && MaterialSlots[MaterialIndex] < 0)
                {
                    MaterialSlots[MaterialIndex] = Combined.MaterialSlots.Size();

                    FMeshAssetSlot& Slot = Combined.MaterialSlots.Emplace();
                    Slot.Name     = Scene.Materials[MaterialIndex].Name;
                    Slot.Material = MaterialRefs[MaterialIndex];
                }

                SubMesh.MaterialIndex = MaterialIndex >= 0 ? MaterialSlots[MaterialIndex] : -1;
                Combined.Mesh.SubMeshes.Add(SubMesh);
            }
        }
    }

    for (FMeshAssetData& Mesh : Meshes)
    {
        FImportedAsset& Asset = OutAssets.Emplace();
        Asset.Name         = Names.Add(MeshFolder, Mesh.Mesh.Name);
        Asset.Type         = EAssetType::Mesh;
        Asset.Guid         = Context.GetGuid(Asset.Name, Asset.Type);
        Asset.Dependencies = MeshAssets::GetDependencies(Mesh);

        if (!MeshAssets::Write(Mesh, Asset.Data, OutError))
        {
            return false;
        }
    }

    OutSettings = WriteSettings(Settings);
    return true;
}

IModelImporter* FModelAssetFactory::FindImporter(const FString& SourcePath) const
{
    for (const TSharedPtr<IModelImporter>& Importer : Importers)
    {
        if (Importer->MatchExtenstion(FStringView(SourcePath)))
        {
            return Importer.Get();
        }
    }

    return nullptr;
}
