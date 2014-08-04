// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// Forward declarations
struct FBlueprintActionMenuBuilder;
struct FBlueprintActionContext;

struct FBlueprintActionMenuUtils
{
	/**
	 * A centralized utility function for constructing blueprint palette menus.
	 * Rolls the supplied Context and FilterClass into a filter that's used to
	 * construct the palette.
	 *
	 * @param  Context		Contains the blueprint that the palette is for.
	 * @param  FilterClass	If not null, then this specifies the class whose members we want listed (and nothing else).
	 * @param  MenuOut		The structure that will be populated with palette menu items.
	 */
	KISMET_API static void MakePaletteMenu(FBlueprintActionContext const& Context, UClass* FilterClass, FBlueprintActionMenuBuilder& MenuOut);
	
	/**
	 * A centralized utility function for constructing blueprint context menus.
	 * Rolls the supplied Context and SelectedProperties into a series of 
	 * filters that're used to construct the menu.
	 *
	 * @param  Context				Contains the blueprint/graph/pin that the menu is for.
	 * @param  SelectedProperties	A set of selected properties to offer contextual menu options for.
	 * @param  MenuOut				The structure that will be populated with context menu items.
	 */
	KISMET_API static void MakeContextMenu(FBlueprintActionContext const& Context, bool bIsContextSensitive, FBlueprintActionMenuBuilder& MenuOut);
};

