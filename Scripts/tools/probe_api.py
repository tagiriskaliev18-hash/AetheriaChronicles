# Scripts/tools/probe_api.py — dumps Python API surface + landscape/world facts to Saved/AetheriaAudit/api_dump.txt
import unreal, os
OUT = unreal.Paths.project_saved_dir() + "AetheriaAudit/api_dump.txt"
lines = []
def P(*a): lines.append(" ".join(str(x) for x in a))
names = ["AnimationLibrary","AnimSequence","AnimSequenceBase","AnimMontage","AnimBlueprint","AnimationBlueprintLibrary",
 "SkeletalMesh","SkeletalMeshComponent","SkeletalMeshEditorSubsystem","StaticMesh","StaticMeshEditorSubsystem","StaticMeshDescription",
 "Landscape","LandscapeProxy","LandscapeEditorSubsystem","LandscapeStreamingProxy","LandscapeInfo","LandscapeLayerInfoObject",
 "MaterialEditingLibrary","MaterialInstanceDynamic","MaterialInstanceConstant","Material","MaterialInstanceMaterialParameterCollection",
 "AssetTools","AssetToolsHelpers","EditorAssetLibrary","EditorActorSubsystem","LevelEditorSubsystem","EditorLoadingAndSavingUtils","EditorLevelLibrary","UnrealEditorSubsystem",
 "NiagaraFunctionLibrary","NiagaraComponent","NiagaraSystem","NiagaraEditorUtilitiesLibrary","ParticleSystemComponent",
 "LevelSequence","LevelSequenceEditorBlueprintLibrary","LevelSequenceBindingLibrary","MovieSceneBinding","SequencerTools","MovieSceneTools","MovieSceneSequenceTools","MovieSceneSequenceEditorExtensions","MovieSceneTrackEditorExtensions","MovieSceneSectionExtensions","MovieSceneSpawnable","MovieScenePossessable",
 "SoundBase","AudioComponent","SoundWave","SoundCue","Sound attenuation","SoundAttenuation","AudioDeviceAsset","QuartzSubsystem","AudioGameplayVolume",
 "SystemLibrary","GameplayStatics","BlueprintMapLibrary","NavigationSystemV1","NavigationSystem","NavigationModifier","NavModifierComponent",
 "WorldPartitionBlueprintLibrary","DataLayerEditorSubsystem","DataLayerInstance","WorldDataLayers","HLODBuilderSubsystem",
 "LightComponent","DirectionalLight","SkyLight","SkyAtmosphere","ExponentialHeightFog","VolumetricCloud","PostProcessVolume","VolumetricCloudEngine","SkyAtmosphereEngine",
 "FoliageEditorSubsystem","FoliageType_InstancedStaticMesh","InstancedStaticMeshComponent","HierarchicalInstancedStaticMeshComponent",
 "LandscapeFoliageSpawner","ProceduralFoliageSpawner","ProceduralFoliageVolume","PCGEditorLibrary","PCGSubsystem","PCGGraph","PCGVolume","LandscapeProxy",
 "PhysicsAsset","PhysicsAssetUtils","PhysicsControlAssetEditorSubsystem","Rig","ControlRigBlueprint","IKRigEditorSubsystem","IKRetargeterEditorSubsystem","IKRetargeter",
 "AnimPreview","AnimationLibrary","Skeleton","SkeletonModifier","SkeletalMeshEditingLibrary","AnimationBlueprintLibrary","AnimCompress","AnimBoneCompressionSettings","AnimCurveCompressionSettings",
 "SubobjectDataSubsystem","SubobjectData","ActorFolder","ActorDesc","StaticMeshActor","StaticMeshComponent","BrushComponent","TextRenderActor","DebugCameraController","PlayerStart","CameraActor","CameraComponent",
 "PhysicsSettings","ParticleSystem","DistanceFieldAtlas","SkeletalMeshLODSettings","ScalabilityReference","SystemSettings","RendererSettings","Scalability","GameUserSettings","PlatformUserId","EnhancedInputLibrary","InputMappingContext","PlayerMappableKeySettings","CurveFloat","CurveLinearColor","TimeManagement",
 "Texture2D","TextureEditorSettings","DecalActor","DecalComponent","PointLight","SpotLight","RectLight","LightmassPortal","ReflectionCapture","PlanarReflection","SphereReflectionCapture","LightComponentBase","SkyLightComponent","PointLightComponent","LightPropagationVolume","VolumetricLightmapDensityVolume","PostProcessComponent","Brush","DefaultTextureUsageSettings",
 "GeometryCollection","GeometryCollectionActor","GeometryCollectionComponent","FieldSystem","ChaosCacheManager","GeometryCollectionAsset","GeometryCollectionActor","MaterialInstanceParameterCollection","GeometryCollectionAssetUtils",
 "AnimNode","AnimationAsset","AnimationBaseLibrary","KismetSystemLibrary","KismetMathLibrary","KismetArrayLibrary","KismetStringLibrary","KismetTextLibrary","SlateBlueprintLibrary","WidgetBlueprint","WidgetBlueprintLibrary","UserWidget","Button","VerticalBox","Image","TextBlock","Border","Overlay","CanvasPanel","SizeBox","Spacer","ProgressBar","Slider","Border","WidgetTree","WidgetLayoutLibrary","SlateLibrary","SlateApplicationLibrary",
 "ViewportInteractor","EditorUtilityLibrary","AssetRegistry","AssetRegistryHelpers","AssetRegistryExtension","ObjectRedirector","ObjectTools","UnrealEditorServices","EditorPerformanceTest","RenderTargetExporter","TextureRenderTarget2D","AutomationLibrary","FunctionalTest","AutomationTest","Log","LogPython","RemoteControl","RemoteControlWebInterface","RemoteControlPreset","RemoteControlActor","RemoteControlLogic","RemoteControlTurtle","CameraModifier"]
for n in names:
    o = getattr(unreal, n, None)
    if o is None:
        P(f"## {n}: <MISSING>")
        continue
    P(f"## {n}:")
    try:
        members = [x for x in dir(o) if not x.startswith("_")]
        P("   " + " ".join(members))
    except Exception as e:
        P("   ERR", e)
P("## Factories: " + " ".join(sorted(x for x in dir(unreal) if "Factory" in x)))
P("## Subsystems: " + " ".join(sorted(x for x in dir(unreal) if "Subsystem" in x)))
P("## unreal dirs (sample): " + " ".join(sorted(x for x in dir(unreal) if any(k in x for k in ("Landscape","Sequencer","MovieScene","Heightmap","Voronoi","Noise","Smart","Lens","Audio","Sound","Music","MetaSound","PathFinding","Mass","Zone","Biome","Runtime","Curve")))))
with open(OUT, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
unreal.log(f"[Probe] wrote {OUT} ({len(lines)} lines)")
