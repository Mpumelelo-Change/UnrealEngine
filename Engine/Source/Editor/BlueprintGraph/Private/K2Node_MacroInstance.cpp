// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "SlateBasics.h"
#include "EditorCategoryUtils.h"
#include "BlueprintActionFilter.h"
#include "K2Node_MacroInstance.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "K2Node_MacroInstance"

UK2Node_MacroInstance::UK2Node_MacroInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReconstructNode = false;
}

void UK2Node_MacroInstance::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.UE4Ver() < VER_UE4_K2NODE_REFERENCEGUIDS)
	{
		MacroGraphReference.SetGraph(MacroGraph_DEPRECATED);
	}
}

bool UK2Node_MacroInstance::IsActionFilteredOut(FBlueprintActionFilter const& Filter)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;

	for (UEdGraph* Graph : FilterContext.Graphs)
	{
		// Macro Instances are not allowed in it's own graph, nor in Function graphs if the macro has latent functions in it
		if (Graph == GetMacroGraph() || (Graph->GetSchema()->GetGraphType(Graph) == GT_Function && FBlueprintEditorUtils::CheckIfGraphHasLatentFunctions(GetMacroGraph())) )
		{
			bIsFilteredOut = true;
			break;
		}
	}
	return bIsFilteredOut;
}

void UK2Node_MacroInstance::PostPasteNode()
{
	const UBlueprint* InstanceOwner = GetBlueprint();

	// Find the owner of the macro graph
	const UEdGraph* MacroGraph = MacroGraphReference.GetGraph();
	UObject* MacroOwner = MacroGraph->GetOuter();
	UBlueprint* MacroOwnerBP = NULL;
	while(MacroOwner)
	{
		MacroOwnerBP = Cast<UBlueprint>(MacroOwner);
		if(MacroOwnerBP)
		{
			break;
		}

		MacroOwner = MacroOwner->GetOuter();
	}
	
	if((MacroOwnerBP != NULL)
		&& (MacroOwnerBP->BlueprintType != BPTYPE_MacroLibrary)
		&& (MacroOwnerBP != InstanceOwner))
	{
		// If this is a graph from another blueprint that is NOT a library, disallow the connection!
		MacroGraphReference.SetGraph(NULL);
	}

	Super::PostPasteNode();
}

void UK2Node_MacroInstance::AllocateDefaultPins()
{
	UK2Node::AllocateDefaultPins();

	PreloadObject(MacroGraphReference.GetBlueprint());
	UEdGraph* MacroGraph = MacroGraphReference.GetGraph();
	
	if (MacroGraph != NULL)
	{
		PreloadObject(MacroGraph);

		// Preload the macro graph, if needed, so that we can get the proper pins
		if (MacroGraph->HasAnyFlags(RF_NeedLoad))
		{
			PreloadObject(MacroGraph);
			FBlueprintEditorUtils::PreloadMembers(MacroGraph);
		}

		for (TArray<UEdGraphNode*>::TIterator NodeIt(MacroGraph->Nodes); NodeIt; ++NodeIt)
		{
			if (UK2Node_Tunnel* TunnelNode = Cast<UK2Node_Tunnel>(*NodeIt))
			{
				// Only want exact tunnel nodes, more specific nodes like composites or macro instances shouldn't be grabbed.
				if (TunnelNode->GetClass() == UK2Node_Tunnel::StaticClass())
				{
					for (TArray<UEdGraphPin*>::TIterator PinIt(TunnelNode->Pins); PinIt; ++PinIt)
					{
						UEdGraphPin* PortPin = *PinIt;

						// We're not interested in any pins that have been expanded internally on the macro
						if (PortPin->ParentPin == NULL)
						{
							UEdGraphPin* NewLocalPin = CreatePin(
								UEdGraphPin::GetComplementaryDirection(PortPin->Direction),
								PortPin->PinType.PinCategory,
								PortPin->PinType.PinSubCategory,
								PortPin->PinType.PinSubCategoryObject.Get(),
								PortPin->PinType.bIsArray,
								PortPin->PinType.bIsReference,
								PortPin->PinName);
							NewLocalPin->DefaultValue = NewLocalPin->AutogeneratedDefaultValue = PortPin->DefaultValue;
						}
					}
				}
			}
		}
	}
}

FText UK2Node_MacroInstance::GetTooltipText() const
{
	UEdGraph* MacroGraph = MacroGraphReference.GetGraph();
	if (FKismetUserDeclaredFunctionMetadata* Metadata = GetAssociatedGraphMetadata(MacroGraph))
	{
		if (!Metadata->ToolTip.IsEmpty())
		{
			return FText::FromString(Metadata->ToolTip);
		}
	}

	if (MacroGraph == nullptr)
	{
		return NSLOCTEXT("K2Node", "Macro_Tooltip", "Macro");
	}
	else if (CachedTooltip.IsOutOfDate())
	{
		// FText::Format() is slow, so we cache this to save on performance
		CachedTooltip = FText::Format(NSLOCTEXT("K2Node", "MacroGraphInstance_Tooltip", "{0} instance"), FText::FromName(MacroGraph->GetFName()));
	}
	return CachedTooltip;
}

FText UK2Node_MacroInstance::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UEdGraph* MacroGraph = MacroGraphReference.GetGraph();
	FText Result = NSLOCTEXT("K2Node", "MacroInstance", "Macro instance");
	if (MacroGraph)
	{
		Result = FText::FromString(MacroGraph->GetName());
	}

	return Result;
}

FLinearColor UK2Node_MacroInstance::GetNodeTitleColor() const
{
	UEdGraph* MacroGraph = MacroGraphReference.GetGraph();
	if (FKismetUserDeclaredFunctionMetadata* Metadata = GetAssociatedGraphMetadata(MacroGraph))
	{
		return Metadata->InstanceTitleColor.ToFColor(false);
	}

	return FLinearColor::White;
}

void UK2Node_MacroInstance::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	if ( Context.Pin == NULL )
	{
		Context.MenuBuilder->BeginSection("K2NodeMacroInstance", NSLOCTEXT("K2Node", "MacroInstanceHeader", "Macro Instance"));
		{
			Context.MenuBuilder->AddMenuEntry(
				NSLOCTEXT("K2Node", "MacroInstanceFindInContentBrowser", "Find in Content Browser"),
				NSLOCTEXT("K2Node", "MacroInstanceFindInContentBrowserTooltip", "Finds the Blueprint Macro Library that contains this Macro in the Content Browser"),
				FSlateIcon(FEditorStyle::GetStyleSetName(), "PropertyWindow.Button_Browse"),
				FUIAction( FExecuteAction::CreateStatic( &UK2Node_MacroInstance::FindInContentBrowser, TWeakObjectPtr<UK2Node_MacroInstance>(this) ) )
				);
		}
		Context.MenuBuilder->EndSection();
	}
}

FKismetUserDeclaredFunctionMetadata* UK2Node_MacroInstance::GetAssociatedGraphMetadata(const UEdGraph* AssociatedMacroGraph)
{
	if (AssociatedMacroGraph)
	{
		// Look at the graph for the entry node, to get the default title color
		TArray<UK2Node_Tunnel*> TunnelNodes;
		AssociatedMacroGraph->GetNodesOfClass(TunnelNodes);

		for (int32 i = 0; i < TunnelNodes.Num(); i++)
		{
			UK2Node_Tunnel* Node = TunnelNodes[i];
			if (Node->IsEditable())
			{
				if (Node->bCanHaveOutputs)
				{
					return &(Node->MetaData);
				}
			}
		}
	}

	return NULL;
}

void UK2Node_MacroInstance::FindInContentBrowser(TWeakObjectPtr<UK2Node_MacroInstance> MacroInstance)
{
	if ( MacroInstance.IsValid() )
	{
		UEdGraph* InstanceMacroGraph = MacroInstance.Get()->MacroGraphReference.GetGraph();
		if ( InstanceMacroGraph )
		{
			UBlueprint* BlueprintToSync = FBlueprintEditorUtils::FindBlueprintForGraph(InstanceMacroGraph);
			if ( BlueprintToSync )
			{
				TArray<UObject*> ObjectsToSync;
				ObjectsToSync.Add( BlueprintToSync );
				GEditor->SyncBrowserToObjects(ObjectsToSync);
			}
		}
	}
}




void UK2Node_MacroInstance::NotifyPinConnectionListChanged(UEdGraphPin* ChangedPin)
{
	Super::NotifyPinConnectionListChanged(ChangedPin);

	const UEdGraphSchema_K2* const Schema = GetDefault<UEdGraphSchema_K2>();

	// added a link?
	if (ChangedPin->LinkedTo.Num() > 0)
	{
		// ... to a wildcard pin?
		bool const bIsWildcardPin = ChangedPin->PinType.PinCategory == Schema->PC_Wildcard;
		if (bIsWildcardPin)
		{
			// get type of pin we just got linked to
			FEdGraphPinType const LinkedPinType = ChangedPin->LinkedTo[0]->PinType;

			// change all other wildcard pins to the new type
			// note we're assuming only one wildcard type per Macro node, for now

			for(int32 PinIdx=0; PinIdx<Pins.Num(); PinIdx++)
			{
				UEdGraphPin* const TmpPin = Pins[PinIdx];
				if (TmpPin)
				{
					if (TmpPin->PinType.PinCategory == Schema->PC_Wildcard)
					{
						// only copy the category stuff to preserve array and ref status
						TmpPin->PinType.PinCategory = LinkedPinType.PinCategory;
						TmpPin->PinType.PinSubCategory = LinkedPinType.PinSubCategory;
						TmpPin->PinType.PinSubCategoryObject = LinkedPinType.PinSubCategoryObject;
					}
				}
			}

			ResolvedWildcardType = LinkedPinType;
			bReconstructNode = true;
		}
	}
 	else
	{
		// reconstruct on disconnects so we can revert back to wildcards if necessary
		bReconstructNode = true;
	}
}



void UK2Node_MacroInstance::NodeConnectionListChanged()
{
	if (bReconstructNode)
	{
		ReconstructNode();

		UBlueprint* const Blueprint = GetBlueprint();
		if (Blueprint && !Blueprint->bBeingCompiled)
		{
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			Blueprint->BroadcastChanged();
		}
	}
}

FString UK2Node_MacroInstance::GetDocumentationLink() const
{
	return TEXT("Shared/GraphNodes/Blueprint/UK2Node_MacroInstance");
}

FString UK2Node_MacroInstance::GetDocumentationExcerptName() const
{
	UEdGraph* MacroGraph = MacroGraphReference.GetGraph();
	if (MacroGraph != NULL)
	{
		return MacroGraph->GetName();
	}
	return Super::GetDocumentationExcerptName();
}

void UK2Node_MacroInstance::PostReconstructNode()
{
	bReconstructNode = false;

	Super::PostReconstructNode();
}

FName UK2Node_MacroInstance::GetPaletteIcon(FLinearColor& OutColor) const
{
	// Special case handling for standard macros
	// @TODO Change this to a SlateBurushAsset pointer on the graph or something similar, to allow any macro to have an icon
	UEdGraph* MacroGraph = MacroGraphReference.GetGraph();
	if(MacroGraph != NULL && MacroGraph->GetOuter()->GetName() == TEXT("StandardMacros"))
	{
		FName MacroName = FName(*MacroGraph->GetName());
		if(	MacroName == TEXT("ForLoop" ) ||
			MacroName == TEXT("ForLoopWithBreak") ||
			MacroName == TEXT("WhileLoop") )
		{
			return TEXT("GraphEditor.Macro.Loop_16x");
		}
		else if( MacroName == TEXT("Gate") )
		{
			return TEXT("GraphEditor.Macro.Gate_16x");
		}
		else if( MacroName == TEXT("Do N") )
		{
			return TEXT("GraphEditor.Macro.DoN_16x");
		}
		else if (MacroName == TEXT("DoOnce"))
		{
			return TEXT("GraphEditor.Macro.DoOnce_16x");
		}
		else if (MacroName == TEXT("IsValid"))
		{
			return TEXT("GraphEditor.Macro.IsValid_16x");
		}
		else if (MacroName == TEXT("FlipFlop"))
		{
			return TEXT("GraphEditor.Macro.FlipFlop_16x");
		}
		else if ( MacroName == TEXT("ForEachLoop") ||
				  MacroName == TEXT("ForEachLoopWithBreak") )
		{
			return TEXT("GraphEditor.Macro.ForEach_16x");
		}
	}

	return TEXT("GraphEditor.Macro_16x");
}

void UK2Node_MacroInstance::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	const UEdGraphSchema_K2* const Schema = GetDefault<UEdGraphSchema_K2>();

	// determine if all wildcard pins are unlinked.
	// if they are, we should revert them all back to wildcard status
	bool bAllWildcardsAreUnlinked = true;
	for (auto PinIt = Pins.CreateConstIterator(); PinIt; PinIt++)
	{
		// for each of the wildcard pins...
		UEdGraphPin* const Pin = *PinIt;
		if ( Pin->PinType.PinCategory == Schema->PC_Wildcard )
		{
			// find it in the old pins array (where it might not be a wildcard)
			// and see if it's unlinked
			for (auto OldPinIt = OldPins.CreateConstIterator(); OldPinIt; OldPinIt++)
			{
				UEdGraphPin const* const OldPin = *OldPinIt;
				if (OldPin->PinName == Pin->PinName)
				{
					if (OldPin->LinkedTo.Num() > 0)
					{
						bAllWildcardsAreUnlinked = false;
						break;
					}
				}
			}
		}
	}

	if (bAllWildcardsAreUnlinked == false)
	{
		// Copy pin types from old pins for wildcard pins
		for (auto PinIt = Pins.CreateConstIterator(); PinIt; PinIt++)
		{
			UEdGraphPin* const Pin = *PinIt;
			if ( Pin->PinType.PinCategory == Schema->PC_Wildcard )
			{
				// find it in the old pins and copy the type
				for (auto OldPinIt = OldPins.CreateConstIterator(); OldPinIt; OldPinIt++)
				{
					UEdGraphPin const* const OldPin = *OldPinIt;
					if (OldPin->PinName == Pin->PinName)
					{
						Pin->PinType = OldPin->PinType;
					}
				}
			}
		}
	}
	else
	{
		// no type
		ResolvedWildcardType.ResetToDefaults();
	}
}

FText UK2Node_MacroInstance::GetActiveBreakpointToolTipText() const
{
	return LOCTEXT("ActiveBreakpointToolTip", "Execution will break inside the macro.");
}

bool UK2Node_MacroInstance::HasExternalBlueprintDependencies(TArray<class UStruct*>* OptionalOutput) const
{
	UBlueprint* OtherBlueprint = MacroGraphReference.GetBlueprint();
	const bool bResult = OtherBlueprint && (OtherBlueprint != GetBlueprint());
	if (bResult && OptionalOutput)
	{
		if (UClass* OtherClass = *OtherBlueprint->GeneratedClass)
		{
			OptionalOutput->Add(OtherClass);
		}
	}
	return bResult;
}

void UK2Node_MacroInstance::GetNodeAttributes( TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes ) const
{
	FString MacroName( TEXT( "InvalidMacro" ));

	if( UEdGraph* MacroGraph = GetMacroGraph() )
	{
		MacroName = MacroGraph->GetName();
	}

	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Type" ), TEXT( "Macro" ) ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Class" ), GetClass()->GetName() ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Name" ), MacroName ));
}

FText UK2Node_MacroInstance::GetMenuCategory() const
{
	FText MenuCategory = FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Utilities);
	if (UEdGraph* MacroGraph = GetMacroGraph())
	{
		FKismetUserDeclaredFunctionMetadata* MacroGraphMetadata = UK2Node_MacroInstance::GetAssociatedGraphMetadata(MacroGraph);
		if ((MacroGraphMetadata != nullptr) && !MacroGraphMetadata->Category.IsEmpty())
		{
			MenuCategory = FText::FromString(MacroGraphMetadata->Category);
		}
	}

	return MenuCategory;
}

FBlueprintNodeSignature UK2Node_MacroInstance::GetSignature() const
{
	FBlueprintNodeSignature NodeSignature = Super::GetSignature();
	NodeSignature.AddSubObject(GetMacroGraph());

	return NodeSignature;
}

#undef LOCTEXT_NAMESPACE
