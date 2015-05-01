// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/AssetEditorManager.h"
#include "SPaperEditorViewport.h"

//////////////////////////////////////////////////////////////////////////
// STileSetSelectorViewport

class STileSetSelectorViewport : public SPaperEditorViewport
{
public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTileViewportSelectionChanged, const FIntPoint& /*TopLeft*/, const FIntPoint& /*Dimensions*/);

	SLATE_BEGIN_ARGS(STileSetSelectorViewport) {}
	SLATE_END_ARGS()

	~STileSetSelectorViewport();

	void Construct(const FArguments& InArgs, UPaperTileSet* InTileSet, class FEdModeTileMap* InTileMapEditor);

	void ChangeTileSet(UPaperTileSet* InTileSet);

	FOnTileViewportSelectionChanged& GetTileSelectionChanged()
	{
		return OnTileSelectionChanged;
	}

protected:
	// SWidget interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	// End of SWidget interface

	// SPaperEditorViewport interface
	virtual FText GetTitleText() const override;
	// End of SPaperEditorViewport interface

	// SEditorViewport interface
	virtual void BindCommands() override;
	virtual void OnFocusViewportToSelection() override;
	// End of SEditorViewport interface

private:
	void OnSelectionChanged(FMarqueeOperation Marquee, bool bIsPreview);
	void RefreshSelectionRectangle();

private:
	TWeakObjectPtr<class UPaperTileSet> TileSetPtr;
	TSharedPtr<class FTileSetEditorViewportClient> TypedViewportClient;
	class FEdModeTileMap* TileMapEditor;
	FIntPoint SelectionTopLeft;
	FIntPoint SelectionDimensions;
	bool bPendingZoom;

	FOnTileViewportSelectionChanged OnTileSelectionChanged;
};