// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NamedKeyArea.h"
#include "ClipboardTypes.h"


struct FRichCurve;
class UMovieSceneSection;


/**
 * A key area for float keys.
 */
class MOVIESCENETOOLS_API FFloatCurveKeyArea
	: public FNamedKeyArea
{
public:

	/**
	* Creates a new key area for editing float curves.
	*
	* @param InCurve The rich curve which has the float keys.
	* @param InOwningSection The section which owns the curve which is being displayed and edited by this area.
	* @param InColor An optional color which should be used to draw the background of this key area.
	*/
	FFloatCurveKeyArea(FRichCurve* InCurve, UMovieSceneSection* InOwningSection, TOptional<FLinearColor> InColor = TOptional<FLinearColor>())
		: Color(InColor)
		, Curve(InCurve)
		, OwningSection(InOwningSection)
	{ }

	/**
	* Creates a new key area for editing float curves whose value can be overridden externally.
	*
	* @param InCurve The rich curve which has the float keys.
	* @param InExternalValue An attribute which can provide an external value for this key area.  External values are
	*        useful for things like property tracks where the property value can change without changing the animation
	*        and we want to be able to key and update using the new property value.
	* @param InOwningSection The section which owns the curve which is being displayed and edited by this area.
	* @param InColor An optional color which should be used to draw the background of this key area.
	*/
	FFloatCurveKeyArea(FRichCurve* InCurve, TAttribute<TOptional<float>> InExternalValue, UMovieSceneSection* InOwningSection, TOptional<FLinearColor> InColor = TOptional<FLinearColor>())
		: Color(InColor)
		, Curve(InCurve)
		, OwningSection(InOwningSection)
		, ExternalValue(InExternalValue)
	{ }

public:

	//` IKeyArea interface

	virtual TArray<FKeyHandle> AddKeyUnique(float Time, EMovieSceneKeyInterpolation InKeyInterpolation, float TimeToCopyFrom = FLT_MAX) override;
	virtual TOptional<FKeyHandle> DuplicateKey(FKeyHandle KeyToDuplicate) override;
	virtual bool CanCreateKeyEditor() override;
	virtual TSharedRef<SWidget> CreateKeyEditor(ISequencer* Sequencer) override;
	virtual void DeleteKey(FKeyHandle KeyHandle) override;
	virtual TOptional<FLinearColor> GetColor() override;
	virtual ERichCurveExtrapolation GetExtrapolationMode(bool bPreInfinity) const override;
	virtual ERichCurveInterpMode GetKeyInterpMode(FKeyHandle KeyHandle) const override;
	virtual TSharedPtr<FStructOnScope> GetKeyStruct(FKeyHandle KeyHandle) override;
	virtual ERichCurveTangentMode GetKeyTangentMode(FKeyHandle KeyHandle) const override;
	virtual float GetKeyTime(FKeyHandle KeyHandle) const override;
	virtual UMovieSceneSection* GetOwningSection() override;
	virtual FRichCurve* GetRichCurve() override;
	virtual TArray<FKeyHandle> GetUnsortedKeyHandles() const override;
	virtual FKeyHandle MoveKey(FKeyHandle KeyHandle, float DeltaPosition) override;
	virtual void SetExtrapolationMode(ERichCurveExtrapolation ExtrapMode, bool bPreInfinity) override;
	virtual bool CanSetExtrapolationMode() const override;
	virtual void SetKeyInterpMode(FKeyHandle KeyHandle, ERichCurveInterpMode InterpMode) override;
	virtual void SetKeyTangentMode(FKeyHandle KeyHandle, ERichCurveTangentMode TangentMode) override;
	virtual void SetKeyTime(FKeyHandle KeyHandle, float NewKeyTime) const override;
	virtual void CopyKeys(FMovieSceneClipboardBuilder& ClipboardBuilder, const TFunctionRef<bool(FKeyHandle, const IKeyArea&)>& KeyMask) const override;
	virtual void PasteKeys(const FMovieSceneClipboardKeyTrack& KeyTrack, const FMovieSceneClipboardEnvironment& SrcEnvironment, const FSequencerPasteEnvironment& DstEnvironment) override;

protected:

	void OnValueChanged(float InValue);

private:

	/** The key area's color. */
	TOptional<FLinearColor> Color;

	/** The curve which provides the keys for this key area. */
	FRichCurve* Curve;

	/** The section that owns this key area. */
	UMovieSceneSection* OwningSection;

	/** The value displayed by this key area supplied externally. */
	TAttribute<TOptional<float>> ExternalValue;
};
