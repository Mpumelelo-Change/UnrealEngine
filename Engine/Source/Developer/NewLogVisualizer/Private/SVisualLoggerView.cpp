#include "LogVisualizer.h"
#include "SVisualLoggerView.h"
#include "TimeSliderController.h"
#include "ITimeSlider.h"
#include "STimeSlider.h"
#include "SSearchBox.h"
#include "SSequencerSectionOverlay.h"
#include "STimelinesContainer.h"

#define LOCTEXT_NAMESPACE "SVisualLoggerFilters"

//template <typename ItemType>
//class SLogListView : public SListView<ItemType>
//{
//public:
//	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
//	{
//		if (!MouseEvent.IsLeftShiftDown())
//		{
//			return SListView<ItemType>::OnMouseWheel(MyGeometry, MouseEvent);
//		};
//
//		return FReply::Unhandled();
//	}
//
//	void RefreshList()
//	{
//		const TArray<ItemType>& ItemsSourceRef = (*this->ItemsSource);
//
//		for (int32 Index = 0; Index < ItemsSourceRef.Num(); ++Index)
//		{
//			TSharedPtr< SLogsTableRow > TableRow = StaticCastSharedPtr< SLogsTableRow >(this->WidgetGenerator.GetWidgetForItem(ItemsSourceRef[Index]));
//			if (TableRow.IsValid())
//			{
//				TableRow->UpdateEntries();
//			}
//		}
//
//	}
//};

void SVisualLoggerView::GetTimelines(TArray<TSharedPtr<STimeline> >& OutList, bool bOnlySelectedOnes)
{
	OutList = bOnlySelectedOnes ? TimelinesContainer->GetSelectedNodes() : TimelinesContainer->GetAllNodes();
}

void SVisualLoggerView::Construct(const FArguments& InArgs, const TSharedRef<FUICommandList>& InCommandList, TSharedPtr<IVisualLoggerInterface> VisualLoggerInterface)
{
	AnimationOutlinerFillPercentage = .25f;
	VisualLoggerEvents = VisualLoggerInterface->GetVisualLoggerEvents();

	FVisualLoggerTimeSliderArgs TimeSliderArgs;
	TimeSliderArgs.ViewRange = InArgs._ViewRange;
	TimeSliderArgs.ClampMin = InArgs._ViewRange.Get().GetLowerBoundValue();
	TimeSliderArgs.ClampMax = InArgs._ViewRange.Get().GetUpperBoundValue();
	TimeSliderArgs.ScrubPosition = InArgs._ScrubPosition;

	TSharedPtr<FSequencerTimeSliderController> TimeSliderController(new FSequencerTimeSliderController(TimeSliderArgs));
	VisualLoggerInterface->SetTimeSliderController(TimeSliderController);

	// Create the top and bottom sliders
	const bool bMirrorLabels = true;
	TSharedRef<ITimeSlider> TopTimeSlider = SNew(STimeSlider, TimeSliderController.ToSharedRef()).MirrorLabels(bMirrorLabels);
	TSharedRef<ITimeSlider> BottomTimeSlider = SNew(STimeSlider, TimeSliderController.ToSharedRef()).MirrorLabels(bMirrorLabels);

	TSharedPtr<SScrollBox> ScrollBox;
	TSharedRef<SScrollBar> ScrollBar =
		SNew(SScrollBar)
		.Thickness(FVector2D(5.0f, 5.0f));

	ChildSlot
		[
			SNew(SBorder)
			.Padding(2)
			.BorderImage(FLogVisualizerStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SSplitter)
				.Orientation(Orient_Vertical)
				.PhysicalSplitterHandleSize(2)
				+ SSplitter::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(SearchSplitter, SSplitter)
						.Orientation(Orient_Horizontal)
						.OnSplitterFinishedResizing(this, &SVisualLoggerView::OnSearchSplitterResized)
						+ SSplitter::Slot()
						.Value(0.25)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.Padding(FMargin(0))
							.VAlign(VAlign_Center)
							[
								SNew(SBox)
								.Padding(FMargin(0, 0, 4, 0))
								[
									// Search box for searching through the outliner
									SNew(SSearchBox)
									.OnTextChanged(this, &SVisualLoggerView::OnSearchChanged)
								]
							]
						]
						+ SSplitter::Slot()
						.Value(0.75)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							[
#if 0 //top time slider disabled to test idea with filter's search box
								SNew(SBorder)
								// @todo Sequencer Do not change the paddings or the sliders scrub widgets wont line up
								.Padding(FMargin(0.0f, 2.0f, 0.0f, 0.0f))
								.BorderImage(FLogVisualizerStyle::Get().GetBrush("ToolPanel.GroupBorder"))
								.BorderBackgroundColor(FLinearColor(.50f, .50f, .50f, 1.0f))
								[
									TopTimeSlider
								]
#else
								SNew(SBox)
								.Padding(FMargin(0, 0, 4, 0))
								[
									SNew(SSearchBox)
									.OnTextChanged(InArgs._OnFiltersSearchChanged)
									.HintText(LOCTEXT("FiltersSearchHint", "Log Category Search"))
								]
#endif
							]
						]
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							MakeSectionOverlay(TimeSliderController.ToSharedRef(), InArgs._ViewRange, InArgs._ScrubPosition, false)
						]
						+ SOverlay::Slot()
						[
							SAssignNew(ScrollBox, SScrollBox)
							.ExternalScrollbar(ScrollBar)
						]
						+ SOverlay::Slot()
						[
							MakeSectionOverlay(TimeSliderController.ToSharedRef(), InArgs._ViewRange, InArgs._ScrubPosition, true)
						]
						+SOverlay::Slot()
						.HAlign(HAlign_Right)
						[
							ScrollBar
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(TAttribute<float>(this, &SVisualLoggerView::GetAnimationOutlinerFillPercentage))
						[
							SNew(SSpacer)
						]
						+ SHorizontalBox::Slot()
						.Padding(FMargin(0.0f, 0.0f, 0.0f, 0.0f))
						.FillWidth(1.0f)
						[
							SNew(SBorder)
							// @todo Sequencer Do not change the paddings or the sliders scrub widgets wont line up
							.Padding(FMargin(0.0f, 0.0f, 0.0f, 2.0f))
							.BorderImage(FLogVisualizerStyle::Get().GetBrush("ToolPanel.GroupBorder"))
							.BorderBackgroundColor(FLinearColor(.50f, .50f, .50f, 1.0f))
							[
								BottomTimeSlider
							]
						]
					]
				]
			]
		];

		ScrollBox->AddSlot()
		[
			SAssignNew(TimelinesContainer, STimelinesContainer, SharedThis(this), TimeSliderController.ToSharedRef())
			.VisualLoggerInterface(VisualLoggerInterface)
		];

}

void SVisualLoggerView::OnSearchSplitterResized()
{
	SSplitter::FSlot const& LeftSplitterSlot = SearchSplitter->SlotAt(0);
	SSplitter::FSlot const& RightSplitterSlot = SearchSplitter->SlotAt(1);

	SetAnimationOutlinerFillPercentage(LeftSplitterSlot.SizeValue.Get() / RightSplitterSlot.SizeValue.Get());
}

void SVisualLoggerView::OnSearchChanged(const FText& Filter)
{
	TimelinesContainer->OnSearchChanged(Filter);
}

TSharedRef<SWidget> SVisualLoggerView::MakeSectionOverlay(TSharedRef<FSequencerTimeSliderController> TimeSliderController, const TAttribute< TRange<float> >& ViewRange, const TAttribute<float>& ScrubPosition, bool bTopOverlay)
{
	return
		SNew(SHorizontalBox)
		.Visibility(EVisibility::HitTestInvisible)
		+ SHorizontalBox::Slot()
		.FillWidth(TAttribute<float>(this, &SVisualLoggerView::GetAnimationOutlinerFillPercentage))
		[
			// Take up space but display nothing. This is required so that all areas dependent on time align correctly
			SNullWidget::NullWidget
		]
	+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SSequencerSectionOverlay, TimeSliderController)
			.DisplayScrubPosition(bTopOverlay)
			.DisplayTickLines(!bTopOverlay)
		];
}

void SVisualLoggerView::OnNewLogEntry(const FVisualLogDevice::FVisualLogEntryItem& Entry)
{
	TimelinesContainer->OnNewLogEntry(Entry);
}

void SVisualLoggerView::OnFiltersChanged()
{
	TimelinesContainer->OnFiltersChanged();
}
#undef LOCTEXT_NAMESPACE
