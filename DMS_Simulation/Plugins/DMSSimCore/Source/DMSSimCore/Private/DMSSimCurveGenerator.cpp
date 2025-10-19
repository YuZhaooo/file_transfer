#include "DMSSimCurveGenerator.h"

using TAnimationPoint = DMSSimMontageBuilder::TAnimationPoint;

namespace DMSSimCurveGenerator
{
namespace
{
	constexpr float MIN_BLENDOUT_TIME = 0.0001f;

	UCurveVector* LoadCurve(const TArray<TAnimationPoint>& Points)
	{
		UCurveVector* const CurveVector = NewObject<UCurveVector>();
		for (int i = 0; i < 3; ++i)
		{
			auto& FloatCurve = CurveVector->FloatCurves[i];
			for (const auto& Point : Points)
			{
				FloatCurve.AddKey(Point.Time, Point.Point[i]);
			}
		}
		return CurveVector;
	}

	float AddCurve(const TCurve& Curve, const float PrevBlendOut, const float Offset, const bool LastAnimation, TArray<TAnimationPoint>& CombinedPoints)
	{
		UCurveVector* const CurveVector = LoadCurve(Curve.Points);

		float BlendOut = 0.0f;
		if (!LastAnimation)
		{
			BlendOut = std::max(Curve.BlendOut, MIN_BLENDOUT_TIME);
		}
		float StartPos = Curve.StartPos + PrevBlendOut / 2.0f;
		float EndPos = Curve.EndPos;
		StartPos = std::min(StartPos, EndPos);
		EndPos = std::max(StartPos, EndPos - (BlendOut / 2.0f));
		const float Time = EndPos - StartPos + (BlendOut + PrevBlendOut) / 2.0f;

		const FVector Point0 = CurveVector->GetVectorValue(StartPos);
		CombinedPoints.Push(TAnimationPoint{ Offset + PrevBlendOut / 2.0f, Point0 });
		for (const auto& Point : Curve.Points)
		{
			if (Point.Time > StartPos && Point.Time < EndPos)
			{
				const FVector Point1 = CurveVector->GetVectorValue(Point.Time);
				CombinedPoints.Push(TAnimationPoint{ Point.Time + Offset - Curve.StartPos, Point1 });
			}
		}

		const FVector Point2 = CurveVector->GetVectorValue(EndPos);
		CombinedPoints.Push(TAnimationPoint{ EndPos + Offset - Curve.StartPos, Point2 });
		CurveVector->ConditionalBeginDestroy();
		return Time;
	}
} // anonymous namespace

UCurveVector* Generate(const TArray<TCurve>& Curves, float& CurveTime)
{
	CurveTime = 0;
	TArray<TAnimationPoint> CombinedPoints;
	float PrevBlendOut = 0.0f;
	float Time = 0.0f;
	for (size_t i = 0; i < Curves.Num(); ++i)
	{
		const auto& Curve = Curves[i];
		Time += AddCurve(Curve, PrevBlendOut, Time, (i + 1) == Curves.Num(), CombinedPoints);
		PrevBlendOut = std::max(Curve.BlendOut, MIN_BLENDOUT_TIME);
	}
	if (CombinedPoints.Num() != 0)
	{
		CurveTime = Time;
		return LoadCurve(CombinedPoints);
	}
	return nullptr;
}

} // namespace DMSSimCurveGenerator

