#pragma once
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/ITextureImporter.h"
#include "Engine/Assets/ImodelImporter.h"
#include "Engine/Assets/ModelCreateInfo.h"
#include "Engine/Assets/VertexFormat.h"

#include "Engine/Assets/AssetImporters/FBXImporter.h"
#include "Engine/Assets/AssetImporters/ModelImporter.h"
#include "Engine/Assets/AssetImporters/OBJImporter.h"
#include "Engine/Assets/AssetImporters/TextureImporterBase.h"
#include "Engine/Assets/AssetImporters/TextureImporterDDS.h"

#include "Engine/Core/ObjectClass.h"
#include "Engine/Core/Object.h"

#include "Engine/Debug/InputDebugInputHandler.h"

#include "Engine/Resources/Material.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Resource.h"
#include "Engine/Resources/Texture.h"

#include "Engine/EngineUI/Editor/EditorConsoleInputFieldWidget.h"
#include "Engine/EngineUI/Editor/EditorDockspaceWidget.h"
#include "Engine/EngineUI/Editor/EditorLogOutputWidget.h"
#include "Engine/EngineUI/Editor/EditorSceneHierarchyWidget.h"
#include "Engine/EngineUI/Editor/EditorViewportWidget.h"
#include "Engine/EngineUI/Runtime/RuntimeConsoleWidget.h"
#include "Engine/EngineUI/FrameProfilerWidget.h"

#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Actors/PlayerController.h"
#include "Engine/World/Actors/PlayerInput.h"

#include "Engine/World/Components/ActorComponent.h"
#include "Engine/World/Components/InputComponent.h"
#include "Engine/World/Components/StaticMeshComponent.h"
#include "Engine/World/Components/SceneComponent.h"
#include "Engine/World/Components/SkyboxComponent.h"

#include "Engine/World/Lights/DirectionalLight.h"
#include "Engine/World/Lights/Light.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Lights/SkyLight.h"
#include "Engine/World/Lights/SpotLight.h"

#include "Engine/World/Reflections/LightProbe.h"

#include "Engine/World/Camera.h"
#include "Engine/World/SceneViewport.h"
#include "Engine/World/World.h"

#include "Engine.h"
#include "EngineModule.h"
