// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "ReimportFeedbackContext.h"
#include "NotificationManager.h"
#include "SNotificationList.h"
#include "INotificationWidget.h"
#include "MessageLogModule.h"
#include "SHyperlink.h"

#define LOCTEXT_NAMESPACE "ReimportContext"

class SWidgetStack : public SVerticalBox
{
	SLATE_BEGIN_ARGS(SWidgetStack){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, int32 InMaxNumVisible)
	{
		MaxNumVisible = InMaxNumVisible;
		SlideCurve = FCurveSequence(0.f, .5f, ECurveEaseFunction::QuadOut);
		SizeCurve = FCurveSequence(0.f, .5f, ECurveEaseFunction::QuadOut);

		StartSlideOffset = 0;
		StartSizeOffset = FVector2D(ForceInitToZero);
	}

	FVector2D ComputeTotalSize() const
	{
		FVector2D Size(ForceInitToZero);
		for (int32 Index = 0; Index < FMath::Min(NumSlots(), MaxNumVisible); ++Index)
		{
			const FVector2D& ChildSize = Children[Index].GetWidget()->GetDesiredSize();
			if (ChildSize.X > Size.X)
			{
				Size.X = ChildSize.X;
			}
			Size.Y += ChildSize.Y + Children[Index].SlotPadding.Get().GetTotalSpaceAlong<Orient_Vertical>();
		}
		return Size;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		const float Lerp = SizeCurve.GetLerp();
		FVector2D DesiredSize = ComputeTotalSize() * Lerp + StartSizeOffset * (1.f-Lerp);
		DesiredSize.X = FMath::Min(500.f, DesiredSize.X);

		return DesiredSize;
	}

	virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const override
	{
		if (Children.Num() == 0)
		{
			return;
		}

		const float Alpha = 1.f - SlideCurve.GetLerp();
		float PositionSoFar = AllottedGeometry.GetLocalSize().Y + StartSlideOffset*Alpha;

		for (int32 Index = 0; Index < NumSlots(); ++Index)
		{
			const SBoxPanel::FSlot& CurChild = Children[Index];
			const EVisibility ChildVisibility = CurChild.GetWidget()->GetVisibility();

			if (ChildVisibility != EVisibility::Collapsed)
			{
				const FVector2D ChildDesiredSize = CurChild.GetWidget()->GetDesiredSize();

				const FMargin SlotPadding(CurChild.SlotPadding.Get());
				const FVector2D SlotSize(AllottedGeometry.Size.X, ChildDesiredSize.Y + SlotPadding.GetTotalSpaceAlong<Orient_Vertical>());

				const AlignmentArrangeResult XAlignmentResult = AlignChild<Orient_Horizontal>( SlotSize.X, CurChild, SlotPadding );
				const AlignmentArrangeResult YAlignmentResult = AlignChild<Orient_Vertical>( SlotSize.Y, CurChild, SlotPadding );

				ArrangedChildren.AddWidget( ChildVisibility, AllottedGeometry.MakeChild(
					CurChild.GetWidget(),
					FVector2D( XAlignmentResult.Offset, PositionSoFar - SlotSize.Y + YAlignmentResult.Offset ),
					FVector2D( XAlignmentResult.Size, YAlignmentResult.Size )
					));

				PositionSoFar -= SlotSize.Y;
			}
		}
	}

	void Add(const TSharedRef<SWidget>& InWidget)
	{
		TSharedPtr<SWidgetStackItem> NewItem;

		InsertSlot(0)
		.AutoHeight()
		[
			SAssignNew(NewItem, SWidgetStackItem)
			[
				InWidget
			]
		];
		
		{
			auto Widget = Children[0].GetWidget();
			Widget->SlatePrepass();

			const float WidgetHeight = Widget->GetDesiredSize().Y;
			StartSlideOffset += WidgetHeight;
			// Fade in time is 1 second x the proportion of the slide amount that this widget takes up
			NewItem->FadeIn(WidgetHeight / StartSlideOffset);

			if (!SlideCurve.IsPlaying())
			{
				SlideCurve.Play(AsShared());
			}
		}

		const FVector2D NewSize = ComputeTotalSize();
		if (NewSize != StartSizeOffset)
		{
			StartSizeOffset = NewSize;

			if (!SizeCurve.IsPlaying())
			{
				SizeCurve.Play(AsShared());
			}
		}
	}
	
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		if (!SlideCurve.IsPlaying())
		{
			StartSlideOffset = 0;
		}

		// Delete any widgets that are now offscreen
		if (Children.Num() != 0)
		{
			const float Alpha = 1.f - SlideCurve.GetLerp();
			float PositionSoFar = AllottedGeometry.GetLocalSize().Y + Alpha * StartSlideOffset;

			for (int32 Index = 0; PositionSoFar > 0 && Index < NumSlots(); ++Index)
			{
				const SBoxPanel::FSlot& CurChild = Children[Index];
				const EVisibility ChildVisibility = CurChild.GetWidget()->GetVisibility();

				if (ChildVisibility != EVisibility::Collapsed)
				{
					const FVector2D ChildDesiredSize = CurChild.GetWidget()->GetDesiredSize();
					PositionSoFar -= ChildDesiredSize.Y + CurChild.SlotPadding.Get().GetTotalSpaceAlong<Orient_Vertical>();
				}
			}

			for (int32 Index = MaxNumVisible; Index < Children.Num(); )
			{
				if (StaticCastSharedRef<SWidgetStackItem>(Children[Index].GetWidget())->bIsFinished)
				{
					Children.RemoveAt(Index);
				}
				else
				{
					++Index;
				}
			}
		}
	}

	class SWidgetStackItem : public SCompoundWidget
	{
		SLATE_BEGIN_ARGS(SWidgetStackItem){}
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			bIsFinished = false;

			ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("NoBorder"))
				.ColorAndOpacity(this, &SWidgetStackItem::GetColorAndOpacity)
				.Padding(0)
				[
					InArgs._Content.Widget
				]
			];
		}
		
		void FadeIn(float Time)
		{
			OpacityCurve = FCurveSequence(0.f, Time, ECurveEaseFunction::QuadOut);
			OpacityCurve.Play(AsShared());
		}

		virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
		{
			if (!bIsFinished && OpacityCurve.IsAtStart() && OpacityCurve.IsInReverse())
			{
				bIsFinished = true;
			}
		}

		FLinearColor GetColorAndOpacity() const
		{
			return FLinearColor(1.f, 1.f, 1.f, OpacityCurve.GetLerp());
		}

		bool bIsFinished;
		FCurveSequence OpacityCurve;
	};

	FCurveSequence SlideCurve, SizeCurve;

	float StartSlideOffset;
	FVector2D StartSizeOffset;

	int32 MaxNumVisible;
};

class SWidgetStack;

/** Feedback context that overrides GWarn for import operations to prevent popup spam */
class SReimportFeedback : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReimportFeedback) : _ExpireDuration(3.f) {}

		SLATE_ATTRIBUTE(FText, MainText)

		SLATE_ARGUMENT(float, ExpireDuration)
		SLATE_EVENT(FSimpleDelegate, OnExpired)

		SLATE_EVENT(FSimpleDelegate, OnPauseClicked)
		SLATE_EVENT(FSimpleDelegate, OnAbortClicked)

	SLATE_END_ARGS()

	/** Construct this widget */
	void Construct(const FArguments& InArgs)
	{
		ExpireDuration = InArgs._ExpireDuration;
		OnExpired = InArgs._OnExpired;

		MainText = InArgs._MainText;
		bPaused = false;
		bExpired = false;

		auto OpenMessageLog = []{
			FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
			MessageLogModule.OpenMessageLog("AssetReimport");
		};

		ChildSlot
		[
			SNew(SBorder)
			.Padding(FMargin(10))
			.BorderImage(FCoreStyle::Get().GetBrush("NotificationList.ItemBackground"))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.Padding(FMargin(0))
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					[
						SNew(STextBlock)
						.Text(this, &SReimportFeedback::GetMainText)
						.Font(FCoreStyle::Get().GetFontStyle(TEXT("NotificationList.FontLight")))
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(0,0,4,0))
					[
						SAssignNew(PauseButton, SButton)
						.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
						.ToolTipText(LOCTEXT("PauseTooltip", "Temporarily pause processing of these source content files"))
						.OnClicked(this, &SReimportFeedback::OnPauseClicked, InArgs._OnPauseClicked)
						[
							SNew(SImage)
							.ColorAndOpacity(FLinearColor(.8f,.8f,.8f,1.f))
							.Image(this, &SReimportFeedback::GetPlayPauseBrush)
						]
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SAssignNew(AbortButton, SButton)
						.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
						.ToolTipText(LOCTEXT("AbortTooltip", "Permanently abort processing of these source content files"))
						.OnClicked(this, &SReimportFeedback::OnAbortClicked, InArgs._OnAbortClicked)
						[
							SNew(SImage)
							.ColorAndOpacity(FLinearColor(0.8f,0.8f,0.8f,1.f))
							.Image(FEditorStyle::GetBrush("GenericStop"))
						]
					]
				]

				+ SVerticalBox::Slot()
				.Padding(FMargin(0, 5, 0, 0))
				.AutoHeight()
				[
					SAssignNew(WidgetStack, SWidgetStack, 3)
				]

				+ SVerticalBox::Slot()
				.Padding(FMargin(0, 5, 0, 0))
				.AutoHeight()
				.HAlign(HAlign_Right)
				[
					SNew(SHyperlink)
					.Visibility(this, &SReimportFeedback::GetHyperlinkVisibility)
					.Text(LOCTEXT("OpenMessageLog", "Open message log"))
					.TextStyle(FCoreStyle::Get(), "SmallText")
					.OnNavigate_Lambda(OpenMessageLog)
				]
			]
		];
	}

	/** Add a widget to this feedback's widget stack */
	void Add(const TSharedRef<SWidget>& Widget)
	{
		WidgetStack->Add(Widget);
	}

	/** Disable input to this widget's dynamic content (except the message log hyperlink) */
	void Disable()
	{
		// Get the final text to display
		CachedMainText = MainText.Get(FText());

		ExpireTimeout = FTimeLimit(ExpireDuration);

		WidgetStack->SetVisibility(EVisibility::HitTestInvisible);
		PauseButton->SetVisibility(EVisibility::Collapsed);
		AbortButton->SetVisibility(EVisibility::Collapsed);
	}

	/** Enable, if previously disabled */
	void Enable()
	{
		ExpireTimeout = FTimeLimit();

		bPaused = false;
		WidgetStack->SetVisibility(EVisibility::Visible);
		PauseButton->SetVisibility(EVisibility::Visible);
		AbortButton->SetVisibility(EVisibility::Visible);
	}

private:
	
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		if (bExpired)
		{
			return;
		}
		else if(ExpireTimeout.IsValid())
		{
			if (ExpireTimeout.Exceeded())
			{	
				OnExpired.ExecuteIfBound();
				bExpired = true;
			}
		}
		else
		{
			CachedMainText = MainText.Get(FText());
		}
	}

	/** Get the main text of this widget */
	FText GetMainText() const
	{
		return CachedMainText;
	}

	/** Get the play/pause image */
	const FSlateBrush* GetPlayPauseBrush() const
	{
		return bPaused ? FEditorStyle::GetBrush("GenericPlay") : FEditorStyle::GetBrush("GenericPause");
	}

	/** Called when pause is clicked */
	FReply OnPauseClicked(FSimpleDelegate UserOnClicked)
	{
		bPaused = !bPaused;

		UserOnClicked.ExecuteIfBound();
		return FReply::Handled();
	}
	
	/** Called when abort is clicked */
	FReply OnAbortClicked(FSimpleDelegate UserOnClicked)
	{
		//Destroy();
		UserOnClicked.ExecuteIfBound();
		return FReply::Handled();
	}

	/** Get the visibility of the hyperlink to open the message log */
	EVisibility GetHyperlinkVisibility() const
	{
		return WidgetStack->NumSlots() != 0 ? EVisibility::Visible : EVisibility::Collapsed;
	}

private:

	/** The expire timeout used to fire OnExpired. Invalid when no timeout is set. */
	FTimeLimit ExpireTimeout;

	/** Amount of time to wait after this widget has been disabled before calling OnExpired */
	float ExpireDuration;

	/** Event that is called when this widget has been inactive and open for too long, and will fade out */
	FSimpleDelegate OnExpired;

	/** Cached main text for the notification */
	FText CachedMainText;
	TAttribute<FText> MainText;

	/** Whether we are paused and/or expired */
	bool bPaused, bExpired;

	/** The widget stack, displaying contextural information about the current state of the process */
	TSharedPtr<SWidgetStack> WidgetStack;
	TSharedPtr<SButton> PauseButton, AbortButton;
};

FReimportFeedbackContext::FReimportFeedbackContext(const TAttribute<FText>& InMainText, const FSimpleDelegate& InOnPauseClicked, const FSimpleDelegate& InOnAbortClicked)
	: bSuppressSlowTaskMessages(false)
	, MainTextAttribute(InMainText)
	, OnPauseClickedEvent(InOnPauseClicked)
	, OnAbortClickedEvent(InOnAbortClicked)
	, MessageLog("AssetReimport")
{
}

void FReimportFeedbackContext::Show()
{
	if (NotificationContent.IsValid())
	{
		NotificationContent->Enable();
	}
	else
	{
		NotificationContent = SNew(SReimportFeedback)
			.OnExpired(this, &FReimportFeedbackContext::OnNotificationExpired)
			.MainText(MainTextAttribute)
			.OnPauseClicked(OnPauseClickedEvent)
			.OnAbortClicked(OnAbortClickedEvent);

		FNotificationInfo Info(AsShared());
		Info.bFireAndForget = false;

		Notification = FSlateNotificationManager::Get().AddNotification(Info);

		MessageLog.NewPage(FText::Format(LOCTEXT("MessageLogPageLabel", "Outstanding source content changes {0}"), FText::AsTime(FDateTime::Now())));
	}
}

void FReimportFeedbackContext::Hide()
{
	if (Notification.IsValid())
	{
		NotificationContent->Disable();
		Notification->SetCompletionState(SNotificationItem::CS_Success);
	}
}

void FReimportFeedbackContext::OnNotificationExpired()
{
	if (Notification.IsValid())
	{
		MessageLog.Notify(FText(), EMessageSeverity::Error);
		Notification->Fadeout();

		NotificationContent = nullptr;
		Notification = nullptr;
	}
}

void FReimportFeedbackContext::AddMessage(EMessageSeverity::Type Severity, const FText& Message)
{
	MessageLog.Message(Severity, Message);
	AddWidget(SNew(STextBlock).Text(Message));
}

void FReimportFeedbackContext::AddWidget(const TSharedRef<SWidget>& Widget)
{
	if (NotificationContent.IsValid())
	{
		NotificationContent->Add(Widget);
	}
}

TSharedRef<SWidget> FReimportFeedbackContext::AsWidget()
{
	return NotificationContent.ToSharedRef();
}

void FReimportFeedbackContext::StartSlowTask(const FText& Task, bool bShowCancelButton)
{
	FFeedbackContext::StartSlowTask(Task, bShowCancelButton);

	if (NotificationContent.IsValid() && !bSuppressSlowTaskMessages && !Task.IsEmpty())
	{
		NotificationContent->Add(SNew(STextBlock).Text(Task));
	}
}

#undef LOCTEXT_NAMESPACE