// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModuleDescriptor.h"
#include "CustomBuildSteps.h"
#include "LocalizationDescriptor.h"

/**
 * Descriptor for a plugin reference. Contains the information required to enable or disable a plugin for a given platform.
 */
struct PROJECTS_API FPluginReferenceDescriptor
{
	/** Name of the plugin */
	FString Name;

	/** Whether it should be enabled by default */
	bool bEnabled;

	/** Whether this plugin is optional, and the game should silently ignore it not being present */
	bool bOptional;
	
	/** Description of the plugin for users that do not have it installed. */
	FString Description;

	/** URL for this plugin on the marketplace, if the user doesn't have it installed. */
	FString MarketplaceURL;

	/** If enabled, list of platforms for which the plugin should be enabled (or all platforms if blank). */
	TArray<FString> PlatformAllowList;

	/** If enabled, list of platforms for which the plugin should be disabled. */
	TArray<FString> PlatformDenyList;
 
	/** If enabled, list of target configurations for which the plugin should be enabled (or all target configurations if blank). */
	TArray<EBuildConfiguration> TargetConfigurationAllowList;

	/** If enabled, list of target configurations for which the plugin should be disabled. */
	TArray<EBuildConfiguration> TargetConfigurationDenyList;

	/** If enabled, list of targets for which the plugin should be enabled (or all targets if blank). */
	TArray<EBuildTargetType> TargetAllowList;

	/** If enabled, list of targets for which the plugin should be disabled. */
	TArray<EBuildTargetType> TargetDenyList;

	/** The list of supported target platforms for this plugin. This field is copied from the plugin descriptor, and supplements the user's allowed/denied platforms. */
	TArray<FString> SupportedTargetPlatforms;

	/** When true, empty SupportedTargetPlatforms and PlatformAllowList are interpreted as 'no platforms' with the expectation that explicit platforms will be added in plugin platform extensions */
	bool bHasExplicitPlatforms;


	/** Constructor */
	FPluginReferenceDescriptor(const FString& InName = TEXT(""), bool bInEnabled = false);

	/** Determines whether the plugin is enabled for the given platform */
	bool IsEnabledForPlatform(const FString& Platform) const;

	/** Determines whether the plugin is enabled for the given target configuration */
	bool IsEnabledForTargetConfiguration(EBuildConfiguration Configuration) const;

	/** Determines whether the plugin is enabled for the given target */
	bool IsEnabledForTarget(EBuildTargetType TargetType) const;

	/** Determines if the referenced plugin is supported for the given platform */
	bool IsSupportedTargetPlatform(const FString& Platform) const;

	/** Reads the descriptor from the given JSON object */
	bool Read(const FJsonObject& Object, FText* OutFailReason = nullptr);

	/** Reads the descriptor from the given JSON object */
	bool Read(const FJsonObject& Object, FText& OutFailReason);

	/** Reads an array of modules from the given JSON object */
	static bool ReadArray(const FJsonObject& Object, const TCHAR* Name, TArray<FPluginReferenceDescriptor>& OutModules, FText* OutFailReason = nullptr);

	/** Reads an array of modules from the given JSON object */
	static bool ReadArray(const FJsonObject& Object, const TCHAR* Name, TArray<FPluginReferenceDescriptor>& OutModules, FText& OutFailReason);

	/** Writes a descriptor to JSON */
	void Write(TJsonWriter<>& Writer) const;
	
	/** Updates the given json object with values in this descriptor */
	void UpdateJson(FJsonObject& JsonObject) const;

	/** Writes an array of plugin references to JSON */
	static void WriteArray(TJsonWriter<>& Writer, const TCHAR* ArrayName, const TArray<FPluginReferenceDescriptor>& Plugins);

	/** Updates an array of plugin references in the specified JSON field (indexed by plugin name) */
	static void UpdateArray(FJsonObject& JsonObject, const TCHAR* ArrayName, const TArray<FPluginReferenceDescriptor>& Plugins);
};
