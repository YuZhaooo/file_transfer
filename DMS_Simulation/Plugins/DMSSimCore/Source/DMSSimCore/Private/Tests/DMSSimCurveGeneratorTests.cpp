#include "DMSSimCurveGenerator.h"

using TCurve = DMSSimCurveGenerator::TCurve;

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(DMSSimCurveGeneratorTest1, "DMSSim.CurveGenerator.Tests1", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool DMSSimCurveGeneratorTest1::RunTest(const FString& Parameters)
{
	const TCurve Curve =
	{
		{
			{
				0.0f,
				{ 1.0f, 0.0f, 0.0f }
			},
			{
				1.0f,
				{ 1.0f, 0.0f, 1.0f }
			},
			{
				2.0f,
				{ 1.0f, 0.0f, 2.0f }
			},
			{
				3.0f,
				{ 1.0f, 0.0f, 1.0f }
			},
		},
		0.0f,
		3.0f,
		3.0f
	};

	TArray<TCurve> Curves;
	Curves.Push(Curve);

	float TotalTime = 0;
	UCurveVector* const CurveVector = DMSSimCurveGenerator::Generate(Curves, TotalTime);

	float MinTime = 0;
	float MaxTime = 0;
	CurveVector->GetTimeRange(MinTime, MaxTime);

	TestTrue("MinTime", FMath::IsNearlyEqual(MinTime, 0.0f, 0.00001f));
	TestTrue("MaxTime", FMath::IsNearlyEqual(MaxTime, 3.0f, 0.00001f));
	TestTrue("TotalTime", FMath::IsNearlyEqual(TotalTime, 3.0f, 0.00001f));

	float MinValue = 0;
	float MaxValue = 0;
	CurveVector->GetValueRange(MinValue, MaxValue);
	TestTrue("MinValue", FMath::IsNearlyEqual(MinValue, 0.0f, 0.00001f));
	TestTrue("MaxValue", FMath::IsNearlyEqual(MaxValue, 2.0f, 0.00001f));

	const auto& FloatCurve = CurveVector->FloatCurves[0];
	TestEqual("Number of keys", FloatCurve.GetNumKeys(), 4);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(DMSSimCurveGeneratorTest2, "DMSSim.CurveGenerator.Tests2", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool DMSSimCurveGeneratorTest2::RunTest(const FString& Parameters)
{
	const TCurve Curve0 =
	{
		{
			{
				0.0f,
				{ 1.0f, 0.0f, 0.0f }
			},
			{
				1.0f,
				{ 1.0f, 0.0f, 1.0f }
			},
			{
				2.0f,
				{ 1.0f, 0.0f, 2.0f }
			},
			{
				3.0f,
				{ 1.0f, 0.0f, 1.0f }
			},
		},
		0.0f,
		3.0f,
		3.0f,
		2.0f
	};

	const TCurve Curve1 =
	{
		{
			{
				0.0f,
				{ 1.0f, 0.0f, 0.0f }
			},
			{
				1.0f,
				{ 1.0f, 0.0f, 1.0f }
			},
			{
				2.0f,
				{ 1.0f, 0.0f, 2.0f }
			},
		},
		0.0f,
		2.0f,
		2.0f,
		1.0f
	};

	const TCurve Curve2 =
	{
		{
			{
				0.0f,
				{ 1.0f, 0.0f, 0.0f }
			},
			{
				1.0f,
				{ 1.0f, 0.0f, 1.0f }
			},
			{
				2.0f,
				{ 1.0f, 0.0f, 2.0f }
			},
			{
				3.0f,
				{ 1.0f, 0.0f, 1.0f }
			},
		},
		0.0f,
		3.0f,
		3.0f,
		2.0f
	};

	TArray<TCurve> Curves;
	Curves.Push(Curve0);
	Curves.Push(Curve1);
	Curves.Push(Curve2);

	float TotalTime = 0;
	UCurveVector* const CurveVector = DMSSimCurveGenerator::Generate(Curves, TotalTime);

	float MinTime = 0;
	float MaxTime = 0;
	CurveVector->GetTimeRange(MinTime, MaxTime);

	TestTrue("MinTime", FMath::IsNearlyEqual(MinTime, 0.0f, 0.00001f));
	TestTrue("MaxTime", FMath::IsNearlyEqual(MaxTime, 8.0f, 0.00001f));
	TestTrue("TotalTime", FMath::IsNearlyEqual(TotalTime, 8.0f, 0.00001f));

	float MinValue = 0;
	float MaxValue = 0;
	CurveVector->GetValueRange(MinValue, MaxValue);
	TestTrue("MinValue", FMath::IsNearlyEqual(MinValue, 0.0f, 0.00001f));
	TestTrue("MaxValue", FMath::IsNearlyEqual(MaxValue, 2.0f, 0.00001f));

	const auto& FloatCurve = CurveVector->FloatCurves[0];
	TestEqual("Number of keys", FloatCurve.GetNumKeys(), 9);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(DMSSimCurveGeneratorTest3, "DMSSim.CurveGenerator.Tests3", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool DMSSimCurveGeneratorTest3::RunTest(const FString& Parameters)
{
	const TCurve Curve0 =
	{
		{
			{
				0.0f,
				{ 1.0f, 0.0f, 0.0f }
			},
			{
				1.0f,
				{ 2.0f, 0.0f, 0.0f }
			},
			{
				2.0f,
				{ 3.0f, 0.0f, 0.0f }
			},
			{
				3.0f,
				{ 4.0f, 0.0f, 0.0f }
			},
		},
		0.0f,
		3.0f,
		3.0f,
		0.0f
	};

	const TCurve Curve1 =
	{
		{
			{
				0.0f,
				{ 1.0f, 0.0f, 0.0f }
			},
			{
				1.0f,
				{ 2.0f, 0.0f, 0.0f }
			},
			{
				2.0f,
				{ 3.0f, 0.0f, 0.0f }
			},
		},
		0.0f,
		2.0f,
		2.0f,
		0.0f
	};

	TArray<TCurve> Curves;
	Curves.Push(Curve0);
	Curves.Push(Curve1);

	float TotalTime = 0;
	UCurveVector* const CurveVector = DMSSimCurveGenerator::Generate(Curves, TotalTime);

	float MinTime = 0;
	float MaxTime = 0;
	CurveVector->GetTimeRange(MinTime, MaxTime);

	TestTrue("MinTime", FMath::IsNearlyEqual(MinTime, 0.0f, 0.00001f));
	TestTrue("MaxTime", FMath::IsNearlyEqual(MaxTime, 5.0f, 0.00001f));
	TestTrue("TotalTime", FMath::IsNearlyEqual(TotalTime, 5.0f, 0.00001f));

	float MinValue = 0;
	float MaxValue = 0;
	CurveVector->GetValueRange(MinValue, MaxValue);
	TestTrue("MinValue", FMath::IsNearlyEqual(MinValue, 0.0f, 0.001f));
	TestTrue("MaxValue", FMath::IsNearlyEqual(MaxValue, 4.0f, 0.001f));

	const auto& FloatCurve = CurveVector->FloatCurves[0];
	TestEqual("Number of keys", FloatCurve.GetNumKeys(), 7);
	const auto& Keys = FloatCurve.Keys;
	TestTrue("Key 0 Value", FMath::IsNearlyEqual(Keys[0].Value, 1.0f, 0.001f));
	TestTrue("Key 0 Time", FMath::IsNearlyEqual(Keys[0].Time, 0.0f, 0.001f));
	TestTrue("Key 1", FMath::IsNearlyEqual(Keys[1].Value, 2.0f, 0.001f));
	TestTrue("Key 1 Time", FMath::IsNearlyEqual(Keys[1].Time, 1.0f, 0.001f));
	TestTrue("Key 2", FMath::IsNearlyEqual(Keys[2].Value, 3.0f, 0.001f));
	TestTrue("Key 2 Time", FMath::IsNearlyEqual(Keys[2].Time, 2.0f, 0.001f));
	TestTrue("Key 3", FMath::IsNearlyEqual(Keys[3].Value, 4.0f, 0.001f));
	TestTrue("Key 3 Time", FMath::IsNearlyEqual(Keys[3].Time, 3.0f, 0.001f));
	TestTrue("Key 4", FMath::IsNearlyEqual(Keys[4].Value, 1.0f, 0.001f));
	TestTrue("Key 4 Time", FMath::IsNearlyEqual(Keys[4].Time, 3.0f, 0.001f));
	TestTrue("Key 5", FMath::IsNearlyEqual(Keys[5].Value, 2.0f, 0.001f));
	TestTrue("Key 5 Time", FMath::IsNearlyEqual(Keys[5].Time, 4.0f, 0.001f));
	TestTrue("Key 6", FMath::IsNearlyEqual(Keys[6].Value, 3.0f, 0.001f));
	TestTrue("Key 6 Time", FMath::IsNearlyEqual(Keys[6].Time, 5.0f, 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(DMSSimCurveGeneratorTest4, "DMSSim.CurveGenerator.Tests4", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool DMSSimCurveGeneratorTest4::RunTest(const FString& Parameters)
{
	const TCurve Curve0 =
	{
		{
			{
				0.0f,
				{ 1.0f, 0.0f, 0.0f }
			},
			{
				1.0f,
				{ 2.0f, 0.0f, 0.0f }
			},
			{
				2.0f,
				{ 3.0f, 0.0f, 0.0f }
			},
			{
				3.0f,
				{ 4.0f, 0.0f, 0.0f }
			},
		},
		0.0f,
		3.0f,
		3.0f,
		3.0f
	};

	const TCurve Curve1 =
	{
		{
			{
				0.0f,
				{ 1.0f, 0.0f, 0.0f }
			},
			{
				1.0f,
				{ 2.0f, 0.0f, 0.0f }
			},
			{
				2.0f,
				{ 3.0f, 0.0f, 0.0f }
			},
			{
				3.0f,
				{ 4.0f, 0.0f, 0.0f }
			},
		},
		0.0f,
		3.0f,
		3.0f,
		3.0f
	};

	TArray<TCurve> Curves;
	Curves.Push(Curve0);
	Curves.Push(Curve1);

	float TotalTime = 0;
	UCurveVector* const CurveVector = DMSSimCurveGenerator::Generate(Curves, TotalTime);

	float MinTime = 0;
	float MaxTime = 0;
	CurveVector->GetTimeRange(MinTime, MaxTime);

	TestTrue("MinTime", FMath::IsNearlyEqual(MinTime, 0.0f, 0.00001f));
	TestTrue("MaxTime", FMath::IsNearlyEqual(MaxTime, 6.0f, 0.00001f));
	TestTrue("TotalTime", FMath::IsNearlyEqual(TotalTime, 6.0f, 0.00001f));

	float MinValue = 0;
	float MaxValue = 0;
	CurveVector->GetValueRange(MinValue, MaxValue);
	TestTrue("MinValue", FMath::IsNearlyEqual(MinValue, 0.0f, 0.00001f));
	TestTrue("MaxValue", FMath::IsNearlyEqual(MaxValue, 4.0f, 0.00001f));

	const auto& FloatCurve = CurveVector->FloatCurves[0];
	TestEqual("Number of keys", FloatCurve.GetNumKeys(), 6);

	const auto& Keys = FloatCurve.Keys;
	TestTrue("Key 0 Value", FMath::IsNearlyEqual(Keys[0].Value, 1.0f, 0.001f));
	TestTrue("Key 0 Time", FMath::IsNearlyEqual(Keys[0].Time, 0.0f, 0.001f));
	TestTrue("Key 1", FMath::IsNearlyEqual(Keys[1].Value, 2.0f, 0.001f));
	TestTrue("Key 1 Time", FMath::IsNearlyEqual(Keys[1].Time, 1.0f, 0.001f));
	TestTrue("Key 2", FMath::IsNearlyEqual(Keys[2].Value, 2.5f, 0.001f));
	TestTrue("Key 2 Time", FMath::IsNearlyEqual(Keys[2].Time, 1.5f, 0.001f));
	TestTrue("Key 3", FMath::IsNearlyEqual(Keys[3].Value, 2.5f, 0.001f));
	TestTrue("Key 3 Time", FMath::IsNearlyEqual(Keys[3].Time, 4.5f, 0.001f));
	TestTrue("Key 4", FMath::IsNearlyEqual(Keys[4].Value, 3.0f, 0.001f));
	TestTrue("Key 4 Time", FMath::IsNearlyEqual(Keys[4].Time, 5.0f, 0.001f));
	TestTrue("Key 5", FMath::IsNearlyEqual(Keys[5].Value, 4.0f, 0.001f));
	TestTrue("Key 5 Time", FMath::IsNearlyEqual(Keys[5].Time, 6.0f, 0.001f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(DMSSimCurveGeneratorTest5, "DMSSim.CurveGenerator.Tests5", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool DMSSimCurveGeneratorTest5::RunTest(const FString& Parameters)
{
	const TCurve Curve0 =
	{
		{
			{
				0.0f,
				{ 1.0f, 0.0f, 0.0f }
			},
			{
				1.0f,
				{ 2.0f, 0.0f, 0.0f }
			},
			{
				2.0f,
				{ 3.0f, 0.0f, 0.0f }
			},
			{
				3.0f,
				{ 4.0f, 0.0f, 0.0f }
			},
		},
		0.5f,
		2.4f,
		1.9f,
		0.5f
	};

	const TCurve Curve1 =
	{
		{
			{
				0.0f,
				{ 1.0f, 0.0f, 0.0f }
			},
			{
				1.0f,
				{ 2.0f, 0.0f, 0.0f }
			},
			{
				2.0f,
				{ 3.0f, 0.0f, 0.0f }
			},
			{
				3.0f,
				{ 4.0f, 0.0f, 0.0f }
			},
		},
		0.0f,
		3.0f,
		3.0f,
		3.0f
	};

	TArray<TCurve> Curves;
	Curves.Push(Curve0);
	Curves.Push(Curve1);

	float TotalTime = 0;
	UCurveVector* const CurveVector = DMSSimCurveGenerator::Generate(Curves, TotalTime);

	float MinTime = 0;
	float MaxTime = 0;
	CurveVector->GetTimeRange(MinTime, MaxTime);

	TestTrue("MinTime", FMath::IsNearlyEqual(MinTime, 0.0f, 0.00001f));
	TestTrue("MaxTime", FMath::IsNearlyEqual(MaxTime, 4.9f, 0.00001f));
	TestTrue("TotalTime", FMath::IsNearlyEqual(TotalTime, 4.9f, 0.00001f));

	float MinValue = 0;
	float MaxValue = 0;
	CurveVector->GetValueRange(MinValue, MaxValue);
	TestTrue("MinValue", FMath::IsNearlyEqual(MinValue, 0.0f, 0.00001f));
	TestTrue("MaxValue", FMath::IsNearlyEqual(MaxValue, 4.0f, 0.00001f));

	const auto& FloatCurve = CurveVector->FloatCurves[0];
	TestEqual("Number of keys", FloatCurve.GetNumKeys(), 8);

	const auto& Keys = FloatCurve.Keys;
	TestTrue("Key 0 Value", FMath::IsNearlyEqual(Keys[0].Value, 1.5f, 0.001f));
	TestTrue("Key 0 Time", FMath::IsNearlyEqual(Keys[0].Time, 0.0f, 0.001f));
	TestTrue("Key 1", FMath::IsNearlyEqual(Keys[1].Value, 2.0f, 0.001f));
	TestTrue("Key 1 Time", FMath::IsNearlyEqual(Keys[1].Time, 0.5f, 0.001f));
	TestTrue("Key 2", FMath::IsNearlyEqual(Keys[2].Value, 3.0f, 0.001f));
	TestTrue("Key 2 Time", FMath::IsNearlyEqual(Keys[2].Time, 1.5f, 0.001f));
	TestTrue("Key 3", FMath::IsNearlyEqual(Keys[3].Value, 3.15f, 0.001f));
	TestTrue("Key 3 Time", FMath::IsNearlyEqual(Keys[3].Time, 1.65f, 0.001f));
	TestTrue("Key 4", FMath::IsNearlyEqual(Keys[4].Value, 1.25f, 0.001f));
	TestTrue("Key 4 Time", FMath::IsNearlyEqual(Keys[4].Time, 2.15f, 0.001f));
	TestTrue("Key 5", FMath::IsNearlyEqual(Keys[5].Value, 2.0f, 0.001f));
	TestTrue("Key 5 Time", FMath::IsNearlyEqual(Keys[5].Time, 2.9f, 0.001f));
	TestTrue("Key 6", FMath::IsNearlyEqual(Keys[6].Value, 3.0f, 0.001f));
	TestTrue("Key 6 Time", FMath::IsNearlyEqual(Keys[6].Time, 3.9f, 0.001f));
	TestTrue("Key 7", FMath::IsNearlyEqual(Keys[7].Value, 4.0f, 0.001f));
	TestTrue("Key 7 Time", FMath::IsNearlyEqual(Keys[7].Time, 4.9f, 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(DMSSimCurveGeneratorTest6, "DMSSim.CurveGenerator.Tests6", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool DMSSimCurveGeneratorTest6::RunTest(const FString& Parameters)
{
	const TCurve Curve0 =
	{
		{
			{
				0.0f,
				{ 1.0f, 0.0f, 0.0f }
			},
			{
				1.0f,
				{ 2.0f, 0.0f, 0.0f }
			},
			{
				2.0f,
				{ 3.0f, 0.0f, 0.0f }
			},
			{
				3.0f,
				{ 4.0f, 0.0f, 0.0f }
			},
		},
		0.3f,
		1.7f,
		1.4f,
		0.5f
	};

	const TCurve Curve1 =
	{
		{
			{
				0.0f,
				{ 1.0f, 0.0f, 0.0f }
			},
			{
				1.0f,
				{ 2.0f, 0.0f, 0.0f }
			},
			{
				2.0f,
				{ 3.0f, 0.0f, 0.0f }
			},
			{
				3.0f,
				{ 4.0f, 0.0f, 0.0f }
			},
		},
		0.7f,
		2.9f,
		2.2f,
		1.3f
	};

	const TCurve Curve2 =
	{
		{
			{
				0.0f,
				{ 1.0f, 0.0f, 0.0f }
			},
			{
				1.0f,
				{ 2.0f, 0.0f, 0.0f }
			},
			{
				2.0f,
				{ 3.0f, 0.0f, 0.0f }
			},
			{
				3.0f,
				{ 4.0f, 0.0f, 0.0f }
			},
		},
		0.0f,
		3.0f,
		3.0f,
		3.0f
	};

	TArray<TCurve> Curves;
	Curves.Push(Curve0);
	Curves.Push(Curve1);
	Curves.Push(Curve2);

	float TotalTime = 0;
	UCurveVector* const CurveVector = DMSSimCurveGenerator::Generate(Curves, TotalTime);

	float MinTime = 0;
	float MaxTime = 0;
	CurveVector->GetTimeRange(MinTime, MaxTime);

	TestTrue("MinTime", FMath::IsNearlyEqual(MinTime, 0.0f, 0.00001f));
	TestTrue("MaxTime", FMath::IsNearlyEqual(MaxTime, 6.6f, 0.00001f));
	TestTrue("TotalTime", FMath::IsNearlyEqual(TotalTime, 6.6f, 0.00001f));

	float MinValue = 0;
	float MaxValue = 0;
	CurveVector->GetValueRange(MinValue, MaxValue);
	TestTrue("MinValue", FMath::IsNearlyEqual(MinValue, 0.0f, 0.00001f));
	TestTrue("MaxValue", FMath::IsNearlyEqual(MaxValue, 4.0f, 0.00001f));

	const auto& FloatCurve = CurveVector->FloatCurves[0];
	TestEqual("Number of keys", FloatCurve.GetNumKeys(), 11);

	const auto& Keys = FloatCurve.Keys;
	TestTrue("Key 0 Value", FMath::IsNearlyEqual(Keys[0].Value, 1.3f, 0.001f));
	TestTrue("Key 0 Time", FMath::IsNearlyEqual(Keys[0].Time, 0.0f, 0.001f));
	TestTrue("Key 1", FMath::IsNearlyEqual(Keys[1].Value, 2.0f, 0.001f));
	TestTrue("Key 1 Time", FMath::IsNearlyEqual(Keys[1].Time, 0.7f, 0.001f));
	TestTrue("Key 2", FMath::IsNearlyEqual(Keys[2].Value, 2.45f, 0.001f));
	TestTrue("Key 2 Time", FMath::IsNearlyEqual(Keys[2].Time, 1.15f, 0.001f));
	TestTrue("Key 3", FMath::IsNearlyEqual(Keys[3].Value, 1.95f, 0.001f));
	TestTrue("Key 3 Time", FMath::IsNearlyEqual(Keys[3].Time, 1.65f, 0.001f));
	TestTrue("Key 4", FMath::IsNearlyEqual(Keys[4].Value, 2.0f, 0.001f));
	TestTrue("Key 4 Time", FMath::IsNearlyEqual(Keys[4].Time, 1.7f, 0.001f));
	TestTrue("Key 5", FMath::IsNearlyEqual(Keys[5].Value, 3.0f, 0.001f));
	TestTrue("Key 5 Time", FMath::IsNearlyEqual(Keys[5].Time, 2.7f, 0.001f));
	TestTrue("Key 6", FMath::IsNearlyEqual(Keys[6].Value, 3.25f, 0.001f));
	TestTrue("Key 6 Time", FMath::IsNearlyEqual(Keys[6].Time, 2.95f, 0.001f));
	TestTrue("Key 7", FMath::IsNearlyEqual(Keys[7].Value, 1.65f, 0.001f));
	TestTrue("Key 7 Time", FMath::IsNearlyEqual(Keys[7].Time, 4.25f, 0.001f));
	TestTrue("Key 8", FMath::IsNearlyEqual(Keys[8].Value, 2.0f, 0.001f));
	TestTrue("Key 8 Time", FMath::IsNearlyEqual(Keys[8].Time, 4.6f, 0.001f));
	TestTrue("Key 9", FMath::IsNearlyEqual(Keys[9].Value, 3.0f, 0.001f));
	TestTrue("Key 9 Time", FMath::IsNearlyEqual(Keys[9].Time, 5.6f, 0.001f));
	TestTrue("Key 10", FMath::IsNearlyEqual(Keys[10].Value, 4.0f, 0.001f));
	TestTrue("Key 10 Time", FMath::IsNearlyEqual(Keys[10].Time, 6.6f, 0.001f));
	return true;
}


#endif //WITH_DEV_AUTOMATION_TESTS
